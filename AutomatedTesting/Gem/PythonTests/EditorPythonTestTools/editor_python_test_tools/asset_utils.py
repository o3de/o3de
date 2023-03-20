"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Built-in Imports
from __future__ import annotations

# Open 3D Engine Imports
import azlmbr.bus as bus
import azlmbr.asset as azasset
import azlmbr.math as math


class Asset:
    """
    Used to find Asset Id by its path and path of asset by its Id
    If a component has any asset property, then this class object can be called as:
        asset_id = editor_python_test_tools.editor_entity_utils.EditorComponent.get_component_property_value(<arguments>)
        asset = asset_utils.Asset(asset_id)
    """
    def __init__(self, id: azasset.AssetId):
        self.id: azasset.AssetId = id

    # Creation functions
    @classmethod
    def find_asset_by_path(cls, path: str, RegisterType: bool = False) -> Asset:
        """
        :param path: Absolute file path of the asset
        :param RegisterType: Whether to register the asset if it's not in the database,
                             default to false for the general case
        :return: Asset object associated with file path
        """
        asset_id = azasset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", path, math.Uuid(), RegisterType)
        assert asset_id.is_valid(), f"Couldn't find Asset with path: {path}"
        asset = cls(asset_id)
        return asset

    # Methods
    def get_path(self) -> str:
        """
        :return: Absolute file path of Asset
        """
        assert self.id.is_valid(), "Invalid Asset Id"
        return azasset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetPathById", self.id)
