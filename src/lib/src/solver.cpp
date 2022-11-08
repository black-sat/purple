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

  static logic::atom apply(action const& a) {
    auto rel = a.precondition.sigma()->relation(a.name);
    std::vector<logic::variable> vars;
    for(auto d : a.params) 
      vars.push_back(d.variable());
    
    return rel(vars);
  }

  static logic::formula encode(effect const& e) {
    logic::formula fluents = 
      logic::big_and(e.sigma, e.fluents, [](auto x) { return x; });
    logic::formula preds = 
      logic::big_and(e.sigma, e.predicates, [](auto x) { return x; });

    return fluents && preds;
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

  static logic::formula replace(
    logic::formula f, 
    std::vector<logic::term> const &patterns,
    std::vector<logic::var_decl> const &replacements
  ) {
    std::vector<logic::term> rvars;

    for(auto d : replacements)
      rvars.push_back(d.variable());

    return logic::replace(f, patterns, rvars);
  }

  static temporal::formula frame(domain d, predicate p, bool change) {
    logic::alphabet &sigma = d.sigma;

    temporal::formula head = sigma.top();
    
    if(change)
      head = !p(p.params) && X(p(p.params));
    else
      head = p(p.params) && X(!p(p.params));

    auto body = big_or(sigma, d.actions, [&](action const& a) -> logic::formula 
    {
      std::vector<logic::formula> pre;
      
      logic::formula act = apply(a);

      for(effect const& e : a.effects) {
        if(e.positive == change) {
          for(logic::atom t : e.predicates) {
            if(t.rel() == p.name) {
              pre.push_back(replace(
                e.precondition ? *e.precondition : sigma.top(), 
                t.terms(), p.params
              ));
              
              act = replace(act, t.terms(), p.params);
            }
          }
        }
      }
      
      logic::formula preformula = big_or(sigma, pre, [&](auto f) { return f; });

      std::vector<logic::var_decl> closure;
      for(auto param : a.params) {
        if(std::find(p.params.begin(), p.params.end(), param) == p.params.end())
          closure.push_back(param);
      }

      if(closure.empty())
        return act && preformula;

      return logic::exists_block(closure, act && preformula);
    });

    return temporal::forall_block(p.params, implies(head, body));
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
          logic::formula pre = e.precondition ? *e.precondition : sigma.top();
          return temporal::forall_block(
            a.params, implies(apply(a) && pre, X(encode(e)))
          );
        });
      });
   
    temporal::formula frames = 
      big_and(sigma, d.predicates, [&](predicate const& pred) {
        return frame(d, pred, true) && frame(d, pred, false);
      });

    temporal::formula transition = preconditions && effects && frames;

    return init && G(transition) && F(p.goal);
  }

  tribool solver::solve(domain const& d, problem const& p) const {
    black::solver slv;

    std::optional<logic::scope> xi = scope(d, p);
    if(!xi)
      return tribool::undef;

    temporal::formula encoding = encode(d, p);
    
    //std::cerr << to_string(encoding) << "\n";

    return slv.solve(*xi, encoding, /* finite = */ true);
  }

}
