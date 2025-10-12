cmake_minimum_required(VERSION 3.16)

# HMI Application (Qt-based) - Clean version with only essential files
if(BUILD_GUI AND (Qt5_FOUND OR Qt6_FOUND))
    # Set up Qt (version-agnostic)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)

    # Header files - organized by component
    set(HMI_HEADERS
        # UI Components
        ui/include/navigation_main_window.h
        ui/include/map_widget.h
        
        # Controllers
        controllers/include/integrated_navigation_controller.h
        
        # Service Cores
        services/include/positioning_service_core.h
        services/include/routing_service_core.h
        services/include/guidance_service_core.h
        services/include/map_service_core.h
        
        # Models and Interfaces
        models/include/navigation_models.h
        models/include/service_interfaces.h
    )

    # Source files - organized by component
    set(HMI_SOURCES
        # UI Components
        ui/src/navigation_main_window.cpp
        ui/src/map_widget.cpp
        
        # Controllers
        controllers/src/integrated_navigation_controller.cpp
        
        # Service Cores
        services/src/positioning_service_core.cpp
        services/src/routing_service_core.cpp
        services/src/guidance_service_core.cpp
        services/src/map_service_core.cpp
        
        # Models
        models/src/navigation_models.cpp
    )

    # UI files
    set(HMI_UI_FILES
        ui/navigation_main_window.ui
    )

    # Resource files (Qt version-agnostic)
    if(QT_VERSION_MAJOR EQUAL 6)
        qt6_add_resources(RESOURCE_FILES resources.qrc)
    else()
        qt5_add_resources(RESOURCE_FILES resources.qrc)
    endif()
    
    # GUI application with Qt Designer support
    add_executable(nav_hmi_gui
        src/main.cpp
        ${HMI_SOURCES}
        ${HMI_HEADERS}
        ${HMI_UI_FILES}
        ${RESOURCE_FILES}
    )
    
    target_include_directories(nav_hmi_gui PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/include
        ${CMAKE_CURRENT_SOURCE_DIR}/controllers/include
        ${CMAKE_CURRENT_SOURCE_DIR}/services/include
        ${CMAKE_CURRENT_SOURCE_DIR}/models/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    )
    
    # Qt linking (version-agnostic)
    if(QT_VERSION_MAJOR EQUAL 6)
        target_link_libraries(nav_hmi_gui
            nav_common
            nav_service_base
            Qt6::Core
            Qt6::Widgets
            Qt6::Network
            Threads::Threads
        )
    else()
        target_link_libraries(nav_hmi_gui
            nav_common
            nav_service_base
            Qt5::Core
            Qt5::Widgets
            Qt5::Network
            Qt5::UiTools
            Threads::Threads
        )
    endif()
    
    # Platform-specific linking
    if(WIN32)
        target_link_libraries(nav_hmi_gui ws2_32)
        set_target_properties(nav_hmi_gui PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    elseif(QNX)
        target_link_libraries(nav_hmi_gui ${QNX_C_LIB} ${QNX_SOCKET_LIB})
    endif()
    
    target_compile_features(nav_hmi_gui PRIVATE cxx_std_17)
    
    # Copy resources to build directory
    add_custom_command(TARGET nav_hmi_gui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources
        ${CMAKE_CURRENT_BINARY_DIR}/resources
        COMMENT "Copying resources to build directory"
    )
    
    # Install GUI executable
    install(TARGETS nav_hmi_gui
        RUNTIME DESTINATION bin
        COMPONENT gui
    )
    
    message(STATUS "Building GUI HMI with Qt${QT_VERSION_MAJOR} Designer and enhanced map widget")
else()
    message(STATUS "Qt GUI components not available - skipping GUI build")
endif()