"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import shutil, os, time
import pytest
import ly_test_tools.o3de.editor_test_utils as editor_test_utils
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest

@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class MaterialProperty_ProcPrefabWorks(EditorSingleTest):
        from .tests import MaterialProperty_ProcPrefabWorks as test_module

    class UserDefinedProperties_Works(EditorSingleTest):
        from .tests import UserDefinedProperties_Works as test_module

        @classmethod
        def setup(cls, instance, request, workspace, editor_test_results, launcher_platform):
            # close out any previous O3DE active tool instances
            editor_test_utils.kill_all_ly_processes(include_asset_processor=True)

            # make a copy so that the test can generate a .dbgsg file for default_mat.fbx
            project_path = os.path.join(workspace.paths.engine_root(), workspace.project)
            default_mat_src = os.path.join(project_path, 'Assets/ap_test_assets/default_mat.fbx')
            instance.default_mat_file = os.path.join('Assets/ap_test_assets', f'default_mat_{time.monotonic_ns()}.fbx')
            instance.default_mat_dst = os.path.join(project_path, instance.default_mat_file)
            shutil.copy(default_mat_src, instance.default_mat_dst)

            # run the AP Batch with debug output to get .dbgsg file for the new file
            workspace.asset_processor.batch_process(extra_params="--debugOutput")

        @classmethod
        def teardown(cls, instance, request, workspace, editor_test_results, launcher_platform):
            if os.path.exists(instance.default_mat_dst):
                os.remove(instance.default_mat_dst)

            # compute the cache path
            cache_folder = workspace.paths.cache()
            platform = workspace.asset_processor_platform
            if platform == 'windows':
                platform = 'pc'
            cache_folder = os.path.join(cache_folder, platform)

            # compute the file name to the .dbgsg file
            root, _ = os.path.splitext(instance.default_mat_file)
            dbgsg_path = os.path.join(cache_folder, root + ".dbgsg").lower()
            if os.path.isfile(dbgsg_path) == False:
                raise Exception(f"Missing file {dbgsg_path}")

            # find the user defined property o3de_default_material in the .dbgsg file
            material_UDP = 'o3de_default_material: gem/sponza/assets/objects/sponza_mat_bricks.azmaterial'
            found_UDP = False
            with open(dbgsg_path) as f:
                content = f.readlines()
                for line in content:
                    if line.rstrip().endswith(material_UDP):
                        found_UDP = True
                        break

            if found_UDP == False:
                raise Exception(f"Missing {material_UDP} in {dbgsg_path}")
