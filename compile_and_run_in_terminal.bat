set /p build_type="Build type (Release|Debug):"
cmd /K "call compile_and_run.bat %build_type%"
