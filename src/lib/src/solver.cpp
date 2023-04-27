// 
// PURPLE - Expressive Automated Planner based on BLACK
// 
// (C) 2022 Nicola Gigante
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <purple/solver.hpp>

#include <black/solver/solver.hpp>
#include <black/logic/prettyprint.hpp>

#include <string_view>
#include <iostream>

namespace purple {

  using namespace std::literals;

  static
  std::optional<logic::scope> scope(domain const& d, problem const& p) {
    logic::alphabet *sigma = d.sigma;
    logic::scope xi{*sigma};

    // declare sorts
    for(logic::sort_decl decl : p.types)
      xi.declare(decl);

    // declare predicates
    for(predicate pred : d.predicates)
      xi.declare(pred.name, pred.params);

    // declare relations corresponding to actions
    for(action a : d.actions)
      xi.declare(sigma->relation(a.name), a.params);

    return xi;
  }

  static logic::formula 
  _exists(std::vector<logic::var_decl> params, logic::formula matrix) {
    if(params.empty())
      return matrix;
    return black::logic::exists(params, matrix);
  }

  static logic::formula
  logic_forall(std::vector<logic::var_decl> params, logic::formula matrix) {
    if(params.empty())
      return matrix;
    return black::logic::forall(params, matrix);
  }

  static temporal::formula
  temporal_forall(
    std::vector<logic::var_decl> params, temporal::formula matrix
  ) {
    if(params.empty())
      return matrix;
    return black::logic::forall(params, matrix);
  }

  static 
  logic::formula apply(action const& a, std::vector<logic::var_decl> decls) 
  {
    if(decls.empty())
      return a.precondition.sigma()->proposition(a.name);
    auto rel = a.precondition.sigma()->relation(a.name);
    return rel(decls);
  }

  static logic::formula apply(action const& a) {
    return apply(a, a.params);
  }

  static logic::formula encode(effect const& e) {
    return 
      logic::big_and(*e.sigma, e.fluents, [&](auto p) -> logic::formula {
        if(e.positive)
          return p;
        return !p;
      }) && 
      logic::big_and(*e.sigma, e.predicates, [&](auto p) -> logic::formula {
        if(e.positive)
          return p;
        return !p;
      });
  }

  static bool mentions(logic::formula f, logic::relation r, bool positive) {
    using namespace logic;

    return f.match(
      [&](atom, auto rel, auto) {
        if(rel == r && positive)
          return true;
        return false;
      },
      [&](quantifier q) {
        return mentions(q.matrix(), r, positive);
      },
      [&](negation, auto arg) {
        return mentions(arg, r, !positive);
      },
      [&](unary, auto arg) {
        return mentions(arg, r, positive);
      },
      [&](binary, auto left, auto right) {
        return mentions(left, r, positive) || mentions(right, r, positive);
      },
      [](otherwise) { return false; }
    );
  }

  static logic::formula encode(domain const& d, state const& s) {
    std::vector<logic::proposition> negatives;
    for(auto prop : d.fluents) {
      if(std::find(s.fluents.begin(), s.fluents.end(), prop) == s.fluents.end())
        negatives.push_back(prop);
    }

    logic::formula props = 
      big_and(*d.sigma, s.fluents) && big_and(*d.sigma, negatives, [](auto p) {
        return !p;
      });
    
    logic::formula preds =
      big_and(*d.sigma, d.predicates, [&](predicate const& pred) {
        std::vector<logic::formula> guards;
        for(logic::atom a : s.predicates) {
          if(a.rel() != pred.name)
            continue;
          
          black_assert(pred.params.size() == a.terms().size());

          std::vector<logic::formula> eqs;
          for(size_t i = 0; i < pred.params.size(); ++i)
            eqs.push_back(pred.params[i].variable() == a.terms()[i]);
          
          guards.push_back(big_and(*d.sigma, eqs));
        }

        return logic_forall(pred.params, 
          logic::iff(pred.name(pred.params), big_or(*d.sigma, guards))
        );
      });

    return props && preds;
  }

  static temporal::formula frame(domain d, logic::proposition p, bool change) {
    logic::alphabet *sigma = d.sigma;

    temporal::formula head = sigma->top();
    
    if(change)
      head = !p && X(p);
    else
      head = p && X(!p);

    auto body = big_or(*sigma, d.actions, [&](action const& a) {
      std::vector<logic::formula> pre;

      for(effect const& e : a.effects)
        if(e.positive == change)
          for(logic::proposition q : e.fluents)
            if(p == q)
              pre.push_back(e.precondition);
      
      return _exists(a.params, apply(a) && big_or(*sigma, pre));
    });

    return implies(head, body);
  }

  static temporal::formula frame(domain d, predicate p, bool change) {
    logic::alphabet *sigma = d.sigma;

    temporal::formula head = sigma->top();
    
    if(change)
      head = !p(p.params) && X(p(p.params));
    else
      head = p(p.params) && X(!p(p.params));

    auto body = big_or(*sigma, d.actions, [&](action const& a) {
      std::vector<logic::formula> mappings;
      std::vector<logic::formula> pre;

      for(effect const& e : a.effects) {
        if(e.positive == change) {
          for(logic::atom t : e.predicates) {
            if(t.rel() == p.name) {
              pre.push_back(e.precondition);
              for(size_t i = 0; i < t.terms().size(); ++i) {
                mappings.push_back(t.terms()[i] == p.params[i].variable());
              }
            }
          }
        }
      }
      
      return _exists(a.params, 
        apply(a) && big_and(*sigma, mappings) && big_or(*sigma, pre)
      );
    });

    return temporal_forall(p.params, implies(head, body));
  }

  static logic::formula parallelism(domain const& d) {
    std::vector<logic::formula> axioms;

    for(action const& a1 : d.actions) {
      for(action const& a2 : d.actions) {
        if(a1.name == a2.name)
          continue;
        axioms.push_back(
          _exists(a1.params, !apply(a1)) || 
          _exists(a2.params, !apply(a2))
        );
      }
    }

    for(action const& a : d.actions) { 
      std::vector<logic::var_decl> primes;
      for(logic::var_decl decl : a.params) {
        logic::variable prime = 
          d.sigma->variable(std::tuple{"_prime_"sv, decl.variable()});
        primes.push_back(d.sigma->var_decl(prime, decl.sort()));
      }

      std::vector<logic::formula> guards;
      for(size_t i = 0; i < a.params.size(); ++i)
        guards.push_back(a.params[i].variable() != primes[i].variable());
      
      logic::formula guard = big_or(*d.sigma, guards);

      axioms.push_back(
        logic_forall(a.params, 
          implies(apply(a), logic_forall(primes, 
            implies(guard, !apply(a, primes))
          ))
        )
      );
    }

    return logic::big_and(*d.sigma, axioms);
  }

  static temporal::formula 
  encode(domain const& d, problem const& p) 
  {
    logic::alphabet *sigma = d.sigma;

    logic::formula init = encode(d, p.init);

    logic::formula preconditions = 
      logic::big_and(*sigma, d.actions, [&](action const& a) {
        return logic_forall(a.params, implies(apply(a), a.precondition));
      });

    temporal::formula effects = 
      temporal::big_and(*sigma, d.actions, [&](action const& a) {
        return temporal::big_and(*sigma, a.effects, [&](effect const& e) {
          return temporal_forall(
            a.params, implies(apply(a) && e.precondition, X(encode(e)))
          );
        });
      });
   
    temporal::formula frames = 
      big_and(*sigma, d.predicates, [&](predicate const& pred) {
        return frame(d, pred, true) && frame(d, pred, false);
      }) &&
      big_and(*sigma, d.fluents, [&](logic::proposition prop) {
        return frame(d, prop, true) && frame(d, prop, false);
      });

    logic::formula semantics = parallelism(d);

    temporal::formula transition = 
      preconditions && effects && frames && semantics;

    return 
      init && G(transition) && p.trajectory && F(p.goal & wX(sigma->bottom()));
  }

  [[maybe_unused]]
  static void tracer(black::solver::trace_t trace) {
    static size_t k = 0;
    if(trace.type == black::solver::trace_t::stage) {
      k = std::get<size_t>(trace.data);
    }

    if(trace.type == black::solver::trace_t::unrav) {
      std::cerr << k << "-unrav: " << 
        to_string(std::get<logic::formula>(trace.data)) << "\n";
    }

    if(trace.type == black::solver::trace_t::empty) {
      std::cerr << k << "-empty: " << 
        to_string(std::get<logic::formula>(trace.data)) << "\n";
    }
    
    if(trace.type == black::solver::trace_t::prune) {
      std::cerr << k << "-prune: " << 
        to_string(std::get<logic::formula>(trace.data)) << "\n";
    }
  }

  tribool solver::solve(domain const& d, problem const& p) {
    _slv = black::solver{};
    _d = &d;
    _p = &p;

    std::optional<logic::scope> xi = scope(d, p);
    if(!xi)
      return tribool::undef;

    temporal::formula encoding = encode(d, p);
    
    //std::cerr << to_string(encoding) << "\n";

    //_slv.set_tracer(tracer);
    //_slv.set_sat_backend("cvc5");

    return _slv.solve(*xi, encoding, /* finite = */ true); //, 9000, true);
  }

  static std::optional<logic::domain_ref>
  domain_of_type(problem const *p, logic::sort s) {
    for(logic::sort_decl sdecl : p->types)
      if(s == sdecl.sort())
        return sdecl.domain();
    
    return {};
  }

  static bool increment(
    std::vector<size_t> &indexes, std::vector<logic::domain_ref> const& domains
  ) {
    for(size_t i = 0; i < indexes.size(); ++i) {
      indexes[i] = (indexes[i] + 1) % domains[i]->elements().size();
      if(indexes[i] != 0)
        break;
      if(i == indexes.size() - 1)
        return false;
    }

    return true;
  }


  std::optional<plan::step> solver::get_step(size_t t) const {
    for(action const &a : _d->actions) {
      if(a.params.empty()) {
        logic::proposition p = _d->sigma->proposition(a.name);
        if(_slv.model()->value(p, t))
          return plan::step{a, {}};
        continue;
      }

      std::vector<logic::domain_ref> domains;
      for(logic::var_decl decl : a.params) {
        auto dom = domain_of_type(_p, decl.sort());
        black_assert(dom.has_value());
        domains.push_back(*dom);
      }
      std::vector<size_t> indexes(domains.size(), 0);
      do {
        std::vector<logic::variable> args;
        for(size_t i = 0; i < domains.size(); i++) {
          args.push_back(domains[i]->elements()[indexes[i]]);
        }
        
        logic::relation rel = _d->sigma->relation(a.name);
        if(_slv.model()->value(rel(args), t)) {
          return plan::step{a, args};
        }
      } while(increment(indexes, domains));
    }

    return {};
  }

  std::optional<plan> solver::solution() const {
    if(!_d || !_p)
      return {};

    plan s;

    for(size_t t = 0; t < _slv.model()->size() - 1; ++t) {
      auto step = get_step(t);
      black_assert(step.has_value());
      s.steps.push_back(*step);
    }
    
    return s;
  }

}
