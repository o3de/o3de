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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_TRANSFORMHELPERS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_TRANSFORMHELPERS_H
#pragma once


#include "Cry_Math.h"


namespace TransformHelpers
{
    // Format of forwardUpAxes: "<signOfForwardAxis><forwardAxis><signOfUpAxis><upAxis>".
    // Example of forwardUpAxes: "-Y+Z".
    // Returns 0 if successful, or returns a pointer to an error message in case of an error.
    // In case of success: X axis in res represents "forward" direction,
    // Y axis represents "up" direction.
    inline const char* GetForwardUpAxesMatrix(Matrix33& res, const char* forwardUpAxes)
    {
        Vec3 axisX(ZERO);
        Vec3 axisY(ZERO);

        for (int i = 0; i < 2; ++i)
        {
            Vec3& v = (i == 0) ? axisX : axisY;

            const float val = forwardUpAxes[i * 2 + 0] == '-' ? -1.0f : +1.0f;

            switch (forwardUpAxes[i * 2 + 1])
            {
            case 'X':
            case 'x':
                v.x = val;
                break;
            case 'Y':
            case 'y':
                v.y = val;
                break;
            case 'Z':
            case 'z':
                v.z = val;
                break;
            default:
                assert(0);
                return "Found a bad axis character in forwardUpAxes string";
            }
        }

        if (axisX == axisY)
        {
            assert(0);
            return "Forward and up axes are equal in forwardUpAxes string";
        }

        const Vec3 axisZ = axisX.cross(axisY);

        res.SetFromVectors(axisX, axisY, axisZ);

        return 0;
    }


    // Computes transform matrix that converts everything from forwardUpAxesSrc
    // coordinate system to forwardUpAxesDst coordinate system.
    // Format of forwardUpAxesXXX: "<signOfForwardAxis><forwardAxis><signOfUpAxis><upAxis>".
    // Example of forwardUpAxesXXX: "-Y+Z".
    // Returns 0 if successful, or returns a pointer to an error message in case of an error.
    // In case of success puts computed transform into res.
    // See comments to GetForwardUpAxesMatrix().
    inline const char* ComputeForwardUpAxesTransform(Matrix34& res, const char* forwardUpAxesSrc, const char* forwardUpAxesDst)
    {
        Matrix33 srcToWorld;
        Matrix33 dstToWorld;

        const char* const err0 = GetForwardUpAxesMatrix(srcToWorld, forwardUpAxesSrc);
        const char* const err1 = GetForwardUpAxesMatrix(dstToWorld, forwardUpAxesDst);

        if (err0 || err1)
        {
            return err0 ? err0 : err1;
        }

        res = Matrix34(dstToWorld * srcToWorld.GetTransposed());

        return 0;
    }


    inline Matrix34 ComputeOrthonormalMatrix(const Matrix34& m)
    {
        Vec3 x = m.GetColumn0();
        x.Normalize();

        Vec3 y = m.GetColumn1();

        Vec3 z = x.cross(y);
        z.Normalize();

        y = z.cross(x);

        Matrix34 result;
        result.SetFromVectors(x, y, z, m.GetTranslation());

        return result;
    }
}

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_TRANSFORMHELPERS_H
