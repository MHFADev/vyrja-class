# Vyrja RMM agent plugin (experimental)
#
# The RMM agent is disabled by default at every level:
#   * build time: this directory only configures with -DWITH_RMM=ON
#     (the RMM edition, i.e. release tags containing "-rmm", forces it
#     on automatically; it can also be excluded explicitly via
#     -DSKIP_PLUGINS="rmmagent")
#   * runtime: the RMM/* configuration gates default to off
#     (RMM/Enabled=false, see core/src/VeyonConfigurationProperties.h)
#
# See RMM.md at the repository root for the architecture and the
# phased implementation plan.

if(NOT WITH_RMM)
	message(STATUS "Skipping plugin rmmagent (WITH_RMM is OFF)")
	return()
endif()

include(BuildVeyonPlugin)

build_veyon_plugin(rmmagent
	NAME RmmAgent
	SOURCES
	RmmAgentPlugin.cpp
	RmmAgentPlugin.h)
