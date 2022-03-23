#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Temporary fix for the linker issue caused by OpenEXR libs on debug builds
set(LY_LINK_OPTIONS INTERFACE "$<$<CONFIG:debug>:/INCREMENTAL>")
