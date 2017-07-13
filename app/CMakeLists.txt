cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
add_library(image-graph-processing SHARED src/main/cpp/ImageGraphProcessing.cpp)
target_link_libraries( image-graph-processing
                       android
                       jnigraphics
                       log
                       m
                      )