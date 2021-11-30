#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# RHI/Metal requires Catalina which we have just one node in Jenkins. Atom can enable
# this again in their stream, but cannot push this change until we put more Catalina
# nodes in Jenkins
set(PAL_TRAIT_ATOM_RHI_METAL_SUPPORTED TRUE)
