#!/bin/env python3.11

import unified_planning as up

from unified_planning.shortcuts import *

env = up.environment.get_environment()
env.factory.add_engine('PURPLE', 'up_purple', 'PurpleEngineImpl')

Room = UserType("Room")

position = Fluent("position", BoolType(), r = Room)
connected = Fluent("connected", BoolType(), from_ = Room, to = Room)

go = InstantaneousAction("go", from_ = Room, to = Room)
from_ = go.parameter("from_")
to = go.parameter("to")
go.add_precondition(
    And(position(from_), Or(connected(from_, to), connected(to, from_)))
)
go.add_effect(position(from_), False)
go.add_effect(position(to), True)

problem = Problem("kitchen")

kitchen = Object("kitchen", Room)
toilet = Object("toilet", Room)
bedroom = Object("bedroom", Room)
coridor = Object("coridor", Room)
balcony = Object("balcony", Room)
        
problem.add_objects([kitchen, toilet, bedroom, coridor, balcony])

problem.add_fluent(position, default_initial_value=False)
problem.add_fluent(connected, default_initial_value=False)
problem.add_action(go)

problem.set_initial_value(position(balcony), True)
problem.set_initial_value(connected(kitchen, coridor), True)
problem.set_initial_value(connected(toilet, coridor), True)
problem.set_initial_value(connected(bedroom, coridor), True)
problem.set_initial_value(connected(bedroom, balcony), True)

problem.add_goal(position(kitchen))

with OneshotPlanner(name='PURPLE') as planner:
    result = planner.solve(problem)
    if result.status in unified_planning.engines.results.POSITIVE_OUTCOMES:
        print(f"PURPLE found this plan: {result.plan}")
    else:
        print("No plan found.")
