#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT

def validate_xyz_is_float(x: float, y: float, z: float, error_message: str) -> None:
    '''
    Function for testing if X, Y, Z used in editor component tests are all actually floats.

    x: The x-point position to validate if it's a float.
    y: The y-point position to validate if it's a float.
    z: The z-point position to validate if it's a float.
    error_message: The error message to assert if values are not float.

    return: boolean result of testing whether each input is a float.
    '''
    assert isinstance(x, float) and isinstance(y, float) and isinstance(z, float), message



