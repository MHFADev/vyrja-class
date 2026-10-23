include(BuildVeyonPlugin)

set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)

# Interception API library, vendored under 3rdparty/interception/library
# (the MSYS2/GitHub-hosted Windows toolchain has no system-wide Interception SDK)
add_library(interception STATIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/interception/library/interception.c)
target_include_directories(interception PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/interception/library)
target_compile_definitions(interception PUBLIC INTERCEPTION_STATIC)
target_compile_options(interception PRIVATE -Wno-error)

build_veyon_plugin(windows-platform
	NAME WindowsPlatform
	SOURCES
	WindowsPlatformPlugin.cpp
	WindowsPlatformConfigurationPage.h
	WindowsPlatformConfigurationPage.cpp
	WindowsPlatformConfigurationPage.ui
	WindowsCoreFunctions.cpp
	WindowsDeviceFunctions.cpp
	WindowsFilesystemFunctions.cpp
	WindowsInputDeviceFunctions.cpp
	WindowsKeyboardShortcutTrapper.cpp
	WindowsNetworkFunctions.cpp
	WindowsServerProcess.cpp
	WindowsServiceControl.cpp
	WindowsServiceCore.cpp
	WindowsServiceFunctions.cpp
	WindowsSessionFunctions.cpp
	WindowsUserFunctions.cpp
	WtsSessionManager.cpp
	DesktopInputController.cpp
	SasEventListener.cpp
	XEventLog.cpp
	${ultravnc_DIR}/winvnc/winvnc/vnckeymap.cpp
	${ultravnc_DIR}/addon/ms-logon/authSSP/authSSP.cpp
	${ultravnc_DIR}/addon/ms-logon/authSSP/GenClientServerContext.cpp

	WindowsPlatformConfiguration.h
	WindowsPlatformPlugin.h
	WindowsCoreFunctions.h
	WindowsDeviceFunctions.h
	WindowsFilesystemFunctions.h
	WindowsInputDeviceFunctions.h
	WindowsKeyboardShortcutTrapper.h
	WindowsNetworkFunctions.h
	WindowsServerProcess.h
	WindowsServiceControl.h
	WindowsServiceCore.h
	WindowsServiceFunctions.h
	WindowsSessionFunctions.h
	WindowsSmartObjects.h
	WindowsUserFunctions.h
	WtsSessionManager.h
	DesktopInputController.h
	SasEventListener.h
	../common/LogonHelper.h
	../common/LogonHelper.cpp
	../common/PersistentLogonCredentials.h
	../common/PersistentLogonCredentials.cpp
	../common/PlatformSessionManager.h
	../common/PlatformSessionManager.cpp
	../common/ServiceDataManager.h
	../common/ServiceDataManager.cpp
	XEventLog.h
	windows.qrc
	)

target_include_directories(windows-platform PRIVATE
	../common
	${ultravnc_DIR}/addon/ms-logon/authSSP
	${ultravnc_DIR}/winvnc/winvnc
	${ultravnc_DIR}
	)

target_link_libraries(windows-platform PRIVATE -lws2_32 -lwtsapi32 -lnetapi32 -luserenv -limm32 -lntdll interception -liphlpapi -lsetupapi -lsecur32)
target_link_libraries(windows-platform PRIVATE Qt6::GuiPrivate)
target_compile_definitions(windows-platform PRIVATE ULTRAVNC_VEYON_SUPPORT)
target_compile_options(windows-platform PRIVATE "-Wno-unknown-pragmas;-flarge-source-files;-Wno-misleading-indentation")
set_source_files_properties(WindowsNetworkFunctions.cpp PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE SKIP_PRECOMPILE_HEADERS TRUE)
