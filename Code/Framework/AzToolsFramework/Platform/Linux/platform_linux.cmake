#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(compile_defs $<$<CONFIG:debug>:ENABLE_UNDOCACHE_CONSISTENCY_CHECKS>
                 O3DE_PYTHON_SITE_PACKAGE_SUBPATH="${LY_PYTHON_VENV_SITE_PACKAGES}" 
                 PYTHON_SHARED_LIBRARY="${LY_PYTHON_SHARED_LIB}"
                 AZ_TRAIT_PYTHON_LOADER_PYTHON_HOME_BIN_SUBPATH)

if (NOT O3DE_INSTALLER_BUILD)
    list(APPEND compile_defs AZ_TRAIT_PYTHON_LOADER_ENABLE_EXPLICIT_LOADING)
endif()

set(LY_COMPILE_DEFINITIONS PRIVATE ${compile_defs})
