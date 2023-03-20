"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# fmt: off
class Tests():
    azshader_was_removed =          ("azshader was removed",        "Failed to remove azshader")
    azshader_was_compiled =         ("azshader was compiled",       "Failed to compile azshader")
    no_error_occurred =             ("No errors detected",          "Errors were detected")
# fmt: on


def ShaderAssetBuilder_RecompilesShaderAsChainOfDependenciesChanges():
    """
    This test validates: "Shader Builders May Fail When Multiple New Files Are Added"
    It creates source assets to compile a particular shader.
    1- The first phase generates the source assets out of order and slowly. The AP should
       wakeup each time one of the source dependencies appears but will fail each time. Only when the
       last dependency appears then the shader should build successfully. 
    2- The second phase is similar as above, except that all source assets will be created
       at once and We also expect that in the end the shader is built successfully.
    """
    import os
    import azlmbr.legacy.general as general

    from Atom.atom_utils.atom_tools_utils import ShaderAssetTestHelper
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    # Required for automated tests
    helper.init_idle()

    game_root_path = os.path.normpath(general.get_game_folder())
    game_asset_path = os.path.join(game_root_path, "Assets")

    base_dir = os.path.dirname(__file__)
    src_assets_subdir = os.path.join(base_dir, "TestAssets", "ShaderAssetBuilder")

    with Tracer() as error_tracer:
        # The script drives the execution of the test, to return the flow back to the editor,
        # we will tick it one time
        general.idle_wait_frames(1)
 
        # This is the order in which the source assets should be deployed
        # to avoid source dependency issues with the old MCPP-based CreateJobs. 
        file_list = [
            "Test2Color.azsli",
            "Test3Color.azsli",
            "Test1Color.azsli",
            "DependencyValidation.azsl",
            "DependencyValidation.shader"
        ]

        reverse_file_list = file_list[::-1]

        # Remove files in reverse order
        ShaderAssetTestHelper.remove_files(game_asset_path, reverse_file_list)

        # Wait here until the azshader doesn't exist anymore.
        azshader_name = "assets/dependencyvalidation.azshader"
        helper.wait_for_condition(lambda: not ShaderAssetTestHelper.asset_exists(azshader_name), 5.0)

        Report.critical_result(Tests.azshader_was_removed, not ShaderAssetTestHelper.asset_exists(azshader_name))

        ShaderAssetTestHelper.copy_tmp_files_in_order(src_assets_subdir, file_list, game_asset_path, 1.0)

        # Give enough time to AP to compile the shader
        helper.wait_for_condition(lambda: ShaderAssetTestHelper.asset_exists(azshader_name), 60.0)

        Report.critical_result(Tests.azshader_was_compiled, ShaderAssetTestHelper.asset_exists(azshader_name))

        # The first part was about compiling the shader under normal conditions.
        # Let's remove the files from the previous phase and will proceed
        # to make the source files visible to the AP in reverse order. The
        # ShaderAssetBuilder will only succeed when the last file becomes visible.
        ShaderAssetTestHelper.remove_files(game_asset_path, reverse_file_list)
        helper.wait_for_condition(lambda: not ShaderAssetTestHelper.asset_exists(azshader_name), 5.0)
        Report.critical_result(Tests.azshader_was_removed, not ShaderAssetTestHelper.asset_exists(azshader_name))

        # Remark, if you are running this test manually from the Editor with "pyRunFile",
        # You'll notice how the AP issues notifications that it fails to compile the shader
        # as the source files are being copied to the "Assets" subfolder.
        # Those errors are OK and also expected because We need the AP to wake up as each
        # reported source dependency exists. Once the last file is copied then all source
        # dependencies are fully satisfied and the shader should compile successfully.
        # And this summarizes the importance of this Test: The previous version
        # of ShaderAssetBuilder::CreateJobs was incapable of compiling the shader under the conditions
        # presented in this test, but with the new version of ShaderAssetBuilder::CreateJobs, which
        # doesn't use MCPP for #include files discovery, it should eventually compile the shader
        # once all the source files are in place.
        ShaderAssetTestHelper.copy_tmp_files_in_order(src_assets_subdir, reverse_file_list, game_asset_path, 3.0)

        # Give enough time to AP to compile the shader
        helper.wait_for_condition(lambda: ShaderAssetTestHelper.asset_exists(azshader_name), 60.0)

        Report.critical_result(Tests.azshader_was_compiled, ShaderAssetTestHelper.asset_exists(azshader_name))

        # The last phase of the test puts stress on potential race conditions
        # when all required files appear as soon as possible.

        # First Clean up.
        # Remove left over files.
        ShaderAssetTestHelper.remove_files(game_asset_path, reverse_file_list)
        helper.wait_for_condition(lambda: not ShaderAssetTestHelper.asset_exists(azshader_name), 5.0)
        Report.critical_result(Tests.azshader_was_removed, not ShaderAssetTestHelper.asset_exists(azshader_name))

        # Now let's copy all the source files to the "Assets" folder as fast as possible.
        ShaderAssetTestHelper.copy_tmp_files_in_order(src_assets_subdir, reverse_file_list, game_asset_path)

        # Give enough time to AP to compile the shader
        helper.wait_for_condition(lambda: ShaderAssetTestHelper.asset_exists(azshader_name), 60.0)

        Report.critical_result(Tests.azshader_was_compiled, ShaderAssetTestHelper.asset_exists(azshader_name))

        # All good, let's cleanup leftover files before closing the test.
        ShaderAssetTestHelper.remove_files(game_asset_path, reverse_file_list)
        helper.wait_for_condition(lambda: not ShaderAssetTestHelper.asset_exists(azshader_name), 5.0)

        # Look for errors to raise.
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(ShaderAssetBuilder_RecompilesShaderAsChainOfDependenciesChanges)
