# BuildVeyonApplication.cmake - Copyright (c) 2019-2026 Tobias Junghans
#
# description: build Veyon application
# usage: build_veyon_application(<NAME> <SOURCES>)
include (WindowsBuildHelpers)

function(build_veyon_application TARGET)
	cmake_parse_arguments(PARSE_ARGV 1 arg
		""
		""
		"SOURCES")

	if(VEYON_BUILD_ANDROID)
		add_library(${TARGET} SHARED ${arg_SOURCES})
	else()
		add_executable(${TARGET} ${arg_SOURCES})
		install(TARGETS ${TARGET} RUNTIME DESTINATION bin)
	endif()
	target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
	set_target_properties(${TARGET} PROPERTIES COMPILE_OPTIONS "${CMAKE_COMPILE_OPTIONS_PIE}")
	set_target_properties(${TARGET} PROPERTIES LINK_OPTIONS "${CMAKE_LINK_OPTIONS_PIE}")
	target_link_libraries(${TARGET} PRIVATE veyon-core)
	set_default_target_properties(${TARGET})
	if(WITH_PCH)
		target_precompile_headers(${TARGET} REUSE_FROM veyon-application-pch)
	endif()
	if (VEYON_BUILD_WINDOWS)
		# Ship the binaries with Vyrja file names on Windows instead of
		# the upstream Veyon ones. Only the output file names change:
		# CMake target names, the Windows service ID and all
		# configuration/data paths intentionally stay untouched, so
		# existing Veyon data and settings remain in place.
		if(${TARGET} STREQUAL "veyon-master")
			set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "vyrja-classroom")
		elseif(${TARGET} STREQUAL "veyon-configurator")
			set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "vyrja-setup")
		elseif(${TARGET} STREQUAL "veyon-service")
			set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "vyrja-service")
		elseif(${TARGET} STREQUAL "veyon-server")
			set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "vyrja-server")
		elseif(${TARGET} STREQUAL "veyon-worker")
			set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "vyrja-worker")
		elseif(${TARGET} STREQUAL "veyon-cli")
			set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "vyrja-cli")
		elseif(${TARGET} STREQUAL "veyon-wcli")
			set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "vyrja-wcli")
		endif()
		add_windows_resources(${TARGET} ${ARGN})
	endif()
endfunction()
