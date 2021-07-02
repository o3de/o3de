#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
objects = general.get_all_objects("AnimObject", "") # Get the name list of all anim objects in the level.
general.clear_selection()
# If there is any object with the geometry file whose name contains "\\story\\", select it
for obj in objects:
    geometry_file = general.get_entity_geometry_file(obj)
    if geometry_file.find("\\story\\") != -1:
        general.select_object(obj)
