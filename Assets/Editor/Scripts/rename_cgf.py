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
objects = general.get_all_objects("", "") # Get the name list of all objects in the level.
# If there is any object with the geometry file of "objects\\default\\primitive_box.cgf",
# change it to "objects\\default\\primitive_cube.cgf".
for obj in objects:
    geometry_file = general.get_entity_geometry_file(obj)
    if geometry_file == "objects\\default\\primitive_box.cgf":
        general.set_entity_geometry_file(obj, "objects\\default\\primitive_cube.cgf")
