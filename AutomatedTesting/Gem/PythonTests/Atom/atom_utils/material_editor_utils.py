"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.math as math
import azlmbr.bus as bus
import azlmbr.materialeditor as materialeditor


def get_property(document_id: math.Uuid, property_name: str) -> any:
    """
    Gets a property value for a given document_id and property name.
    :param document_id: The UUID of a given document file.
    :param property_name: The property name to search for the property value.
    :return: property value or invalid value if the document is not open or the property_name can't be found
    """
    return materialeditor.MaterialDocumentRequestBus(bus.Event, "GetPropertyValue", document_id, property_name)


def set_property(
        document_id: math.Uuid, property_name: str, value: math.Uuid or int or float or math.Color) -> None:
    """
    Sets a property value for a given document_id, property_name, and value.
    :param document_id: The UUID of a given document file.
    :param property_name: The property name to search for the property value.
    :param value: The value to set on the property.
    :return: None
    """
    materialeditor.MaterialDocumentRequestBus(bus.Event, "SetPropertyValue", document_id, property_name, value)
