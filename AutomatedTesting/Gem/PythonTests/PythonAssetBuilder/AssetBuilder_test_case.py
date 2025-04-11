"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import azlmbr.bus
import azlmbr.asset
import azlmbr.editor
import azlmbr.math

print('Starting mock asset tests')
handler = azlmbr.editor.EditorEventBusHandler()

def on_notify_editor_initialized(args):
    # These tests are meant to check that the test_asset.mock source asset turned into
    # a test_asset.mock_asset product asset via the Python asset builder system
    mockAssetType = azlmbr.math.Uuid_CreateString('{9274AD17-3212-4651-9F3B-7DCCB080E467}')
    mockAssetPath = 'gem/pythontests/pythonassetbuilder/test_asset.mock_asset'
    assetId = azlmbr.asset.AssetCatalogRequestBus(azlmbr.bus.Broadcast, 'GetAssetIdByPath', mockAssetPath, mockAssetType, False)
    if (assetId.is_valid() is False):
        print(f'Mock AssetId is not valid! Got {assetId.to_string()} instead')
    else:
        print(f'Mock AssetId is valid!')

    assetIdString = assetId.to_string()
    if (assetIdString.endswith(':528cca58') is False):
        print(f'Mock AssetId {assetIdString} has unexpected sub-id for {mockAssetPath}!')
    else:
        print(f'Mock AssetId has expected sub-id for {mockAssetPath}!')

    print ('Mock asset exists')

    # These tests detect if the geom_group.fbx file turns into a number of azmodel product assets
    def test_azmodel_product(generatedModelAssetPath):
        azModelAssetType = azlmbr.math.Uuid_CreateString('{2C7477B6-69C5-45BE-8163-BCD6A275B6D8}')
        assetId = azlmbr.asset.AssetCatalogRequestBus(azlmbr.bus.Broadcast, 'GetAssetIdByPath', generatedModelAssetPath, azModelAssetType, False)
        assetIdString = assetId.to_string()
        if (assetId.is_valid()):
            print(f'AssetId found for asset ({generatedModelAssetPath}) found')
        else:
            print(f'Asset at path {generatedModelAssetPath} has unexpected asset ID ({assetIdString})!')

    test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_z_positive.fbx.azmodel')
    test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_z_negative.fbx.azmodel')
    test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_y_positive.fbx.azmodel')
    test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_y_negative.fbx.azmodel')
    test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_x_positive.fbx.azmodel')
    test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_x_negative.fbx.azmodel')
    test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_center.fbx.azmodel')

    # clear up notification handler
    global handler
    handler.disconnect()
    handler = None

    print('Finished mock asset tests')
    azlmbr.editor.EditorToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'ExitNoPrompt')

handler.connect()
handler.add_callback('NotifyEditorInitialized', on_notify_editor_initialized)
