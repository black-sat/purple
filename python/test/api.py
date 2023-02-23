#!/bin/env python3.11

import black_sat
from black_sat import *
from pypurple import *

sigma = alphabet()

from_ = sigma.variable("from")
to = sigma.variable("to")
r = sigma.variable("r")

room = sigma.named_sort("room")

position = predicate(sigma.relation("position"), [r[room]])
connected = predicate(sigma.relation("connected"), [from_[room], to[room]])

go = \
  action( \
    "go", [from_[room], to[room]], \
    position(from_) & (connected(from_, to) | connected(to, from_)), \
    [effect(position(from_), False), effect(position(to))] \
  )

home_domain = domain(sigma, [room], [], [position, connected], [go])

kitchen = sigma.variable("kitchen")
toilet = sigma.variable("toilet")
bedroom = sigma.variable("bedroom")
coridor = sigma.variable("coridor")
balcony = sigma.variable("balcony")

rooms = black_sat.domain([kitchen, toilet, bedroom, coridor, balcony])

my_home = problem( \
  sigma, [sigma.sort_decl(room, rooms)], \
  state([], [ \
    position(balcony), \
    connected(kitchen, coridor), \
    connected(toilet, coridor), \
    connected(bedroom, coridor), \
    connected(bedroom, balcony) \
  ]), \
  position(kitchen) \
)

slv = solver()

result = slv.solve(home_domain, my_home)

if result == True:
  print("Plan found")
  p = slv.solution
  print(f"Steps: {len(p.steps)}")
  t = 0
  for s in p.steps:
    a = sigma.relation(s.action.name)
    print(f"t = {t}: {a(*s.args)}")
    t = t + 1


