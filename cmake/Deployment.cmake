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

# Define options that control the different options for deployment for target platforms

ly_set(LY_ASSET_DEPLOY_MODE "LOOSE" CACHE STRING "Set the Asset deployment when deploying to the target platform (LOOSE, PAK, VFS)")
ly_set(LY_OVERRIDE_PAK_FOLDER_ROOT "" CACHE STRING "Optional root path to where Pak file folders are stored. By default, blank will use a predefined 'paks' root.")


