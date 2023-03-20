#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

get_target_property(libraries 3rdParty::Python INTERFACE_LINK_LIBRARIES)

set(LY_COMPILE_DEFINITIONS PRIVATE PYTHON_SHARED_LIBRARY_PATH="${libraries}")
