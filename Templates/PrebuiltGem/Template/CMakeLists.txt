# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# {END_LICENSE}

# Setup the gem meta variables that includes the "gem_path", "gem_name" and "gem_version"
# The "pal_dir" variable is also set which refers to the Platform/<platform-name> directory
# based on the directory of this CMakeLists.txt
o3de_gem_setup("${Name}")

add_subdirectory(Code)
