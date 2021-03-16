cmake_minimum_required(VERSION 3.5)

project(seirfilter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# change accordingly
include_directories("/usr/local/include")
include_directories("../orpp/orpp")
include_directories("./")

# nlopt has to be installed, change accordingly
find_library(LNLOPT libnlopt.so "/usr/local/lib" )

add_executable(seirfilter new.cpp rates.cpp)

target_link_libraries("seirfilter" ${LNLOPT} )


