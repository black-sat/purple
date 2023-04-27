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

cmake_minimum_required(VERSION 3.10)

project(
  purple 
  VERSION 0.1.6
  LANGUAGES C CXX
  DESCRIPTION "PURPLE (Expressive Automated Planner based on BLACK)"
)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

set(CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake"
    ${CMAKE_MODULE_PATH})

include(EnableWarnings) # Common warnings to enable or disable
include(GNUInstallDirs) # Correct and portable installation paths
include(CheckIPOSupported) # Check if we can do LTO

# dependencies
find_package(black 0.10.3 REQUIRED)

#
# Fix the RPATH for the installation 
#
if(APPLE)
  set(CMAKE_MACOSX_RPATH 1)
  list(
    APPEND CMAKE_INSTALL_RPATH 
    ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
  )
endif()


set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DBLACK_ASSERT_DISABLE")
check_ipo_supported(RESULT IPO_SUPPORTED LANGUAGES C CXX)

if(IPO_SUPPORTED)
  message(STATUS "Link-Time Optimization enabled")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE YES)
endif()

# Black library and frontend
add_subdirectory(src/lib)
add_subdirectory(python)
add_subdirectory(tests)

