"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

These test values are intended to be utilized alongside the PhysX Collider
"""

CONTACT_OFFSET_TESTS = {
    "Negative Value Test": (-1.0, False),
    "Zero Value Test": (0.0, False),
    "Small Value Test": (0.2, True),
    "Max Boundary Value Test": (50.0, True),
    "Max Out of Range Test": (100.0, False),
    "Greater Than Rest Offset Test": (1.0, True)
}

REST_OFFSET_TESTS = {
    "Negative Value Test": (-1.0, False),
    "Zero Value Test": (0.0, True),
    "Small Value Test": (0.03, True),
    "Greater Than Contact Offset Test": (1.5, True)
}

CYLINDER_HEIGHT_TESTS = {
    # Common Tests Height Values cannont be used due to api allowing negative, zero, and small values
    # TODO: Create a bug to track this issue
    "Negative Value Test": (-1.0, True),
    "Zero Value Test": (0.0, True),
    "Small Value Test": (0.1, True),
    "Min Value Test": (0.5, True),
    "Normal Value Test": (2.5, True),
    "Large Value Test": (255.0, True),
    "Large Value Boundary Test": (5000.0, True)
}

COLLIDER_RADIUS_TESTS = {
    # Common Tests Radial Values can't be used due API allowing a negative value to be set.
    # TODO: Create a Bug to track this issue.
    "Negative Value Test": (-1.0, True),
    "Zero Value Test": (0.0, True),
    "Small Value Test": (0.1, True),
    "Min Value Test": (0.5, True),
    "Normal Value Test": (2.5, True),
    "Large Value Test": (255.0, True),
    "Large Value Boundary Test": (2500.0, True)
}