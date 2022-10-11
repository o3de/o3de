"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Holds constants used across both hydra and non-hydra scripts.
"""

import azlmbr.math as math

class EditorEntityBehaviors:
    def __init__(self, editor_entity: EditorEntity) -> None:
        self.component = editor_entity.add_component(PHYSX_COLLIDER)

# Component Property Behaviors
    def toggle_component_switch(self, component_property_path: str) -> None:
        """

        """
        start_value = self.component.get_component_property_value(component_property_path)
        if start_value:
            self.component.set_component_property_value(component_property_path, False)
        else:
            self.component.set_component_property_value(component_property_path, True)

        end_value = self.component.get_component_property_value(component_property_path)

        assert (start_value == end_value), \
            f"Failure: Could not toggle the switch for {self.get_component_name()} : {component_property_path}."

    def set_vector3_component_property(self, component_property_path: str, position: azlmbr.math.Vector3) -> None:
        """

        """
        self.component.set_component_property_value(component_property_path, position)

        set_position = self.component.get_component_property_value(component_property_path)
        assert (position == set_position), \
            f"Failure: Expected Position did not match Set Position when setting the vector3 on " \
            f"{self.component.get_component_name()} : {component_property_path}."