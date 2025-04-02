# Setup

#### 1. Ensure the following dependencies are installed:
- Vulkan SDK (https://vulkan.lunarg.com/sdk/home)
- CMake (Version 3.28+)
- Python (needed to build glslang tools)
- Optional: Ninja (for faster builds)

#### 2. Clone repo with submodules using `--recursive`
```bash
git clone --recursive https://github.com/Linus045/vulkan_experiments.git
```

Or to add update submodules after cloning:
```bash
git submodule update --init
```

If you've downloaded the zip archive you need to manually add the submodules since they're not included with GitHub's zip archive:
```bash
# Initialize a local git repository
git init

# Remove the empty directories
rmdir 3rdparty/*

# Add the submodules
git submodule add https://github.com/glfw/glfw.git 3rdparty/glfw
git submodule add https://github.com/icaven/glm.git 3rdparty/glm
git submodule add https://github.com/KhronosGroup/glslang.git 3rdparty/glslang
git submodule add https://github.com/tinyobjloader/tinyobjloader.git 3rdparty/tinyobjloader
git submodule add https://github.com/ocornut/imgui.git 3rdparty/imgui
```

#### 3.1 Compile Project (Custom setup or Linux)
Go back into the project root directory and compile
```
cmake -S . -B build
cmake --build build
```

If you have Ninja installed, you can use it to speed up the build process.
Add the generator flag `-G Ninja` to the cmake command:
```
cmake -S . -B build -G Ninja
cmake --build build
```

Optional additional flags:
```
Set the build type to Debug or Release:
 -DCMAKE_BUILD_TYPE="<Debug/Release>"

If using clangd as LSP
 -DCMAKE_EXPORT_COMPILE_COMMANDS="ON"

If you want to use clang as the compiler
 -DCMAKE_C_COMPILER=clang
 -DCMAKE_CXX_COMPILER=clang++


```

#### 3.2 Compile Project (Windows with Visual Studio)

To use Visual Studio, it is recommended to install the `CMake Toolset` and the `C++ Toolset`.
With the `CMake Toolset` installed, you can open the root directory in Visual Studio and open the `CMakeLists.txt` file in the root directory.
Visual Studio will automatically detect the CMake project and configure it for you.
You might need to set the 'Run' application to `vulkan_experiments`.


#### 4.1 Run programm (Linux)
```
./build/bin/vulkan_experiments
```

#### 4.2 Run programm (Windows)
Simply press the `Run` button in Visual Studio.


# Display FPS Counter
To display the FPS add the following environment variable for your user:
```
VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_monitor
```
This will display the FPS in the top left corner (title) of the window.


