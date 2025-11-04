# Docker support for O3DE

O3DE supports construction of a Docker image on the Linux environment. The following instructions will allow you to build and launch O3DE through a Docker container, with project data being persisted on your local machine.

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
| INPUT_IMAGE             | The base ubuntu docker image to base the build on                          | ubuntu
| INPUT_TAG               | The base ubuntu docker image tag to base the build on                      | jammy
| INPUT_ARCHITECTURE      | The CPU architecture (amd64/aarch64). Will require QEMU if cross compiling | amd64
| O3DE_REPO               | The git repo for O3DE                                                      | https://github.com/o3de/o3de
| O3DE_BRANCH             | The branch for O3DE                                                        | development
| O3DE_COMMIT             | The commit on the branch for O3DE (or HEAD)                                | HEAD

## Examples
The following example build commands are based on the general use case for O3DE. 

If there are any ROS2 gem based projects that will be created or used by the image, then you will need to follow the **ROS Example** section below.

You can build a minimal Docker image that can create or use non-simulation projects using the `ubuntu` input image and either the `jammy` or `noble` input tag. To build a Docker image that supports robotic simulations using ROS, you must use the `ros` input image and either the `humble` or `jazzy` input tag. (Refer to the [Docker distribution of ROS](https://hub.docker.com/_/ros/))

### Locally

To build the Docker image from the local O3DE Docker context, run the following command from the `Docker` subfolder inside the o3de root folder.

#### General
```
docker build -f Dockerfile --build-arg INPUT_IMAGE=ubuntu --build-arg INPUT_TAG=jammy -t amd64/o3de:ubuntu.jammy .
```
#### ROS2 enabled
```
docker build -f Dockerfile --build-arg INPUT_IMAGE=ros --build-arg INPUT_TAG=humble -t amd64/o3de:ros.humble .
```

### From github
You can also build the Docker image from the O3DE Docker context directly from github with the following command (based on the development branch)

#### General
```
docker build -t amd64/o3de:ubuntu.jammy https://github.com/o3de/o3de.git#development:Docker
```

#### ROS2 enabled
```
docker build --build-arg INPUT_IMAGE=ros --build-arg INPUT_TAG=humble -t amd64/o3de:ros.humble https://github.com/o3de/o3de.git#development:Docker
```


# Running the Docker image locally
Docker containers by themselves are stateless, so in order for the container to act as a stateful container, some local preparations and additional runtime arguments are needed to be passed into the Docker run command.

## Setting up O3DE-specific folders
O3DE will use two folders for its runtime environment. The manifest folder (\$HOME/.o3de) and the O3DE Home folder (\$HOME/O3DE). On normal installations/build of the full O3DE engine, these folders are created on-demand based on the current logged in user. Since Docker containers by default are stateless, all the information that is downloaded, updated, deleted, etc from these two folders as part of normal operations will no longer exist once the container is disconnected, thus losing any work that was done while using O3DE. In order to utilize the O3DE Docker container to create and maintain O3DE projects beyond the container session, we will need to create these two folders manually and pass them in as mapped volumes to its corresponding specific folders within the container. 

The following example creates these folders under a parent `docker` folder in the current user's `$HOME` directory, specific to the name of the Docker image.

```
mkdir -p $HOME/docker/o3de/manifest

mkdir -p $HOME/docker/o3de/home
```

>**Note** The two folders must be created specifically for the O3DE Docker container that is being connected to. There is no version or compatibility checks in the Docker container, so normal O3DE startup in the container may fail if it does not use the same mapped folders it used during its initial launch.

These two folders will be added as part of the `docker run` argument list:

```
-v "$HOME/docker/o3de/manifest:/home/o3de/.o3de" -v "$HOME/docker/o3de/home:/home/o3de/O3DE"
```

## Using the current user id and group id
The Docker image will map the current user and group id to a Docker-created user and group when running the container. This will allow reading and writing to the mapped O3DE-specific folders described earlier and will add persistence to the Docker container through the mapped folders. The user id and group id are passed to the Docker run command as two arguments: `UID` and `GID`:

```
--env UID=$(id -u) --env GID=$(id -g)
```


Putting it all together, you can launch the O3DE Docker container with the following command:

```
xhost +local:root

docker run --env UID=$(id -u) --env GID=$(id -g) -v "$HOME/docker/o3de/manifest:/home/o3de/.o3de" -v "$HOME/docker/o3de/home:/home/o3de/O3DE" --rm --gpus all -e DISPLAY=:1 -v /tmp/.X11-unix:/tmp/.X11-unix -it amd64/o3de:ubuntu.jammy
```

For the ROS container based example, add the `ros.humble` tag.
```
xhost +local:root

docker run --env UID=$(id -u) --env GID=$(id -g) -v "$HOME/docker/o3de/manifest:/home/o3de/.o3de" -v "$HOME/docker/o3de/home:/home/o3de/O3DE" --rm --gpus all -e DISPLAY=:1 -v /tmp/.X11-unix:/tmp/.X11-unix -it amd64/o3de:ros.humble
```

Once the Docker container is launched, you will be logged into the O3DE-enabled terminal. At this point, typing in `o3de` in the command line will launch the O3DE Project Manager where you will be able to create, build, and launch O3DE projects.
