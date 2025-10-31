cmake_minimum_required(VERSION 3.20)
project(TowerDefense CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- vcpkg toolchain si non passé en ligne de commande
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# --- Sources
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(TowerDefense ${SRC_FILES})
target_include_directories(TowerDefense PRIVATE include)

# --- Warnings (GCC/Clang)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(TowerDefense PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- SFML via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SFML REQUIRED sfml-all)

message(STATUS "Using SFML via pkg-config")
target_link_libraries(TowerDefense PRIVATE ${SFML_LIBRARIES})
target_include_directories(TowerDefense PRIVATE ${SFML_INCLUDE_DIRS})

# --- Tests (Catch2 v3)
enable_testing()
add_executable(tests tests/test_sanity.cpp)
target_include_directories(tests PRIVATE include)
find_package(Catch2 3 REQUIRED)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
add_test(NAME unit COMMAND tests)

# --- Assets: lien symbolique vers ../assets (Linux/macOS)
#     Ainsi, l'exécutable lancé depuis build/ voit "assets/..."
if(UNIX AND NOT APPLE)
  add_custom_command(TARGET TowerDefense POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:TowerDefense>/assets
    COMMENT "Symlink assets -> build/assets"
  )
elseif(APPLE)
  add_custom_command(TARGET TowerDefense POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:TowerDefense>/assets
    COMMENT "Symlink assets -> build/assets (macOS)"
  )
endif()
