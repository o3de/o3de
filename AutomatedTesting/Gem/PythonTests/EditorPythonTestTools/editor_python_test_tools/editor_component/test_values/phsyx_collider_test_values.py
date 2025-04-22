"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

These test values are intended to be utilized alongside the PhysX Collider
"""

CYLINDER_SUBDIVISION_TESTS = {
    # "Negative Value Test": (-1, False), # o3de/o3de#12608 - Crash if subdivision set below 3
    # "Zero Value Test": (0, False), # o3de/o3de#12608 - Crash if subdivision set below 3
    # "Out Of Lower Bounds Test": (2, False), # o3de/o3de#12608 - Crash if subdivision set below 3
    "Minimum Value Test": (3, True),
    "Middle Value Test": (60, True),
    "Max Value Test": (125, True),
    # "Out of Maximum Bounds Test": (256, False) # o3de/o3de#12608 - Crash if subdivision set above 125
}

CONTACT_OFFSET_TESTS = {
    "Negative Value Test": (-1.0, False),
    "Zero Value Test": (0.0, False),
    "Small Value Test": (0.2, True),
    "Max Boundary Value Test": (50.0, True),
    #"Max Out of Range Test": (100.0, False), # o3de/o3de#13220: Value can be set above max value using API.
    "Greater Than Rest Offset Test": (1.0, True)
}

REST_OFFSET_TESTS = {
    # "Negative Value Test": (-1.0, False), # o3de/o3de#13220: Value can be set below max value using API.
    "Zero Value Test": (0.0, True),
    "Small Value Test": (0.03, True),
    "Greater Than Contact Offset Test": (1.5, False)
}

CYLINDER_HEIGHT_TESTS = {
    "Negative Value Test": (-1.0, True),
    "Zero Value Test": (0.0, True),
    "Small Value Test": (0.1, True),
    "Min Value Test": (0.5, True),
    "Normal Value Test": (2.5, True),
    "Large Value Test": (255.0, True),
    "Large Value Boundary Test": (5000.0, True)
}

COLLIDER_RADIUS_TESTS = {
    "Negative Value Test": (-1.0, True),
    "Zero Value Test": (0.0, True),
    "Small Value Test": (0.1, True),
    "Min Value Test": (0.5, True),
    "Normal Value Test": (2.5, True),
    "Large Value Test": (255.0, True),
    "Large Value Boundary Test": (2500.0, True)
}
