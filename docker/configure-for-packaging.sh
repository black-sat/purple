#!/bin/bash

cmake \
    -DPython3_EXECUTABLE=/usr/bin/python3.10 \
    -DPYTHON_EXECUTABLE=/usr/bin/python3.10 \
    -DBUILD_SHARED_LIBS=NO \
    ..