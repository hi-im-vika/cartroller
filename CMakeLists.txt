cmake_minimum_required(VERSION 3.10)
project(cartroller)

set(CMAKE_CXX_STANDARD 17)
set(OpenGL_GL_PREFERENCE LEGACY)
add_executable(cartroller src/CCartroller.cpp src/Mesh.cpp)

add_subdirectory(vendor)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE cppbase)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE imgui::imgui)

find_package(OpenGL REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OpenGL::GL)

find_package(GLEW REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GLEW::GLEW)

# Or use the header-only version
find_package(glm CONFIG REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glm::glm-header-only)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE spdlog::spdlog_header_only)