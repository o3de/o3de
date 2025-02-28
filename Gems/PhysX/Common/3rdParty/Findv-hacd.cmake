#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

get_property(v_hacd_gem_root GLOBAL PROPERTY "@GEMROOT:PhysXCommon@")
ly_add_external_target(
    NAME v-hacd
    3RDPARTY_ROOT_DIRECTORY "${v_hacd_gem_root}/3rdParty/v-hacd"
    VERSION
)