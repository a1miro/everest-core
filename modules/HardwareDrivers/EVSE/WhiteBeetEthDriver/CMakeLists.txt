#
# AUTO GENERATED - MARKED REGIONS WILL BE KEPT
# template version 3
#

# module setup:
#   - ${MODULE_NAME}: module name
ev_setup_cpp_module()

# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1
# insert your custom targets and additional config variables here

# Include directories
target_include_directories(${MODULE_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/whitebeet_comms"
        "${CMAKE_CURRENT_SOURCE_DIR}/board_support"
        "${CMAKE_CURRENT_SOURCE_DIR}/slac"
        "${CMAKE_CURRENT_SOURCE_DIR}/charger"
        "${CMAKE_CURRENT_SOURCE_DIR}/powermeter"
        "${CMAKE_CURRENT_SOURCE_DIR}/rcd"
        "${CMAKE_CURRENT_SOURCE_DIR}/connector_lock"
)

# Find and link threading support
find_package(Threads REQUIRED)
target_link_libraries(${MODULE_NAME}
    PRIVATE
        Threads::Threads
)

# ev@bcc62523-e22b-41d7-ba2f-825b493a3c97:v1

# sources for implementations
target_sources(${MODULE_NAME}
    PRIVATE
        "whitebeet_comms/WhiteBeetEthernet.cpp"
        "whitebeet_comms/WhiteBeetProtocol.cpp"
        "whitebeet_comms/EthernetFraming.cpp"
        "board_support/evse_board_supportImpl.cpp"
        "slac/slacImpl.cpp"
        "charger/ISO15118_chargerImpl.cpp"
        "powermeter/powermeterImpl.cpp"
        "rcd/ac_rcdImpl.cpp"
        "connector_lock/connector_lockImpl.cpp"
)