from editor_python_test_tools.editor_component.EditorEntityComponentProperty import EditorEntityComponentProperty
from editor_python_test_tools.editor_entity_utils import EditorComponent

class EditorEntityIntegerProperty(EditorEntityComponentProperty):
    """
    Defines the behaviors for interacting with an Editor Entity boolean property
    """
    def __init__(self, property_path: str, editor_component: EditorComponent, clamped: bool):
        super.__init__(property_path, editor_component, clamped)

    def get(self) -> bool:
        return self.editor_component.get_component_property_value(self.property_path)

    def set(self, value: bool):
        self.editor_component.set_component_property_value(self.property_path, value)
