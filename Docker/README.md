# Docker support for O3DE

O3DE supports construction of a Docker image on the Linux environment.

## Prerequisites

* [Hardware requirements of o3de](https://www.o3de.org/docs/welcome-guide/requirements/)
* Any Linux distribution that supports Docker and the NVIDIA container toolkit (see below)
* At least 60 GB of free disk space
* Docker installed and configured
  * **Note** It is recommended to have Docker installed correctly and in a secure manner so that the Docker commands in this guide do not require elevated priviledges (sudo) in order to run them. See [Docker Engine post-installation steps](https://docs.docker.com/engine/install/linux-postinstall/) for more details.
* [NVidia container toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/install-guide.html#docker)

# Building the Docker image
The Docker script provides the following arguments for building the Docker image, with each argument provide a default value.

| Argument                | Description                                                                | Default
|-------------------------|----------------------------------------------------------------------------|-------------
| INPUT_IMAGE             | The base ubuntu docker image to base the build on                          | ubuntu:22.04
| INPUT_ARCHITECTURE      | The CPU architecture (amd64/aarch64). Will require QEMU if cross compiling | amd64
| O3DE_REPO               | The git repo for O3DE                                                      | https://github.com/o3de/o3de
| O3DE_BRANCH             | The branch for O3DE                                                        | development
| O3DE_COMMIT             | The commit on the branch for O3DE (or HEAD)                                | HEAD
| O3DE_EXTRAS_REPO        | The git repo for O3DE Extras                                               | https://github.com/o3de/o3de-extras
| O3DE_EXTRAS_BRANCH      | The branch for O3DE Extras                                                 | development
| O3DE_EXTRAS_COMMIT      | The commit on the branch for O3DE Extras (or HEAD)                         | HEAD

## Example

### Locally (From the o3de/Docker folder)

```
docker build -f Dockerfile -t amd64/o3de:latest .
```

### From github
```
docker build -t amd64/o3de:latest https://github.com/o3de/o3de.git#development:Docker
```


# Running the Docker image locally
Docker containers by themselves are stateless, so in order for the container to act as a stateful container, some local preparations and additional runtime arguments are needed to be passed into the Docker run command.

## Setting up O3DE-specific folders
O3DE will use two folders for its runtime environment. The manifest folder (\$HOME/.o3de) and the O3DE Home folder (\$HOME/O3DE). On normal installations/build of the full O3DE engine, these folders are created on-demand based on the current logged in user. Since Docker containers by default are stateless, all the information that is downloaded, updated, deleted, etc from these two folders as part of normal operations will no longer exist once the container is disconnected, thus losing any work that was done while using O3DE. In order to utilize the O3DE Docker container to create and maintain O3DE projects beyond the container session, we will need to create these two folders manually and pass them in as mapped volumes to its corresponding specific folders within the container. 

The following example creates these folders under a parent `docker` folder in the current user's `$HOME` directory, specific to the name of the Docker image.

```
mkdir -p $HOME/docker/amd-o3de/manifest

mkdir -p $HOME/docker/amd-o3de/home
```

>**Note** The two folders must be created specifically for the O3DE Docker container that is being connected to. There is no version or compatibility checks in the Docker container, so normal O3DE startup in the container may fail if it does not use the same mapped folders it used during its initial launch.

These two folders will be added as part of the `docker run` argument list:

```
-v "$HOME/docker/amd-o3de/manifest:/home/o3de/.o3de" -v "$HOME/docker/amd-o3de/home:/home/o3de/O3DE"
```

## Using the current user id and group id
The Docker image will map the current user and group id to a Docker create user and group when running the container. This will allow reading and writing to the mapped O3DE-specific folders described earlier and will add persistence to the Docker container through the mapped folders. The user id and group id are passed to the Docker run command as two arguments: `UID` and `PID`:

```
--env UID=$(id -u) --env GID=$(id -g)
```


Putting it all together, you can launch the Docker container with the following command

```
xhost +local:root

docker run --env UID=$(id -u) --env GID=$(id -g) -v "$HOME/.o3de:/home/o3de/.o3de" -v "$HOME/O3DE:/home/o3de/O3DE"  --rm --gpus all -e DISPLAY=:1 -v /tmp/.X11-unix:/tmp/.X11-unix -it amd64/o3de
```

