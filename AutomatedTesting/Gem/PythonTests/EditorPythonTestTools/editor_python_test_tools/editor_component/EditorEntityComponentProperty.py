from editor_python_test_tools.editor_entity_utils import EditorComponent


class EditorEntityComponentProperty:

    def __init__(self, editor_component: EditorComponent, path: str, clamped: bool = True):
        self.property_path = path
        self.editor_component = editor_component
        self.clamped = clamped
