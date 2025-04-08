@echo off

IF [%1] == [] (
  echo "Please define CMAKE_BUILD_TYPE: ./compile_and_run.bat <build type> [mangohud]"
  echo "Examples:"
  echo "./compile_and_run.bat Debug            - run in debug mode"
  echo "./compile_and_run.bat Debug mangohud   - run in debug mode with mangohud (if installed)"
  echo "./compile_and_run.bat Release mangohud - run in release mode with mangohud (if installed)"

  exit /b 1
)


IF ["%~2"] == ["MANGOHUD"] GOTO :exportmangohud
IF ["%~2"] == ["mangohud"] GOTO :exportmangohud
IF ["%~2"] == ["Mangohud"] GOTO :exportmangohud
GOTO :runcmake

:exportmangohud
echo "Setting MANGOHUD environment variable"
export "MANGOHUD=1"


:runcmake

cmake "-B" "build" "-S" "." "-G" "Visual Studio 17 2022" "-DCMAKE_BUILD_TYPE="%~1"" && cmake "--build" "build" --config "%~1" && PUSHD "build/bin" && call "vulkan_raytracer.exe"
POPD
