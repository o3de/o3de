"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests:
    mesh_entity_creation = (
        "Mesh Entity successfully created",
        "P0: Mesh Entity failed to be created")
    mesh_component_added = (
        "Entity has a Mesh component",
        "P0: Entity failed to find Mesh component")
    model_asset_specified = (
        "Model Asset set",
        "P0: Model Asset not set")
    model_asset_is_optimized = (
        "valenaactor.azmodel has <= 231,904 vertices",
        "P0: valenaactor.azmodel has not been fully optimized")

def on_model_ready():
    print("on model ready")

def Atom_MeshOptimizerTest():
    """
    Summary:
    Tests the Mesh component can be added to an entity and has the expected functionality.

    Test setup:
    - Wait for Editor idle loop.
    - Open the "Base" level.

    Expected Behavior:
    The vertex count is less than or equal to 231,904.

    Test Steps:
    1) Create a Mesh entity with no components.
    2) Add a Mesh component to Mesh entity.
    3) Specify the Mesh component asset
    4) Verify that the model has the expected vertex count

    :return: None
    """

    import os

    from PySide2 import QtWidgets

    import azlmbr.editor
    from functools import partial
    import azlmbr.legacy.general as general
    from Atom.atom_utils.atom_constants import (MESH_LOD_TYPE,
                                                AtomComponentProperties)
    from editor_python_test_tools import pyside_utils
    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, TestHelper, Tracer
    import editor_python_test_tools.hydra_editor_utils as hydra

    class OnModelReadyHelper:
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

    with Tracer() as error_tracer:
        # Test setup begins.
        # Setup: Wait for Editor idle loop before executing Python hydra scripts then open "Base" level.
        TestHelper.init_idle()
        TestHelper.open_level("Graphics", "base_empty")

        # Test steps begin.
        # 1. Create a Mesh entity with no components.
        mesh_entity = EditorEntity.create_editor_entity(AtomComponentProperties.mesh())
        Report.critical_result(Tests.mesh_entity_creation, mesh_entity.exists())

        # 2. Add a Mesh component to Mesh entity.
        mesh_component = mesh_entity.add_component(AtomComponentProperties.mesh())
        Report.critical_result(
            Tests.mesh_component_added,
            mesh_entity.has_component(AtomComponentProperties.mesh()))

        # 5. Set Mesh component asset property
        model_path = os.path.join('valena', 'valenaactor.azmodel')
        model = Asset.find_asset_by_path(model_path)
        onModelReadyHelper = OnModelReadyHelper()
        onModelReadyHelper.wait_for_on_model_ready(mesh_entity.id, mesh_component, model.id)
        Report.result(Tests.model_asset_specified,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Model Asset')) == model.id)
        
        vertex_count = mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Vertex Count LOD0'))

        Report.result(Tests.model_asset_is_optimized,
                      mesh_component.get_component_property_value(
                          AtomComponentProperties.mesh('Vertex Count LOD0')) <= 231904)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Atom_MeshOptimizerTest)
