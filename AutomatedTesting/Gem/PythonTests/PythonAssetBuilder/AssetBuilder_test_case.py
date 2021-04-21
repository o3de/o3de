"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
import azlmbr.bus
import azlmbr.asset
import azlmbr.editor
import azlmbr.math
import azlmbr.legacy.general

def raise_and_stop(msg):
    print (msg)
    azlmbr.editor.EditorToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'ExitNoPrompt')

# These tests are meant to check that the test_asset.mock source asset turned into
# a test_asset.mock_asset product asset via the Python asset builder system
mockAssetType = azlmbr.math.Uuid_CreateString('{9274AD17-3212-4651-9F3B-7DCCB080E467}', 0)
mockAssetPath = 'gem/pythontests/pythonassetbuilder/test_asset.mock_asset'
assetId = azlmbr.asset.AssetCatalogRequestBus(azlmbr.bus.Broadcast, 'GetAssetIdByPath', mockAssetPath, mockAssetType, False)
if (assetId.is_valid() is False):
    raise_and_stop(f'Mock AssetId is not valid! Got {assetId.to_string()} instead')

assetIdString = assetId.to_string()
if (assetIdString.endswith(':528cca58') is False):
    raise_and_stop(f'Mock AssetId {assetIdString} has unexpected sub-id for {mockAssetPath}!')

print ('Mock asset exists')

# These tests detect if the geom_group.fbx file turns into a number of azmodel product assets
def test_azmodel_product(generatedModelAssetPath, expectedSubId):
    azModelAssetType = azlmbr.math.Uuid_CreateString('{2C7477B6-69C5-45BE-8163-BCD6A275B6D8}', 0)
    assetId = azlmbr.asset.AssetCatalogRequestBus(azlmbr.bus.Broadcast, 'GetAssetIdByPath', generatedModelAssetPath, azModelAssetType, False)
    assetIdString = assetId.to_string()
    if (assetIdString.endswith(':' + expectedSubId) is False):
        raise_and_stop(f'Asset has unexpected asset ID ({assetIdString}) for ({generatedModelAssetPath})!')
    else:
        print(f'Expected subId for asset ({generatedModelAssetPath}) found')

test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_z_positive_1.azmodel', '10315ae0')
test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_z_negative_1.azmodel', '10661093')
test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_y_positive_1.azmodel', '10af8810')
test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_y_negative_1.azmodel', '10f8c263')
test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_x_positive_1.azmodel', '100ac47f')
test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_x_negative_1.azmodel', '105d8e0c')
test_azmodel_product('gem/pythontests/pythonassetbuilder/geom_group_fbx_cube_100cm_center_1.azmodel', '1002d464')

azlmbr.editor.EditorToolsApplicationRequestBus(azlmbr.bus.Broadcast, 'ExitNoPrompt')
