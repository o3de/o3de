# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# The cpu architecture to base the docker base script from
ARG INPUT_ARCHITECTURE=amd64

# The root to base the docker script base from
ARG INPUT_IMAGE=ubuntu:22.04

# Pull in the base ubuntu docker
FROM ${INPUT_ARCHITECTURE}/${INPUT_IMAGE}

# Setup the 'o3de' user for this image (with a default group id, which will be updated in the entrypoint)
ENV O3DE_USER=o3de 
RUN addgroup --gid 1000 "$O3DE_USER" \
    && adduser --gid 1000 --gecos "" --disabled-password "$O3DE_USER"

ENV WORKSPACE=/data/workspace

WORKDIR $WORKSPACE

# O3DE Arguments
ARG O3DE_REPO=https://github.com/o3de/o3de
ARG O3DE_BRANCH=development
ARG O3DE_COMMIT=HEAD

# O3DE Extras Arguments
ARG O3DE_EXTRAS_REPO=https://github.com/o3de/o3de-extras
ARG O3DE_EXTRAS_BRANCH=development
ARG O3DE_EXTRAS_COMMIT=HEAD

# O3DE Environment
ENV O3DE_REPO=$O3DE_REPO
ENV O3DE_BRANCH=$O3DE_BRANCH
ENV O3DE_COMMIT=$O3DE_COMMIT
ENV O3DE_ROOT=$WORKSPACE/o3de

# O3DE Extras Environment Variables
ENV O3DE_EXTRAS_REPO=$O3DE_EXTRAS_REPO
ENV O3DE_EXTRAS_BRANCH=$O3DE_EXTRAS_BRANCH
ENV O3DE_EXTRAS_COMMIT=$O3DE_EXTRAS_COMMIT
ENV O3DE_EXTRAS_ROOT=$WORKSPACE/o3de-extras

ENV LANG=en_US.UTF-8

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
            libpcre2-16-0 \
            libunwind-dev \
            libzstd-dev \
            ninja-build \
            python3-pip \
            software-properties-common \
            sudo

RUN usermod -a -G sudo $O3DE_USER \
    && echo "$O3DE_USER ALL=(ALL:ALL) NOPASSWD: ALL" | tee /etc/sudoers.d/$O3DE_USER

COPY build.sh $WORKSPACE/
COPY entrypoint.sh $WORKSPACE/

RUN git clone --single-branch -b $O3DE_BRANCH $O3DE_REPO $O3DE_ROOT && \
    git -C $O3DE_ROOT lfs install && \
    git -C $O3DE_ROOT lfs pull && \
    git -C $O3DE_ROOT reset --hard $O3DE_COMMIT

RUN $O3DE_ROOT/python/get_python.sh

ENV O3DE_BUILD=$WORKSPACE/build

ENV CONFIGURATION=profilesudo
ENV OUTPUT_DIRECTORY=$O3DE_BUILD
ENV O3DE_PACKAGE_TYPE=DEB
ENV CMAKE_OPTIONS="-G 'Ninja Multi-Config' -DLY_PARALLEL_LINK_JOBS=4 -DLY_DISABLE_TEST_MODULES=TRUE -DO3DE_INSTALL_ENGINE_NAME=o3de-sdk -DLY_STRIP_DEBUG_SYMBOLS=TRUE"
ENV EXTRA_CMAKE_OPTIONS="-DLY_INSTALLER_AUTO_GEN_TAG=TRUE -DLY_INSTALLER_DOWNLOAD_URL=${INSTALLER_DOWNLOAD_URL} -DLY_INSTALLER_LICENSE_URL=${INSTALLER_DOWNLOAD_URL}/license -DO3DE_INCLUDE_INSTALL_IN_PACKAGE=TRUE"
ENV CPACK_OPTIONS="-D CPACK_UPLOAD_URL=${CPACK_UPLOAD_URL}"
ENV CMAKE_TARGET=all

RUN cd $O3DE_ROOT && \
    ./scripts/build/Platform/Linux/build_installer_linux.sh

#RUN su $O3DE_USER -c "sudo dpkg -i $O3DE_BUILD/CPackUploads/o3de_4.2.0.deb"

#RUN cmake -S ${O3DE_ROOT} -B ${O3DE_BUILD} -G 'Ninja Multi-Config' -DLY_DISABLE_TEST_MODULES=TRUE -DO3DE_INSTALL_ENGINE_NAME=o3de-sdk -DLY_STRIP_DEBUG_SYMBOLS=TRUE -DLY_INSTALLER_AUTO_GEN_TAG=TRUE

#ENV O3DE_PACKAGE_TYPE=DEB
#ENV 
# RUN cmake --build ${O3DE_BUILD} --target all --config profile 


#RUN cd $O3DE_ROOT && \
#    python/python.sh $O3DE_ROOT/scripts/build/ci_build.py -p Linux -t installer

#RUN cd $WORKSPACE \
#   && ./build.sh \
#   && rm build.sh

ENV NVIDIA_VISIBLE_DEVICES all
ENV NVIDIA_DRIVER_CAPABILITIES all

ENTRYPOINT ["/bin/bash", "-c"]
#ENTRYPOINT ["/data/workspace/entrypoint.sh"]
