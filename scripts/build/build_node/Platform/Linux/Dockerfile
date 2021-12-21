#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# This is a base Dockerfile to use for self-containing local or remote development environments
#
# Once docker is installed, build a local image with this command:
# `docker build /localDockerfilepath -t ubuntu-build:latest`
#
# To build using a local repo on disk, run this command:
# `docker run -it -v /localo3depath:/data/workspace/o3de -v /localbuildpath:/data/workspace/o3de/build -v /local3rdPartypath:/root/.o3de/3rdParty \
# --name build-o3de -d ubuntu-build:latest /bin/sh -c 'cd /data/workspace/o3de && python/python.sh -u scripts/build/ci_build.py --platform Linux --type profile'`
# 
# Attach to the running build to interact or view logs using this command:
# `docker attach build-o3de`

FROM ubuntu:20.04
 
WORKDIR /data/workspace

# Initilize apt cache
RUN apt-get clean && apt-get update

# Setup time zone and locale data (necessary for SSL and HTTPS packages)
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install tzdata locales keyboard-configuration

RUN sed -i -e 's/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/' /etc/locale.gen && \
    dpkg-reconfigure --frontend=noninteractive locales && \
    update-locale LANG=en_US.UTF-8

ENV LANG en_US.UTF-8

# Install common tools
RUN apt-get -y install tar sudo less vim lsof firewalld net-tools pciutils \
                   file wget kmod xz-utils ca-certificates binutils kbd \
                   python3-pip bind9-utils jq bc unzip git git-lfs lsb-release \
                   software-properties-common
 
# Install build and development tools
RUN git clone --no-checkout https://github.com/o3de/o3de.git .o3de && \
    cd .o3de && \
    git sparse-checkout init --cone && \
    git sparse-checkout set scripts/build/build_node && \
    cd scripts/build/build_node/Platform/Linux && \
    ./install-ubuntu.sh

# Install supported version of cmake if build tool installation runs into issues
ENV CMAKE_VER 3.21.1-0kitware1ubuntu20.04.1
RUN $(cmake --version) || apt-get -y install cmake=${CMAKE_VER} cmake-data=${CMAKE_VER}

# Symlink clang version to non-versioned clang and set cc to clang
RUN find /usr/bin/ -name clang*  | sed -E 's/^(\/usr\/bin\/.*)(\-[0-9]*)$/ln -s -v \1\2 \1/' | xargs -d '\n' -n 1 bash -c && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100
