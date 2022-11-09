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

#include <iostream>

namespace purple {

  static
  std::optional<logic::scope> scope(domain const& d, problem const& p) {
    logic::alphabet &sigma = d.sigma;
    logic::scope xi{sigma};

    // declare sorts
    for(logic::sort_decl decl : p.types) {
      xi.declare(decl);
    }

    // declare predicates
    for(predicate pred : d.predicates) {
      std::vector<logic::sort> sorts;
      for(logic::var_decl v : pred.params)
        sorts.push_back(v.sort());

      xi.declare(pred.name, sorts);
    }

    // declare relations corresponding to actions
    for(action a : d.actions) {
      std::vector<logic::sort> sorts;
      for(logic::var_decl v : a.params)
        sorts.push_back(v.sort());
      
      xi.declare(sigma.relation(a.name), sorts);
    }

    return xi;
  }

  static logic::atom apply(action const& a, std::vector<logic::var_decl> decls) 
  {
    auto rel = a.precondition.sigma()->relation(a.name);
    std::vector<logic::variable> vars;
    for(auto d : decls) 
      vars.push_back(d.variable());
    
    return rel(vars);
  }

  static logic::atom apply(action const& a) {
    return apply(a, a.params);
  }

  static logic::formula encode(effect const& e) {
    return 
      logic::big_and(e.sigma, e.fluents) && 
      logic::big_and(e.sigma, e.predicates);
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
        return mentions(q.block().matrix(), r, positive);
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

  static temporal::formula frame(domain d, logic::proposition p, bool change) {
    logic::alphabet &sigma = d.sigma;

    temporal::formula head = sigma.top();
    
    if(change)
      head = !p && X(p);
    else
      head = p && X(!p);

    auto body = big_or(sigma, d.actions, [&](action const& a) {
      std::vector<logic::formula> pre;

      for(effect const& e : a.effects)
        if(e.positive == change)
          for(logic::proposition q : e.fluents)
            if(p == q)
              pre.push_back(e.precondition);
      
      return logic::exists_block(a.params, apply(a) && big_or(sigma, pre));
    });

    return implies(head, body);
  }

  static temporal::formula frame(domain d, predicate p, bool change) {
    logic::alphabet &sigma = d.sigma;

    temporal::formula head = sigma.top();
    
    if(change)
      head = !p(p.params) && X(p(p.params));
    else
      head = p(p.params) && X(!p(p.params));

    auto body = big_or(sigma, d.actions, [&](action const& a) {
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
      
      return logic::exists_block(a.params, 
        apply(a) && big_and(sigma, mappings) && big_or(sigma, pre)
      );
    });

    return temporal::forall_block(p.params, implies(head, body));
  }

  static logic::formula parallelism(domain const& d) {
    std::vector<logic::formula> axioms;
    for(action const& a1 : d.actions) {
      for(action const& a2 : d.actions) {
        if(a1.name == a2.name)
          continue;
        axioms.push_back(
          logic::exists_block(a1.params, !apply(a1)) || 
          logic::exists_block(a2.params, !apply(a2))
        );
      }
    }

    for(action const& a : d.actions) { 
      std::vector<logic::var_decl> primes;
      for(logic::var_decl decl : a.params) {
        logic::variable prime = 
          d.sigma.variable(std::tuple{decl.variable(), 1});
        primes.push_back(d.sigma.var_decl(prime, decl.sort()));
      }

      std::vector<logic::formula> guards;
      for(size_t i = 0; i < a.params.size(); ++i)
        guards.push_back(a.params[i].variable() != primes[i].variable());
      
      logic::formula guard = big_or(d.sigma, guards);

      axioms.push_back(
        logic::forall_block(a.params, 
          implies(apply(a), logic::forall_block(primes, 
            implies(guard, !apply(a, primes))
          ))
        )
      );
    }

    return logic::big_and(d.sigma, axioms);
  }

  static temporal::formula 
  encode(domain const& d, problem const& p) 
  {
    logic::alphabet &sigma = d.sigma;

    logic::formula init = big_and(sigma, p.init, [&](effect const& e) {
      return encode(e);
    });

    logic::formula preconditions = 
      logic::big_and(sigma, d.actions, [&](action const& a) {
        return logic::forall_block(a.params, implies(apply(a), a.precondition));
      });

    temporal::formula effects = 
      temporal::big_and(sigma, d.actions, [&](action const& a) {
        return temporal::big_and(sigma, a.effects, [&](effect const& e) {
          return temporal::forall_block(
            a.params, implies(apply(a) && e.precondition, X(encode(e)))
          );
        });
      });
   
    temporal::formula frames = 
      big_and(sigma, d.predicates, [&](predicate const& pred) {
        return frame(d, pred, true) && frame(d, pred, false);
      }) &&
      big_and(sigma, d.fluents, [&](logic::proposition prop) {
        return frame(d, prop, true) && frame(d, prop, false);
      });

    logic::formula semantics = parallelism(d);

    temporal::formula transition = 
      preconditions && effects && frames && semantics;

    return init && G(transition) && F(p.goal);
  }

  tribool solver::solve(domain const& d, problem const& p) const {
    black::solver slv;

    std::optional<logic::scope> xi = scope(d, p);
    if(!xi)
      return tribool::undef;

    temporal::formula encoding = encode(d, p);
    
    std::cerr << to_string(encoding) << "\n";

    return slv.solve(*xi, encoding, /* finite = */ true);
  }

}
