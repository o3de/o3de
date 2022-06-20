"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import PySide2

import editor_python_test_tools.pyside_utils as pyside_utils

def get_component_combobox_values(component_name, property_name, log_fn=None):
    """
    Retrieves the Combo box values from a Component. Assumes the Entity has been selected and that the component is
    visible in the Entity Inspector.
    Works by inspecting the list of child widgets for the Entity from the Entity Inspector, looking for the right type
    of widget (QFrame) with a label containing the name of the component. Then, the QFrame is inspected to find a
    QComboBox with the property name.

    :param component_name: Name of the component to inspect.
    :param property_name: Name of the property to inspect.
    :param log_fn: Function used to log messages, should take a string as argument like log_fn('message to log').
    :return: A list containing the values from the Combo box.
    """

    def _log_fn_wrapper(message):
        log_fn(message) if log_fn else None

    editor_window = pyside_utils.get_editor_main_window()
    entity_inspector = editor_window.findChild(PySide2.QtWidgets.QDockWidget, 'Entity Inspector')
    assert entity_inspector, 'Entity Inspector widget is not valid.'
    entity_inspector.update()

    component_list_widget = entity_inspector.findChild(PySide2.QtWidgets.QWidget, 'm_componentListContents')
    component_list_children = component_list_widget.children()
    assert component_list_children, 'Could not retrieve components for the entity.'

    # Iterate over the widgets that are children of the component list. On each one, retrieve the first child QFrame
    # that has a label with the "component_name" as text.
    # If that label is found, the same QFrame is inspected for a child frame with the name matching the "property_name".
    # This property frame should have the combo box as child.
    for component_widget in component_list_children:
        if type(component_widget) is PySide2.QtWidgets.QFrame:
            component_label = pyside_utils.find_child_by_pattern(component_widget, {'text': component_name})
            if component_label:
                property_frame = component_widget.findChild(PySide2.QtWidgets.QFrame, property_name)

                if not property_frame:
                    _log_fn_wrapper(f'QFrame not found as child of the component widget {component_widget}.')
                    continue

                combobox = pyside_utils.find_child_by_pattern(property_frame, {'type': PySide2.QtWidgets.QComboBox})

                if not combobox:
                    _log_fn_wrapper(f'QComboBox not found as child of the property frame {property_frame}.')
                    continue

                item_count = combobox.count()

                values = []
                for index in range(item_count):
                    values.append(combobox.itemText(index))

                if not values:
                    _log_fn_wrapper('The QComboBox does not have values to retrieve.')

                return values

    _log_fn_wrapper('Matching component and property not found in Component list, or the list is empty.')
    return None
