# 
# PURPLE - Expressive Automated Planner based on BLACK
# 
# (C) 2022 Nicola Gigante
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


option(PURPLE_ENABLE_PYTHON_BINDINGS "Enable Python bindings" ON)

if(PURPLE_ENABLE_PYTHON_BINDINGS)

  find_package(Python3 COMPONENTS Interpreter Development)
  find_package(pybind11 QUIET)

  message(STATUS "Python sitelib: ${Python3_SITELIB}")

  if(NOT Python3_FOUND)
    message(STATUS "Python NOT found. Disabling Python bindings...") 
  endif()

  if(NOT pybind11_FOUND)
    message(STATUS "PyBind11 NOT found. Disabling Python bindings...")
  endif()

  if(Python3_FOUND AND pybind11_FOUND)

    message(STATUS "Python and PyBind11 found! Enabling Python bindings...")

    pybind11_add_module(pypurple EXCLUDE_FROM_ALL src/bindings.cpp)

    target_link_libraries(pypurple PUBLIC purple)
    set_target_properties(pypurple PROPERTIES OUTPUT_NAME purple_plan)

    # install(TARGETS pypurple DESTINATION "${Python3_SITELIB}")

    get_target_property(
      PURPLE_INCLUDE_DIRS 
      purple
      INCLUDE_DIRECTORIES
    )
    
    get_target_property(
      BLACK_INCLUDE_DIRS 
      black::black 
      INTERFACE_INCLUDE_DIRECTORIES
    )

    configure_file(setup.py.in setup.py)
  
  endif()
    
endif()

