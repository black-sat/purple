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


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <purple/problem.hpp>
#include <purple/solver.hpp>

namespace logic = purple::logic;
namespace temporal = purple::temporal;

namespace py = pybind11;

namespace pypurple {

  using syntax = black::logic::LTLPFO;
  namespace internal = black_internal::logic;

  template<typename List>
  struct make_universal_variant;

  template<black::syntax_element ...Elements>
  struct make_universal_variant<black::syntax_list<Elements...>> {
    using type = std::variant<internal::element_type_of_t<syntax, Elements>...>;
  };

  using universal_variant_t = typename 
    make_universal_variant<internal::universal_fragment_t::list>::type;

  template<typename T>
  inline auto specialize(T&& t) {
    return t;
  }

  template<typename H>
    requires black::hierarchy<std::remove_cvref_t<H>>
  inline auto specialize(H&& h) {
    return h.match(
      [](auto x) {
        return universal_variant_t{x};
      }
    );
  }

  template<typename H>
    requires black::hierarchy<std::remove_cvref_t<H>>
  inline std::optional<universal_variant_t> specialize(std::optional<H> h) {
    if(!h)
      return std::nullopt;
    return std::optional{universal_variant_t{specialize(*h)}};
  }

  static logic::atom to_fo(temporal::atom a) {
    auto fo = a.to<black::logic::atom<black::logic::FO>>(); 
    if(!fo)
      throw py::type_error("Expected atom<FO>, given atom<LTLPFO>");
    
    return *fo;
  }

  static std::vector<logic::atom> to_fo(std::vector<temporal::atom> atoms) {
    std::vector<logic::atom> result;

    for(auto a : atoms) {
      result.push_back(to_fo(a));
    }

    return result;
  }

  static logic::formula to_fo(temporal::formula f) {
    auto fo = f.to<black::logic::formula<black::logic::FO>>(); 
    if(fo)
      return *fo;

    throw py::type_error("Expected formula<FO>, given formula<LTLPFO>");
  }

}

PYBIND11_MODULE(purple_plan, m) {

  using namespace pypurple;

  py::class_<purple::effect> effect(m, "effect");

  effect.def_readwrite("precondition", &purple::effect::precondition);
  effect.def_readwrite("fluents", &purple::effect::fluents);
  effect.def_readwrite("predicates", &purple::effect::predicates);
  effect.def_readwrite("positive", &purple::effect::positive);

  effect.def(py::init([](
    temporal::formula pre, std::vector<logic::proposition> fluents,
    std::vector<temporal::atom> predicates, bool pos
  ){
    return purple::effect(to_fo(pre), fluents, to_fo(predicates), pos);
  }), 
    py::arg("pre"), py::arg("fluents"), py::arg("predicates"), 
    py::arg("pos") = true
  );

  effect.def(py::init([](
    logic::alphabet &sigma, std::vector<logic::proposition> fluents,
    std::vector<temporal::atom> predicates, bool pos
  ){
    return purple::effect(sigma, fluents, to_fo(predicates), pos);
  }), 
    py::arg("sigma"), py::arg("fluents"), py::arg("predicates"), 
    py::arg("pos") = true
  );

  effect.def(py::init([](
    temporal::formula pre, logic::proposition f, bool pos
  ){
    return purple::effect(to_fo(pre), f, pos);
  }), 
    py::arg("pre"), py::arg("f"), py::arg("pos") = true
  );

  effect.def(py::init([](
    logic::proposition f, bool pos
  ){
    return purple::effect(f, pos);
  }), py::arg("f"), py::arg("pos") = true);

  effect.def(py::init([](
    temporal::atom a, bool pos
  ){
    return purple::effect(to_fo(a), pos);
  }), py::arg("a"), py::arg("pos") = true);

  py::class_<purple::state> state(m, "state");
  state.def(py::init([](
    std::vector<logic::proposition> fluents, 
    std::vector<temporal::atom> predicates
  ) {
    return purple::state{fluents, to_fo(predicates)};
  }));
  state.def_readwrite("fluents", &purple::state::fluents);
  state.def_readwrite("predicates", &purple::state::predicates);


  py::class_<purple::action> action(m, "action");
  action.def(py::init([](
    black::identifier name,
    std::vector<black::var_decl> params,
    temporal::formula precondition,
    std::vector<purple::effect> effects
  ) {
    return purple::action{name, params, to_fo(precondition), effects};
  }));

  action.def_readwrite("name", &purple::action::name);
  action.def_readwrite("params", &purple::action::params);
  action.def_readwrite("precondition", &purple::action::precondition);
  action.def_readonly("effects", &purple::action::effects);

  py::class_<purple::predicate> predicate(m, "predicate");
  predicate.def(py::init([](
    logic::relation name, std::vector<black::var_decl> params
  ){
    return purple::predicate{name, params};
  }));
  predicate.def("__call__", [](purple::predicate const&self, py::args args){
    std::vector<logic::variable> vars;
    for(auto arg : args) {
      try {
        vars.push_back(py::cast<logic::variable>(arg));
        continue;
      } catch(...) { }

      try {
        black::var_decl d = py::cast<black::var_decl>(arg);
        vars.push_back(d.variable());
        continue;
      } catch(...) { }

      throw py::type_error("Expected list of `variable` or `var_decl`");
    }

    return specialize(self(vars));
  });

  predicate.def_readwrite("name", &purple::predicate::name);
  predicate.def_readwrite("params", &purple::predicate::params);

  py::class_<purple::domain> domain(m, "domain");
  domain.def(py::init([](
    black::alphabet *sigma,
    std::vector<logic::named_sort> types,
    std::vector<logic::proposition> fluents,
    std::vector<purple::predicate> predicates,
    std::vector<purple::action> actions
  ){
    return purple::domain{sigma, types, fluents, predicates, actions};
  }));

  domain.def_readwrite("types", &purple::domain::types);
  domain.def_readwrite("fluents", &purple::domain::fluents);
  domain.def_readwrite("predicates", &purple::domain::predicates);
  domain.def_readwrite("actions", &purple::domain::actions);

  py::class_<purple::problem> problem(m, "problem");
  problem.def(py::init([](
    black::alphabet *sigma,
    std::vector<black::sort_decl> types,
    purple::state init,
    temporal::formula goal,
    temporal::formula trajectory
  ){
    return purple::problem{sigma, types, init, to_fo(goal), trajectory};
  }));
  
  problem.def_readwrite("types", &purple::problem::types);
  problem.def_readwrite("init", &purple::problem::init);
  problem.def_readwrite("goal", &purple::problem::goal);
  problem.def_readwrite("trajectory", &purple::problem::trajectory);

  py::class_<purple::plan::step> step(m, "step");
  step.def(py::init([](
    purple::action a, std::vector<logic::variable> args
  ){
    return purple::plan::step{a, args};
  }));
  step.def_readwrite("action", &purple::plan::step::action);
  step.def_readwrite("args", &purple::plan::step::args);

  py::class_<purple::plan> plan(m, "plan");
  plan.def(py::init([](std::vector<purple::plan::step> steps){
    return purple::plan{steps};
  }));
  plan.def_readonly("steps", &purple::plan::steps);

  py::class_<purple::solver> solver(m, "solver");
  solver.def(py::init<>());
  solver.def("solve", [](
    purple::solver &self, purple::domain const& d, purple::problem const& p)
  {
    return self.solve(d, p);
  });
  solver.def_property_readonly("solution", &purple::solver::solution);

}

