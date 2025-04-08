# Setup

## 1. Install Dependencies

Ensure the following dependencies are installed
<details>
  <summary> Linux (Ubuntu) </summary>

### 1.1 Linux (Ubuntu) Build Dependencies
Build dependencies for Linux using the `apt` package manager:
- git
- cmake
- build-essential
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
	- Note: alternatively it's possible to manually install the required packages instead of the SDK, see note below

`sudo apt install git cmake build-essential`

- For GLFW on Wayland:
	- libgl-dev
	- libwayland-dev
	- pkg-config
	- libxkbcommon-dev

`sudo apt install libgl-dev libwayland-dev pkg-config libxkbcommon-dev`

- For GLFW on X11:
	- libgl-dev
	- libwayland-dev
	- libx11-dev
	- libxrandr-dev
	- libxinerama-dev
	- libxcursor-dev
	- libxi-dev

`sudo apt install libgl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`


#### Note: If you don't want to install the Vulkan SDK you can install the following packages instead:
- libvulkan-dev
- vulkan-utility-libraries-dev
- vulkan-validationlayers (for debug builds)
- vulkan-tools (optional for verifying installation see below)

`sudo apt install libvulkan-dev vulkan-utility-libraries-dev vulkan-validationlayers`
</details>

<details>
  <summary> Windows Visual Studio </summary>

### 1.2 Windows Visual Studio Build Dependencies
On Windows the easiest way to build the project is to use Visual Studio:
- [Visual Studio](https://visualstudio.microsoft.com/downloads/) 2022 with:
	- C++ tools ("Desktop development with C++")
	- CMake tools (Individual components -> "C++ CMake tools for Windows")
- [Git](https://git-scm.com/downloads/win)
- [Python](https://www.python.org/downloads/) (needed to build glslang)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

</details>

___

## 2. (Optional) Verify Vulkan installation
It is recommended to verify that Vulkan is installed correctly before building the project.
To do this the Vulkan SDK provides a small program called `vkcube` which displays a rotating cube.
If everything works correctly you should see a rotating cube in a window.


<details>
  <summary> Linux (Ubuntu) </summary>

### 2.1 Linux
Note: If you did not install the SDK you need to install the `vulkan-tools` package first.
___

General information about the Vulkan installation can be queried with the `vulkaninfo` command.

To test if Vulkan works correctly run the following commands:
```bash
vkcube
```

Depending on the windowing system you're using you might need to provide the `--wsi` flag e.g.:

On X11:
```bash
vkcube --wsi xlib
```
or on Wayland:
```bash
vkcube --wsi wayland
```

</details>

<details>
  <summary> Windows Visual Studio </summary>


### 2.2 Windows
Open a terminal and run the command `vkcube` or search for the "Vulkan Cube" program in the start menu.

General information about the Vulkan installation can be queried with the `vulkaninfo` command or by opening the "Vulkan Configurator" and selecting `Tools->Vulkan Info` in the menu bar.

</details>

___

## 3. Clone repo

<details>
  <summary> Windows + Linux (Ubuntu) </summary>

### 3.1 Clone with submodules
When cloning the repository submodules need to be cloned as well.
To do that use the `--recursive` flag:
```bash
git clone --recursive https://github.com/Linus045/vulkan_raytracer.git vulkan_raytracer

# change into the directory for the next step
cd vulkan_raytracer
```

### 3.1 Manually clone submodules
If you have already cloned the repository without the submodules, you can initialize and clone the submodules with:
```bash
# clone the repository
git clone https://github.com/Linus045/vulkan_raytracer.git vulkan_raytracer

# change into the directory and load the submodules
cd vulkan_raytracer
git submodule update --init
```

### 3.2 Download zip archive and manually add submodules
If you've downloaded the zip archive you need to manually add the submodules since they're not included with GitHub's zip archive:
```bash
# Navigate into the directory where you extracted the zip archive
cd <path to extracted vulkan_raytracer zip directory>

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

</details>

___

## 4 Compile Project

<details>
  <summary> Linux (Ubuntu) </summary>

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


#### Optional Flags:
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

</details>
<details>
  <summary> Windows Visual Studio </summary>


### 4.2 Windows with Visual Studio

To use Visual Studio, ensure the `CMake Toolset` and the `C++ Development Toolset` are installed.

Open the project's root directory in Visual Studio (as a Folder) and open the `CMakeLists.txt` file located in the root directory.
Visual Studio will automatically detect the CMake project and configure the project for you.

To set the build type, simply change it in the drop-down to `Debug/Release` inside Visual Studio.

Now simply build the project.

</details>

___
## 5 Run programm

<details>
  <summary> Linux (Ubuntu) </summary>

### 5.1 Linux
The program looks for the compiled shader files in the `./shaders` directory which is relative to the executable (see `build/bin/shaders/`).

That means to run the program, you first need to navigate to the `<project root>/build/bin` directory and run the executable from there:
```bash
cd ./build/bin
./vulkan_raytracer
```

</details>
<details>
  <summary> Windows Visual Studio </summary>

### 5.2 Windows with Visual Studio
You might need to set the executable to `vulkan_raytracer`.

Afterwards simply press the `Start` button in Visual Studio.

</details>

___

# Display FPS Counter
To display the FPS add the following environment variable for your user:
```
VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_monitor
```
This will display the FPS on the left side in the window's titlebar.
