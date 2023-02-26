# 
# PURPLE - Expressive Automated Planner based on BLACK
# 
# (C) 2023 Nicola Gigante
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import typing
import warnings
import unified_planning as up

import pypurple as purple
import black_sat as black


credits = up.engines.Credits('PURPLE',
                  'Nicola Gigante',
                  'nicola.gigante@unibz.it',
                  'https://www.black-sat.org',
                  'MIT Licence',
                  'Classical Planner based on BLACK.',
                  'Classical Planner based on BLACK.\nSee: https://www.black-sat.org'
                )

class FormulasConverter(up.model.walkers.DagWalker):
    def __init__(self, sigma: black.alphabet):
        up.model.walkers.DagWalker.__init__(self)
        self._sigma = sigma        

    def convert(self, expression):
        """Converts the given expression."""
        return self.walk(expression)

    def walk_and(self, expression, args):
        return black.big_and(self._sigma, args)

    def walk_or(self, expression, args):
        return black.big_or(self._sigma, args)

    def walk_not(self, expression, args):
        assert len(args) == 1
        return ~args[0]

    def walk_implies(self, expression, args):
        assert len(args) == 2
        return black.implies(args[0], args[1])

    def walk_iff(self, expression, args):
        assert len(args) == 2
        return black.iff(args[0], args[1])

    def walk_fluent_exp(self, expression, args):
        fluent = expression.fluent()
        if fluent.arity == 0:
            if fluent.type.is_bool_type():
                return self._sigma.proposition(fluent.name)
            return self._sigma.variable(fluent.name)
        r = self._sigma.relation(fluent.name)
        return r(*args)

    def walk_param_exp(self, expression, args):
        assert len(args) == 0
        param = expression.parameter()
        if param.type.is_bool_type():
            return self._sigma.proposition(param.name)
        return self._sigma.variable(param.name)

    def walk_object_exp(self, expression, args):
        assert len(args) == 0
        return self._sigma.variable(expression.object().name)

    def walk_bool_constant(self, expression, args):
        assert len(args) == 0
        if expression.is_true():
            return self._sigma.top()
        else:
            return self._sigma.bottom()

class PurpleEngineImpl(
        up.engines.Engine,
        up.engines.mixins.OneshotPlannerMixin
    ):
    """ Implementation of the up-purple Engine. """

    def __init__(self, weight = None, heuristic = None, **options):
        up.engines.Engine.__init__(self)
        up.engines.mixins.OneshotPlannerMixin.__init__(self)
        self._sigma = black.alphabet()

    @property
    def name(self):
        return 'PURPLE'

    @staticmethod
    def supported_kind():
        supported_kind = up.model.ProblemKind()
        supported_kind.set_problem_class('ACTION_BASED') 
        supported_kind.set_typing('FLAT_TYPING') # ???
        supported_kind.set_conditions_kind('NEGATIVE_CONDITIONS')
        supported_kind.set_conditions_kind('DISJUNCTIVE_CONDITIONS')
        return supported_kind

    @staticmethod
    def supports(problem_kind: 'up.model.ProblemKind'):
        return problem_kind <= PurpleEngineImpl.supported_kind()

    @staticmethod
    def supports_plan(plan_kind: 'up.plans.PlanKind'):
        return plan_kind == up.plans.PlanKind.SEQUENTIAL_PLAN

    @staticmethod
    def satisfies(optimality_guarantee: up.engines.OptimalityGuarantee):
        return False

    @staticmethod
    def get_credits(**kwargs):
        return credits

    def _solve(self, problem, heuristic = None, timeout = None, os = None):
        assert isinstance(problem, up.model.Problem)
        if timeout is not None:
            warnings.warn('PURPLE does not support timeout.', UserWarning)
        if os is not None:
            warnings.warn('PURPLE does not support output stream.', UserWarning)
        if heuristic is not None:
            warnings.warn('PURPLE does not support heuristics', UserWarning)
        
        self._expr_manager = problem.environment.expression_manager
        self._objects = {}
        self._actions = {}

        # 1. convert problem to PURPLE
        domain, instance = self._convert_problem(problem)
        
        # 2. call solver
        slv = purple.solver()
        result = slv.solve(domain, instance)
        
        if result == True:
            # 3. convert plan to UP
            assert slv.solution
            plan = self._convert_plan(slv.solution)
            return up.engines.PlanGenerationResult(
                up.engines.PlanGenerationResultStatus.SOLVED_SATISFICING, 
                plan, self.name
            )
        elif result == False:
            return up.engines.PlanGenerationResult(
                up.engines.PlanGenerationResultStatus.UNSOLVABLE_PROVEN
            )
        else:
            return up.engines.PlanGenerationResult(
                up.engines.PlanGenerationResultStatus.UNSOLVABLE_INCOMPLETELY
            )
    
    def _convert_expr(self, expr):
        converter = FormulasConverter(self._sigma)
        return converter.convert(expr)

    def _convert_type(self, type_):
        if type_.is_bool_type():
            return None
        if type_.is_user_type():
            utype = typing.cast(up.model.types._UserType, type_)
            return self._sigma.named_sort(utype.name)
        raise NotImplemented

    def _convert_type_decl(self, problem, type_):
        if not type_.is_user_type():
            raise NotImplemented
        
        sort = self._convert_type(type_)
        elements = []
        for o in problem.objects(type_):
            e = self._sigma.variable(o.name)
            elements.append(e)
            self._objects[o.name] = o

        return self._sigma.sort_decl(sort, black.domain(elements))


    def _convert_param(self, param):
        return self._sigma.var_decl(
            self._sigma.variable(param.name), self._convert_type(param.type)
        )

    def _convert_fluent(self, fluent):
        if fluent.arity == 0:
            return self._sigma.proposition(fluent.name)
        return self._sigma.relation(fluent.name)

    def _convert_fluent_decl(self, fluent):
        assert fluent.arity > 0
        name = fluent.name
        params = [self._convert_param(p) for p in fluent.signature]
        return purple.predicate(self._sigma.relation(name), params)

    def _convert_effect(self, effect):
        condition = self._convert_expr(effect.condition)
        if condition is None:
            condition = self._sigma.top()
        fluents = []
        predicates = []
        if len(effect.fluent.args) == 0:
            fluents = [self._convert_expr(effect.fluent)]
        else:
            predicates = [self._convert_expr(effect.fluent)]
        assert effect.value.is_bool_constant()
        pos = effect.value.bool_constant_value()

        return purple.effect(condition, fluents, predicates, pos)

    def _convert_action(self, action):
        name = action.name
        params = [self._convert_param(p) for p in action.parameters]
        precondition = black.big_and(
            self._sigma, [self._convert_expr(e) for e in action.preconditions]
        )
        effects = [self._convert_effect(e) for e in action.effects]

        a = purple.action(name, params, precondition, effects)
        self._actions[name] = action
        return a

    def _convert_init(self, problem):
        fluents = []
        predicates = []
        for fluent, value in problem.initial_values.items():
            assert value.is_bool_constant()
            if value.bool_constant_value():
                if len(fluent.args) == 0:
                    fluents.append(self._convert_expr(fluent))
                else:
                    predicates.append(self._convert_expr(fluent))
        return purple.state(fluents, predicates)

    def _convert_problem(self, problem):
        types = [self._convert_type(t) for t in problem.user_types]
        fluents = [
            self._convert_fluent(f) for f in problem.fluents if f.arity == 0
        ]
        predicates = [
            self._convert_fluent_decl(f) \
                for f in problem.fluents if f.arity > 0
        ]
        actions = [self._convert_action(a) for a in problem.actions]

        domain = purple.domain(self._sigma, types, fluents, predicates, actions)

        type_decls = [
            self._convert_type_decl(problem, t) for t in problem.user_types
        ]
        init = self._convert_init(problem)
        goal = black.big_and(
            self._sigma, [self._convert_expr(e) for e in problem.goals]
        )

        instance = purple.problem(self._sigma, type_decls, init, goal)

        return (domain, instance)

    def _convert_back_variable(self, var):
        return self._expr_manager.ObjectExp(self._objects[str(var.name)])

    def _convert_plan(self, plan):
        actions = []
        for step in plan.steps:
            action = self._actions[str(step.action.name)]
            assert action is not None
            params = tuple([self._convert_back_variable(v) for v in step.args])
            actions.append(up.plans.ActionInstance(action, params))
        
        return up.plans.SequentialPlan(actions)
