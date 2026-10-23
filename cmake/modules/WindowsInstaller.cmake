set(WINDOWS_INSTALL_FILES "veyon-${VEYON_WINDOWS_ARCH}-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_BUILD}")

set(DLLDIR "${MINGW_PREFIX}/bin")
set(DLLDIR_LIB "${MINGW_PREFIX}/lib")
string(REGEX MATCH "^[^.]+" GCC_VERSION_MAJOR ${CMAKE_CXX_COMPILER_VERSION})
if(EXISTS "${MINGW_PREFIX}/lib/gcc/${MINGW_TARGET}")
	set(DLLDIR_GCC "${MINGW_PREFIX}/lib/gcc/${MINGW_TARGET}/${GCC_VERSION_MAJOR}")
else()
	set(DLLDIR_GCC "/usr/lib/gcc/${MINGW_TARGET}/${GCC_VERSION_MAJOR}-posix")
endif()
if(VEYON_BUILD_WIN64)
	set(DLL_GCC "libgcc_s_seh-1.dll")
	set(DLL_DDENGINE "ddengine64.dll")
else()
	set(DLL_GCC "libgcc_s_dw2-1.dll")
	set(DLL_DDENGINE "ddengine.dll")
endif()

# The packaging steps are POSIX shell commands. As add_custom_target()
# commands they are executed through cmd.exe on native Windows hosts
# (MSYS2), which rejects them ("The syntax of the command is incorrect."),
# so they live in a generated shell script instead - see
# .ci/windows/windows-packaging.sh.in for the actual steps.
configure_file(${CMAKE_SOURCE_DIR}/.ci/windows/windows-packaging.sh.in ${CMAKE_BINARY_DIR}/windows-packaging.sh @ONLY)

add_custom_target(windows-binaries
	COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIGURATION>
	COMMAND sh ${CMAKE_BINARY_DIR}/windows-packaging.sh staging
)

add_custom_target(create-windows-installer
	COMMAND sh ${CMAKE_BINARY_DIR}/windows-packaging.sh installer
	DEPENDS windows-binaries
)

add_custom_target(prepare-dev-nsi
	COMMAND sh ${CMAKE_BINARY_DIR}/windows-packaging.sh devnsi
	DEPENDS windows-binaries)

add_custom_target(dev-nsi
	DEPENDS prepare-dev-nsi create-windows-installer
)
