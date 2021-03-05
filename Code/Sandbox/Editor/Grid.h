/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_GRID_H
#define CRYINCLUDE_EDITOR_GRID_H

#pragma once

/** Definition of grid used in 2D viewports.
*/
class SANDBOX_API CGrid
{
public:
    //! Resolution of grid, it must be multiply of 2.
    double size;
    //! Draw major lines every Nth grid line.
    int majorLine;
    //! True if grid enabled.
    bool bEnabled;
    //! Meters per grid unit.
    double scale;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    Ang3 rotationAngles;
    Vec3 translation;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    //! If snap to angle.
    bool bAngleSnapEnabled;
    double angleSnap;

    //////////////////////////////////////////////////////////////////////////
    CGrid();

    //! Snap vector to this grid.
    Vec3 Snap(const Vec3& vec) const;
    Vec3 Snap(const Vec3& vec, double fZoom) const;

    //! Snap angle to current angle snapping value.
    double SnapAngle(double angle) const;
    //! Snap angle to current angle snapping value.
    Ang3 SnapAngle(const Ang3& angle) const;

    //! Enable or disable grid.
    void Enable(bool enable) { bEnabled = enable; }
    //! Check if grid enabled.
    bool IsEnabled() const { return bEnabled; }

    //! Enables or disable angle snapping.
    void EnableAngleSnap(bool enable) { bAngleSnapEnabled = enable; };

    //! Return if snapping of angle is enabled.
    bool IsAngleSnapEnabled() const { return bAngleSnapEnabled; };
    //! Returns ammount of snapping for angle in degrees.
    double GetAngleSnap() const { return angleSnap; };

    void Serialize(XmlNodeRef& xmlNode, bool bLoading);

    //! Get transformation matrix of gird.
    Matrix34 GetMatrix() const;
};


#endif // CRYINCLUDE_EDITOR_GRID_H
