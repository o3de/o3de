"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# TODO: Delete ../bundle_mode_in_editor_tests.py because this file replaces that file.
def Bundle_Mode_Tests_In_Editor():
    import pyside_utils
    from editor_python_test_tools.utils import TestHelper
    from functools import partial

    class MissingAssetHandlerHelper:
        def __init__(self):
            self.isFileMissing = False
    
        def on_file_missing(self, parameters):
            import re
            print(f"File is missing {parameters}")

            # These aren't being pulled in as default assets, nor are they being pulled in as dependencies.
            # Skip them for now, issues with default assets in release builds will be resolved at another time.
            skip_missing_list = [
                ".*StandardPBR_.*\\.bin",
                ".*Shadowmap_.*\\.bin",
                ".*DepthPass_.*\\.bin",
                ".*MeshMotionVector_.*\\.bin"
            ]

            for skip_missing in skip_missing_list:
                match_result = re.match(skip_missing, parameters[0])
                if match_result != None:
                    # This matches an asset to ignore for file missing messages
                    print("Ignoring this missing asset")
                    return

            # TODO: This should have a list of expected to be missing assets, to verify the correct assets are showing up as missing from bundles.
            # isFileMissing shouldn't be marked true until all files are found.
            self.isFileMissing = True
        
        def on_file_missing_predicate(self):
            return self.isFileMissing

        def wait_for_file_missing(self, level_folder, level_name):
            self.missingAssetHandler = azlmbr.bus.NotificationHandler('MissingAssetNotificationBus')
            self.missingAssetHandler.connect()
            self.missingAssetHandler.add_callback('FileMissing', self.on_file_missing)

            waitCondition = partial(self.on_file_missing_predicate)

            # Trigger an asset load that should be missing
            TestHelper.open_level(level_folder, level_name)

            # TODO: Can this be updated to instead just respond to a callback that the level finished loading?
            # This will be necessary for the success case, where no files should show up as missing.
            # TODO: Or should we update the messaging to include a message for assets correctly loaded from bundles,
            # so this test can listen, and mark itself as done when all assets its watching have shown up as missing from bundles or loaded from bundles?
            if TestHelper.wait_for_condition(waitCondition, 20.0):
                return True
            else:
                return False
            
    def run_test():
        from editor_python_test_tools.utils import Report

        TestHelper.init_idle()
        
        bundle_path = sys.argv[1]

        azlmbr.legacy.general.set_cvar_integer("sys_report_files_not_found_in_paks", 1)
        azlmbr.legacy.general.run_console(f"loadbundles {bundle_path}")

        missingAssetHandlerHelper = MissingAssetHandlerHelper()
        #TODO : Pick a different first level to test with, ideally a level built specifically for automated tests.
        missingAssetHandlerHelper.wait_for_file_missing("Graphics", "hermanubis")

        Report.critical_result(("First test level correctly reported expected assets as missing.", 
            "First test level incorrectly reported zero assets missing from bundles."), missingAssetHandlerHelper.isFileMissing == True)
        
        missingAssetHandlerHelper = MissingAssetHandlerHelper()
        missingAssetHandlerHelper.wait_for_file_missing("", "TestDependenciesLevel")
        Report.critical_result(("Good - no missing asset", "Bad - missing asset"), missingAssetHandlerHelper.isFileMissing == False)

    run_test()
    

if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Bundle_Mode_Tests_In_Editor)
