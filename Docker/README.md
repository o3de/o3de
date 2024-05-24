

docker run --env UID=$(id -u) --env GID=$(id -g) -v "$(pwd):/home/o3de/temp" --rm --gpus all -e DISPLAY=:1 --network="bridge" -v /tmp/.X11-unix:/tmp/.X11-unix -it amd64/o3de /bin/bash

docker build -f Dockerfile -t amd64/o3de:latest .


