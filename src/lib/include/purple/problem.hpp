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

#include <vector>
#include <unordered_map>

namespace purple {

  namespace logic = black::logic::fragments::FO;

  using black::identifier;

  // parameter of a schematic action
  struct parameter {
    logic::variable var;
    logic::sort type;
  };

  struct effect {
    bool positive = true;
    std::vector<logic::proposition> fluents;
    std::vector<logic::atom> predicates;
  };

  // schematic or ground instantaneous action
  struct action {
    identifier name;
    std::vector<parameter> params;

    logic::formula precondition;
    std::vector<effect> effects;

    bool is_ground() const { return params.empty(); }
  };

  // schematic fluent (a.k.a. predicate)
  struct predicate {
    logic::relation name;
    std::vector<parameter> params;
  };

  // planning domain
  struct domain {
    std::vector<logic::custom_sort> types;
    std::vector<logic::proposition> fluents;
    std::vector<predicate> predicates;
    std::vector<action> actions;
  };

  // planning problem
  struct problem {
    std::unordered_map<logic::custom_sort, std::vector<logic::variable>> types;

    logic::formula init;
    logic::formula goal;
  };

}

#endif // PURPLE_PROBLEM_HPP
