# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# The cpu architecture 
ARG INPUT_ARCHITECTURE=amd64

# The root docker image 
ARG INPUT_IMAGE=ubuntu

# The root docker tag 
ARG INPUT_TAG=jammy

# Pull in the base ubuntu docker
FROM ${INPUT_ARCHITECTURE}/${INPUT_IMAGE}:${INPUT_TAG}

ARG INPUT_IMAGE
ARG INPUT_TAG

ENV INPUT_IMAGE=$INPUT_IMAGE
ENV WORKSPACE=/data/workspace

WORKDIR $WORKSPACE

# O3DE Arguments
ARG O3DE_REPO=https://github.com/o3de/o3de
ARG O3DE_BRANCH=development
ARG O3DE_COMMIT=HEAD

# O3DE Environment
ENV O3DE_REPO=$O3DE_REPO
ENV O3DE_BRANCH=$O3DE_BRANCH
ENV O3DE_COMMIT=$O3DE_COMMIT
ENV O3DE_ROOT=$WORKSPACE/o3de

ENV LANG=en_US.UTF-8

# Validate the tag if the image is 'ubuntu'
RUN if [ "$INPUT_IMAGE" = "ubuntu" ]; then \
        if [ "$INPUT_TAG" != "jammy" ] && [ "$INPUT_TAG" != "nobel" ]; then \
            echo "ERROR: Unsupported ubuntu tag: ${INPUT_TAG}. Must be either 'jammy' or 'nobel'"; \
            exit 1;\
        fi \
    fi

# Validate the tag if the image is 'ros'
RUN if [ "$INPUT_IMAGE" = "ros" ]; then \
        if [ "$INPUT_TAG" != "humble" ] && [ "$INPUT_TAG" != "jazzy" ]; then \
            echo "ERROR: Unsupported ros tag: ${INPUT_TAG}. Must be either 'humble' or 'jazzy'"; \
            exit 1;\
        fi \
    fi

# Setup time zone and locale data (necessary for SSL and HTTPS packages)
RUN apt-get update \
    && DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata locales keyboard-configuration curl \
    && sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen \
    && dpkg-reconfigure --frontend=noninteractive locales \
    && update-locale LANG=en_US.UTF-8 \
    && rm -rf /var/lib/apt/lists/*

# Install the required ubuntu packages
RUN apt-get update \
    && apt-get install -y \
            binutils \
            clang \
            cmake \
            cmake-qt-gui \
            git \
            git-lfs \
            libglu1-mesa-dev \
            libxcb-xinerama0 \
            libfontconfig1-dev \
            libnvidia-gl-470 \
            libxcb-xkb-dev \
            libxkbcommon-x11-dev \
            libxkbcommon-dev \
            libxcb-xfixes0-dev \
            libxcb-xinput-dev \
            libxcb-xinput0 \
            libxcb-icccm4-dev \
            libxcb-image0-dev \
            libxcb-keysyms1-dev \
            libxcb-render-util0-dev \
            libpcre2-16-0 \
            libunwind-dev \
            libzstd-dev \
            ninja-build \
            python3-pip \
            software-properties-common \
            mesa-common-dev \
            libvulkan1 \
            gedit \
            xdg-utils \
            desktop-file-utils \
            nautilus \
            sudo

# If this is a 'ros' image, add additional ros packages that are used by the current ros project templates.
RUN if [ "$INPUT_IMAGE" = "ros" ]; then \
     apt-get install -y ros-${ROS_DISTRO}-ackermann-msgs \
                        ros-${ROS_DISTRO}-control-toolbox \
                        ros-${ROS_DISTRO}-nav-msgs \
                        ros-${ROS_DISTRO}-gazebo-msgs \
                        ros-${ROS_DISTRO}-xacro \
                        ros-${ROS_DISTRO}-moveit \
                        ros-${ROS_DISTRO}-moveit-resources \
                        ros-${ROS_DISTRO}-depth-image-proc; \
    fi

# Setup the 'o3de' user for this image (with a default group id, which will be updated in the entrypoint)
ENV O3DE_USER=o3de 

RUN addgroup --gid 1000 "$O3DE_USER" \
    && adduser --gid 1000 --gecos "" --disabled-password "$O3DE_USER" \
    && usermod -a -G sudo $O3DE_USER \
    && echo "$O3DE_USER ALL=(ALL:ALL) NOPASSWD: ALL" | tee /etc/sudoers.d/$O3DE_USER

COPY build.sh $WORKSPACE/
COPY entrypoint.sh $WORKSPACE/

# Run the script to clone, build, and install O3DE as an SDK installer
RUN cd $WORKSPACE \
   && ./build.sh \
   && rm build.sh

ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES all

ENTRYPOINT ["/bin/bash", "-c", "/data/workspace/entrypoint.sh"]
