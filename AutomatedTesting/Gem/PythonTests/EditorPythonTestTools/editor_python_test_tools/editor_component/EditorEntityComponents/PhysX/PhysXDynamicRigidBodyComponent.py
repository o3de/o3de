"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

This util is intended to store common test values for editor component tests.
"""

from editor_python_test_tools.editor_component.EditorEntityComponents.EditorEntityComponent import EditorEntityComponent
from editor_python_test_tools.editor_component.EditorComponentProperties.EditorEntityVector3Property import EditorEntityVector3Property
from editor_python_test_tools.editor_component.EditorComponentProperties.EditorEntityFloatProperty import EditorEntityFloatProperty
from editor_python_test_tools.editor_component.EditorComponentProperties.EditorEntityBoolProperty import EditorEntityBoolProperty
from editor_python_test_tools.editor_component.EditorComponentProperties.EditorEntityIntegerProperty import EditorEntityIntegerProperty
from editor_python_test_tools.editor_entity_utils import EditorEntity


class PhysXDynamicRigidBodyComponent(EditorEntityComponent):

    class Path:
        """
        container class for paths to the component's properties
        """
        INITIAL_LINEAR_VELOCITY_PATH = "Configuration|Initial linear velocity"
        INITIAL_ANGULAR_VELOCITY_PATH = "Configuration|Initial angular velocity"
        LINEAR_DAMPENING = "Configuration|Linear damping"
        ANGULAR_DAMPENING = "Configuration|Angular damping"
        SLEEP_THRESHOLD = "Configuration|Sleep threshold"
        START_ASLEEP = "Configuration|Start asleep"
        INTERPOLATE_MOTION = "Configuration|Interpolate motion"
        GRAVITY_ENABLED = "Configuration|Gravity enabled"
        COMPUTE_MASS = "Configuration|Compute Mass"
        LINEAR_ACCESS_LOCKING_X = "Configuration|Linear Axis Locking|Lock X"
        LINEAR_ACCESS_LOCKING_Y = "Configuration|Linear Axis Locking|Lock Y"
        LINEAR_ACCESS_LOCKING_Z = "Configuration|Linear Axis Locking|Lock Z"
        ANGULAR_ACCESS_LOCKING_X = "Configuration|Angular Axis Locking|Lock X"
        ANGULAR_ACCESS_LOCKING_Y = "Configuration|Angular Axis Locking|Lock Y"
        ANGULAR_ACCESS_LOCKING_Z = "Configuration|Angular Axis Locking|Lock Z"
        COMPUTE_COM = "Configuration|Compute COM"
        COMPUTE_INERTIA = "Configuration|Compute inertia"
        MAXIMUM_ANGULAR_VELOCITY = "Configuration|Maximum angular velocity"
        INCLUDE_NON_SIMULATED_SHAPES_IN_MASS = "Configuration|Include non-simulated shapes in Mass"
        DEBUG_DRAW_COM = "Configuration|Debug draw COM"
        SOLVER_POSITION_ITERATIONS = "PhysX-Specific Configuration|Solver Position Iterations"
        SOLVER_VELOCITY_ITERATIONS = "PhysX-Specific Configuration|Solver Velocity Iterations"

    def __init__(self, editor_entity: EditorEntity, component_name="PhysX Dynamic Rigid Body"):
        super().__init__(editor_entity, component_name)
        #Determine if we need a 'Type' property
        self.initial_linear_velocity = EditorEntityVector3Property(self.component, self.Path.INITIAL_LINEAR_VELOCITY_PATH)
        self.initial_angular_velocity = EditorEntityVector3Property(self.component, self.Path.INITIAL_ANGULAR_VELOCITY_PATH)
        self.linear_dampening = EditorEntityFloatProperty(self.component, self.Path.LINEAR_DAMPENING)
        self.angular_dampening = EditorEntityFloatProperty(self.component, self.Path.ANGULAR_DAMPENING)
        self.sleep_threshold = EditorEntityFloatProperty(self.component, self.Path.SLEEP_THRESHOLD)
        self.start_asleep = EditorEntityBoolProperty(self.component, self.Path.START_ASLEEP)
        self.interpolate_motion = EditorEntityBoolProperty(self.component, self.Path.INTERPOLATE_MOTION)
        self.gravity_enabled = EditorEntityBoolProperty(self.component, self.Path.GRAVITY_ENABLED)
        #Determine if we need a 'CCD' property
        self.compute_mass = EditorEntityBoolProperty(self.component, self.Path.COMPUTE_MASS)
        self.linear_access_locking_x = EditorEntityBoolProperty(self.component, self.Path.LINEAR_ACCESS_LOCKING_X)
        self.linear_access_locking_y = EditorEntityBoolProperty(self.component, self.Path.LINEAR_ACCESS_LOCKING_Y)
        self.linear_access_locking_z = EditorEntityBoolProperty(self.component, self.Path.LINEAR_ACCESS_LOCKING_Z)
        self.angular_access_locking_x = EditorEntityBoolProperty(self.component, self.Path.ANGULAR_ACCESS_LOCKING_X)
        self.angular_access_locking_y = EditorEntityBoolProperty(self.component, self.Path.ANGULAR_ACCESS_LOCKING_Y)
        self.angular_access_locking_z = EditorEntityBoolProperty(self.component, self.Path.ANGULAR_ACCESS_LOCKING_Z)
        self.compute_COM = EditorEntityBoolProperty(self.component, self.Path.COMPUTE_COM)
        self.compute_inertia = EditorEntityBoolProperty(self.component, self.Path.COMPUTE_INERTIA)
        self.maximum_angular_velocity = EditorEntityFloatProperty(self.component, self.Path.MAXIMUM_ANGULAR_VELOCITY)
        self.include_non_simulated_shapes_in_mass = EditorEntityBoolProperty(self.component, self.Path.INCLUDE_NON_SIMULATED_SHAPES_IN_MASS)
        self.debug_draw_COM = EditorEntityBoolProperty(self.component, self.Path.DEBUG_DRAW_COM)
        self.solver_position_iterations = EditorEntityIntegerProperty(self.component, self.Path.SOLVER_POSITION_ITERATIONS)
        self.solver_velocity_iterations = EditorEntityIntegerProperty(self.component, self.Path.SOLVER_VELOCITY_ITERATIONS)
