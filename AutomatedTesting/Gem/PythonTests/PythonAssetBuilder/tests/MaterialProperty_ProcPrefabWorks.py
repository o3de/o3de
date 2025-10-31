"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def ReportError(msg):
    from editor_python_test_tools.utils import Report
    Report.result(msg, False)
    raise Exception(msg)

def MaterialProperty_ProcPrefabWorks():
    # Description: the AutomatedTesting\Assets\ap_test_assets\default_mat.fbx
    #  source asset should produce a procedural prefab named
    #  'assets/ap_test_assets/default_mat_fbx.procprefab' which is a JSON
    #  file. In that JSON file, there should be a field named 'assetHint' and
    #  have the value 'gem/sponza/assets/objects/sponza_mat_bricks.azmaterial'
    #  This test validates these data points.

    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper
    import azlmbr.legacy.general
    import azlmbr.prefab
    import json

    # Required for automated tests
    TestHelper.init_idle()

    # Open the test level
    TestHelper.open_level(directory="", level="Base")
    azlmbr.legacy.general.idle_wait_frames(1)

    templateId = azlmbr.prefab.LoadTemplate('assets/ap_test_assets/default_mat_fbx.procprefab')
    jsonTemplate = azlmbr.prefab.SaveTemplateToString(templateId)
    if (len(jsonTemplate) == 0):
        ReportError('Could not load JSON template for default_mat_fbx.procprefab')

    foundItems = []

    def find_azmaterial(d, foundItems):
        a_list = d.items() if isinstance(d, dict) else enumerate(d)
        for k, v in a_list:
            if isinstance(v, dict) or isinstance(v, list):
                find_azmaterial(v, foundItems)
            elif (k == 'assetHint' and v == 'gem/sponza/assets/objects/sponza_mat_bricks.azmaterial'):
                foundItems.append(True)

    find_azmaterial(json.loads(jsonTemplate), foundItems)
    if (len(foundItems) == 0):
        ReportError('Could not find material asset in prefab template')

    # all tests worked
    Report.result("Found the expected fields in the default_mat_fbx.procprefab", True)

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MaterialProperty_ProcPrefabWorks)
