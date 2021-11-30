"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations


class BasicResourceAttributes(object):
    """
    Data structure to store individual basic resource attributes
    """
    def __init__(self) -> None:
        self._type: str = ""
        self._name_id: str = ""
        self._account_id: str = ""
        self._region: str = ""

    def __eq__(self, other: BasicResourceAttributes) -> bool:
        return (isinstance(other, BasicResourceAttributes)
                and self._type == other.type
                and self._name_id == other.name_id
                and self._account_id == other.account_id
                and self._region == other.region)

    def __hash__(self) -> int:
        return hash((self._type, self._name_id, self._account_id, self._region))

    def __str__(self) -> str:
        return self._print_format()

    def __repr__(self) -> str:
        return self._print_format()

    def _print_format(self) -> str:
        return f"Type:{self._type},NameId:{self._name_id},AccountId:{self._account_id},Region:{self._region}"

    @property
    def type(self) -> str:
        return self._type

    @type.setter
    def type(self, new_type: str) -> None:
        self._type = new_type

    @property
    def name_id(self) -> str:
        return self._name_id

    @name_id.setter
    def name_id(self, new_name_id: str) -> None:
        self._name_id = new_name_id

    @property
    def account_id(self) -> str:
        return self._account_id

    @account_id.setter
    def account_id(self, new_account_id: str) -> None:
        self._account_id = new_account_id

    @property
    def region(self) -> str:
        return self._region

    @region.setter
    def region(self, new_region: str) -> None:
        self._region = new_region

    def is_valid(self) -> bool:
        return not self._type == "" and not self._name_id == "" \
               and not self._account_id == "" and not self._region == ""


class BasicResourceAttributesBuilder(object):
    """
    The BasicResourceAttributesBuilder class provides a builder pattern for BasicResourceAttributes
    """
    def __init__(self) -> None:
        self._resource: BasicResourceAttributes = BasicResourceAttributes()

    def build(self) -> BasicResourceAttributes:
        resource: BasicResourceAttributes = self._resource
        self.reset()
        return resource

    def build_account_id(self, account_id_value: str) -> BasicResourceAttributesBuilder:
        self._resource.account_id = account_id_value
        return self

    def build_name_id(self, name_id_value: str) -> BasicResourceAttributesBuilder:
        self._resource.name_id = name_id_value
        return self

    def build_region(self, region_value: str) -> BasicResourceAttributesBuilder:
        self._resource.region = region_value
        return self

    def build_type(self, type_value: str) -> BasicResourceAttributesBuilder:
        self._resource.type = type_value
        return self

    def reset(self) -> None:
        self._resource = BasicResourceAttributes()
