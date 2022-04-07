
# Note that this does not find any ros2 package in particular, but determines whether a distro is sourced properly
# Can be extended to handle supported / unsupported distros
if (NOT DEFINED ENV{ROS_DISTRO} OR NOT DEFINED ENV{AMENT_PREFIX_PATH})
    message(WARNING, "To build ROS2 Gem a ROS distribution needs to be sourced, but none detected")
    set(ROS2_FOUND FALSE)
    return()
endif()
set(ROS2_FOUND TRUE)