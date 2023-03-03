"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

This util is intended to store common test values for editor component tests.
"""

# Integer Tests
INT_TESTS_NEGATIVE_EXPECT_PASS = {
    "Negative Value Test": (-1.0, True),
    "Zero Value Test": (0.0, True),
    "Small Value Test": (0.1, True),
    "Normal Value Test": (2.5, True),
    "Large Value Test": (255.0, True),
    "Large Value Boundary Test": (5000.0, True)
}

INT_TESTS_NEGATIVE_EXPECT_FAIL = {
    "Negative Value Test": (-1, False),
    "Zero Value Test": (0, True),
    "Small Value Test": (1, True),
    "Normal Value Test": (25, True),
    "Large Value Test": (255, True),
    "Large Value Boundary Test": (5000, True)
}

INT_TESTS_NEGATIVE_BOUNDARY_EXPECT_FAIL = {
    "Negative Value Test": (-1, False),
    "Zero Value Test": (0, True),
    "Small Value Test": (1, True),
    "Normal Value Test": (25, True),
    "Large Value Test": (255, True),
    "Large Value Boundary Test": (5000, False)
}

# Float Tests
FLOAT_TESTS_NEGATIVE_EXPECT_PASS = {
    "Negative Value Test": (-1.0, True),
    "Zero Value Test": (0.0, True),
    "Small Value Test": (0.1, True),
    "Normal Value Test": (2.5, True),
    "Large Value Test": (255.0, True),
    "Large Value Boundary Test": (5000.0, True)
}

FLOAT_HEIGHT_TESTS = {
    "Negative Value Test": (-1.0, False),
    "Zero Value Test": (0.0, False),
    "Small Value Test": (0.1, False),
    "Min Value Test": (0.5, True),
    "Normal Value Test": (2.5, True),
    "Large Value Test": (255.0, True),
    "Large Value Boundary Test": (5000.0, True)
}

FLOAT_RADIAL_TESTS = {
    "Negative Value Test": (-1.0, False),
    "Zero Value Test": (0.0, True),
    "Small Value Test": (0.1, True),
    "Min Value Test": (0.5, True),
    "Normal Value Test": (2.5, True),
    "Large Value Test": (255.0, True),
    "Large Value Boundary Test": (2500.0, True)
}

FLOAT_TESTS_NEGATIVE_EXPECT_FAIL = {
    "Negative Value Test": (-1.0, False),
    "Zero Value Test": (0.0, True),
    "Small Value Test": (0.1, True),
    "Normal Value Test": (2.5, True),
    "Large Value Test": (255.0, True),
    "Large Value Boundary Test": (5000.0, True)
}

# Vector3 Tests
VECTOR3_TESTS_NEGATIVE_EXPECT_PASS = {
    "Negative Value Test": (-1.0, -1.0, -1.0, True),
    "Zero Value Test": (0.0, 0.0, 0.0, True),
    "Small Value Test": (0.1, 0.1, 0.1, True),
    "Normal Value Test": (2.5, 2.5, 2.5, True),
    "Large Value Test": (255.0, 255.0, 255.0, True),
    "Large Value Boundary Test": (5000.0, 5000.0, 5000.0, True)
}

VECTOR3_TESTS_NEGATIVE_EXPECT_FAIL = {
    "Negative Value Test": (-1.0, -1.0, -1.0, False),
    "Zero Value Test": (0.0, 0.0, 0.0, True),
    "Small Value Test": (0.1, 0.1, 0.1, True),
    "Normal Value Test": (2.5, 2.5, 2.5, True),
    "Large Value Test": (255.0, 255.0, 255.0, True),
    "Large Value Boundary Test": (5000.0, 5000.0, 5000.0, True)
}
