"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations
from typing import List

from model.basic_resource_attributes import (BasicResourceAttributes, BasicResourceAttributesBuilder)


class ResourceMappingAttributesStatus(object):
    """
    Data structure to store resource mapping attributes status info
    """
    SUCCESS_STATUS_VALUE: str = "Success"
    MODIFIED_STATUS_VALUE: str = "Modified"
    MODIFIED_STATUS_DESCRIPTION: str = "Resource modified. Choose <b>Save Changes</b> to keep the changes."
    FAILURE_STATUS_VALUE: str = "Failure"

    def __init__(self, value: str = "", descriptions: List[str] = []) -> None:
        self._value: str = value
        self._descriptions: List[str] = descriptions

    def __eq__(self, other: ResourceMappingAttributesStatus) -> bool:
        return (isinstance(other, ResourceMappingAttributesStatus)
                and self._value == other.value
                and self._descriptions == other.descriptions)

    def __str__(self) -> str:
        return self._print_format()

    def __repr__(self) -> str:
        return self._print_format()

    def _print_format(self) -> str:
        return self._value

    @property
    def value(self) -> str:
        return self._value

    @property
    def descriptions(self) -> List[str]:
        return self._descriptions

    def descriptions_in_tooltip(self) -> str:
        descriptions_in_tooltip: str = ""
        description: str
        for description in self._descriptions:
            descriptions_in_tooltip += f"<p>{description}</p>"
        return descriptions_in_tooltip


class ResourceMappingAttributes(BasicResourceAttributes):
    """
    Data structure to store individual resource mapping attributes
    """
    def __init__(self) -> None:
        super(ResourceMappingAttributes, self).__init__()
        self._key_name: str = ""
        self._status: ResourceMappingAttributesStatus = ResourceMappingAttributesStatus()

    def __eq__(self, other: ResourceMappingAttributes) -> bool:
        return (isinstance(other, ResourceMappingAttributes)
                and self._key_name == other.key_name
                and super(ResourceMappingAttributes, self).__eq__(other))

    def __hash__(self) -> int:
        return hash((self._key_name, super(ResourceMappingAttributes, self).__hash__()))

    def _print_format(self) -> str:
        return f"Key:{self._key_name},{super(ResourceMappingAttributes, self)._print_format()},Status:{self._status}"

    @property
    def key_name(self) -> str:
        return self._key_name

    @key_name.setter
    def key_name(self, new_key_name) -> None:
        self._key_name = new_key_name

    @property
    def status(self) -> ResourceMappingAttributesStatus:
        return self._status

    @status.setter
    def status(self, new_status: ResourceMappingAttributesStatus) -> None:
        self._status = new_status

    def is_valid(self) -> bool:
        return not self._key_name == "" and super(ResourceMappingAttributes, self).is_valid()


class ResourceMappingAttributesBuilder(BasicResourceAttributesBuilder):
    """
    The ResourceMappingAttributesBuilder class provides a builder pattern for ResourceMappingAttributes
    """
    def __init__(self) -> None:
        super(ResourceMappingAttributesBuilder, self).__init__()
        self.reset()

    def build_key_name(self, key_name_value: str) -> ResourceMappingAttributesBuilder:
        self._resource.key_name = key_name_value
        return self

    def build_status(self, status_value: ResourceMappingAttributesStatus) -> ResourceMappingAttributesBuilder:
        self._resource.status = status_value
        return self

    def reset(self) -> None:
        self._resource = ResourceMappingAttributes()
