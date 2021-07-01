/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "Quaternion.h"
#include <AzCore/std/typetraits/aligned_storage.h>

namespace MCore
{
    // spherical quadratic interpolation
    Quaternion Quaternion::Squad(const Quaternion& p, const Quaternion& a, const Quaternion& b, const Quaternion& q, float t)
    {
        Quaternion q0(p.Slerp(q, t));
        Quaternion q1(a.Slerp(b, t));
        return q0.Slerp(q1, 2.0f * t * (1.0f - t));
    }


    // returns the approximately normalized linear interpolated result [t must be between 0..1]
    Quaternion Quaternion::NLerp(const Quaternion& to, float t) const
    {
        AZ_Assert(t > -MCore::Math::epsilon && t < (1 + MCore::Math::epsilon), "Expected t to be between 0..1");
        static const float weightCloseToOne = 1.0f - MCore::Math::epsilon;

        // Early out for boundaries (common cases)
        if (t < MCore::Math::epsilon)
        {
            return *this;
        }
        else if (t > weightCloseToOne)
        {
            return to;
        }

    #if AZ_TRAIT_USE_PLATFORM_SIMD_SSE
        __m128 num1, num2, num3, num4, fromVec, toVec;
        const float omt = 1.0f - t;
        float dot;

        // perform dot product between this quat and the 'to' quat
        num4 = _mm_setzero_ps();            // sets sum to zero
        fromVec = _mm_loadu_ps(&x);         //
        toVec = _mm_loadu_ps(&to.x);        //
        num3 = _mm_mul_ps(fromVec, toVec);  // performs multiplication   num3 = a[3]*b[3]  a[2]*b[2]  a[1]*b[1]  a[0]*b[0]
        num3 = _mm_hadd_ps(num3, num3);     // performs horizontal addition - num3=  a[3]*b[3]+ a[2]*b[2]  a[1]*b[1]+a[0]*b[0]  a[3]*b[3]+ a[2]*b[2]  a[1]*b[1]+a[0]*b[0]
        num4 = _mm_add_ps(num4, num3);      // performs vertical addition
        num4 = _mm_hadd_ps(num4, num4);
        _mm_store_ss(&dot, num4);           // store the dot result

        if (dot < 0.0f)
        {
            t = -t;
        }

        // calculate interpolated value
        num2 = _mm_load_ps1(&omt);
        num3 = _mm_load_ps1(&t);
        num4 = _mm_mul_ps(fromVec, num2);   // omt * xyzw
        num1 = _mm_mul_ps(toVec, num3);     // t * to.xyzw
        num2 = _mm_add_ps(num1, num4);      // interpolated value

        // calculate the square length
        num4 = _mm_setzero_ps();
        num3 = _mm_mul_ps(num2, num2);      // square length
        num1 = _mm_hadd_ps(num3, num3);
        num4 = _mm_add_ps(num4, num1);
        num3 = _mm_hadd_ps(num4, num4);
        //num4 = _mm_rsqrt_ps( num3 );      // length (argh, too inaccurate on some models)

        AZStd::aligned_storage<sizeof(float) * 4, 16>::type numFloatStorage;
        float* numFloat = reinterpret_cast<float*>(&numFloatStorage);

        _mm_store_ps(numFloat, num3);
        const float invLen = Math::InvSqrt(numFloat[0]);
        num4 = _mm_load_ps1(&invLen);

        // calc inverse length, which normalizes everything
        num1 = _mm_mul_ps(num2, num4);

        _mm_store_ps(numFloat, num1);
        return Quaternion(numFloat[0], numFloat[1], numFloat[2], numFloat[3]);
    #else
        const float omt = 1.0f - t;
        const float dot = x * to.x + y * to.y + z * to.z + w * to.w;
        if (dot < 0.0f)
        {
            t = -t;
        }

        // calculate the interpolated values
        const float newX = (omt * x + t * to.x);
        const float newY = (omt * y + t * to.y);
        const float newZ = (omt * z + t * to.z);
        const float newW = (omt * w + t * to.w);

        // calculate the inverse length
        //  const float invLen = 1.0f / Math::FastSqrt( newX*newX + newY*newY + newZ*newZ + newW*newW );
        //  const float invLen = Math::FastInvSqrt( newX*newX + newY*newY + newZ*newZ + newW*newW );
        const float invLen = Math::InvSqrt(newX * newX + newY * newY + newZ * newZ + newW * newW);

        // return the normalized linear interpolation
        return Quaternion(newX * invLen,
            newY * invLen,
            newZ * invLen,
            newW * invLen);
    #endif
    }



    // returns the linear interpolated result [t must be between 0..1]
    Quaternion Quaternion::Lerp(const Quaternion& to, float t) const
    {
        const float omt = 1.0f - t;
        const float cosom = x * to.x + y * to.y + z * to.z + w * to.w;
        if (cosom < 0.0f)
        {
            t = -t;
        }

        // return the linear interpolation
        return Quaternion(omt * x + t * to.x,
            omt * y + t * to.y,
            omt * z + t * to.z,
            omt * w + t * to.w);
    }



    // quaternion from an axis and angle
    Quaternion::Quaternion(const AZ::Vector3& axis, float angle)
    {
        const float squaredLength = axis.GetLengthSq();
        if (squaredLength > 0.0f)
        {
            const float halfAngle = angle * 0.5f;
            const float sinScale = Math::Sin(halfAngle) / Math::Sqrt(squaredLength);
            x = axis.GetX() * sinScale;
            y = axis.GetY() * sinScale;
            z = axis.GetZ() * sinScale;
            w = Math::Cos(halfAngle);
        }
        else
        {
            x = y = z = 0.0f;
            w     = 1.0f;
        }
    }



    // quaternion from a spherical rotation
    Quaternion::Quaternion(const AZ::Vector2& spherical, float angle)
    {
        const float latitude  = spherical.GetX();
        const float longitude = spherical.GetY();

        const float s = Math::Sin(angle / 2.0f);
        const float c = Math::Cos(angle / 2.0f);

        const float sin_lat = Math::Sin(latitude);
        const float cos_lat = Math::Cos(latitude);

        const float sin_lon = Math::Sin(longitude);
        const float cos_lon = Math::Cos(longitude);

        x = s * cos_lat * sin_lon;
        y = s * sin_lat;
        z = s * sin_lat * cos_lon;
        w = c;
    }


    // convert to an axis and angle
    void Quaternion::ToAxisAngle(AZ::Vector3* axis, float* angle) const
    {
        *angle = 2.0f * Math::ACos(w);

        const float sinHalfAngle = Math::Sin(*angle * 0.5f);
        if (sinHalfAngle > 0.0f)
        {
            const float invS = 1.0f / sinHalfAngle;
            axis->Set(x * invS, y * invS, z * invS);
        }
        else
        {
            axis->Set(0.0f, 1.0f, 0.0f);
            *angle = 0.0f;
        }
    }


    //  converts from unit quaternion to spherical rotation angles
    void Quaternion::ToSpherical(AZ::Vector2* spherical, float* angle) const
    {
        AZ::Vector3 axis;
        ToAxisAngle(&axis, angle);

        float longitude;
        if (axis.GetX() * axis.GetX() + axis.GetZ() * axis.GetZ() < 0.0001f)
        {
            longitude = 0.0f;
        }
        else
        {
            longitude = Math::ATan2(axis.GetX(), axis.GetZ());
            if (longitude < 0.0f)
            {
                longitude += Math::twoPi;
            }
        }

        spherical->SetX(-Math::ASin(axis.GetY()));
        spherical->SetY(longitude);
    }



    // setup the quaternion from a roll, pitch and yaw
    Quaternion& Quaternion::SetEuler(float pitch, float yaw, float roll)
    {
        // METHOD #1:
        const float halfYaw     = yaw   * 0.5f;
        const float halfPitch   = pitch * 0.5f;
        const float halfRoll    = roll  * 0.5f;

        const float cY = Math::Cos(halfYaw);
        const float sY = Math::Sin(halfYaw);
        const float cP = Math::Cos(halfPitch);
        const float sP = Math::Sin(halfPitch);
        const float cR = Math::Cos(halfRoll);
        const float sR = Math::Sin(halfRoll);

        x = cY * sP * cR - sY * cP * sR;
        y = cY * sP * sR + sY * cP * cR;
        z = cY * cP * sR - sY * sP * cR;
        w = cY * cP * cR + sY * sP * sR;

        //  Normalize();    // we might be able to leave the normalize away, but better safe than not, this is more robust :)

        return *this;

        /*

            // METHOD #2:
            Quaternion Qx(Vector3(sP, 0, 0), cP);
            Quaternion Qy(Vector3(0, sY, 0), cY);
            Quaternion Qz(Vector3(0, 0, sR), cR);

            Quaternion result = Qx * Qy * Qz;

            x = result.x;
            y = result.y;
            z = result.z;
            w = result.w;

            return *this;
        */
    }



    // convert the quaternion to a matrix
    Matrix Quaternion::ToMatrix() const
    {
        Matrix m;

        const float xx = x * x;
        const float xy = x * y, yy = y * y;
        const float xz = x * z, yz = y * z, zz = z * z;
        const float xw = x * w, yw = y * w, zw = z * w, ww = w * w;

        MMAT(m, 0, 0) = +xx - yy - zz + ww;
        MMAT(m, 0, 1) = +xy + zw + xy + zw;
        MMAT(m, 0, 2) = +xz - yw + xz - yw;
        MMAT(m, 0, 3) = 0.0f;
        MMAT(m, 1, 0) = +xy - zw + xy - zw;
        MMAT(m, 1, 1) = -xx + yy - zz + ww;
        MMAT(m, 1, 2) = +yz + xw + yz + xw;
        MMAT(m, 1, 3) = 0.0f;
        MMAT(m, 2, 0) = +xz + yw + xz + yw;
        MMAT(m, 2, 1) = +yz - xw + yz - xw;
        MMAT(m, 2, 2) = -xx - yy + zz + ww;
        MMAT(m, 2, 3) = 0.0f;
        MMAT(m, 3, 0) = 0.0f;
        MMAT(m, 3, 1) = 0.0f;
        MMAT(m, 3, 2) = 0.0f;
        MMAT(m, 3, 3) = 1.0f;

        return m;
    }



    // construct the quaternion from a given rotation matrix
    Quaternion Quaternion::ConvertFromMatrix(const Matrix& m)
    {
        Quaternion result;

        const float trace = MMAT(m, 0, 0) + MMAT(m, 1, 1) + MMAT(m, 2, 2);
        if (trace > 0.0f /*Math::epsilon*/)
        {
            const float s = 0.5f / Math::Sqrt(trace + 1.0f);
            result.w = 0.25f / s;
            result.x = (MMAT(m, 1, 2) - MMAT(m, 2, 1)) * s;
            result.y = (MMAT(m, 2, 0) - MMAT(m, 0, 2)) * s;
            result.z = (MMAT(m, 0, 1) - MMAT(m, 1, 0)) * s;
        }
        else
        {
            if (MMAT(m, 0, 0) > MMAT(m, 1, 1) && MMAT(m, 0, 0) > MMAT(m, 2, 2))
            {
                const float s = 2.0f * Math::Sqrt(1.0f + MMAT(m, 0, 0) - MMAT(m, 1, 1) - MMAT(m, 2, 2));
                const float oneOverS = 1.0f / s;
                result.x = 0.25f * s;
                result.y = (MMAT(m, 1, 0) + MMAT(m, 0, 1)) * oneOverS;
                result.z = (MMAT(m, 2, 0) + MMAT(m, 0, 2)) * oneOverS;
                result.w = (MMAT(m, 1, 2) - MMAT(m, 2, 1)) * oneOverS;
            }
            else
            if (MMAT(m, 1, 1) > MMAT(m, 2, 2))
            {
                const float s = 2.0f * Math::Sqrt(1.0f + MMAT(m, 1, 1) - MMAT(m, 0, 0) - MMAT(m, 2, 2));
                const float oneOverS = 1.0f / s;
                result.x = (MMAT(m, 1, 0) + MMAT(m, 0, 1)) * oneOverS;
                result.y = 0.25f * s;
                result.z = (MMAT(m, 2, 1) + MMAT(m, 1, 2)) * oneOverS;
                result.w = (MMAT(m, 2, 0) - MMAT(m, 0, 2)) * oneOverS;
            }
            else
            {
                const float s = 2.0f * Math::Sqrt(1.0f + MMAT(m, 2, 2) - MMAT(m, 0, 0) - MMAT(m, 1, 1));
                const float oneOverS = 1.0f / s;
                result.x = (MMAT(m, 2, 0) + MMAT(m, 0, 2)) * oneOverS;
                result.y = (MMAT(m, 2, 1) + MMAT(m, 1, 2)) * oneOverS;
                result.z = 0.25f * s;
                result.w = (MMAT(m, 0, 1) - MMAT(m, 1, 0)) * oneOverS;
            }
        }

        /*
            const float trace = MMAT(m,0,0) + MMAT(m,1,1) + MMAT(m,2,2) + 1.0f;
            if (trace > Math::epsilon)
            {
                const float s = 0.5f / Math::Sqrt(trace);
                result.w = 0.25f / s;
                result.x = ( MMAT(m,1,2) - MMAT(m,2,1) ) * s;
                result.y = ( MMAT(m,2,0) - MMAT(m,0,2) ) * s;
                result.z = ( MMAT(m,0,1) - MMAT(m,1,0) ) * s;
            }
            else
            {
                if (MMAT(m,0,0) > MMAT(m,1,1) && MMAT(m,0,0) > MMAT(m,2,2))
                {
                    const float s = 2.0f * Math::Sqrt( 1.0f + MMAT(m,0,0) - MMAT(m,1,1) - MMAT(m,2,2));
                    const float oneOverS = 1.0f / s;
                    result.x = 0.25f * s;
                    result.y = (MMAT(m,1,0) + MMAT(m,0,1) ) * oneOverS;
                    result.z = (MMAT(m,2,0) + MMAT(m,0,2) ) * oneOverS;
                    result.w = (MMAT(m,2,1) - MMAT(m,1,2) ) * oneOverS;
                }
                else
                if (MMAT(m,1,1) > MMAT(m,2,2))
                {
                    const float s = 2.0f * Math::Sqrt( 1.0f + MMAT(m,1,1) - MMAT(m,0,0) - MMAT(m,2,2));
                    const float oneOverS = 1.0f / s;
                    result.x = (MMAT(m,1,0) + MMAT(m,0,1) ) * oneOverS;
                    result.y = 0.25f * s;
                    result.z = (MMAT(m,2,1) + MMAT(m,1,2) ) * oneOverS;
                    result.w = (MMAT(m,2,0) - MMAT(m,0,2) ) * oneOverS;
                }
                else
                {
                    const float s = 2.0f * Math::Sqrt( 1.0f + MMAT(m,2,2) - MMAT(m,0,0) - MMAT(m,1,1) );
                    const float oneOverS = 1.0f / s;
                    result.x = (MMAT(m,2,0) + MMAT(m,0,2) ) * oneOverS;
                    result.y = (MMAT(m,2,1) + MMAT(m,1,2) ) * oneOverS;
                    result.z = 0.25f * s;
                    result.w = (MMAT(m,1,0) - MMAT(m,0,1) ) * oneOverS;
                }
            }
        */
        return result;
    }


    // convert a quaternion to euler angles (in degrees)
    AZ::Vector3 Quaternion::ToEuler() const
    {
        /*
            // METHOD #1:

            Vector3 euler;

            float matrix[3][3];
            float cx,sx;
            float cy,sy,yr;
            float cz,sz;

            matrix[0][0] = 1.0 - (2.0 * y * y) - (2.0 * z * z);
            matrix[1][0] = (2.0 * x * y) + (2.0 * w * z);
            matrix[2][0] = (2.0 * x * z) - (2.0 * w * y);
            matrix[2][1] = (2.0 * y * z) + (2.0 * w * x);
            matrix[2][2] = 1.0 - (2.0 * x * x) - (2.0 * y * y);

            sy = -matrix[2][0];
            cy = Math::Sqrt(1 - (sy * sy));
            yr = Math::ATan2(sy,cy);
            euler.y = yr;

            // avoid divide by zero only where y ~90 or ~270
            if (sy != 1.0 && sy != -1.0)
            {
                cx = matrix[2][2] / cy;
                sx = matrix[2][1] / cy;
                euler.x = Math::ATan2(sx,cx);

                cz = matrix[0][0] / cy;
                sz = matrix[1][0] / cy;
                euler.z = Math::ATan2(sz,cz);
            }
            else
            {
                matrix[1][1] = 1.0 - (2.0 * x * x) - (2.0 * z * z);
                matrix[1][2] = (2.0 * y * z) - (2.0 * w * x);
                cx = matrix[1][1];
                sx = -matrix[1][2];
                euler.x = Math::ATan2(sx,cx);

                cz = 1.0;
                sz = 0.0;
                euler.z = Math::ATan2(sz,cz);
            }

            return euler;
        */

        /*
            // METHOD #2:
            Matrix mat = ToMatrix();

            //
            float cy = Math::Sqrt(mat.m44[0][0]*mat.m44[0][0] + mat.m44[0][1]*mat.m44[0][1]);
            if (cy > 16.0*Math::epsilon)
            {
                result.x = -atan2(mat.m44[1][2], mat.m44[2][2]);
                result.y = -atan2(-mat.m44[0][2], cy);
                result.z = -atan2(mat.m44[0][1], mat.m44[0][0]);
            }
            else
            {
                result.x = -atan2(-mat.m44[2][1], mat.m44[1][1]);
                result.y = -atan2(-mat.m44[0][2], cy);
                result.z = 0.0;
            }

            return result;
        */

        // METHOD #3 (without conversion to matrix first):
        // TODO: safety checks?
        float m00 = 1.0f - (2.0f * ((y * y) + z * z));
        float m01 = 2.0f * (x * y + w * z);

        AZ::Vector3 result(
            Math::ATan2(2.0f * (y * z + w * x), 1.0f - (2.0f * ((x * x) + (y * y)))),
            Math::ATan2(-2.0f * (x * z - w * y), Math::Sqrt((m00 * m00) + (m01 * m01))),
            Math::ATan2(m01, m00)
            );

        return result;
    }

    float Quaternion::GetEulerZ() const
    {
        float m00 = 1.0f - (2.0f * ((y * y) + z * z));
        float m01 = 2.0f * (x * y + w * z);
        return Math::ATan2(m01, m00);
    }

    // returns the spherical interpolated result [t must be between 0..1]
    Quaternion Quaternion::Slerp(const Quaternion& to, float t) const
    {
        float cosom = (x * to.x) + (y * to.y) + (z * to.z) + (w * to.w);
        float scale0, scale1, scale1sign = 1.0f;

        if (cosom < 0.0f)
        {
            scale1sign   = -1.0f;
            cosom       *= -1.0f;
        }

        if ((1.0 - cosom) > Math::epsilon)
        {
            const float omega       = Math::ACos(cosom);
            const float sinOmega    = Math::Sin(omega);
            const float oosinom     = 1.0f / sinOmega;
            scale0      = Math::Sin((1.0f - t) * omega) * oosinom;
            scale1      = Math::Sin(t * omega) * oosinom;
        }
        else
        {
            scale0 = 1.0f - t;
            scale1 = t;
        }

        scale1 *= scale1sign;

        return Quaternion(scale0 * x + scale1 * to.x,
            scale0 * y + scale1 * to.y,
            scale0 * z + scale1 * to.z,
            scale0 * w + scale1 * to.w);
    }


    // set as delta rotation
    Quaternion Quaternion::CreateDeltaRotation(const AZ::Vector3& fromVector, const AZ::Vector3& toVector)
    {
        Quaternion q;
        q.SetAsDeltaRotation(fromVector, toVector);
        return q;
    }


    // set as delta rotation but limited
    Quaternion Quaternion::CreateDeltaRotation(const AZ::Vector3& fromVector, const AZ::Vector3& toVector, float maxAngleRadians)
    {
        Quaternion q;
        q.SetAsDeltaRotation(fromVector, toVector, maxAngleRadians);
        return q;
    }


    // set as delta rotation
    void Quaternion::SetAsDeltaRotation(const AZ::Vector3& fromVector, const AZ::Vector3& toVector)
    {
        // check if we are in parallel or not
        const float dot = fromVector.Dot(toVector);
        if (dot < 0.99999f) // we have rotated compared to the forward direction
        {
            const float angleRadians = Math::ACos(dot);
            const AZ::Vector3 rotAxis = fromVector.Cross(toVector);
            *this = Quaternion(rotAxis, angleRadians);
        }
        else
        {
            Identity();
        }
    }


    // set as delta rotation, but limited
    void Quaternion::SetAsDeltaRotation(const AZ::Vector3& fromVector, const AZ::Vector3& toVector, float maxAngleRadians)
    {
        // check if we are in parallel or not
        const float dot = fromVector.Dot(toVector);
        if (dot < 0.99999f) // we have rotated compared to the forward direction
        {
            const float angleRadians = Math::ACos(dot);
            const float rotAngle = Min(angleRadians, maxAngleRadians);
            const AZ::Vector3 rotAxis = fromVector.Cross(toVector);
            *this = Quaternion(rotAxis, rotAngle);
        }
        else
        {
            Identity();
        }
    }


    /*
       Decompose the rotation on to 2 parts.
       1. Twist - rotation around the "direction" vector
       2. Swing - rotation around axis that is perpendicular to "direction" vector
       The rotation can be composed back by
       rotation = swing * twist

       has singularity in case of swing_rotation close to 180 degrees rotation.
       if the input quaternion is of non-unit length, the outputs are non-unit as well
       otherwise, outputs are both unit
    */
    void Quaternion::DecomposeSwingTwist(const AZ::Vector3& direction, Quaternion* outSwing, Quaternion* outTwist) const
    {
        AZ::Vector3 rotAxis(x, y, z);
        AZ::Vector3 p = Projected(rotAxis, direction); // return projection v1 on to v2 (parallel component)
        outTwist->Set(p.GetX(), p.GetY(), p.GetZ(), w);
        outTwist->Normalize();
        *outSwing = *this * outTwist->Conjugated();
    }


    // rotate the current quaternion and renormalize it
    void Quaternion::RotateFromTo(const AZ::Vector3& fromVector, const AZ::Vector3& toVector)
    {
        *this = CreateDeltaRotation(fromVector, toVector) * *this;
        Normalize();
    }
}   // namespace MCore

