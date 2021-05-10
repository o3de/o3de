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

string(TIMESTAMP current_year "%Y")
set(LY_VERSION_COPYRIGHT_YEAR ${current_year} CACHE STRING "Open 3D Engine's copyright year")
set(LY_VERSION_STRING "0.0.0.0" CACHE STRING "Open 3D Engine's version")
set(LY_VERSION_BUILD_NUMBER 0 CACHE STRING "Open 3D Engine's build number")
set(LY_VERSION_ENGINE_NAME "o3de" CACHE STRING "Open 3D Engine's engine name")
