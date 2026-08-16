cmake_minimum_required(VERSION 4.1.0)
project(realtime-raytracer)

# We will let the subdirectories specify their C++ standard (Walnut requires 17, RayTracer requires 26)

add_subdirectory(Walnut)
add_subdirectory(RayTracer)
