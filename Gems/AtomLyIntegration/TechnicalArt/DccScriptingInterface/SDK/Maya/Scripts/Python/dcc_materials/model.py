# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from PySide2.QtCore import QAbstractItemModel, QModelIndex, Qt


class MaterialsModel(QAbstractItemModel):
    def __init__(self, headers, data, parent=None):
        super(MaterialsModel, self).__init__(parent)

        self.rootItem = TreeNode(headers)
        self.parents = [self.rootItem]
        self.indentations = [0]
        self.create_data(data)

    def create_data(self, data, indent=-1):
        """
        Recursive loop that structures Model data into tree form.
        :param data: Row information.
        :param indent: Column information. This helps to facilitate the creation of nested rows.
        :return:
        """
        if type(data) == dict:
            indent += 1
            position = 4 * indent
            for key, value in data.items():
                if position > self.indentations[-1]:
                    if self.parents[-1].childCount() > 0:
                        self.parents.append(self.parents[-1].child(self.parents[-1].childCount() - 1))
                        self.indentations.append(position)
                else:
                    while position < self.indentations[-1] and len(self.parents) > 0:
                        self.parents.pop()
                        self.indentations.pop()
                parent = self.parents[-1]
                parent.insertChildren(parent.childCount(), 1, parent.columnCount())
                parent.child(parent.childCount() - 1).setData(0, key)
                value_string = str(value) if type(value) != dict else str('')
                parent.child(parent.childCount() - 1).setData(1, value_string)
                try:
                    self.create_data(value, indent)
                except RuntimeError:
                    pass

    @staticmethod
    def get_attribute_value(search_string, search_column):
        """ Convenience function for quickly accessing row information based on attribute keys. """
        for childIndex in range(search_column.childCount()):
            child_item = search_column.child(childIndex)
            child_value = child_item.itemData
            if child_value[0] == search_string:
                return child_value[1]
        return None

    def index(self, row, column, index=QModelIndex()):
        """ Returns the index of the item in the model specified by the given row, column and parent index """
        if not self.hasIndex(row, column, index):
            return QModelIndex()
        if not index.isValid():
            item = self.rootItem
        else:
            item = index.internalPointer()

        child = item.child(row)
        if child:
            return self.createIndex(row, column, child)
        return QModelIndex()

    def parent(self, index):
        """
        Returns the parent of the model item with the given index If the item has no parent,
        an invalid QModelIndex is returned
        """
        if not index.isValid():
            return QModelIndex()
        item = index.internalPointer()
        if not item:
            return QModelIndex()

        parent = item.parentItem
        if parent == self.rootItem:
            return QModelIndex()
        else:
            return self.createIndex(parent.childNumber(), 0, parent)

    def rowCount(self, index=QModelIndex()):
        """
        Returns the number of rows under the given parent. When the parent is valid it means that
        rowCount is returning the number of children of parent
        """
        if index.isValid():
            parent = index.internalPointer()
        else:
            parent = self.rootItem
        return parent.childCount()

    def columnCount(self, index=QModelIndex()):
        """ Returns the number of columns for the children of the given parent """
        return self.rootItem.columnCount()

    def data(self, index, role=Qt.DisplayRole):
        """ Returns the data stored under the given role for the item referred to by the index """
        if index.isValid() and role == Qt.DisplayRole:
            return index.internalPointer().data(index.column())
        elif not index.isValid():
            return self.rootItem.data(index.column())

    def headerData(self, section, orientation, role=Qt.DisplayRole):
        """ Returns the data for the given role and section in the header with the specified orientation """
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return self.rootItem.data(section)


class TreeNode(object):
    def __init__(self, data, parent=None):
        self.parentItem = parent
        self.itemData = data
        self.children = []

    def child(self, row):
        return self.children[row]

    def childCount(self):
        return len(self.children)

    def childNumber(self):
        if self.parentItem is not None:
            return self.parentItem.children.index(self)

    def columnCount(self):
        return len(self.itemData)

    def data(self, column):
        return self.itemData[column]

    def insertChildren(self, position, count, columns):
        if position < 0 or position > len(self.children):
            return False
        for row in range(count):
            data = [v for v in range(columns)]
            item = TreeNode(data, self)
            self.children.insert(position, item)

    def parent(self):
        return self.parentItem

    def setData(self, column, value):
        if column < 0 or column >= len(self.itemData):
            return False
        self.itemData[column] = value
