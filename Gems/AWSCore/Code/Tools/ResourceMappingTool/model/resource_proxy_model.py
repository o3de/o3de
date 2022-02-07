"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
from PySide2.QtCore import (QAbstractItemModel, QModelIndex, QSortFilterProxyModel)
import re
from typing import (List, Set)

from model.basic_resource_attributes import BasicResourceAttributes
from model.resource_abstract_model import ResourceAbstractModel
from model.resource_mapping_attributes import (ResourceMappingAttributes, ResourceMappingAttributesStatus)
from model.resource_table_model import (ResourceTableConstants, ResourceTableModel)
from model.resource_tree_model import (ResourceTreeConstants, ResourceTreeModel)

logger = logging.getLogger(__name__)

_TABLE_MODEL_QUERY_TEMPLATE: str = "key={(.*)},type={(.*)},nameid={(.*)},accountid={(.*)},region={(.*)}"
_TREE_MODEL_QUERY_TEMPLATE: str = "type={(.*)},nameid={(.*)}"


class ResourceProxyModel(QSortFilterProxyModel):
    """ResourceProxyModel
    Custom proxy model is designed to support sorting and filtering data passed between view and source model,
    and it is used as a model interface

    TODO: add error handling when it is ready
    """
    def __init__(self) -> None:
        super(ResourceProxyModel, self).__init__()
        self._filter_text: str = ""

    @property
    def filter_text(self) -> str:
        return self._filter_text

    @filter_text.setter
    def filter_text(self, new_filter_text: str) -> None:
        self._filter_text = new_filter_text
        self.invalidateFilter()

    def _table_model_filter_logic(self, filter_text: str, source_row: int,
                                  source_model: ResourceTreeModel, source_parent: QModelIndex) -> bool:
        match_result: bool = False
        index: int
        for index in range(0, ResourceTableConstants.TABLE_STATE_COLUMN_INDEX):
            model_index: QModelIndex = source_model.index(source_row, index, source_parent)
            match_result |= filter_text in model_index.internalPointer()
        return match_result

    def _tree_model_filter_logic(self, filter_text: str, source_row: int,
                                 source_model: ResourceTreeModel, source_parent: QModelIndex) -> bool:
        match_result: bool = False
        model_index: QModelIndex = source_model.index(source_row, 0, source_parent)
        if model_index.internalPointer().is_root_node() and source_model.has_child:
            return True  # always show root node or matched child nodes won't be presented
        else:
            match_result |= filter_text in model_index.internalPointer().resource.type
            match_result |= filter_text in model_index.internalPointer().resource.name_id
            return match_result

    def filterAcceptsRow(self, source_row: int, source_parent: QModelIndex) -> bool:
        """QSortFilterProxyModel override"""
        source_model: QAbstractItemModel = self.sourceModel()
        assert source_model, "Source model should not be None"

        if not self._filter_text:
            return True  # if search text is empty, show all results

        if isinstance(source_model, ResourceTableModel):
            return self._table_model_filter_logic(self._filter_text, source_row, source_model, source_parent)
        elif isinstance(source_model, ResourceTreeModel):
            return self._tree_model_filter_logic(self._filter_text, source_row, source_model, source_parent)
        else:
            logger.error(f"Unexpected source model {source_model}")

    """ResourceTableModel specific actions"""
    def add_resource(self, resource: ResourceMappingAttributes) -> None:
        """Add resource into source model, and view will reflect to the data changes"""
        source_model: ResourceTableModel = self.sourceModel()
        assert source_model, "Source model should not be None"
        source_model.add_resource(resource)

    def get_resources(self) -> List[ResourceMappingAttributes]:
        """Get resources from source model"""
        source_model: ResourceTableModel = self.sourceModel()
        assert source_model, "Source model should not be None"
        return source_model.resources

    def override_resource_status(self, row: int, attribute_status: ResourceMappingAttributesStatus) -> None:
        """Override column attributes for all resources in source model"""
        source_model: ResourceTableModel = self.sourceModel()
        assert source_model, "Source model should not be None"

        source_model.override_resource_status(row, attribute_status)

    def override_all_resources_status(self, attribute_status: ResourceMappingAttributesStatus) -> None:
        """Override column attributes for all resources in source model"""
        source_model: ResourceTableModel = self.sourceModel()
        assert source_model, "Source model should not be None"

        source_model.override_all_resources_status(attribute_status)

    def remove_resources(self, resource_indices: List[QModelIndex]) -> None:
        """Remove resources based on selected indices from source model, and view will reflect to data changes"""
        source_model: ResourceTableModel = self.sourceModel()
        assert source_model, "Source model should not be None"

        rows_to_delete: Set[int] = set()
        index: QModelIndex
        for index in resource_indices:
            source_index: QModelIndex = self.mapToSource(index)
            rows_to_delete.add(source_index.row())
        sorted_rows: List[int] = sorted(rows_to_delete, reverse=True)
        row: int
        for row in sorted_rows:
            source_model.remove_resource(row)

    """ResourceTreeModel specific actions"""
    def deduplicate_selected_import_resources(self, indices: List[QModelIndex]) -> List[BasicResourceAttributes]:
        """Deduplicate selected indices and return a list of unique import resources"""
        source_model: ResourceTreeModel = self.sourceModel()
        assert source_model, "Source model should not be None"

        if not indices:
            return []

        unique_resources: Set[BasicResourceAttributes] = set()
        model_index: QModelIndex
        for model_index in indices:
            source_index: QModelIndex = self.mapToSource(model_index)
            if source_model.has_child:
                if not source_index.internalPointer().is_root_node():
                    unique_resources.add(source_index.internalPointer().resource)
            else:
                unique_resources.add(source_index.internalPointer().resource)
        return list(unique_resources)

    """Common resource model actions"""
    def emit_source_model_layout_changed(self) -> None:
        """Emit layoutChanged signal on behalf of source model"""
        source_model: QAbstractItemModel = self.sourceModel()
        assert source_model, "Source model should not be None"

        if (isinstance(source_model, ResourceTableModel)
                or isinstance(source_model, ResourceTreeModel)):
            source_model.layoutChanged.emit()
        else:
            logger.error(f"Unexpected source model {source_model}")

    def load_resource(self, resource: BasicResourceAttributes) -> None:
        """Load resource into source model, and view will not reflect to data changes"""
        source_model: ResourceAbstractModel = self.sourceModel()
        assert source_model, "Source model should not be None"

        if (isinstance(source_model, ResourceTableModel)
                or isinstance(source_model, ResourceTreeModel)):
            source_model.load_resource(resource)
        else:
            logger.error(f"Unexpected source model {source_model}")

    def map_from_source_rows(self, source_rows: List[int]) -> List[int]:
        source_model: QAbstractItemModel = self.sourceModel()
        assert source_model, "Source model should not be None"

        proxy_rows: List[int] = []
        row: int
        for row in source_rows:
            proxy_rows.append(self.mapFromSource(source_model.index(row, 0)).row())
        return proxy_rows

    def reset(self) -> None:
        """Reset resources in the source model, and view will reflect to data changes"""
        source_model: ResourceAbstractModel = self.sourceModel()
        assert source_model, "Source model should not be None"

        if (isinstance(source_model, ResourceTableModel)
                or isinstance(source_model, ResourceTreeModel)):
            source_model.reset()
        else:
            logger.error(f"Unexpected source model {source_model}")
