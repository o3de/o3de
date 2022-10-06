"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import azlmbr.bus
from functools import partial
from editor_python_test_tools.utils import TestHelper
from Atom.atom_utils.atom_constants import AtomComponentProperties


class OnModelReady:
    def __init__(self):
        self.isModelReady = False

    def model_is_ready_predicate(self):
        """
        A predicate function what will be used in wait_for_condition.
        """
        return self.isModelReady

    def on_model_ready(self, parameters):
        self.isModelReady = True

    def wait_for_on_model_ready(self, entityId, mesh_component, model_id):
        self.isModelReady = False
        # Connect to the MeshNotificationBus
        # Listen for notifications when entities are created/deleted
        self.onModelReadyHandler = azlmbr.bus.NotificationHandler('MeshComponentNotificationBus')
        self.onModelReadyHandler.connect(entityId)
        self.onModelReadyHandler.add_callback('OnModelReady', self.on_model_ready)

        waitCondition = partial(self.model_is_ready_predicate)

        mesh_component.set_component_property_value(AtomComponentProperties.mesh('Model Asset'), model_id)
        if TestHelper.wait_for_condition(waitCondition, 20.0):
            return True
        else:
            return False
