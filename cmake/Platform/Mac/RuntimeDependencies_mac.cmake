#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_BUILD_FIXUP_BUNDLE TRUE CACHE BOOL "Fix bundles on build (deploys frameworks and calls fixup_bundle)")

set(LY_RUNTIME_DEPENDENCIES_TEMPLATE ${LY_ROOT_FOLDER}/cmake/Platform/Mac/runtime_dependencies_mac.cmake.in)
include(cmake/Platform/Common/RuntimeDependencies_common.cmake)