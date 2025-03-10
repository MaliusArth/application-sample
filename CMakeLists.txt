cmake_minimum_required(VERSION 3.10)
project(pipeline)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	message(FATAL_ERROR "Only MSVC is supported.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# message(STATUS "${CMAKE_CXX_FLAGS}")
string(REPLACE "/GR"   "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
# string(REPLACE "/MDd" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_compile_options(
	-WX         # treat warnings as errors
	-utf-8      # source files are UTF-8 encoded
	-GR-        # disable RTTI
	-EHa-       # disable exceptions support
	-Oi         # enable intrinsics
	-arch:AVX2  # optimize for AVX2 instruction set
)
add_compile_definitions(
	SIMDJSON_EXCEPTIONS=0
	_HAS_EXCEPTIONS=0  # disable STL exceptions
	_CRT_SECURE_NO_WARNINGS
	# _STATIC_CPPLIB
	UNICODE
	_UNICODE
)
add_link_options(
	-opt:ref           # remove unused code & data
	-incremental:no    # disable incremental builds
)

set(SHARED_SOURCE_FILES
	"utils.h"
	"pipeline.h"
	"pipeline.cpp"
	"deps/simdjson/simdjson.h"
	"deps/simdjson/simdjson.cpp"
)
add_executable(pipeline_cli "main_cli.cpp" ${SHARED_SOURCE_FILES})
add_executable(pipeline_gui "main_gui.cpp" ${SHARED_SOURCE_FILES})
target_sources(pipeline_gui PRIVATE
	"deps/imgui/imgui.h"
	"deps/imgui/imgui.cpp"
	"deps/imgui/imgui_internal.h"
	"deps/imgui/imgui_demo.cpp"
	"deps/imgui/imgui_draw.cpp"
	"deps/imgui/imgui_tables.cpp"
	"deps/imgui/imgui_widgets.cpp"
	"deps/imgui/imstb_rectpack.h"
	"deps/imgui/imstb_textedit.h"
	"deps/imgui/imstb_truetype.h"
	"deps/imgui/imconfig.h"
	"deps/imgui/backends/imgui_impl_dx11.cpp"
	"deps/imgui/backends/imgui_impl_win32.cpp"
	"deps/imnodes-0.5/imnodes.h"
	"deps/imnodes-0.5/imnodes.cpp"
	"deps/imnodes-0.5/imnodes_internal.h"
)
target_include_directories(pipeline_gui PRIVATE
	"deps/imgui"
	"deps/imgui/backends"
	"deps/imnodes-0.5"
	"$ENV{WindowsSdkDir}Include/um"
	"$ENV{WindowsSdkDir}Include/shared"
	"$ENV{DXSDK_DIR}Include"
)
target_link_directories(pipeline_gui PRIVATE "$ENV{DXSDK_DIR}/Lib/x86")
target_link_libraries(  pipeline_gui PRIVATE d3d11.lib d3dcompiler.lib)
target_compile_definitions(pipeline_gui PRIVATE IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
# cl.exe
# /TP
# /DWIN32
# /D_WINDOWS
# /W3
# /GR enable rtti
# /EHsc Enables standard C++ stack unwinding. Catches only standard C++ exceptions when you use catch(...) syntax. The compiler assumes that functions declared as extern "C" never throw a C++ exception.
# /MDd Use the debug multithread-specific and DLL-specific version of the runtime library. Defines _DEBUG, _MT, and _DLL. The linker uses the MSVCRTD.lib import library to resolve runtime symbols.
# /Zi separate PDB file
# /Ob0 Disables inline expansions (default under /Od)
# /Od disable optimizations (default)
# /RTC1 Enable fast checks (/RTCsu)
# /FS force using MS symbol server
# D:\\dev\\malius\\application-sample\\main_cli.cpp
