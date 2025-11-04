"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

class Tests():
    rga_file_exist        = ("rga file exist",        "RGA executable doesn't exist! To downlad RGA in O3DE, set O3DE_RADEON_GPU_ANALYZER_ENABLED=true in CmakeCache.txt in build folder, and rerun cmake.")
    azshader_was_removed  = ("azshader was removed",  "Failed to remove azshader")
    azshader_was_compiled = ("azshader was compiled", "Failed to compile azshader")
    livereg_was_created   = ("livereg was created",   "Failed to created livereg")
    disassem_was_created  = ("disassem was created",  "Failed to created disassem")
    no_error_occurred     = ("No errors detected",    "Errors were detected")

def RGAtest():

    import sys
    import os

    from Atom.atom_utils.atom_tools_utils import ShaderAssetTestHelper
    import azlmbr.legacy.general as general
    from editor_python_test_tools.utils import TestHelper as helper
    from editor_python_test_tools.utils import Tracer

    def _file_exists(file_path):
        return os.path.exists(file_path)
    
    # Required for automated tests
    helper.init_idle()

    game_root_path = os.path.normpath(general.get_game_folder())
    game_build_path = os.path.normpath(general.get_build_folder())
    game_asset_path = os.path.join(game_root_path, "Assets")
    game_cache_path = os.path.join(game_root_path, "Cache")
    base_dir = os.path.dirname(__file__)
    src_assets_subdir = os.path.join(base_dir, "TestAssets", "ShaderAssetBuilder")

    with Tracer() as error_tracer:
        # The script drives the execution of the test, to return the flow back to the editor,
        # we will tick it one time
        general.idle_wait_frames(1)
 
        # This is the order in which the source assets should be deployed
        # to avoid source dependency issues with the old MCPP-based CreateJobs. 
        file_list = [
            "RgaShader.shadervariantlist",
            "RgaShader.shader",
            "RgaShader.azsl"
        ]
        
        # Check if rga executable exist
        if sys.platform == 'win32':
            rga_path = os.path.join(game_build_path, "_deps", "rga-src", "rga.exe")
        else:
            rga_path = os.path.join(game_build_path, "_deps", "rga-src", "rga")
        Report.critical_result(Tests.rga_file_exist, _file_exists(rga_path))

        # Remove files 
        ShaderAssetTestHelper.remove_files(game_asset_path, file_list)

        # Wait here until the azshader doesn't exist anymore.
        azshader_name = "assets/rgashader.azshader"
        helper.wait_for_condition(lambda: not ShaderAssetTestHelper.asset_exists(azshader_name), 5.0)
        Report.critical_result(Tests.azshader_was_removed, not ShaderAssetTestHelper.asset_exists(azshader_name))

        ShaderAssetTestHelper.copy_tmp_files_in_order(src_assets_subdir, file_list, game_asset_path)

        # Give enough time to AP to compile the shader
        helper.wait_for_condition(lambda: ShaderAssetTestHelper.asset_exists(azshader_name), 60.0)
        Report.critical_result(Tests.azshader_was_compiled, ShaderAssetTestHelper.asset_exists(azshader_name))

        # Check analysis data
        livereg_path = os.path.normpath(os.path.join(game_cache_path, "pc/assets/gfx1035_livereg_1_frag.txt"))
        disassem_path = os.path.normpath(os.path.join(game_cache_path, "pc/assets/gfx1035_disassem_1_frag.txt"))
        helper.wait_for_condition(lambda: _file_exists(livereg_path), 5.0)
        helper.wait_for_condition(lambda: _file_exists(disassem_path), 5.0)
        Report.critical_result(Tests.livereg_was_created, _file_exists(livereg_path))
        Report.critical_result(Tests.disassem_was_created, _file_exists(disassem_path))

        # All good, let's cleanup leftover files before closing the test.
        ShaderAssetTestHelper.remove_files(game_asset_path, file_list)
        helper.wait_for_condition(lambda: not ShaderAssetTestHelper.asset_exists(azshader_name), 5.0)

        # Look for errors to raise.
        helper.wait_for_condition(lambda: error_tracer.has_errors, 1.0)
        Report.result(Tests.no_error_occurred, not error_tracer.has_errors)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(RGAtest)