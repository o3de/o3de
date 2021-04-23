"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.landscapecanvas as landscapecanvas

import editor_python_test_tools.hydra_editor_utils as hydra


def find_nodes_matching_entity_component(component_name, entity_id):
    """
    Finds all matching nodes given a component name and entity to search on.
    :param component_name: String of component name to search for
    :param entity_id: The entity ID to search for the given component node on
    :return List of matching nodes
    """
    component = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentOfType', entity_id,
                                             hydra.get_component_type_id(component_name))
    if component.IsSuccess():
        component_id = component.GetValue()
        component_node = landscapecanvas.LandscapeCanvasRequestBus(bus.Broadcast, 'GetAllNodesMatchingEntityComponent',
                                                                   component_id)
        if component_node:
            print(f"{component_name} node found on entity {entity_id.ToString()}")
        return component_node
