"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import azlmbr.legacy.general as general


class PrefabWaiter:
    """
    Wait utilities for CRUD operations on Prefabs in Editor automated tests
    """
    @staticmethod
    def wait_for_propagation():
        """
        Script thread releases execution to main thread for 1 frame to allow for prefab propagation to occur
        """
        general.idle_wait_frames(1)
