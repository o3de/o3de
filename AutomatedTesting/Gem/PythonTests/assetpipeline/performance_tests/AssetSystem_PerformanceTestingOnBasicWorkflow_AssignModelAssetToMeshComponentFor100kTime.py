"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import time


class Tests:
    mesh_component_added = (
        "Mesh component added to entity",
        "Failed to add Mesh component to entity"
    )
    set_mesh_asset = (
        "Assign mesh asset to the mesh component",
        "Failed to assign mesh asset to the mesh component"
    )


class Timer:
    def __init__(self):
        self._start_time = None

    def start(self):
        self._start_time = time.perf_counter()

    def log_time(self, message):
        from editor_python_test_tools.utils import Report

        elapsed_time = time.perf_counter() - self._start_time
        Report.info(f'{message}: {elapsed_time} seconds\n')


def AssetSystem_PerformanceTestingOnBasicWorkflow_AssignModelAssetToMeshComponentFor100kTime():
    """
    Summary:
    Performance testing on a basic asset system workflow for assigning an asset to the mesh component

    Expected Behavior:
    Operation on asset IDs can be completed within a reasonable time period and
    won't change significantly after updating the asset ID format.

    Test Steps:
    1) Open level
    2) Create entity
    5) Add/verify Mesh component
    6) Assign model assets to the Mesh component for 10K times
    """

    import os

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.asset as asset
    import azlmbr.globals as _globals
    import azlmbr.math as math

    import editor_python_test_tools.hydra_editor_utils as hydra
    from editor_python_test_tools.editor_entity_utils import EditorEntity

    # 1) Open an existing simple level
    hydra.open_base_level()

    # 2) create a test entity
    test_entity = EditorEntity.create_editor_entity_at((0.0, 0.0, 0.0), name="Test_Entity")
    assert test_entity.exists(), 'Failed to create the test entity'

    # 3) add a mesh component to the test entity
    add_component_outcome = editor.EditorComponentAPIBus(
        bus.Broadcast, "AddComponentsOfType", test_entity.id, [_globals.property.EditorMeshComponentTypeId]
    )
    assert add_component_outcome.IsSuccess(), 'Failed to assign mesh asset to the mesh component'

    # 4) Retrieve a list of model asset IDs
    model_assets = list()
    model_asset_path = os.path.join("objects", "_primitives", "_sphere_1x1.azmodel")
    model_assets.append(asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", model_asset_path, math.Uuid(), False))
    model_asset_path = os.path.join("objects", "_primitives", "_cylinder_1x1.azmodel")
    model_assets.append(asset.AssetCatalogRequestBus(
        bus.Broadcast, "GetAssetIdByPath", model_asset_path, math.Uuid(), False))

    # 5) Assign a model asset to the mesh component for 10K times
    mesh_component_id = add_component_outcome.GetValue()[0]
    model_asset_path = 'Controller|Configuration|Model Asset'
    timer = Timer()
    timer.start()
    for iteration in range(100000):
        asset_index = iteration % len(model_assets)
        set_property_outcome = editor.EditorComponentAPIBus(
            bus.Broadcast, 'SetComponentProperty', mesh_component_id,
            model_asset_path, model_assets[asset_index])
        get_property_outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'GetComponentProperty', mesh_component_id, model_asset_path)
        assert set_property_outcome.IsSuccess() and \
               get_property_outcome.IsSuccess() and \
               model_assets[asset_index] == get_property_outcome.GetValue(), \
               'Failed to assign mesh asset to the mesh component'

    timer.log_time('AssetSystem_PerformanceTestingOnBasicWorkflow_AssignModelAssetToMeshComponentFor100kTime')


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(AssetSystem_PerformanceTestingOnBasicWorkflow_AssignModelAssetToMeshComponentFor100kTime)
