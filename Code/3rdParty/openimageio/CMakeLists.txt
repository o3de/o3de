#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

o3de_find_3rdparty_package(expat)
o3de_find_3rdparty_package(OpenImageIO)

ly_install_files(
    FILES CMakeLists.txt
    DESTINATION Code/3rdParty/openimageio
)
