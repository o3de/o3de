
function(target_depends_on_ros2_packages TARGET_NAME)
    if (NOT DEFINED ENV{ROS_DISTRO})
        message(WARNING, "To build ROS2 Gem a ROS distribution needs to be sourced, but none detected")
    endif()
    #message("Building ROS2 Gem with ros2 $ENV{ROS_DISTRO}")

    #TODO - compare to previous env since we need to rerun cmake if we source a different ros2 env!
    #TODO - can be done with a file that is in CONFIGURE_DEPENDS so that a change triggers build
    if (NOT DEFINED ENV{AMENT_PREFIX_PATH})
        message(WARNING, "$ENV{AMENT_PREFIX_PATH} missing from env. ROS 2 needs to be sourced to build ROS2 Gem")
    endif()
    set (_ament_prefix_path "$ENV{AMENT_PREFIX_PATH}")

    # ros2 directories with libraries, e.g. /opt/ros/galactic/lib, locally built custom packages etc.
    set(_ros2_library_directories)
    set(_ros2_include_directories)
    set(_ros2_package_libraries)
    foreach(_ros2_packages_path IN LISTS _ament_prefix_path)
        #message("including packages: ${ros2_packages_path}")
        string(REPLACE ":" ";" _ros2_packages_path ${_ros2_packages_path})
        list(APPEND _ros2_library_directories "${_ros2_packages_path}/lib")
        list(APPEND _ros2_include_directories "${_ros2_packages_path}/include")
    endforeach()
    foreach(_package IN LISTS ARGN)
        #message("processing package: ${_package}")
        find_package(${_package} REQUIRED)
        list(APPEND _ros2_package_libraries "${${_package}_LIBRARIES}")
    endforeach()
    #message("Found include directories: ${_ros2_include_directories}")
    #message("Found libraries: ${_ros2_package_libraries}")
    target_include_directories(${TARGET_NAME} PUBLIC ${_ros2_include_directories})
    target_link_libraries(${TARGET_NAME} PUBLIC ${_ros2_package_libraries})
endfunction()
