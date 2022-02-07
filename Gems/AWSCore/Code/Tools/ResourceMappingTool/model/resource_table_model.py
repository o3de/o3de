"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import copy
import logging
from typing import List
from PySide2.QtCore import (QAbstractTableModel, QModelIndex, Qt)

from model.resource_abstract_model import ResourceAbstractModel
from model.resource_mapping_attributes import (ResourceMappingAttributes, ResourceMappingAttributesStatus)

logger = logging.getLogger(__name__)

_TABLE_HORIZONTAL_HEADER_NAMES: List[str] =\
    ["Key Name", "Type", "Name/ID", "Account ID", "Region", "Status"]


class ResourceTableConstants(object):
    TABLE_KEYNAME_COLUMN_INDEX: int = 0
    TABLE_TYPE_COLUMN_INDEX: int = 1
    TABLE_NAME_OR_ID_COLUMN_INDEX: int = 2
    TABLE_ACCOUNTID_COLUMN_INDEX: int = 3
    TABLE_REGION_COLUMN_INDEX: int = 4
    TABLE_STATE_COLUMN_INDEX: int = 5


class ResourceTableModelMeta(type(ResourceAbstractModel), type(QAbstractTableModel)):
    """Combine custom abstract meta and Qt meta for solving metaclass conflict"""
    pass


class ResourceTableModel(ResourceAbstractModel, QAbstractTableModel, metaclass=ResourceTableModelMeta):
    """
    Data structure to store all resource mapping attributes.
    This data structure is inheriting from QAbstractTableModel,
    which is defined to support QTableView only
    """
    def __init__(self) -> None:
        super(ResourceTableModel, self).__init__()
        self._resources: List[ResourceMappingAttributes] = []

    def _get_resource_str_type_attribute(self, resource: ResourceMappingAttributes, column: int) -> str:
        if column == ResourceTableConstants.TABLE_KEYNAME_COLUMN_INDEX:
            return resource.key_name
        elif column == ResourceTableConstants.TABLE_TYPE_COLUMN_INDEX:
            return resource.type
        elif column == ResourceTableConstants.TABLE_NAME_OR_ID_COLUMN_INDEX:
            return resource.name_id
        elif column == ResourceTableConstants.TABLE_ACCOUNTID_COLUMN_INDEX:
            return resource.account_id
        elif column == ResourceTableConstants.TABLE_REGION_COLUMN_INDEX:
            return resource.region
        return ""

    def _get_resource_attribute(self, row: int, column: int) -> str:
        if 0 <= row < len(self._resources):
            resource: ResourceMappingAttributes = self._resources[row]
            attribute: str = self._get_resource_str_type_attribute(resource, column)
            if attribute:
                return attribute
            elif column == ResourceTableConstants.TABLE_STATE_COLUMN_INDEX:
                return resource.status.value
        return ""

    def _set_resource_attribute(self, row: int, column: int, value: str) -> None:
        if 0 <= row < len(self._resources):
            resource: ResourceMappingAttributes = self._resources[row]
            old_resource: ResourceMappingAttributes = copy.deepcopy(resource)

            if column == ResourceTableConstants.TABLE_KEYNAME_COLUMN_INDEX:
                resource.key_name = value
            elif column == ResourceTableConstants.TABLE_TYPE_COLUMN_INDEX:
                resource.type = value
            elif column == ResourceTableConstants.TABLE_NAME_OR_ID_COLUMN_INDEX:
                resource.name_id = value
            elif column == ResourceTableConstants.TABLE_ACCOUNTID_COLUMN_INDEX:
                resource.account_id = value
            elif column == ResourceTableConstants.TABLE_REGION_COLUMN_INDEX:
                resource.region = value

            if not old_resource == resource:
                resource.status = \
                    ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.MODIFIED_STATUS_VALUE,
                                                    [ResourceMappingAttributesStatus.MODIFIED_STATUS_DESCRIPTION])

    @property
    def resources(self) -> List[ResourceMappingAttributes]:
        return self._resources

    def columnCount(self, index: QModelIndex = QModelIndex()) -> int:
        """QAbstractTableModel override"""
        return len(_TABLE_HORIZONTAL_HEADER_NAMES)

    def data(self, index: QModelIndex, role: int = Qt.DisplayRole) -> object:
        """QAbstractTableModel override"""
        row: int = index.row()
        column: int = index.column()
        if 0 <= row < len(self._resources):
            resource: ResourceMappingAttributes = self._resources[row]
            attribute: str = self._get_resource_str_type_attribute(resource, column)
            if attribute:
                if role == Qt.DisplayRole or role == Qt.EditRole or role == Qt.ToolTipRole:
                    return attribute
            elif column == ResourceTableConstants.TABLE_STATE_COLUMN_INDEX:
                if role == Qt.ToolTipRole:
                    return resource.status.descriptions_in_tooltip()
                if role == Qt.DisplayRole:
                    return resource.status.value

    def headerData(self, section: int, orientation: int, role: int = Qt.DisplayRole) -> object:
        """QAbstractTableModel override"""
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return _TABLE_HORIZONTAL_HEADER_NAMES[section]

    def index(self, row: int, column: int, parent: QModelIndex = QModelIndex()) -> QModelIndex:
        """QAbstractTableModel override"""
        return self.createIndex(row, column, self._get_resource_attribute(row, column))

    def insertRows(self, row: int, count: int = 1, index: QModelIndex = QModelIndex()) -> bool:
        """QAbstractTableModel override"""
        self.beginInsertRows(QModelIndex(), row, row + count - 1)
        resource_mapping_attributes: ResourceMappingAttributes = ResourceMappingAttributes()
        resource_mapping_attributes.status = \
            ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.MODIFIED_STATUS_VALUE,
                                            [ResourceMappingAttributesStatus.MODIFIED_STATUS_DESCRIPTION])
        self._resources.append(resource_mapping_attributes)
        self.endInsertRows()
        return True

    def flags(self, index: QModelIndex) -> Qt.ItemFlags:
        """QAbstractTableModel override"""
        flags: Qt.ItemFlags = super(self.__class__, self).flags(index)
        if index.column() != ResourceTableConstants.TABLE_STATE_COLUMN_INDEX:
            flags |= Qt.ItemIsEditable
        flags |= Qt.ItemIsSelectable
        flags |= Qt.ItemIsEnabled
        return flags

    def remove_rows(self, row: int, count: int = 1) -> bool:
        """QAbstractTableModel override"""
        if 0 <= row < len(self._resources):
            self.beginRemoveRows(QModelIndex(), row, row + count - 1)
            self._resources.pop(row)
            self.endRemoveRows()
            return True
        return False

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        """QAbstractTableModel override"""
        return len(self._resources)

    def setData(self, index: QModelIndex, value: object, role: int = Qt.EditRole) -> bool:
        """QAbstractTableModel override"""
        if role == Qt.EditRole:
            row: int = index.row()
            column: int = index.column()
            self._set_resource_attribute(row, column, str(value))
            self.dataChanged.emit(index, index)
            return True
        return False

    def add_resource(self, resource: ResourceMappingAttributes) -> None:
        """Add individual resource into table model, and reflect view change accordingly"""
        last_row: int = len(self._resources)
        self.beginInsertRows(QModelIndex(), last_row, last_row)
        self._resources.append(resource)
        self.endInsertRows()

    def load_resource(self, resource: ResourceMappingAttributes) -> None:
        """ResourceAbstractModel override"""
        self._resources.append(resource)

    def remove_resource(self, row: int) -> bool:
        """Remove resource from table model based on row number, and reflect view change accordingly"""
        return self.remove_rows(row)

    def reset(self) -> None:
        """ResourceAbstractModel override"""
        self._resources.clear()
        self.layoutChanged.emit()

    def override_resource_status(self, row: int, resource_status: ResourceMappingAttributesStatus) -> None:
        """Override resource status based on row index, and reflect view change accordingly"""
        if 0 <= row < len(self._resources):
            resource: ResourceMappingAttributes = self._resources[row]
            if not resource.status == resource_status:
                resource.status = resource_status
                self.dataChanged.emit(self.index(row, ResourceTableConstants.TABLE_STATE_COLUMN_INDEX),
                                      self.index(row, ResourceTableConstants.TABLE_STATE_COLUMN_INDEX))

    def override_all_resources_status(self, resource_status: ResourceMappingAttributesStatus) -> None:
        """Override all resources status with new value, and reflect view change accordingly"""
        row_count: int
        for row_count in range(len(self._resources)):
            self.override_resource_status(row_count, resource_status)
