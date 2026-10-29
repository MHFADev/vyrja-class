# Vyrja RMM agent plugin (experimental)
#
# The RMM agent is disabled by default at every level:
#   * build time: this directory only configures with -DWITH_RMM=ON
#     (it can also be excluded explicitly via -DSKIP_PLUGINS="rmmagent")
#   * runtime: the RMM/* configuration gates default to off
#     (RMM/Enabled=false, see core/src/VeyonConfigurationProperties.h)
#
# See RMM.md at the repository root for the architecture and the
# phased implementation plan (Phase 3A scaffold).

if(NOT WITH_RMM)
	message(STATUS "Skipping plugin rmmagent (WITH_RMM is OFF)")
	return()
endif()

include(BuildVeyonPlugin)

# TODO(Phase 3B): implement the RMM agent feature plugin here:
#   * telemetry collector running in a feature worker process
#     (bounded queue, coarse timer, never in the VNC/render path)
#   * client side of the VyrjaRmmBridge named pipe hosted by the
#     Windows service (see RMM.md, "IPC" section)
#
# build_veyon_plugin(rmmagent
# 	NAME RmmAgent
# 	SOURCES
# 	${rmmagent_SOURCES})
