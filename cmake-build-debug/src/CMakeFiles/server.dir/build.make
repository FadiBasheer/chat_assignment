# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.21

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/charlie/work/clion-2021.2.3/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /home/charlie/work/clion-2021.2.3/bin/cmake/linux/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/charlie/work/Term2/Assignment_4_v2

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug

# Include any dependencies generated for this target.
include src/CMakeFiles/server.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/CMakeFiles/server.dir/compiler_depend.make

# Include the progress variables for this target.
include src/CMakeFiles/server.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/server.dir/flags.make

src/CMakeFiles/server.dir/common.c.o: src/CMakeFiles/server.dir/flags.make
src/CMakeFiles/server.dir/common.c.o: ../src/common.c
src/CMakeFiles/server.dir/common.c.o: src/CMakeFiles/server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object src/CMakeFiles/server.dir/common.c.o"
	cd /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/src && $(CMAKE_COMMAND) -E __run_co_compile --tidy="clang-tidy;-checks=*,-llvmlibc-restrict-system-libc-headers,-cppcoreguidelines-init-variables,-clang-analyzer-security.insecureAPI.strcpy,-concurrency-mt-unsafe,-android-cloexec-accept,-android-cloexec-dup,-google-readability-todo,-cppcoreguidelines-avoid-magic-numbers,-readability-magic-numbers,-cert-dcl03-c,-hicpp-static-assert,-misc-static-assert,-altera-struct-pack-align,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling;--quiet;--extra-arg-before=--driver-mode=gcc" --source=/home/charlie/work/Term2/Assignment_4_v2/src/common.c -- /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/server.dir/common.c.o -MF CMakeFiles/server.dir/common.c.o.d -o CMakeFiles/server.dir/common.c.o -c /home/charlie/work/Term2/Assignment_4_v2/src/common.c

src/CMakeFiles/server.dir/common.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/server.dir/common.c.i"
	cd /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/charlie/work/Term2/Assignment_4_v2/src/common.c > CMakeFiles/server.dir/common.c.i

src/CMakeFiles/server.dir/common.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/server.dir/common.c.s"
	cd /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/charlie/work/Term2/Assignment_4_v2/src/common.c -o CMakeFiles/server.dir/common.c.s

src/CMakeFiles/server.dir/server.c.o: src/CMakeFiles/server.dir/flags.make
src/CMakeFiles/server.dir/server.c.o: ../src/server.c
src/CMakeFiles/server.dir/server.c.o: src/CMakeFiles/server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object src/CMakeFiles/server.dir/server.c.o"
	cd /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/src && $(CMAKE_COMMAND) -E __run_co_compile --tidy="clang-tidy;-checks=*,-llvmlibc-restrict-system-libc-headers,-cppcoreguidelines-init-variables,-clang-analyzer-security.insecureAPI.strcpy,-concurrency-mt-unsafe,-android-cloexec-accept,-android-cloexec-dup,-google-readability-todo,-cppcoreguidelines-avoid-magic-numbers,-readability-magic-numbers,-cert-dcl03-c,-hicpp-static-assert,-misc-static-assert,-altera-struct-pack-align,-clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling;--quiet;--extra-arg-before=--driver-mode=gcc" --source=/home/charlie/work/Term2/Assignment_4_v2/src/server.c -- /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT src/CMakeFiles/server.dir/server.c.o -MF CMakeFiles/server.dir/server.c.o.d -o CMakeFiles/server.dir/server.c.o -c /home/charlie/work/Term2/Assignment_4_v2/src/server.c

src/CMakeFiles/server.dir/server.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/server.dir/server.c.i"
	cd /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/charlie/work/Term2/Assignment_4_v2/src/server.c > CMakeFiles/server.dir/server.c.i

src/CMakeFiles/server.dir/server.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/server.dir/server.c.s"
	cd /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/src && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/charlie/work/Term2/Assignment_4_v2/src/server.c -o CMakeFiles/server.dir/server.c.s

# Object files for target server
server_OBJECTS = \
"CMakeFiles/server.dir/common.c.o" \
"CMakeFiles/server.dir/server.c.o"

# External object files for target server
server_EXTERNAL_OBJECTS =

src/server: src/CMakeFiles/server.dir/common.c.o
src/server: src/CMakeFiles/server.dir/server.c.o
src/server: src/CMakeFiles/server.dir/build.make
src/server: /usr/lib/x86_64-linux-gnu/libm.so
src/server: /usr/local/lib/libdc_error.so
src/server: /usr/local/lib/libdc_posix.so
src/server: /usr/local/lib/libdc_util.so
src/server: /usr/local/lib/libdc_fsm.so
src/server: /usr/local/lib/libdc_application.so
src/server: /usr/lib/x86_64-linux-gnu/libncurses.so
src/server: src/CMakeFiles/server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable server"
	cd /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/server.dir/build: src/server
.PHONY : src/CMakeFiles/server.dir/build

src/CMakeFiles/server.dir/clean:
	cd /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/src && $(CMAKE_COMMAND) -P CMakeFiles/server.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/server.dir/clean

src/CMakeFiles/server.dir/depend:
	cd /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/charlie/work/Term2/Assignment_4_v2 /home/charlie/work/Term2/Assignment_4_v2/src /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/src /home/charlie/work/Term2/Assignment_4_v2/cmake-build-debug/src/CMakeFiles/server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/CMakeFiles/server.dir/depend

