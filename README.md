1. Clone repo with submodules
```
git clone --recursive https://github.com/Linus045/vulkan_experiments.git
```

Or to add update submodules after cloning:
```
git submodule update --init
```


2. Build glslang
```
cd 3rdparty/glslang
python3 update_glslang_sources.py
cmake -S . -B build
cmake --build build
```

3. Compile Project
Go back into the project root directory and compile
```
cd ../..

cmake -S . -B build
cmake --build build
```

4. Run programm
```
cd build/bin
./vulkan_experiments
```


# Dependencies
- Vulkan SDK (https://vulkan.lunarg.com/sdk/home)
- CMake
- Python (to build glslang tools)
