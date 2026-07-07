find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache) # does not improve, but may mess up
    message(STATUS "Enabled CCache using: ${CCACHE_PROGRAM}")
else (CCACHE_PROGRAM)
    message(STATUS "Disabled CCache: ccache not found")
endif(CCACHE_PROGRAM)
