#https://github.com/meemknight/cmakeSetup
#Version 1.0.0

cmake_minimum_required(VERSION 3.16)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(PBRTool)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)



# Define MY_SOURCES to be a list of all the source files for my game 
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../shared/*.cpp")

add_executable(PBRTool)

set_property(TARGET PBRTool PROPERTY CXX_STANDARD 17)

target_compile_definitions(PBRTool PUBLIC MINECRAFT_RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../resources/")

target_compile_definitions(PBRTool PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/") # This is useful to get an ASSETS_PATH in your IDE during development but you should comment this if you compile a release version and uncomment the next line
#target_compile_definitions(PBRTool PUBLIC RESOURCES_PATH="./resources/") # Uncomment this line to setup the ASSETS_PATH macro to the final assets directory when you share the game
#add_definitions(-DRESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/")

target_sources(PBRTool PRIVATE ${MY_SOURCES} ${SHARED_SOURCES})


if(MSVC) # If using the VS compiler...

	target_compile_definitions(PBRTool PUBLIC _CRT_SECURE_NO_WARNINGS)

	set_target_properties(PBRTool PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
	
	#set_property(TARGET PBRTool PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDebug<$<CONFIG:Debug>:Debug>")
	#set_property(TARGET PBRTool PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")

endif()

target_include_directories(PBRTool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../shared/")
target_include_directories(PBRTool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_include_directories(PBRTool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/gameLayer/")
target_include_directories(PBRTool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/platform/")


target_link_libraries(PBRTool PRIVATE glm glfw 
	glad stb_image stb_truetype gl2d imgui glui fastNoiseSIMD magic_enum)
