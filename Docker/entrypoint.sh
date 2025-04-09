#!/bin/bash
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# Validate that the UID and GID environment variables are set 
: "${UID:=0}"
: "${GID:=0}"

if [ "$UID" == 0 ]
then
    echo 'The env for the current User ID is required. You need to provide the following arguments: --env UID=\$(id -u)'
    exit 1
fi

if [ "$GID" == 0 ]
then
    echo 'The env for the current Group ID is required. You need to provide the following arguments: --env GID=\$(id -g)'
    exit 1
fi

# Update the O3DE user's UID and GID to match the environment so that it has the same user information/permissions for the mapped volumes used
# for persistence of this Docker container
set -eu

usermod -u "$UID" "$O3DE_USER" 2>/dev/null && {
    groupmod -g "$GID" "$O3DE_USER" 2>/dev/null ||
    usermod -a -G "$GID" "$O3DE_USER"
}
if [ $? -ne 0 ]
then
    echo "Error matching Docker user $O3DE to User ID $$UID and Group ID $GID"
    exit 1
fi

# Make sure the O3DE specific manifest (.o3de) and home (O3DE) folders are mapped correctly for persistence
if [ ! -d "/home/$O3DE_USER/.o3de" ]
then
    echo 'Mapping of the user id was not provided on docker run. You need to provide the following arguments: -v "$HOME/.o3de:/home/o3de/.o3de" -v "$HOME/O3DE:/home/o3de/O3DE"'
    exit 1
elif [ ! -d "/home/$O3DE_USER/O3DE" ]
then
    echo 'Mapping of the user id was not provided on docker run. You need to provide the following arguments: -v "$HOME/.o3de:/home/o3de/.o3de" -v "$HOME/O3DE:/home/o3de/O3DE"'
    exit 1
else
    # Make sure ownership is correct for the mapped O3DE folders
    sudo chown $O3DE_USER:$O3DE_USER -R /home/$O3DE_USER/.o3de
    sudo chown $O3DE_USER:$O3DE_USER -R /home/$O3DE_USER/O3DE

    # Prepare and set the XDG_RUNTIME_DIR value for Qt in order to launch mime applications from Project Manager
    sudo mkdir -p /run/user/$UID
    sudo chown $O3DE_USER:$O3DE_USER /run/user/$UID
    sudo chmod 7700 /run/user/$UID
    echo -e "\nexport XDG_RUNTIME_DIR=/run/user/$UID\n" >> /home/o3de/.bashrc
fi

# Bootstrap python first
su -c "/opt/O3DE/$(ls /opt/O3DE/)/python/get_python.sh" $O3DE_USER 
if [ $? -ne 0 ]
then
    echo "Error Initializing O3DE Python."
    exit 1
fi

# If this is a ROS based image, then add ros setup in the $O3DE user's startup
if [ "$INPUT_IMAGE" == "ros" ]
then
    echo -e "\nsource /opt/ros/${ROS_DISTRO}/setup.bash\n" >> /home/o3de/.bashrc
fi

# Start the session as $O3DE_USER
set -- su $O3DE_USER -g $O3DE_USER "${@}"

exec "$@"
