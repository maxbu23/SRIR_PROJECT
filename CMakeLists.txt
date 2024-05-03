cmake_minimum_required(VERSION 3.10)
project(MyProject CXX)

set(CMAKE_CXX_COMPILER "/opt/nfs/mpich-3.2/bin/mpicxx")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb")

file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

add_executable(run ${SOURCES} ${HEADERS})

add_custom_target(out
  COMMAND /opt/nfs/mpich-3.2/bin/mpiexec -n 4 ./run
  DEPENDS run
  USES_TERMINAL
)
