#!/bin/sh

if [[ -z $1 ]]; then
	echo "Please define CMAKE_BUILD_TYPE: ./compile_and_run.sh <build type> [mangohud]"
	echo "Examples:"
	echo "./compile_and_run.sh Debug            - run in debug mode"
	echo "./compile_and_run.sh Debug mangohud   - run in debug mode with mangohud (if installed)"
	echo "./compile_and_run.sh Release mangohud - run in release mode with mangohud (if installed)"
	exit 1
fi

if [[ $2 = "MANGOHUD" ]] || [[ $2 = "mangohud" ]]|| [[ $2 = "Mangohud" ]]; then
	echo "Setting MANGOHUD environment variable"
	export MANGOHUD=1
fi

# moved to cmake build task
# glslc ./shaders/shader.vert -o ./shaders/vert.spv && \
# glslc ./shaders/shader.frag -o ./shaders/frag.spv && \


current_dir=$(pwd)

cmake -B build -S . -G Ninja \
    -DCMAKE_BUILD_TYPE="$1" \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
	-DCMAKE_LINKER_TYPE=MOLD \
    -DCMAKE_EXPORT_COMPILE_COMMANDS="ON" && \
cmake --build build && \
cd ./build/bin &&  \
./vulkan_experiments

cd $current_dir
