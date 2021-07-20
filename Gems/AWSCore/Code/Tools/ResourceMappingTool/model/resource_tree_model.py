"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations
import logging
from typing import List
from PySide2.QtCore import (QAbstractItemModel, QModelIndex, Qt)

from model import constants
from model.basic_resource_attributes import BasicResourceAttributes
from model.resource_abstract_model import ResourceAbstractModel

logger = logging.getLogger(__name__)

_TREE_HORIZONTAL_HEADER_NAMES: List[str] = ["Type", "Name/ID"]


class ResourceTreeConstants(object):
    TREE_TYPE_COLUMN_INDEX: int = 0
    TREE_NAME_OR_ID_COLUMN_INDEX: int = 1


class ResourceTreeNode(object):
    """
    Data structure to store imported resource attributes, row number,
    parent object, and child objects, which is defined to support
    Qt tree model only
    """
    def __init__(self, resource: BasicResourceAttributes, parent: ResourceTreeNode, row: int) -> None:
        self._resource: BasicResourceAttributes = resource
        self._row: int = row
        self._parent: ResourceTreeNode = parent
        self._children: List[ResourceTreeNode] = []

    @property
    def resource(self) -> BasicResourceAttributes:
        return self._resource

    @property
    def row(self) -> int:
        return self._row

    @property
    def parent(self) -> ResourceTreeNode:
        return self._parent

    @property
    def children(self) -> List[ResourceTreeNode]:
        return self._children

    def add_child(self, resource) -> None:
        """Add child resource"""
        child_row: int = len(self._children)
        self._children.append(ResourceTreeNode(resource, self, child_row))

    def is_root_node(self):
        """Return whether the node is root or not"""
        return self._parent is None


class ResourceTreeModelMeta(type(ResourceAbstractModel), type(QAbstractItemModel)):
    """Combine custom abstract meta and Qt meta for solving metaclass conflict"""
    pass


class ResourceTreeModel(ResourceAbstractModel, QAbstractItemModel, metaclass=ResourceTreeModelMeta):
    """
    Data structure to store all imported resource attributes, and the hierarchy
    between each CFN stack and its resources. This data structure is inheriting
    from QAbstractItemModel, which is defined to support QTreeView only
    """
    def __init__(self, has_child: bool = True) -> None:
        super(ResourceTreeModel, self).__init__()
        self._has_child: bool = has_child
        self._resources: List[ResourceTreeNode] = []

    @property
    def has_child(self) -> bool:
        return self._has_child

    @has_child.setter
    def has_child(self, new_has_child: bool) -> None:
        self._has_child = new_has_child

    def columnCount(self, index: QModelIndex = QModelIndex()) -> int:
        """QAbstractItemModel override"""
        return len(_TREE_HORIZONTAL_HEADER_NAMES)

    def data(self, index: QModelIndex, role: int = Qt.DisplayRole) -> object:
        """QAbstractItemModel override"""
        if not index.isValid():
            return None
        node: ResourceTreeNode = index.internalPointer()
        if role == Qt.DisplayRole or role == Qt.ToolTipRole:
            column: int = index.column()
            if column == ResourceTreeConstants.TREE_TYPE_COLUMN_INDEX:
                return node.resource.type
            elif column == ResourceTreeConstants.TREE_NAME_OR_ID_COLUMN_INDEX:
                return node.resource.name_id
        return None

    def headerData(self, section: int, orientation: int, role: int = Qt.DisplayRole) -> object:
        """QAbstractItemModel override"""
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return _TREE_HORIZONTAL_HEADER_NAMES[section]
        return None

    def index(self, row: int, column: int, parent: QModelIndex = QModelIndex()) -> QModelIndex:
        """QAbstractItemModel override"""
        if not parent.isValid():
            return self.createIndex(row, column, self._resources[row])
        parent_node: ResourceTreeNode = parent.internalPointer()
        return self.createIndex(row, column, parent_node.children[row])

    def parent(self, child: QModelIndex) -> QModelIndex:
        """QAbstractItemModel override"""
        if not child.isValid():
            return QModelIndex()
        node: ResourceTreeNode = child.internalPointer()
        parent_node: ResourceTreeNode = node.parent
        if parent_node is None:
            return QModelIndex()
        else:
            return self.createIndex(parent_node.row, 0, parent_node)

    def rowCount(self, parent: QModelIndex = QModelIndex()) -> int:
        """QAbstractItemModel override"""
        if not parent.isValid():
            return len(self._resources)
        node: ResourceTreeNode = parent.internalPointer()
        return len(node.children)

    def load_resource(self, resource: BasicResourceAttributes) -> None:
        """ResourceAbstractModel override"""
        if resource.type == constants.AWS_RESOURCE_CLOUDFORMATION_STACK_TYPE or not self._has_child:
            # when resource type is cloudformation or resource doesn't have child resource, add it as root node
            root_row: int = len(self._resources)
            self._resources.append(ResourceTreeNode(resource, None, root_row))
        else:
            if not self._resources:
                logger.error(f"No resource root node found, but trying to add child resource {resource}")
                return

            root_node: ResourceTreeNode = self._resources[-1]
            root_node.add_child(resource)

    def reset(self) -> None:
        """ResourceAbstractModel override"""
        self._resources.clear()
        self.layoutChanged.emit()
