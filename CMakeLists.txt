cmake_minimum_required(VERSION 3.20)

# Der Projektname entspricht automatisch dem Verzeichnisnamen.
get_filename_component(APP_NAME "${CMAKE_SOURCE_DIR}" NAME)
project(${APP_NAME} LANGUAGES C)

include(GNUInstallDirs)

find_package(cweb REQUIRED)

find_package(PkgConfig REQUIRED)
find_package(ZLIB REQUIRED)

pkg_check_modules(LIBEVENT REQUIRED IMPORTED_TARGET libevent)
pkg_check_modules(LIBCURL REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(LIBMARIADB REQUIRED IMPORTED_TARGET libmariadb)
pkg_check_modules(LIBCJSON REQUIRED IMPORTED_TARGET libcjson)
pkg_check_modules(LIBBROTLIENC REQUIRED IMPORTED_TARGET libbrotlienc)
pkg_check_modules(LIBBROTLICOMMON REQUIRED IMPORTED_TARGET libbrotlicommon)

set(GENERATED_STAMP "${CMAKE_SOURCE_DIR}/build/generated.stamp")

if(NOT EXISTS ${GENERATED_STAMP})
	execute_process(
		COMMAND cweb build
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMAND_ERROR_IS_FATAL ANY
	)
	execute_process(
		COMMAND ${CMAKE_COMMAND} -E touch ${GENERATED_STAMP}
	)
endif()

add_custom_command(
	OUTPUT ${GENERATED_STAMP}
	COMMAND cweb build
	COMMAND ${CMAKE_COMMAND} -E touch ${GENERATED_STAMP}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "🧩 Generiere C-Code aus .cweb Templates..."
)

add_custom_target(generate_code ALL DEPENDS ${GENERATED_STAMP})

set_property(SOURCE ${GENERATED_STAMP} PROPERTY SYMBOLIC TRUE)

file(GLOB_RECURSE APP_CODE
	CONFIGURE_DEPENDS
	"${CMAKE_SOURCE_DIR}/app/*.c"
)

file(GLOB_RECURSE GENERATED_CODE
	CONFIGURE_DEPENDS
	"${CMAKE_SOURCE_DIR}/build/*.c"
)

set(APP_SOURCES ${APP_CODE} ${GENERATED_CODE})

list(FILTER APP_SOURCES EXCLUDE REGEX "/CMakeFiles/")
list(FILTER APP_SOURCES EXCLUDE REGEX "CMakeCCompilerId\\.c$")
list(REMOVE_DUPLICATES APP_SOURCES)

add_executable(${PROJECT_NAME} ${APP_SOURCES})
add_dependencies(${PROJECT_NAME} generate_code)

target_include_directories(${PROJECT_NAME} PRIVATE
	"${CMAKE_SOURCE_DIR}/app/include"
	"${CMAKE_SOURCE_DIR}/app"
	"${CMAKE_SOURCE_DIR}/build"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
	cweb::cweb
	PkgConfig::LIBEVENT
	PkgConfig::LIBCURL
	PkgConfig::LIBMARIADB
	PkgConfig::LIBCJSON
	PkgConfig::LIBBROTLIENC
	PkgConfig::LIBBROTLICOMMON
	ZLIB::ZLIB
)

target_compile_features(${PROJECT_NAME} PRIVATE c_std_17)
