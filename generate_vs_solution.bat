@echo off

cmake "-B" "build" "-S" "." "-G" "Visual Studio 17 2022" "-DCMAKE_BUILD_TYPE="%~1""
