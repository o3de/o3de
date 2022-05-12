/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector2.h>
#include <MCore/Source/Algorithms.h>
#include <MCore/Source/AzCoreConversions.h>


namespace MCore
{
    // project a 3D point in world space to 2D screen coordinates
    AZ::Vector3 Project(const AZ::Vector3& point, const AZ::Matrix4x4& viewProjMatrix, uint32 screenWidth, uint32 screenHeight)
    {
        // 1. expand homogenous coordinate
        AZ::Vector4 expandedPoint(point.GetX(), point.GetY(), point.GetZ(), 1.0f);

        // 2. multiply by the view and the projection matrix (note that this is a four component matrix multiplication, so no affine one!)
        expandedPoint = viewProjMatrix * expandedPoint;

        // 3. perform perspective division for the x and y coordinates only
        expandedPoint.SetX(expandedPoint.GetX() / expandedPoint.GetW());
        expandedPoint.SetY(expandedPoint.GetY() / expandedPoint.GetW());

        // 4. map them to the screen space
        AZ::Vector3 result(
            (1.0f + expandedPoint.GetX()) * (float)screenWidth  * 0.5f,
            (1.0f - expandedPoint.GetY()) * (float)screenHeight * 0.5f,
            // 5. get the distance to the camera lense plane
            expandedPoint.GetZ());

        return result;
    }


    // unproject into eye space, remember this means the resulting vector will be in camera space already
    // drawing lines that take world space coordinates with those returned coordinates will apply a camera tranform twice
    AZ::Vector3 UnprojectToEyeSpace(float screenX, float screenY, const AZ::Matrix4x4& invProjMat, float windowWidth, float windowHeight, float depth)
    {
        // convert to normalized device coordinates in range of -1 to +1
        const float x = 2.0f * (screenX / windowWidth) - 1.0f;
        const float y = 1.0f - 2.0f * (screenY / windowHeight);

        // convert into clip space
        AZ::Vector4 vec(x, y, 1, 0.0f);
        vec = invProjMat * vec;

        // return the result at the given desired depth
        return AZ::Vector3(vec.GetX(), vec.GetY(), vec.GetZ()).GetNormalized() * depth;
    }


    // unproject screen coordinates to a 3D point in world space
    AZ::Vector3 Unproject(float screenX, float screenY, float screenWidth, float screenHeight, float depth, const AZ::Matrix4x4& invProjMat, const AZ::Matrix4x4& invViewMat)
    {
        // convert to normalized device coordinates in range of -1 to +1
        const float x = 2.0f * (screenX / screenWidth) - 1.0f;
        const float y = 1.0f - 2.0f * (screenY / screenHeight); // flip y

        // convert into clip space
        AZ::Vector4 vec(x, y, 1.0f, 0.0f);
        vec = invProjMat * vec;

        // return the result at the given desired depth and transform from eyespace into world space
        return invViewMat * (AZ::Vector3(vec.GetX(), vec.GetY(), vec.GetZ()).GetNormalized() * depth);
    }


    AZ::Vector3 UnprojectOrtho(float screenX, float screenY, float screenWidth, float screenHeight, float depth, const AZ::Matrix4x4& projectionMatrix, const AZ::Matrix4x4& viewMatrix)
    {
        // 1. normalize the screen coordinates so that it will be in range [-1.0, 1.0]
        const float normalizedX = 2.0f * (screenX / (float)screenWidth) - 1.0f;
        const float normalizedY = 2.0f * (screenY / (float)screenHeight) - 1.0f;

        // 2. expand homogenous coordinate
        AZ::Vector4 expandedPoint(normalizedX, -normalizedY, depth, 1.0f);

        // 3. multiply by the inverse of the projection matrix
        expandedPoint = MCore::InvertProjectionMatrix(projectionMatrix) * expandedPoint;

        // 4. multiply by the inverse of the modelview matrix
        expandedPoint = MCore::InvertProjectionMatrix(viewMatrix) * expandedPoint;

        // 5. perform perspective division
        expandedPoint /= expandedPoint.GetW();
        //MCore::LogDebug("screenCoords: %i %i, unitCubeValues: (%.2f, %.2f, %.2f) result: (%.2f, %.2f, %.2f, %.2f)", screenX, screenY, normalizedX, normalizedY, normalizedDepth, expandedPoint.x, expandedPoint.y, expandedPoint.z, expandedPoint.w);

        // 6. project down to three components again
        return AZ::Vector3(expandedPoint.GetX(), expandedPoint.GetY(), expandedPoint.GetZ());
    }


    // convert from cartesian coordinates into spherical coordinates
    // this uses the y-axis (up) and x-axis (right) as basis
    // the vector needs to be normalized!
    AZ::Vector2 ToSpherical(const AZ::Vector3& normalizedVector)
    {
        return AZ::Vector2(Math::ATan2(normalizedVector.GetY(), normalizedVector.GetX()),
            Math::ACos(normalizedVector.GetZ()));
    }


    // convert from spherical coordinates back into cartesian coordinates
    // this uses the y-axis (up) and x-axis (right) as basis
    AZ::Vector3 FromSpherical(const AZ::Vector2& spherical)
    {
        return AZ::Vector3(
            Math::Cos(spherical.GetX()),
            Math::Sin(spherical.GetX()) * Math::Sin(spherical.GetY()),
            Math::Sin(spherical.GetX()) * Math::Cos(spherical.GetY()));
    }


    // calculate sample rate info
    void CalcSampleRateInfo(float samplesPerSecond, float duration, float* outSampleTimeStep, uint32* outNumSamples)
    {
        if (samplesPerSecond < MCore::Math::epsilon)
        {
            samplesPerSecond = MCore::Math::epsilon;
        }

        float sampleTimeStep = 1.0f / samplesPerSecond;
        if (sampleTimeStep >= duration)
        {
            *outSampleTimeStep = duration;
            *outNumSamples = 2;
            return;
        }

        const uint32 numSamples         = (uint32)(duration * samplesPerSecond) + 1;
        const float timeStepError       = MCore::Math::FMod(duration, sampleTimeStep);
        const float beforeSampleTimeStep = sampleTimeStep;
        sampleTimeStep  += (timeStepError / (float)(numSamples - 1));
        if (sampleTimeStep * (numSamples - 1) > duration + MCore::Math::epsilon)
        {
            sampleTimeStep = beforeSampleTimeStep;
        }

        *outSampleTimeStep  = sampleTimeStep;
        *outNumSamples      = numSamples;
    }


    // calculate the area of the triangles made of the given three points
    double CalcTriangleAreaAccurate(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3)
    {
        // first point
        const double ax = v3.GetX() - v2.GetX();
        const double ay = v3.GetY() - v2.GetY();
        const double az = v3.GetZ() - v2.GetZ();

        // second point
        const double bx = v1.GetX() - v3.GetX();
        const double by = v1.GetY() - v3.GetY();
        const double bz = v1.GetZ() - v3.GetZ();

        // third point
        const double cx = v2.GetX() - v1.GetX();
        const double cy = v2.GetY() - v1.GetY();
        const double cz = v2.GetZ() - v1.GetZ();

        // squared lengths of the triangle sides
        const double squaredA = ax * ax + ay * ay + az * az;
        const double squaredB = bx * bx + by * by + bz * bz;
        const double squaredC = cx * cx + cy * cy + cz * cz;

        // safety check
        if (squaredA == 0.0 || squaredB == 0.0 || squaredC == 0.0)
        {
            return 0.0;
        }

        // calculate the lengths of the triangle
        const double a = sqrt(squaredA);
        const double b = sqrt(squaredB);
        const double c = sqrt(squaredC);

        // heron's formula
        const double halfPerimeter  = (a + b + c) / 2.0;
        const double squaredArea    = halfPerimeter * (halfPerimeter - a) * (halfPerimeter - b) * (halfPerimeter - c);
        const double area           = sqrt(squaredArea);

        // return the result
        return area;
    }


    // calculate the area of the triangles made of the given three points
    float CalcTriangleArea(const AZ::Vector3& v1, const AZ::Vector3& v2, const AZ::Vector3& v3)
    {
        // calculate the lengths of the triangle
        const float a = SafeLength(v3 - v2);
        const float b = SafeLength(v1 - v3);
        const float c = SafeLength(v2 - v1);

        // heron's formula
        const float halfPerimeter   = (a + b + c) / 2.0f;
        const float squaredArea = halfPerimeter * (halfPerimeter - a) * (halfPerimeter - b) * (halfPerimeter - c);
        const float area            = sqrt(squaredArea);

        // return the result
        return area;
    }


    // orthogonal projection on the xz plane
    AZ::Vector2 OrthogonalProject(const AZ::Vector3& pos)
    {
        return AZ::Vector2(pos.GetX(), pos.GetZ());
    }


    // orthogonal unproject from the xz plane back onto the sphere
    AZ::Vector3 OrthogonalUnproject(const AZ::Vector2& uv)
    {
        AZ::Vector3 outDirection;
        outDirection.SetX(uv.GetX());
        outDirection.SetZ(uv.GetY());
        outDirection.SetY(Math::SafeSqrt(-(outDirection.GetX() * outDirection.GetX()) - (outDirection.GetZ() * outDirection.GetZ()) + 1.0f)); // find the right height on the sphere for this ortho xy coord
        return SafeNormalize(outDirection);
    }


    // stereographic project
    AZ::Vector2 StereographicProject(const AZ::Vector3& pos)
    {
        AZ::Vector2 result;
        const float div = (1.0f - pos.GetY()) + Math::epsilon;
        result.SetX(pos.GetX() / div);
        result.SetY(pos.GetZ() / div);
        return result;
    }


    // stereographic unproject
    AZ::Vector3 StereographicUnproject(const AZ::Vector2& uv)
    {
        AZ::Vector3 result;
        const float s = 2.0f  / (uv.GetX() * uv.GetX() + uv.GetY() * uv.GetY() + 1.0f);
        result.SetX(s * uv.GetX());
        result.SetY(1.0f - s);
        result.SetZ(s * uv.GetY());
        return result;
    }


    // check if a given point is inside a 2d convex/concave polygon
    // it does this by checking how many times a line intersects with the poly (how many times it goes inside and outside again)
    bool PointInPoly(AZ::Vector2* verts, size_t numVerts, const AZ::Vector2& point)
    {
        bool c = false;
        for (size_t i = 0, j = numVerts - 1; i < numVerts; j = i++)
        {
            if (((verts[i].GetY() > point.GetY()) != (verts[j].GetY() > point.GetY())) && (point.GetX() < (verts[j].GetX() - verts[i].GetX()) * (point.GetY() - verts[i].GetY()) / (verts[j].GetY() - verts[i].GetY()) + verts[i].GetX()))
            {
                c = !c;
            }
        }

        return c;
    }


    // get the distance to a given edge
    float DistanceToEdge(const AZ::Vector2& edgePointA, const AZ::Vector2& edgePointB, const AZ::Vector2& testPoint)
    {
        // Return minimum distance between line segment vw and point p
        AZ::Vector2 edgeVector = edgePointB - edgePointA;
        const float l2 = edgeVector.GetLengthSq();
        if (l2 < Math::epsilon)
        {
            return (testPoint - edgePointA).GetLength();
        }

        // Consider the line extending the segment, parameterized as v + t (w - v).
        // We find projection of point p onto the line.
        // It falls where t = [(p-v) . (w-v)] / |w-v|^2
        // We clamp t from [0,1] to handle points outside the segment vw.
        const float dot = (testPoint - edgePointA).Dot(edgeVector);
        const float t = Max(0.0f, Min(1.0f, dot / l2));
        const AZ::Vector2 projection = edgePointA + t * edgeVector; // Projection falls on the segment
        return (testPoint - projection).GetLength();
    }


    // check if the test point is inside the polygon
    AZ::Vector2 ClosestPointToPoly(const AZ::Vector2* polyPoints, size_t numPoints, const AZ::Vector2& testPoint)
    {
        AZ::Vector2 result = AZ::Vector2::CreateZero();
        float closestDist = FLT_MAX;
        for (size_t i = 0; i < numPoints; ++i)
        {
            AZ::Vector2 edgePointA;
            AZ::Vector2 edgePointB;
            if (i < numPoints - 1)
            {
                edgePointA = polyPoints[i];
                edgePointB = polyPoints[i + 1];
            }
            else
            {
                edgePointA = polyPoints[i];
                edgePointB = polyPoints[0];
            }

            float dist;
            AZ::Vector2 projection;
            AZ::Vector2 edgeVector = edgePointB - edgePointA;
            const float l2 = edgeVector.GetLengthSq();
            if (l2 < Math::epsilon)
            {
                dist = (testPoint - edgePointA).GetLength();
                projection = edgePointA;
            }
            else
            {
                const float dot = (testPoint - edgePointA).Dot(edgeVector);
                const float t = Max(0.0f, Min(1.0f, dot / l2));
                projection = edgePointA + t * edgeVector;
                dist = (testPoint - projection).GetLength();
            }

            if (dist < closestDist)
            {
                closestDist = dist;
                result      = projection;
            }
        }

        return result;
    }
} // namespace MCore
