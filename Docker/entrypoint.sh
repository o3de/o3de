#!/bin/bash
 

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

set -eu

usermod -u "$UID" "$O3DE_USER" 2>/dev/null && 
    usermod -a -G "$GID" "$O3DE_USER" 2>/dev/null
if [ $? -ne 0 ]
then
    echo "Error matching Docker user $O3DE to User ID $$UID and Group ID $GID"
    exit 1
fi


set -- su $O3DE_USER -g $O3DE_USER "${@}"

if [ ! -d "/home/o3de/.o3de" ]
then
    echo 'Mapping of the user id was not provided on docker run. You need to provide the following arguments: -v "$HOME/.o3de:/home/o3de/.o3de" -v "$HOME/O3DE:/home/o3de/O3DE"'
    exit 1
elif [ ! -d "/home/o3de/O3DE" ]
then
    echo 'Mapping of the user id was not provided on docker run. You need to provide the following arguments: -v "$HOME/.o3de:/home/o3de/.o3de" -v "$HOME/O3DE:/home/o3de/O3DE"'
    exit 1
else
    sudo chown o3de:o3de -R /home/o3de/.o3de
    sudo chown o3de:o3de -R /home/o3de/O3DE
fi

su -c "/opt/O3DE/$(ls /opt/O3DE/)/python/get_python.sh" $O3DE_USER 
if [ $? -ne 0 ]
then
    echo "Error Initializing O3DE Python."
    exit 1
fi


#su -c "/opt/O3DE/$(ls /opt/O3DE/)/scripts/o3de.sh register -agp $O3DE_EXTRAS_ROOT/Gems \
#   && /opt/O3DE/$(ls /opt/O3DE/)/scripts/o3de.sh register -atp $O3DE_EXTRAS_ROOT/Templates \
#   && /opt/O3DE/$(ls /opt/O3DE/)/scripts/o3de.sh register -app $O3DE_EXTRAS_ROOT/Projects" $O3DE_USER 

if [ "$INPUT_IMAGE" == "ros" ]
then
    echo -e "\nsource /opt/ros/${ROS_DISTRO}/setup.bash\n" >> /home/o3de/.bashrc
fi

exec "$@"
