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
fi

exec "$@"
