"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import azlmbr.bus
import azlmbr.asset
from functools import partial
from editor_python_test_tools.utils import TestHelper


class OnModelReloaded:
    def __init__(self):
        self.isModelReloaded = False

    def model_is_reloaded_predicate(self):
        """
        A predicate function what will be used in wait_for_condition.
        """
        return self.isModelReloaded

    def on_model_reloaded(self, parameter):
        self.isModelReloaded = True

    def wait_for_on_model_reloaded(self, asset_id):
        self.isModelReloaded = False
        # Listen for notifications when assets are reloaded
        self.onModelReloadedHandler = azlmbr.asset.AssetBusHandler()
        self.onModelReloadedHandler.connect(asset_id)
        self.onModelReloadedHandler.add_callback('OnAssetReloaded', self.on_model_reloaded)

        waitCondition = partial(self.model_is_reloaded_predicate)

        if TestHelper.wait_for_condition(waitCondition, 20.0):
            return True
        else:
            return False
