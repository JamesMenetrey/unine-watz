cmake_minimum_required (VERSION 3.2)
project (some_project C)

# https://cmake.org/Wiki/CMake_Useful_Variables
set (CMAKE_TOOLCHAIN_FILE CMakeToolchain.txt)

include(GNUInstallDirs)

add_compile_options (-Wall)
#add_compile_options (
#	-Wall -Wbad-function-cast -Wcast-align
#	-Werror-implicit-function-declaration -Wextra
#	-Wfloat-equal -Wformat-nonliteral -Wformat-security
#	-Wformat=2 -Winit-self -Wmissing-declarations
#	-Wmissing-format-attribute -Wmissing-include-dirs
#	-Wmissing-noreturn -Wmissing-prototypes -Wnested-externs
#	-Wpointer-arith -Wshadow -Wstrict-prototypes
#	-Wswitch-default -Wunsafe-loop-optimizations
#	-Wwrite-strings -Werror -fPIC
#)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

file(GLOB dirs *)
foreach(dir ${dirs})
	if(EXISTS ${dir}/CMakeLists.txt)
		add_subdirectory(${dir})
	endif()
endforeach()

set (SRC host/main.c)

add_executable (${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME}
			   PRIVATE ta/include
			   PRIVATE include)

target_link_libraries (${PROJECT_NAME} PRIVATE teec)

install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
