#
# Copyright (c) Contributors to the Open 3D Engine Project
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Plugins should be processed before because we are going to generate the list of plugins 
# that the editor should load
add_subdirectory(Plugins)
add_subdirectory(Editor)
