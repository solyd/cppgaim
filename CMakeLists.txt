cmake_minimum_required(VERSION 3.10)
project(cppgaim)

set(CMAKE_CXX_STANDARD 17)

# find_package for ttf/image uses the files under this dir...
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(external_deps_dir ${CMAKE_SOURCE_DIR}/external)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
MESSAGE("sdl2 include dirs: ${SDL2_INCLUDE_DIRS}")

find_package(SDL2_image REQUIRED)
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
MESSAGE("sdl2 IMAGE include dirs: ${SDL2_IMAGE_INCLUDE_DIR}")

find_package(SDL2_ttf REQUIRED)
include_directories(${SDL2_TTF_INCLUDE_DIR})
MESSAGE("sdl2 TTF include dirs: ${SDL2_TTF_INCLUDE_DIR}")

find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})
MESSAGE("yaml cpp include dirs: ${YAML_CPP_INCLUDE_DIR}")

### TODO(solyd): Not sure if we need to include everything...
#add_subdirectory(${external_deps_dir}/spdlog)
#add_subdirectory(${external_deps_dir}/fmt)
#add_subdirectory(${external_deps_dir}/yaml-cpp)

include_directories(
        ${PROJECT_SOURCE_DIR}/src
        ${external_deps_dir}/spdlog/include
        ${external_deps_dir}/fmt/include
)

add_executable(cppgaim src/main.cpp src/utils/ScopeGuard.h src/sdl/Error.h src/sdl/helpers.h src/utils/custom_unique_ptr.h src/utils/FpsCounter.cpp src/utils/FpsCounter.h)
target_link_libraries(
        cppgaim
        ${SDL2_LIBRARIES}
        ${SDL2_IMAGE_LIBRARY}
        ${SDL2_TTF_LIBRARY}
        ${YAML_CPP_LIBRARIES}
)

set (source "${CMAKE_SOURCE_DIR}/resources")
set (destination "${CMAKE_CURRENT_BINARY_DIR}/resources")
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
        DEPENDS ${destination}
        COMMENT "symbolic link resources folder from ${source} => ${destination}"
)

set (source "${CMAKE_SOURCE_DIR}/config")
set (destination "${CMAKE_CURRENT_BINARY_DIR}/config")
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
        DEPENDS ${destination}
        COMMENT "symbolic link config folder from ${source} => ${destination}"
)
