#!/bin/bash
  
set -e
set -u

: "${UID:=0}"
: "${GID:=${UID}}"

if [ "$UID" != 0 ]
then
    usermod -u "$UID" "$O3DE_USER" 2>/dev/null && {
        groupmod -g "$GID" "$O3DE_USER" 2>/dev/null ||
        usermod -a -G "$GID" "$O3DE_USER"
    }
    set -- su $O3DE_USER -g $O3DE_USER "${@}"
else
    echo 'The env for the current UserID/GroupID is required. You need to provide the following arguments: --env UID=\$(id -u) --env GID=\$(id -g)'
    exit 1
fi

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

/opt/O3DE/$(ls /opt/O3DE/)/python/get_python.sh
if [ $? -ne 0 ]
then
    echo "Error Initializing O3DE Python."
    exit 1
fi

exec "$@"
