"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.
SPDX-License-Identifier: Apache-2.0 OR MIT
Holds constants used across both hydra and non-hydra scripts.
"""
from sys import float_info

"""
Physx Event Bus
"""
GET_WORLD_TRANSLATION = "GetWorldTranslation"
GET_LINEAR_VELOCITY = "GetLinearVelocity"
ON_CALCULATE_NET_FORCE = "OnCalculateNetForce"

"""
General constants
"""
WAIT_TIME_1 = 1
WAIT_TIME_2 = 2
WAIT_TIME_3 = 3
WAIT_TIME_5 = 5

FLOAT_THRESHOLD_EPSILON = float_info.epsilon