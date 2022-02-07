"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# setup path
import azlmbr.object
import azlmbr.math as math

# returns a Vector3 of cartesian coordinates from spherical coordinates
# phi and theta are in degrees
def spherical_to_cartesian(phi, theta, dist=1.0):
    phi = math.Math_DegToRad(phi)
    theta = math.Math_DegToRad(theta)

    x = float(dist) * float(math.Math_Sin(phi)) * float(math.Math_Cos(theta))
    y = float(dist) * float(math.Math_Sin(phi)) * float(math.Math_Sin(theta))
    z = float(dist) * float(math.Math_Cos(phi))

    return math.Vector3(x, y, z)


# converts two Cartesian points into their midpoint, normalized into a vector of size r
def normalize_midpoint(pos1, pos2, r=1.0):
    pos = pos1.Add(pos2).MultiplyFloat(0.5)
    pos.Normalize()

    return pos.MultiplyFloat(r)

