#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(LIBAV_LIB_PATH ${BASE_PATH}/usr/bin/)
set(LIBAV_LIBS 
    avcodec
    avdevice
    avfilter
    avformat
    avresample
    avutil
    swscale
)

list(TRANSFORM LIBAV_LIBS PREPEND ${LIBAV_LIB_PATH}${CMAKE_STATIC_LIBRARY_PREFIX})
list(TRANSFORM LIBAV_LIBS APPEND "${CMAKE_STATIC_LIBRARY_SUFFIX}")


set(LIBAV_SHARED 
    avcodec-56
    avdevice-55
    avfilter-5
    avformat-56
    avresample-2
    avutil-54
    libogg-0
    libopus-0
    libvo-aacenc-0
    libvorbis-0
    libvorbisenc-2
    swscale-3
    zlib1
)

list(TRANSFORM LIBAV_SHARED PREPEND ${LIBAV_LIB_PATH}${CMAKE_SHARED_LIBRARY_PREFIX})
list(TRANSFORM LIBAV_SHARED APPEND "${CMAKE_SHARED_LIBRARY_SUFFIX}")
list(JOIN LIBAV_SHARED ";" LIBAV_SHARED_STRING)
set(LIBAV_RUNTIME_DEPENDENCIES ${LIBAV_SHARED_STRING})
