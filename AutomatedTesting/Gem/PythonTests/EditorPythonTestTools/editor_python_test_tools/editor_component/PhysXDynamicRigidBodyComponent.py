from editor_python_test_tools.editor_component.EditorEntityComponent import EditorEntityComponent
from editor_python_test_tools.editor_component import EditorEntityVector3Property, EditoryEntityFloatProperty, \
    EditorEntityIntegerProperty, EditorEntityBoolProperty
from editor_python_test_tools.editor_entity_utils import EditorComponent


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
        SOLVER_POSITION_ITERATIONS = "Specific Configuration|Solver Position Iterations"
        SOLVER_VELOCITY_ITERATIONS = "Specific Configuration|Solver Velocity Iterations"

    def __init__(self, editor_component: EditorComponent, component_name: str) -> None:
        super.__init__(editor_component, component_name)
        #Need another property class for the 'Type' property
        self.initial_linear_velocity = EditorEntityVector3Property(editor_component, self.Path.INITIAL_LINEAR_VELOCITY_PATH)
        self.initial_angular_velocity = EditorEntityVector3Property(editor_component, self.Path.INITIAL_ANGULAR_VELOCITY_PATH)
        self.linear_dampening = EditoryEntityFloatProperty(editor_component, self.Path.LINEAR_DAMPENING)
        self.angular_dampening = EditoryEntityFloatProperty(editor_component, self.Path.ANGULAR_DAMPENING)
        self.sleep_threshold = EditoryEntityFloatProperty(editor_component, self.Path.SLEEP_THRESHOLD)
        self.start_asleep = EditorEntityBoolProperty(editor_component, self.Path.START_ASLEEP)
        self.interpolate_motion = EditorEntityBoolProperty(editor_component, self.Path.INTERPOLATE_MOTION)
        self.gravity_enabled = EditorEntityBoolProperty(editor_component, self.Path.GRAVITY_ENABLED)
        #Need another property class for 'CCD....maybe'
        self.compute_mass = EditorEntityBoolProperty(editor_component, self.Path.COMPUTE_MASS)
        self.linear_access_locking_x = EditorEntityBoolProperty(editor_component, self.Path.LINEAR_ACCESS_LOCKING_X)
        self.linear_access_locking_y = EditorEntityBoolProperty(editor_component, self.Path.LINEAR_ACCESS_LOCKING_Y)
        self.linear_access_locking_z = EditorEntityBoolProperty(editor_component, self.Path.LINEAR_ACCESS_LOCKING_Z)
        self.angular_access_locking_x = EditorEntityBoolProperty(editor_component, self.Path.ANGULAR_ACCESS_LOCKING_X)
        self.angular_access_locking_y = EditorEntityBoolProperty(editor_component, self.Path.ANGULAR_ACCESS_LOCKING_Y)
        self.angular_access_locking_z = EditorEntityBoolProperty(editor_component, self.Path.ANGULAR_ACCESS_LOCKING_Z)
        self.compute_COM = EditorEntityBoolProperty(editor_component, self.Path.COMPUTE_COM)
        self.compute_inertia = EditorEntityBoolProperty(editor_component, self.Path.COMPUTE_INERTIA)
        self.maximum_angular_velocity = EditoryEntityFloatProperty(editor_component, self.Path.MAXIMUM_ANGULAR_VELOCITY)
        self.include_non_simulated_shapes_in_mass = EditorEntityBoolProperty(editor_component, self.Path.INCLUDE_NON_SIMULATED_SHAPES_IN_MASS)
        self.debug_draw_COM = EditorEntityBoolProperty(editor_component, self.Path.DEBUG_DRAW_COM)
        self.solver_position_iterations = EditorEntityIntegerProperty(editor_component, self.Path.SOLVER_POSITION_ITERATIONS)
        self.solver_velocity_iterations = EditorEntityIntegerProperty(editor_component, self.Path.SOLVER_VELOCITY_ITERATIONS)

