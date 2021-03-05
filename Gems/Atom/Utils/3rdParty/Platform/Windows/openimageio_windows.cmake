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

set(OPENIMAGEIO_LIBS 
    ${BASE_PATH}/win_x64/lib/OpenColorIO.lib
    ${BASE_PATH}/win_x64/lib/OpenImageIO.lib
    ${BASE_PATH}/win_x64/lib/OpenImageIO_Util.lib
)

set(OPENIMAGEIO_RUNTIME_DEPENDENCIES 
    ${BASE_PATH}/win_x64/bin/boost_filesystem-vc141-mt-x64-1_73.dll
    ${BASE_PATH}/win_x64/bin/boost_thread-vc141-mt-x64-1_73.dll
    ${BASE_PATH}/win_x64/bin/Half-2_5.dll
    ${BASE_PATH}/win_x64/bin/heif.dll
    ${BASE_PATH}/win_x64/bin/Iex-2_5.dll
    ${BASE_PATH}/win_x64/bin/IlmImf-2_5.dll
    ${BASE_PATH}/win_x64/bin/IlmThread-2_5.dll
    ${BASE_PATH}/win_x64/bin/Imath-2_5.dll
    ${BASE_PATH}/win_x64/bin/jpeg62.dll
    ${BASE_PATH}/win_x64/bin/lzma.dll
    ${BASE_PATH}/win_x64/bin/libpng16.dll
    ${BASE_PATH}/win_x64/bin/tiff.dll
    ${BASE_PATH}/win_x64/bin/zlib1.dll
    ${BASE_PATH}/win_x64/bin/yaml-cpp.dll

    ${BASE_PATH}/win_x64/bin/OpenColorIO.dll
    ${BASE_PATH}/win_x64/bin/OpenImageIO.dll
    ${BASE_PATH}/win_x64/bin/OpenImageIO_Util.dll

    ${BASE_PATH}/win_x64/bin/iconvert.exe
    ${BASE_PATH}/win_x64/bin/idiff.exe
    ${BASE_PATH}/win_x64/bin/igrep.exe
    ${BASE_PATH}/win_x64/bin/iinfo.exe
    ${BASE_PATH}/win_x64/bin/maketx.exe
    ${BASE_PATH}/win_x64/bin/oiiotool.exe
)
