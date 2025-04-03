#!/bin/sh

generator='Unix Makefiles'
c_compiler=gcc
cpp_compiler=g++
processors_to_use=$(nproc)
build_mode="Release"
generate_compile_commands=""

if [[ -z $1 ]]; then
	# if the script is called without arguments, ask for user input
	read -p "Enter the build mode [Debug|Release|RelWithDebInfo] (default: $build_mode): " input
	if [[ ! -z $input ]]; then
		build_mode=$input
	fi

	read -p "Enter the generator (default: $generator): " input
	if [[ ! -z $input ]]; then
		generator=$input
	fi

	read -p "Enter the compiler for C and C++ seperated by a slash (default: $c_compiler/$cpp_compiler): " input
	if [[ ! -z $input ]]; then
		c_compiler=$(echo $input | cut -d'/' -f1)
		cpp_compiler=$(echo $input | cut -d'/' -f2)
	fi

	read -p "Do you want to generate compile_commands.json? [y/N]: " input
	if [[ $input = "y" ]] || [[ $input = "Y" ]]; then
		generate_compile_commands="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
	fi

	read -p "Enter the number of processors to use for building (default: $processors_to_use): " input
	if [[ ! -z $input ]]; then
		processors_to_use=$input
	fi
else
	# use this if the script is called with arguments (my personal setup)

	# echo "Please define CMAKE_BUILD_TYPE: ./compile_and_run.sh <build type> [mangohud]"
	# echo "Examples:"
	# echo "./compile_and_run.sh Debug            - run in debug mode"
	# echo "./compile_and_run.sh Debug mangohud   - run in debug mode with mangohud (if installed)"
	# echo "./compile_and_run.sh Release mangohud - run in release mode with mangohud (if installed)"
	# exit 1

	generator="Ninja"
	c_compiler="clang"
	cpp_compiler="clang++"
	generate_compile_commands="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

	build_mode=$1
	if [[ $2 = "MANGOHUD" ]] || [[ $2 = "mangohud" ]]|| [[ $2 = "Mangohud" ]]; then
		echo "Setting MANGOHUD environment variable"
		export MANGOHUD=1
	fi
fi


# check if mold is installed
command -v mold > /dev/null 2>&1
mold_available=$?

cmake_mold=""
if [[ $mold_available -eq 0 ]]; then
	cmake_mold="-DCMAKE_LINKER_TYPE=MOLD"
	echo "Found Mold, using it as the linker"
fi


current_dir=$(pwd)
cmake -B build -S . -G "$generator" \
	-DCMAKE_BUILD_TYPE="$build_mode" \
	-DCMAKE_C_COMPILER="$c_compiler" \
	-DCMAKE_CXX_COMPILER="$cpp_compiler" \
	"$cmake_mold" \
	"$generate_compile_commands" \
	&& \
	cmake --build build -- -j"$processors_to_use"\
	&& \
	cd ./build/bin \
	&& \
	./vulkan_experiments

cd $current_dir
