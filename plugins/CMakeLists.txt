# Optional: semicolon-separated list of plugin directories to skip,
# e.g. -DSKIP_PLUGINS="ldap;webapi" (useful when submodules are not checked out)
set(SKIP_PLUGINS "" CACHE STRING "Plugins to exclude from the build")

file(GLOB plugins RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(plugin ${plugins})
	if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${plugin})
		if(plugin IN_LIST SKIP_PLUGINS)
			message(STATUS "Skipping plugin ${plugin}")
		else()
			add_subdirectory(${plugin})
		endif()
	endif()
endforeach()
