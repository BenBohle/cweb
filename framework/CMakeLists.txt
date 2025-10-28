cmake_minimum_required(VERSION 3.16)

project(
	cweb
	VERSION 0.3.0
	DESCRIPTION "Modular C web framework"
	LANGUAGES C
)

option(CWEB_USE_INTERNAL_LIBEVENT "Build against the bundled libevent" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

file(GLOB_RECURSE CWEB_SOURCE_FILES CONFIGURE_DEPENDS
	"${PROJECT_SOURCE_DIR}/src/*.c"
)

if(NOT CWEB_SOURCE_FILES)
	message(FATAL_ERROR "Keine Quell-Dateien unter src/ gefunden")
endif()

set(CWEB_PRIVATE_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src")
file(GLOB CWEB_MODULE_DIRS RELATIVE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/src/*")
foreach(_module ${CWEB_MODULE_DIRS})
	if(IS_DIRECTORY "${PROJECT_SOURCE_DIR}/src/${_module}")
		list(APPEND CWEB_PRIVATE_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src/${_module}")
	endif()
endforeach()
list(REMOVE_DUPLICATES CWEB_PRIVATE_INCLUDE_DIRS)

add_library(cweb ${CWEB_SOURCE_FILES})
add_library(cweb::cweb ALIAS cweb)

set_target_properties(cweb PROPERTIES
	POSITION_INDEPENDENT_CODE ON
	OUTPUT_NAME "cweb"
	VERSION "${PROJECT_VERSION}"
	SOVERSION "${PROJECT_VERSION_MAJOR}"
)

target_include_directories(cweb
    PRIVATE
        "${PROJECT_SOURCE_DIR}/include"    # use local public headers first
        "${PROJECT_SOURCE_DIR}/src"        # private headers for sources
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cweb>
)

if(MSVC)
	# Keep high warning level for MSVC and add Release-only optimizations
	target_compile_options(cweb PRIVATE /W4 /WX)
	# Release-only flags (MSVC optimizer/link-time code generation)
	target_compile_options(cweb PRIVATE
		$<$<CONFIG:Release>:/O2;/Ob2;/GL>
	)
	target_compile_definitions(cweb PRIVATE _FORTIFY_SOURCE=2)
else()
	target_compile_options(cweb PRIVATE
		-Wall
		-Wextra
		-Wpedantic
		-Werror
		-fstack-protector-strong
		-fPIC
		# Release-only optimization flags
		$<$<CONFIG:Release>:-O3>
		$<$<CONFIG:Release>:-march=native>
		$<$<CONFIG:Release>:-flto>
	)
	target_compile_definitions(cweb PRIVATE _FORTIFY_SOURCE=2)
endif()

# Define NDEBUG in Release config and enable IPO/LTO when supported
target_compile_definitions(cweb PRIVATE $<$<CONFIG:Release>:NDEBUG>)
set_target_properties(cweb PROPERTIES
	INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)

if(WIN32)
	target_compile_definitions(cweb PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
	target_compile_definitions(cweb PRIVATE _GNU_SOURCE)
endif()

if(CWEB_USE_INTERNAL_LIBEVENT)
	add_subdirectory(deps/libevent)
	if(TARGET event)
		target_link_libraries(cweb PRIVATE event)
		target_include_directories(cweb PRIVATE "${PROJECT_SOURCE_DIR}/deps/libevent/include")
	else()
		message(FATAL_ERROR "Das interne libevent stellt kein 'event'-Target bereit")
	endif()
else()
	set(_cweb_event_found OFF)
	find_package(Event QUIET)
	if(Event_FOUND)
		if(TARGET Event::Event)
			target_link_libraries(cweb PRIVATE Event::Event)
			set(_cweb_event_found ON)
		elseif(DEFINED EVENT_LIBRARY)
			target_link_libraries(cweb PRIVATE "${EVENT_LIBRARY}")
			if(DEFINED EVENT_INCLUDE_DIR)
				target_include_directories(cweb PRIVATE "${EVENT_INCLUDE_DIR}")
			endif()
			set(_cweb_event_found ON)
		endif()
	endif()

	if(NOT _cweb_event_found)
		find_package(PkgConfig QUIET)
		if(PKG_CONFIG_FOUND)
			pkg_check_modules(LIBEVENT QUIET libevent)
			if(LIBEVENT_FOUND)
				target_link_libraries(cweb PRIVATE ${LIBEVENT_LIBRARIES})
				target_include_directories(cweb PRIVATE ${LIBEVENT_INCLUDE_DIRS})
				target_compile_options(cweb PRIVATE ${LIBEVENT_CFLAGS_OTHER})
				set(_cweb_event_found ON)
			endif()
		endif()
	endif()

	if(NOT _cweb_event_found)
		find_package(PkgConfig QUIET)
		if(NOT PKG_CONFIG_FOUND OR NOT PKG_CONFIG_EXECUTABLE)
			message(FATAL_ERROR "libevent konnte nicht automatisch gefunden werden und pkg-config fehlt. Installiere das Tool (z.B. 'sudo apt install pkg-config') oder setze CWEB_USE_INTERNAL_LIBEVENT=ON")
		endif()
		pkg_check_modules(LIBEVENT REQUIRED libevent)
		target_link_libraries(cweb PRIVATE ${LIBEVENT_LIBRARIES})
		target_include_directories(cweb PRIVATE ${LIBEVENT_INCLUDE_DIRS})
		target_compile_options(cweb PRIVATE ${LIBEVENT_CFLAGS_OTHER})
		set(_cweb_event_found ON)
	endif()
endif()

configure_file(
	"${PROJECT_SOURCE_DIR}/cweb.pc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cweb.pc"
	@ONLY
)

install(TARGETS cweb
	EXPORT cwebTargets
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/cweb/"
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cweb"
	FILES_MATCHING
		PATTERN "*.h"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cweb.pc"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

install(EXPORT cwebTargets
	FILE cwebTargets.cmake
	NAMESPACE cweb::
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cweb"
)

include(CMakePackageConfigHelpers)
set(CWEB_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/cweb")

configure_package_config_file(
	"${PROJECT_SOURCE_DIR}/cmake/cweb-config.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cweb-config.cmake"
	INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cweb"
	NO_CHECK_REQUIRED_COMPONENTS_MACRO
	PATH_VARS CWEB_INSTALL_INCLUDEDIR
)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/cweb-config-version.cmake"
	VERSION "${PROJECT_VERSION}"
	COMPATIBILITY SameMajorVersion
)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/cweb-config.cmake"
	"${CMAKE_CURRENT_BINARY_DIR}/cweb-config-version.cmake"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cweb"
)

# --- Packaging (CPack) ---------------------------------------------------
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "libcweb-dev")
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
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libevent-dev, libcurl4-openssl-dev, libbrotli-dev, zlib1g-dev, libcjson-dev")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Full Stack C Web Framework")

include(CPack)

message(STATUS "cweb ${PROJECT_VERSION} wird gebaut. Shared: ${BUILD_SHARED_LIBS}")
