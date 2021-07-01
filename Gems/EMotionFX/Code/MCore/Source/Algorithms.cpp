/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    bool PointInPoly(AZ::Vector2* verts, uint32 numVerts, const AZ::Vector2& point)
    {
        uint32 c = 0;
        for (uint32 i = 0, j = numVerts - 1; i < numVerts; j = i++)
        {
            if (((verts[i].GetY() > point.GetY()) != (verts[j].GetY() > point.GetY())) && (point.GetX() < (verts[j].GetX() - verts[i].GetX()) * (point.GetY() - verts[i].GetY()) / (verts[j].GetY() - verts[i].GetY()) + verts[i].GetX()))
            {
                c = !c;
            }
        }

        return (c > 0);
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
    AZ::Vector2 ClosestPointToPoly(const AZ::Vector2* polyPoints, uint32 numPoints, const AZ::Vector2& testPoint)
    {
        AZ::Vector2 result;
        float closestDist = FLT_MAX;
        for (uint32 i = 0; i < numPoints; ++i)
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


    // static CRC lookup table
    /*static uint32 CRC32Table[256] =
    {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
        0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
        0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
        0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
        0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
        0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
        0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
        0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
        0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
        0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,

        0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
        0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
        0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
        0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
        0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
        0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
        0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
        0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
        0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
        0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
        0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,

        0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
        0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
        0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
        0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
        0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
        0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
        0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
        0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
        0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
        0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
        0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,

        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
        0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
        0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
        0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
        0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
        0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
        0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
        0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
        0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
        0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
        0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
    };



    // calculate the CRC32
    void CalcCRC32(uint8 byteValue, uint32& CRC)
    {
        CRC = ((CRC) >> 8) ^ MCore::CRC32Table[(byteValue) ^ ((CRC) & 0x000000FF)];
    }*/
} // namespace MCore
