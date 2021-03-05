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
objects = general.get_all_objects("AnimObject", "") # Get the name list of all anim objects in the level.
general.clear_selection()
# If there is any object with the geometry file whose name contains "\\story\\", select it
for obj in objects:
    geometry_file = general.get_entity_geometry_file(obj)
    if geometry_file.find("\\story\\") != -1:
        general.select_object(obj)
