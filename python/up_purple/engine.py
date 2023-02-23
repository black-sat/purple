# Copyright 2021 AIPlan4EU project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


import sys
import warnings
import unified_planning as up
import unified_planning.plans
import unified_planning.engines
import unified_planning.engines.mixins
from unified_planning.model import ProblemKind
from unified_planning.engines import PlanGenerationResultStatus, ValidationResult, ValidationResultStatus, Credits
#from up_tamer.converter import Converter
from fractions import Fraction
from typing import IO, Callable, Optional, Dict, List, Tuple, Union, Set, cast

import pypurple as purple
import black_sat as black


credits = Credits('PURPLE',
                  'Nicola Gigante',
                  'nicola.gigante@unibz.it',
                  'https://www.black-sat.org',
                  'MIT Licence',
                  'Classical Planner based on BLACK.',
                  'Classical Planner based on BLACK.\nSee: https://www.black-sat.org'
                )

class EngineImpl(
        up.engines.Engine,
        up.engines.mixins.OneshotPlannerMixin
    ):
    """ Implementation of the up-purple Engine. """

    def __init__(self, weight: Optional[float] = None,
                 heuristic: Optional[str] = None, **options):
        up.engines.Engine.__init__(self)
        up.engines.mixins.OneshotPlannerMixin.__init__(self)
        self._sigma = black.alphabet()

    @property
    def name(self) -> str:
        return 'PURPLE'

    @staticmethod
    def supported_kind() -> ProblemKind:
        supported_kind = ProblemKind()
        supported_kind.set_problem_class('ACTION_BASED') # type: ignore
        supported_kind.set_typing('FLAT_TYPING') # type: ignore
        supported_kind.set_conditions_kind('NEGATIVE_CONDITIONS') # type: ignore
        supported_kind.set_conditions_kind('DISJUNCTIVE_CONDITIONS')
        return supported_kind

    @staticmethod
    def supports(problem_kind: 'up.model.ProblemKind') -> bool:
        return problem_kind <= EngineImpl.supported_kind()

    @staticmethod
    def supports_plan(plan_kind: 'up.plans.PlanKind') -> bool:
        return plan_kind == up.plans.PlanKind.SEQUENTIAL_PLAN

    @staticmethod
    def satisfies(optimality_guarantee: up.engines.OptimalityGuarantee) -> bool:
        return False

    @staticmethod
    def get_credits(**kwargs) -> Optional[up.engines.Credits]:
        return credits

    def _solve(self, problem: 'up.model.AbstractProblem',
               heuristic: Optional[Callable[["up.model.state.ROState"], Optional[float]]] = None,
               timeout: Optional[float] = None,
               output_stream: Optional[IO[str]] = None) -> 'up.engines.results.PlanGenerationResult':
        assert isinstance(problem, up.model.Problem)
        if timeout is not None:
            warnings.warn('PURPLE does not support timeout.', UserWarning)
        if output_stream is not None:
            warnings.warn('PURPLE does not support output stream.', UserWarning)
        if heuristic is not None:
            warnings.warn('PURPLE does not support heuristics', UserWarning)
        
        # 1. convert problem to PURPLE
        domain, problem = None
        
        # 2. call solver
        slv = purple.solver()
        result = slv.solve(domain, problem)
        
        if result == True:
            # 3. convert plan to UP
            plan = None
            return up.engines.PlanGenerationResult(
                PlanGenerationResultStatus.SOLVED_SATISFICING, plan, self.name
            )
        elif result == False:
            return up.engines.PlanGenerationResult(
                PlanGenerationResultStatus.UNSOLVABLE_PROVEN
            )
        else:
            return up.engines.PlanGenerationResult(
                PlanGenerationResultStatus.UNSOLVABLE_INCOMPLETELY
            )
