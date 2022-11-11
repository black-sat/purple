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

#ifndef PURPLE_PROBLEM_HPP
#define PURPLE_PROBLEM_HPP

#include <black/logic/logic.hpp>
#include <black/support/tribool.hpp>

#include <vector>
#include <unordered_map>

namespace purple {

  namespace logic = black::logic::fragments::FO;
  namespace temporal = black::logic::fragments::LTLPFO;

  using black::identifier;
  using black::tribool;

  struct effect {
    logic::alphabet &sigma;
    logic::formula precondition;
    std::vector<logic::proposition> fluents;
    std::vector<logic::atom> predicates;
    bool positive = true;

    effect(
      logic::formula pre,
      std::vector<logic::proposition> f,
      std::vector<logic::atom> p,
      bool pos = true
    ) : sigma{*pre.sigma()},
        precondition{pre}, fluents{f}, predicates{p}, positive{pos} { }
    
    effect(
      logic::alphabet &s,
      std::vector<logic::proposition> f,
      std::vector<logic::atom> p,
      bool pos = true
    ) : sigma{s}, precondition{sigma.top()}, 
        fluents{f}, predicates{p}, positive{pos} { }

    effect(
      logic::formula pre,
      logic::proposition f,
      bool pos = true
    ) : sigma{*pre.sigma()}, precondition{pre}, fluents{{f}}, positive{pos} { }

    effect(
      logic::formula pre,
      logic::atom p,
      bool pos = true
    ) : sigma{*pre.sigma()}, 
        precondition{pre}, predicates{{p}}, positive{pos} { }

    effect(
      logic::proposition f,
      bool pos = true
    ) : sigma{*f.sigma()}, precondition{sigma.top()},
        fluents{{f}}, positive{pos} { }

    effect(
      logic::atom p,
      bool pos = true
    ) : sigma{*p.sigma()}, precondition{sigma.top()}, 
        predicates{{p}}, positive{pos} { }
  };

  struct state {
    std::vector<logic::proposition> fluents;
    std::vector<logic::atom> predicates;
  };

  // schematic or ground instantaneous action
  struct action {
    identifier name;
    std::vector<black::var_decl> params;

    logic::formula precondition;
    std::vector<effect> effects;
  };

  // schematic fluent (a.k.a. predicate)
  struct predicate {
    logic::relation name;
    std::vector<black::var_decl> params;

    template<typename ...Args>
    auto operator()(Args ...args) const {
      return name(args...);
    }

    auto operator()(std::vector<logic::var_decl> const& decls) const {
      std::vector<logic::variable> vars;
      for(auto d : decls) 
        vars.push_back(d.variable());
      
      return name(vars);
    }
  };

  // planning domain
  struct domain {
    black::alphabet &sigma;
    std::vector<logic::named_sort> types;
    std::vector<logic::proposition> fluents;
    std::vector<predicate> predicates;
    std::vector<action> actions;
  };

  // planning problem
  struct problem {
    black::alphabet &sigma;
    std::vector<black::sort_decl> types;
    state init;
    logic::formula goal;
  };

  struct plan {
    struct step {
      struct action action;
      std::vector<logic::variable> args;
    };

    std::vector<step> steps;
  };

}

#endif // PURPLE_PROBLEM_HPP
