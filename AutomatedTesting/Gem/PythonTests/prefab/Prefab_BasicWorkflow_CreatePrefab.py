"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

def Test():

    CAR_PREFAB_FILE_NAME = 'CAR_prefab'

    def run_test():

        import prefab.Prefab_Operations as prefab
        import prefab.Prefab_Test_Utils as prefab_test_utils

        prefab_test_utils.open_base_tests_level()

        # Create a new Entity at the root level
        car_entity_id = prefab.create_entity()
        car_prefab_entity_ids = [car_entity_id]

        # Checks for prefab creation passed or not
        prefab.create_prefab(car_prefab_entity_ids, CAR_PREFAB_FILE_NAME)


    run_test()


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(Test)
