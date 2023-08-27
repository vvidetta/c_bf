# c_bf

The present repo contains an interpreter for the bf language. It was written as a Proof-of-Concept for unit testing C using C++ (Googletest).

## Building

Building requires Visual Studio 2019 or later.

Before opening the Visual Studio Solution, you need to have Googletest installed on your system. Set up the following environment variables:
* GOOGLETEST_INCLUDE_DIR should point to the GoogleTest include directory.
* GOOGLETEST_LIB_DIR should point to a directory containing pre-built GoogleTest binaries. The solution will search for gtest.lib and gmock.lib in %GOOGLETEST_LIB_DIR%\Debug or %GOOGLETEST_LIB_DIR%\Release depending on the selected build configuration.
