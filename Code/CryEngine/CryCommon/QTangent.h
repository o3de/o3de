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

#ifndef CRYINCLUDE_CRYCOMMON_QTANGENT_H
#define CRYINCLUDE_CRYCOMMON_QTANGENT_H
#pragma once

namespace QTangent {
    // Computes a QTangent from a frame and reflection scalar representing the
    // tangent space.
    // Will also ensure the resulting QTangent is suitable for 16bit quantization.
    ILINE Quat FromFrameReflection(Quat frame, const float reflection)
    {
        frame.v = -frame.v;
        if (frame.w < 0.0f)
        {
            frame = -frame;
        }

        // Make sure w is never 0 by applying the smallest possible bias.
        // This is needed in order to have sign() never return 0 in the shaders.
        static const float BIAS_16BIT = 1.0f / 32767.0f;
        static const float BIAS_SCALE_16BIT = sqrtf(1.0f - BIAS_16BIT * BIAS_16BIT);
        if (frame.w < BIAS_16BIT && frame.w > -BIAS_16BIT)
        {
            frame *= BIAS_SCALE_16BIT;
            frame.w = BIAS_16BIT;
        }

        if (reflection < 0.0f)
        {
            frame = -frame;
        }

        return frame;
    }

    ILINE Quat FromFrameReflection(const Matrix33& frame, const float reflection)
    {
        Quat quat(frame);
        quat.Normalize();
        return FromFrameReflection(quat, reflection);
    }

    ILINE Quat FromFrameReflection16Safe(Matrix33 frame, const float reflection)
    {
        frame.OrthonormalizeFast();
        if (!frame.IsOrthonormalRH(0.1f))
        {
            frame.SetIdentity();
        }

        return FromFrameReflection(frame, reflection);
    }

    ILINE void ToTangentBitangentReflection(const Quat& qtangent, Vec3& tangent, Vec3& bitangent, float& reflection)
    {
        tangent = qtangent.GetColumn0();
        bitangent = qtangent.GetColumn1();
        reflection = qtangent.w < 0.0f ? -1.0f : +1.0f;
    }
} // namespace QTangent

// Auxiliary helper functions

#include <IIndexedMesh.h> // <> required for Interfuscator

ILINE Quat MeshTangentFrameToQTangent(const SMeshTangents& tangents)
{
    SMeshTangents tb = tangents;
    Vec3 tangent32, bitangent32;
    int16 reflection;

    tb.GetTB(tangent32, bitangent32);
    tb.GetR(reflection);

    Matrix33 frame;

    frame.SetRow(0, tangent32);
    frame.SetRow(1, bitangent32);
    frame.SetRow(2, tangent32.Cross(bitangent32).GetNormalized());

    return QTangent::FromFrameReflection16Safe(frame, reflection);
}

ILINE Quat MeshTangentFrameToQTangent(const Vec4sf& tangent, const Vec4sf& bitangent)
{
    return MeshTangentFrameToQTangent(SMeshTangents(tangent, bitangent));
}

ILINE Quat MeshTangentFrameToQTangent(const SPipTangents& tangents)
{
    return MeshTangentFrameToQTangent(SMeshTangents(tangents));
}

ILINE bool MeshTangentsFrameToQTangents(
    const Vec4sf* pTangent, const uint tangentStride,
    const Vec4sf* pBitangent, const uint bitangentStride, const uint count,
    SPipQTangents* pQTangents, const uint qtangentStride)
{
    Quat qtangent;
    for (uint i = 0; i < count; ++i)
    {
        qtangent = MeshTangentFrameToQTangent(*pTangent, *pBitangent);
        SMeshQTangents(qtangent).ExportTo(*pQTangents);

        pTangent = (const Vec4sf*)(((const uint8*)pTangent) + tangentStride);
        pBitangent = (const Vec4sf*)(((const uint8*)pBitangent) + bitangentStride);
        pQTangents = (SPipQTangents*)(((uint8*)pQTangents) + qtangentStride);
    }
    return true;
}

ILINE bool MeshTangentsFrameToQTangents(
    const SPipTangents* pTangents, const uint tangentStride, const uint count,
    SPipQTangents* pQTangents, const uint qtangentStride)
{
    Quat qtangent;
    for (uint i = 0; i < count; ++i)
    {
        qtangent = MeshTangentFrameToQTangent(*pTangents);
        SMeshQTangents(qtangent).ExportTo(*pQTangents);

        pTangents = (const SPipTangents*)(((const uint8*)pTangents) + tangentStride);
        pQTangents = (SPipQTangents*)(((uint8*)pQTangents) + qtangentStride);
    }
    return true;
}

ILINE bool MeshTangentsFrameToQTangents(
    const SMeshTangents* pTangents, const uint tangentStride, const uint count,
    SMeshQTangents* pQTangents, const uint qtangentStride)
{
    Quat qtangent;
    for (uint i = 0; i < count; ++i)
    {
        qtangent = MeshTangentFrameToQTangent(*pTangents);
        *pQTangents = SMeshQTangents(qtangent);

        pTangents = (const SMeshTangents*)(((const uint8*)pTangents) + tangentStride);
        pQTangents = (SMeshQTangents*)(((uint8*)pQTangents) + qtangentStride);
    }
    return true;
}

#endif // CRYINCLUDE_CRYCOMMON_QTANGENT_H

