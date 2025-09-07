cmake_minimum_required(VERSION 3.20)
project(TowerDefense CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg toolchain (optionnel si configuré globalement)
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# Sources
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    src/*.cpp
)

add_executable(TowerDefense ${SRC_FILES})
target_include_directories(TowerDefense PRIVATE include)

# SFML
# --- SFML (compatible 3.x et 2.6) ---
# On essaie d'abord SFML 3 (c'est ce que vcpkg t'a installé)
find_package(SFML 3 QUIET COMPONENTS System Window Graphics Audio)

if(SFML_FOUND)
    message(STATUS "Using SFML ${SFML_VERSION} (targets: SFML::...)")
    target_link_libraries(TowerDefense PRIVATE
        SFML::System SFML::Window SFML::Graphics SFML::Audio
    )
else()
    # Fallback pour SFML 2.6 (cibles en minuscules)
    find_package(SFML 2.6 REQUIRED COMPONENTS system window graphics audio)
    message(STATUS "Using SFML ${SFML_VERSION} (targets: sfml-...)")
    target_link_libraries(TowerDefense PRIVATE
        sfml-system sfml-window sfml-graphics sfml-audio
    )
endif()

# Tests (Catch2)
enable_testing()
add_executable(tests
    tests/test_sanity.cpp
)
target_include_directories(tests PRIVATE include)
find_package(Catch2 3 REQUIRED)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
add_test(NAME unit COMMAND tests)
