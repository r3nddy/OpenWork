cmake_minimum_required(VERSION 3.15)
project(OpenWork VERSION 1.0 LANGUAGES CXX)

# Standar C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output folder 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Conan toolchain
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    message(STATUS "Conan toolchain ditemukan!")
    include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
else()
    message(STATUS "Conan toolchain belum ada.")
endif()

# Python3
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
message(STATUS "Python3 ditemukan: ${Python3_VERSION}")
message(STATUS "Python3 include : ${Python3_INCLUDE_DIRS}")
message(STATUS "Python3 libs    : ${Python3_LIBRARIES}")

# nlohmann_json (via Conan)
find_package(nlohmann_json REQUIRED)

# cpp-httplib (via Conan)
find_package(httplib REQUIRED)

# Source files
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/core/json_storage.cpp
    src/bridge/python_bridge.cpp
    src/services/job_service.cpp
    src/services/profile_service.cpp
    src/services/auth_service.cpp
    src/api/api_server.cpp
)

# Include & Link
target_include_directories(${PROJECT_NAME} PRIVATE
    ${Python3_INCLUDE_DIRS}
    include/
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Python3_LIBRARIES}
    nlohmann_json::nlohmann_json
    httplib::httplib
)