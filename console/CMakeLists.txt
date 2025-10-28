cmake_minimum_required(VERSION 3.16)

# cweb CLI - builds the command line interface for the cweb framework.
project(cweb_cli VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(GNUInstallDirs)

# Collect all console sources.
file(GLOB CLI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
if(NOT CLI_SOURCES)
    message(FATAL_ERROR "Keine Quell-Dateien im console/-Verzeichnis gefunden")
endif()

# Framework lookup: prefer installed package, fallback to local source if requested
set(CWEB_FRAMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../framework")
option(CWEB_CLI_USE_LOCAL_FRAMEWORK "Force building against the local framework sources" OFF)

# Try to locate an installed libcweb first
find_package(cweb 0.3.0 CONFIG QUIET)

if(TARGET cweb::cweb)
    message(STATUS "Using installed libcweb package")
else()
    if(CWEB_CLI_USE_LOCAL_FRAMEWORK)
        if(EXISTS "${CWEB_FRAMEWORK_DIR}/CMakeLists.txt")
            message(STATUS "Installed libcweb not found; building against local framework: ${CWEB_FRAMEWORK_DIR}")
            add_subdirectory("${CWEB_FRAMEWORK_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/framework-build" EXCLUDE_FROM_ALL)
        else()
            message(FATAL_ERROR "Local framework sources not found at ${CWEB_FRAMEWORK_DIR} and no installed libcweb detected")
        endif()
    else()
        message(FATAL_ERROR "libcweb (>= 0.3.0) not found. Install the developer package or rerun with -DCWEB_CLI_USE_LOCAL_FRAMEWORK=ON")
    endif()
endif()

add_executable(cweb_cli ${CLI_SOURCES})
set_target_properties(cweb_cli PROPERTIES OUTPUT_NAME "cweb")

# Ensure POSIX/GNU feature-test macros so strdup and other POSIX functions are declared
target_compile_definitions(cweb_cli PRIVATE
    _POSIX_C_SOURCE=200809L
    _DEFAULT_SOURCE=1
)

# Prefer the exported interface target if the alias exists.
if(TARGET cweb::cweb)
    target_link_libraries(cweb_cli PRIVATE cweb::cweb)
else()
    # Fallback in case the alias is not exported for some reason.
    target_link_libraries(cweb_cli PRIVATE cweb)
endif()

target_include_directories(cweb_cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${CMAKE_CURRENT_SOURCE_DIR}/../framework/include"
)

target_compile_options(cweb_cli PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -fstack-protector-strong
)

target_compile_definitions(cweb_cli PRIVATE
    _FORTIFY_SOURCE=2
    CWEB_CLI_VERSION="${PROJECT_VERSION}"
)

install(TARGETS cweb_cli
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# --- Packaging (CPack) ---------------------------------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "cweb-cli")
set(CPACK_PACKAGE_VENDOR "Ben Bohle")
set(CPACK_PACKAGE_CONTACT "Ben Bohle <kontakt@benbohle.de>")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/BenBohle/cweb_dev")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcweb-dev (>= 0.3.0)")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

include(CPack)
