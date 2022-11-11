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

#include <purple/problem.hpp>
#include <purple/solver.hpp>

#include <black/logic/prettyprint.hpp>

#include <iostream>

int main() {
  black::alphabet sigma;
  auto from = sigma.variable("from");
  auto to = sigma.variable("to");
  auto r = sigma.variable("r");
  
  auto room = sigma.named_sort("room");
  
  purple::predicate position = {
    sigma.relation("position"), {r[room]}
  };

  purple::predicate connected = {
    sigma.relation("connected"), {from[room], to[room]}
  };

  purple::action go = {
    "go",
    {from[room], to[room]},
    position(from) && (connected(from, to) || connected(to, from)),
    {{position(from), false}, {position(to)}}
  };

  purple::domain home_domain = {
    sigma,
    {room},
    {},
    {position, connected},
    {go}
  };

  auto kitchen = sigma.variable("kitchen");
  auto toilet = sigma.variable("toilet");
  auto bedroom = sigma.variable("bedroom");
  auto coridor = sigma.variable("coridor");
  auto balcony = sigma.variable("balcony");

  auto rooms = black::make_domain({
    kitchen,
    toilet,
    bedroom,
    coridor,
    balcony
  });

  purple::problem my_home = {
    sigma,
    {sigma.sort_decl(room, rooms)},
    {
      {},
      {
        position(balcony),
        connected(kitchen, coridor),
        connected(toilet, coridor),
        connected(bedroom, coridor),
        connected(bedroom, balcony)
      }
    },
    position(kitchen)
  };

  purple::solver slv;

  std::cout << "Solving problem...\n";

  purple::tribool result = slv.solve(home_domain, my_home);

  if(result == purple::tribool::undef)
    std::cout << "Unknown result\n";

  if(result == true) {
    std::cout << "Plan found\n";
    purple::plan p = *slv.solution();
    
    for(size_t t = 0; t < p.steps.size(); ++t) {
      purple::plan::step step = p.steps[t];
      
      black::relation action = sigma.relation(step.action.name);

      std::cout << " t = " << t << ": " << to_string(action(step.args)) << "\n";
    }
  }

  if(result == false)
    std::cout << "Plan not found\n";

  return 0;
}
