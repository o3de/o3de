#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_COMPILE_DEFINITIONS PRIVATE $<$<CONFIG:debug>:ENABLE_UNDOCACHE_CONSISTENCY_CHECKS>
                                   O3DE_PYTHON_SITE_PACKAGE_SUBPATH="${LY_PYTHON_VENV_SITE_PACKAGES}")
