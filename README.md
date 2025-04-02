# Setup

## 1. Install Dependencies

Ensure the following dependencies are installed
- Git
- Vulkan SDK (https://vulkan.lunarg.com/sdk/home)
	- alternatively for Linux (Ubuntu) the following packages:
		- libvulkan-dev
		- vulkan-utility-libraries-dev
		- vulkan-validationlayers
- CMake (Version 3.28+vulkan-extra-layers
- Python (needed to build glslang tools)
- Optional (Windows only):  Visual Studio with:
	- C++ Development tools
	- CMake Build tools

## 2. (Optional) Verify Vulkan installation
To test if vulkan works correctly you can optionally install the
`vulkan-tools` package and run the following commands:
```bash
vkcube
```

Depending on the windowing system you're using you might need to provide the `--wsi` flag e.g.:
```bash
vkcube --wsi xlib
```
or:
```bash
vkcube --wsi wayland
```

#### Note
More info about the vulkan installation can then be seen with the `vulkaninfo` command.

## 3. Clone repo

### 3.1 Clone with submodules
When cloning the repository submodules need to be cloned as well.
To do that use the `--recursive` flag:
```bash
git clone --recursive https://github.com/Linus045/vulkan_experiments.git vulkan_experiments

# change into the directory for the next step
cd vulkan_experiments
```

### 3.1 Manually clone submodules
If you have already cloned the repository without the submodules, you can initialize and clone the submodules with:
```bash
# clone the repository
git clone https://github.com/Linus045/vulkan_experiments.git vulkan_experiments

# change into the directory and load the submodules
cd vulkan_experiments
git submodule update --init
```

### 3.2 Download zip archive and manually add submodules
If you've downloaded the zip archive you need to manually add the submodules since they're not included with GitHub's zip archive:
```bash
# Navigate into the directory where you extracted the zip archive
cd <path to extracted vulkan_experiments zip directory>

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

## 4 Compile Project

### 4.1 Linux

#### 4.1.1 Configure Project
Configure the project with CMake:
```
# Debug build
cmake -S . -B build -DCMAKE_BUILD_TYPE="Debug"

# Release build (with debug info)
cmake -S . -B build -DCMAKE_BUILD_TYPE="RelWithDebInfo"

# Release build
cmake -S . -B build -DCMAKE_BUILD_TYPE="Release"
```
##### Note:
If the `CMAKE_BUILD_TYPE` is not provided it will default to `RelWithDebInfo`.
____
Here are some optional additional flags that might be useful.


##### Optional Flags:
Set the build type to Debug or Release:
```
 -DCMAKE_BUILD_TYPE="<Debug|Release|RelWithDebInfo>"
```

If you're using clangd or other tools that rely on the `compile_commands.json` file add:

`-DCMAKE_EXPORT_COMPILE_COMMANDS="ON"`


If you want to use clang as the compiler add:

`-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++`

Depending on your windowing system and installed packages you can compile GLFW for only wayland or x11 by disabling the other:
```
# Disable X11 support
-DGLFW_BUILD_X11=OFF
# or disable Wayland support
-DGLFW_BUILD_WAYLAND=OFF
```

##### Example:
E.g. to configure for `Debug` mode with `compile_commands.json` without X11 support:

`cmake -S . -B build -DCMAKE_BUILD_TYPE="Debug" -DCMAKE_EXPORT_COMPILE_COMMANDS="ON" -DGLFW_BUILD_X11=OFF`

____
#### 4.1.2 Compile Project
Compile the project using cmake's `--build` flag:
```bash
cmake --build build
```
Optionally to use all available cores for faster compilation:
```bash
cmake --build build -- -j $(nproc)
```


### 4.2 Windows with Visual Studio

To use Visual Studio, ensure the `CMake Toolset` and the `C++ Development Toolset` are installed.

Open the project's root directory in Visual Studio (as a Folder) and open the `CMakeLists.txt` file located in the root directory.
Visual Studio will automatically detect the CMake project and configure the project for you.

To set the build type, simply change it in the drop-down to `Debug/Release` inside Visual Studio.

Now simply build the project.


## 5 Run programm
### 5.1 Linus
The program looks for the compiled shader files in the `./shaders` directory which is relative to the executable (see `build/bin/shaders/`).

That means to run the program, you first need to navigate to the `<project root>/build/bin` directory and run the executable from there:
```bash
cd ./build/bin
./vulkan_experiments
```


### 5.2 Windows with Visual Studio
You might need to set the executable to `vulkan_experiments`.

Afterwards simply press the `Start` button in Visual Studio.


# Display FPS Counter
To display the FPS add the following environment variable for your user:
```
VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_monitor
```
This will display the FPS on the left side in the window's titlebar.
