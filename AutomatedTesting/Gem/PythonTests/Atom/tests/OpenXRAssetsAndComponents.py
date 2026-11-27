"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


def openxr_assets_and_components():
    from editor_python_test_tools.asset_utils import Asset
    from editor_python_test_tools.editor_entity_utils import EditorEntity
    from editor_python_test_tools.utils import Report, TestHelper

    TestHelper.init_idle()
    for path in (
        "openxrvk/system.xrprofiles",
        "openxrvk/default.xractions",
        "passes/openxrpasstemplates.azasset",
    ):
        Asset.find_asset_by_path(path)

    TestHelper.open_level("Graphics", "base_empty")
    entity = EditorEntity.create_editor_entity("XR camera")
    entity.add_component("XR Camera Movement")
    Report.result(
        ("XR Camera Movement component is available", "XR Camera Movement component is missing"),
        entity.has_component("XR Camera Movement"),
    )
    entity.delete()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report

    Report.start_test(openxr_assets_and_components)
