"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import azlmbr.legacy.general as general
import azlmbr.math as math


sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from largeworlds.large_worlds_utils import editor_dynveg_test_helper as dynveg


class TestSurfaceDataRefreshes_RemainsStable(EditorTestHelper):
    def __init__(self):
        EditorTestHelper.__init__(self, log_prefix="SurfaceDataRefreshes_RemainsStable", args=["level"])

    def run_test(self):
        """
        Summary:
        The Vegetation Area System can intermittently crash when updating surface data and moving the camera
        around rapidly.  The situation occurs across multiple frames - the surface data updates, which triggers a bunch 
        of sector updates getting added to the update queue.  Then in a subsequent frame, there is no active vegetation 
        area or surface data updates, which triggers "delete all sectors".  The "delete all" wasn't deleting entries from 
        the update queue, so any unprocessed updates would continue to get processed.  If any of those updates referenced 
        a sector that no longer exists, because the camera changed position, then it would assert and crash.

        To repro this bug, this test creates an empty level with a large box shape emitting a surface, and then runs a tight
        loop of camera movements and "surface changed" events that invalidate all surface points.  Because this is a timing
        issue, there's no guarantee that the test below will successfully cause the condition to occur, but it successfully 
        crashed every time it was tested locally prior to the bugfix.

        :return: None
        """
        # 1) Create a test level with the needed test setup
        self.test_success = self.create_level(
                self.get_arg('level'), 
                heightmap_resolution=128, 
                heightmap_meters_per_pixel=1, 
                terrain_texture_resolution=128, 
                use_terrain=False)

        world_center = math.Vector3(512.0, 512.0, 32.0)

        # Add an entity with a 1024 x 1024 box centered at 512,512.
        surface_entity = dynveg.create_surface_entity("Surface Data", world_center, 1024.0, 1024.0, 1.0)

        # Move the camera to the world center
        general.set_current_view_position(world_center.x, world_center.y, world_center.z)

        # 2) Perform the test.  Since the conditions are extremely timing related, and every machine
        # running the test can have different timing conditions, we run through a set of different
        # combinations to try and cause the crash under as many scenarios as possible

        loops_per_surface_changed = [3, 5, 5]
        loops_per_camera_reset = [20, 20, 20]
        camera_speed_per_loop = [10.0, 10.0, 15.0]

        # Setting test success to false to make sure the toggle at the end accurately conveys the loop being successful
        self.test_success = False

        # Loop through all our attempted timing test cases to cause the crash pretty consistently.
        for test_case in range(0,3):
            self.log(f'Starting test case {test_case}')
            self.log(f'Loops per surface changed: {loops_per_surface_changed[test_case]}')
            self.log(f'Loops per camera reset: {loops_per_camera_reset[test_case]}')
            self.log(f'Camera speed per loop: {camera_speed_per_loop[test_case]}')
            for test_counter in range (0,100):

                # Every N loops, invalidate the entire set of surface data.  It's mostly just important for this
                # not to happen *every* iteration, since we need the vegetation system to bounce between having
                # dirty surface points that cause sectors to be refreshed, and having no dirty surface points or
                # active surface areas to trigger a "delete all sectors" condition.
                if (test_counter % loops_per_surface_changed[test_case]) == 0:
                    azlmbr.surface_data.SurfaceDataSystemNotificationBus(azlmbr.bus.Broadcast,
                                                                         'OnSurfaceChanged',
                                                                         surface_entity.id,
                                                                         azlmbr.math.Aabb(),
                                                                         azlmbr.math.Aabb())

                # Move the camera back and forth along the X axis at just the right speed to invalidate sectors that are
                # queued for updating but haven't updated yet, so that when they try to update they crash.
                x_pos = world_center.x + ((test_counter % loops_per_camera_reset[test_case]) * camera_speed_per_loop[test_case])
                general.set_current_view_position(x_pos, world_center.y, world_center.z)

                self.log(f'{test_counter}: {x_pos}')

                # Give a little processing time each iteration.
                general.idle_wait(0.01)

        # If we haven't crashed, then we've succeeded.
        self.test_success = True


test = TestSurfaceDataRefreshes_RemainsStable()
test.run()
