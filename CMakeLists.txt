
cmake_minimum_required(VERSION 3.0)
enable_language(CXX)
project(contecdio CXX)
add_executable( contecdio inout.cpp
  )
target_link_libraries (contecdio cdio )
set_property(TARGET contecdio PROPERTY CXX_STANDARD 14)

install(TARGETS contecdio DESTINATION bin)
