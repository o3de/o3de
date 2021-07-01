/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "Matrix4.h"
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Quaternion.h>
#include <MCore/Source/AzCoreConversions.h>


namespace MCore
{
    // set the matrix to identity
    void Matrix::Identity()
    {
        TMAT(0, 0) = 1.0f;
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = 1.0f;
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = 1.0f;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = 0.0f;
        TMAT(3, 3) = 1.0f;
    }


    // calculate the matrix determinant
    float Matrix::CalcDeterminant() const
    {
        return
            TMAT(0, 0) * TMAT(1, 1) * TMAT(2, 2) +
            TMAT(0, 1) * TMAT(1, 2) * TMAT(2, 0) +
            TMAT(0, 2) * TMAT(1, 0) * TMAT(2, 1) -
            TMAT(0, 2) * TMAT(1, 1) * TMAT(2, 0) -
            TMAT(0, 1) * TMAT(1, 0) * TMAT(2, 2) -
            TMAT(0, 0) * TMAT(1, 2) * TMAT(2, 1);
    }


    // add operator
    Matrix Matrix::operator + (const Matrix& right) const
    {
        Matrix r;
        for (uint32 i = 0; i < 4; ++i)
        {
            MMAT(r, i, 0) = TMAT(i, 0) + MMAT(right, i, 0);
            MMAT(r, i, 1) = TMAT(i, 1) + MMAT(right, i, 1);
            MMAT(r, i, 2) = TMAT(i, 2) + MMAT(right, i, 2);
            MMAT(r, i, 3) = TMAT(i, 3) + MMAT(right, i, 3);
        }
        return r;
    }


    // subtract operator
    Matrix Matrix::operator - (const Matrix& right) const
    {
        Matrix r;
        for (uint32 i = 0; i < 4; ++i)
        {
            MMAT(r, i, 0) = TMAT(i, 0) - MMAT(right, i, 0);
            MMAT(r, i, 1) = TMAT(i, 1) - MMAT(right, i, 1);
            MMAT(r, i, 2) = TMAT(i, 2) - MMAT(right, i, 2);
            MMAT(r, i, 3) = TMAT(i, 3) - MMAT(right, i, 3);
        }
        return r;
    }



    Matrix Matrix::operator * (const Matrix& right) const
    {
        Matrix r;

    #if (AZ_TRAIT_USE_PLATFORM_SIMD_SSE && defined(MCORE_MATRIX_ROWMAJOR))
        const float* m = right.m16;
        const float* n = m16;
        float* t = r.m16;

        __m128 x0;
        __m128 x1;
        __m128 x2;
        __m128 x3;
        __m128 x4;
        __m128 x5;
        __m128 x6;
        __m128 x7;
        x0 = _mm_loadu_ps(&m[0]);
        x1 = _mm_loadu_ps(&m[4]);
        x2 = _mm_loadu_ps(&m[8]);
        x3 = _mm_loadu_ps(&m[12]);
        x4 = _mm_load_ps1(&n[0]);
        x5 = _mm_load_ps1(&n[1]);
        x6 = _mm_load_ps1(&n[2]);
        x7 = _mm_load_ps1(&n[3]);
        x4 = _mm_mul_ps(x4, x0);
        x5 = _mm_mul_ps(x5, x1);
        x6 = _mm_mul_ps(x6, x2);
        x7 = _mm_mul_ps(x7, x3);
        x4 = _mm_add_ps(x4, x5);
        x6 = _mm_add_ps(x6, x7);
        x4 = _mm_add_ps(x4, x6);
        x5 = _mm_load_ps1(&n[4]);
        x6 = _mm_load_ps1(&n[5]);
        x7 = _mm_load_ps1(&n[6]);
        x5 = _mm_mul_ps(x5, x0);
        x6 = _mm_mul_ps(x6, x1);
        x7 = _mm_mul_ps(x7, x2);
        x5 = _mm_add_ps(x5, x6);
        x5 = _mm_add_ps(x5, x7);
        x6 = _mm_load_ps1(&n[7]);
        x6 = _mm_mul_ps(x6, x3);
        x5 = _mm_add_ps(x5, x6);
        x6 = _mm_load_ps1(&n[8]);
        x7 = _mm_load_ps1(&n[9]);
        x6 = _mm_mul_ps(x6, x0);
        x7 = _mm_mul_ps(x7, x1);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[10]);
        x7 = _mm_mul_ps(x7, x2);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[11]);
        x7 = _mm_mul_ps(x7, x3);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[12]);
        x0 = _mm_mul_ps(x0, x7);
        x7 = _mm_load_ps1(&n[13]);
        x1 = _mm_mul_ps(x1, x7);
        x7 = _mm_load_ps1(&n[14]);
        x2 = _mm_mul_ps(x2, x7);
        x7 = _mm_load_ps1(&n[15]);
        x3 = _mm_mul_ps(x3, x7);
        x0 = _mm_add_ps(x0, x1);
        x2 = _mm_add_ps(x2, x3);
        x0 = _mm_add_ps(x0, x2);

        //store result
        _mm_storeu_ps(&t[0], x4);
        _mm_storeu_ps(&t[4], x5);
        _mm_storeu_ps(&t[8], x6);
        _mm_storeu_ps(&t[12], x0);
    #else
        for (uint32 i = 0; i < 4; ++i)
        {
            MMAT(r, i, 0) = TMAT(i, 0) * MMAT(right, 0, 0) + TMAT(i, 1) * MMAT(right, 1, 0) + TMAT(i, 2) * MMAT(right, 2, 0) + TMAT(i, 3) * MMAT(right, 3, 0);
            MMAT(r, i, 1) = TMAT(i, 0) * MMAT(right, 0, 1) + TMAT(i, 1) * MMAT(right, 1, 1) + TMAT(i, 2) * MMAT(right, 2, 1) + TMAT(i, 3) * MMAT(right, 3, 1);
            MMAT(r, i, 2) = TMAT(i, 0) * MMAT(right, 0, 2) + TMAT(i, 1) * MMAT(right, 1, 2) + TMAT(i, 2) * MMAT(right, 2, 2) + TMAT(i, 3) * MMAT(right, 3, 2);
            MMAT(r, i, 3) = TMAT(i, 0) * MMAT(right, 0, 3) + TMAT(i, 1) * MMAT(right, 1, 3) + TMAT(i, 2) * MMAT(right, 2, 3) + TMAT(i, 3) * MMAT(right, 3, 3);
        }
    #endif

        return r;
    }



    Matrix& Matrix::operator += (const Matrix& right)
    {
        for (uint32 i = 0; i < 4; ++i)
        {
            TMAT(i, 0) += MMAT(right, i, 0);
            TMAT(i, 1) += MMAT(right, i, 1);
            TMAT(i, 2) += MMAT(right, i, 2);
            TMAT(i, 3) += MMAT(right, i, 3);
        }
        return *this;
    }


    Matrix Matrix::operator * (float value) const
    {
        Matrix result(*this);
        for (uint32 i = 0; i < 4; ++i)
        {
            MMAT(result, i, 0) *= value;
            MMAT(result, i, 1) *= value;
            MMAT(result, i, 2) *= value;
            MMAT(result, i, 3) *= value;
        }
        return result;
    }


    Matrix& Matrix::operator -= (const Matrix& right)
    {
        for (uint32 i = 0; i < 4; ++i)
        {
            TMAT(i, 0) -= MMAT(right, i, 0);
            TMAT(i, 1) -= MMAT(right, i, 1);
            TMAT(i, 2) -= MMAT(right, i, 2);
            TMAT(i, 3) -= MMAT(right, i, 3);
        }
        return *this;
    }



    Matrix& Matrix::operator *= (const Matrix& right)
    {
    #if (AZ_TRAIT_USE_PLATFORM_SIMD_SSE && defined(MCORE_MATRIX_ROWMAJOR))
        const float* m = right.m16;
        const float* n = m16;
        float* t = this->m16;

        __m128 x0;
        __m128 x1;
        __m128 x2;
        __m128 x3;
        __m128 x4;
        __m128 x5;
        __m128 x6;
        __m128 x7;
        x0 = _mm_loadu_ps(&m[0]);
        x1 = _mm_loadu_ps(&m[4]);
        x2 = _mm_loadu_ps(&m[8]);
        x3 = _mm_loadu_ps(&m[12]);
        x4 = _mm_load_ps1(&n[0]);
        x5 = _mm_load_ps1(&n[1]);
        x6 = _mm_load_ps1(&n[2]);
        x7 = _mm_load_ps1(&n[3]);
        x4 = _mm_mul_ps(x4, x0);
        x5 = _mm_mul_ps(x5, x1);
        x6 = _mm_mul_ps(x6, x2);
        x7 = _mm_mul_ps(x7, x3);
        x4 = _mm_add_ps(x4, x5);
        x6 = _mm_add_ps(x6, x7);
        x4 = _mm_add_ps(x4, x6);
        x5 = _mm_load_ps1(&n[4]);
        x6 = _mm_load_ps1(&n[5]);
        x7 = _mm_load_ps1(&n[6]);
        x5 = _mm_mul_ps(x5, x0);
        x6 = _mm_mul_ps(x6, x1);
        x7 = _mm_mul_ps(x7, x2);
        x5 = _mm_add_ps(x5, x6);
        x5 = _mm_add_ps(x5, x7);
        x6 = _mm_load_ps1(&n[7]);
        x6 = _mm_mul_ps(x6, x3);
        x5 = _mm_add_ps(x5, x6);
        x6 = _mm_load_ps1(&n[8]);
        x7 = _mm_load_ps1(&n[9]);
        x6 = _mm_mul_ps(x6, x0);
        x7 = _mm_mul_ps(x7, x1);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[10]);
        x7 = _mm_mul_ps(x7, x2);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[11]);
        x7 = _mm_mul_ps(x7, x3);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[12]);
        x0 = _mm_mul_ps(x0, x7);
        x7 = _mm_load_ps1(&n[13]);
        x1 = _mm_mul_ps(x1, x7);
        x7 = _mm_load_ps1(&n[14]);
        x2 = _mm_mul_ps(x2, x7);
        x7 = _mm_load_ps1(&n[15]);
        x3 = _mm_mul_ps(x3, x7);
        x0 = _mm_add_ps(x0, x1);
        x2 = _mm_add_ps(x2, x3);
        x0 = _mm_add_ps(x0, x2);

        //store result
        _mm_storeu_ps(&t[0], x4);
        _mm_storeu_ps(&t[4], x5);
        _mm_storeu_ps(&t[8], x6);
        _mm_storeu_ps(&t[12], x0);
    #else
        float v[4];
        for (uint32 i = 0; i < 4; ++i)
        {
            v[0] = TMAT(i, 0);
            v[1] = TMAT(i, 1);
            v[2] = TMAT(i, 2);
            v[3] = TMAT(i, 3);
            TMAT(i, 0) = v[0] * MMAT(right, 0, 0) + v[1] * MMAT(right, 1, 0) + v[2] * MMAT(right, 2, 0) + v[3] * MMAT(right, 3, 0);
            TMAT(i, 1) = v[0] * MMAT(right, 0, 1) + v[1] * MMAT(right, 1, 1) + v[2] * MMAT(right, 2, 1) + v[3] * MMAT(right, 3, 1);
            TMAT(i, 2) = v[0] * MMAT(right, 0, 2) + v[1] * MMAT(right, 1, 2) + v[2] * MMAT(right, 2, 2) + v[3] * MMAT(right, 3, 2);
            TMAT(i, 3) = v[0] * MMAT(right, 0, 3) + v[1] * MMAT(right, 1, 3) + v[2] * MMAT(right, 2, 3) + v[3] * MMAT(right, 3, 3);
        }
    #endif

        return *this;
    }



    // calculate euler angles
    AZ::Vector3 Matrix::CalcEulerAngles() const
    {
        AZ::Vector3 v;
        /*
        // Version with smooth transitions to poles, but slow
            float SignCosY = 1;
            float NearPole = Math::Pow(Math::Abs(m44[2][0]), 12);
            v.y = Math::ASin( -m44[2][0] );
            v.z = Math::ATan( m44[1][0] / m44[0][0] );
            if( Math::Abs(v.y) < Math::Abs(v.z) )
            {
                SignCosY = Math::SignOfCos(v.y);
                v.z = Math::ATan2( SignCosY * m44[1][0], SignCosY * m44[0][0] );
            }
            else
            {
                v.y = Math::ATan2( -m44[2][0], Math::Sqrt( m44[0][0]*m44[0][0] + m44[1][0]*m44[1][0] )
                    * Math::SignOfFloat(Math::SignOfCos(v.z) * m44[0][0]));
                SignCosY = Math::SignOfCos(v.y);
            }
            v.x = (1 - NearPole) * Math::ATan2( SignCosY * m44[2][1], SignCosY * m44[2][2] )
                + NearPole * 0.5 * Math::ATan2(-m44[1][2], m44[1][1]);
            v.y = (1 - NearPole) * v.y + NearPole * Math::ATan2(-m44[2][0], m44[0][0]);
            v.z = (1 - NearPole) * v.z - NearPole * Math::SignOfSin(v.y) * 0.5 * Math::ATan2(-m44[1][2], m44[1][1]);
        */

        if (Math::Abs(TMAT(2, 0)) < 0.9f)
        {
            float SignCosY = 1.0f;
            v.SetY(Math::ASin(-TMAT(2, 0)));
            v.SetZ(Math::ATan(TMAT(1, 0) / TMAT(0, 0)));
            if (Math::Abs(v.GetY()) < Math::Abs(v.GetZ()))
            {
                SignCosY = Math::SignOfCos(v.GetY());
                v.SetZ(Math::ATan2(SignCosY * TMAT(1, 0), SignCosY * TMAT(0, 0)));
            }
            else
            {
                v.SetY(Math::ATan2(-TMAT(2, 0), Math::Sqrt(TMAT(0, 0) * TMAT(0, 0) + TMAT(1, 0) * TMAT(1, 0)) * Math::SignOfFloat(Math::SignOfCos(v.GetZ()) * TMAT(0, 0))));
                SignCosY = Math::SignOfCos(v.GetY());
            }
            v.SetX(Math::ATan2(SignCosY * m44[2][1], SignCosY * TMAT(2, 2)));
        }
        else
        {
            v.SetZ(0.5f * Math::ATan2(-TMAT(1, 2), TMAT(1, 1)));
            v.SetY(Math::ATan2(-TMAT(2, 0), TMAT(0, 0)));
            v.SetX(-Math::SignOfSin(v.GetY()) * v.GetZ());
        }

        v.SetY(-v.GetY());
        v.SetZ(-v.GetZ());

        // get the angles in the range [-pi, pi]
        v.SetX(v.GetX() + Math::twoPi * Math::Floor((-v.GetX()) / Math::twoPi + 0.5f));
        v.SetY(v.GetY() + Math::twoPi * Math::Floor((-v.GetY()) / Math::twoPi + 0.5f));
        v.SetZ(v.GetZ() + Math::twoPi * Math::Floor((-v.GetZ()) / Math::twoPi + 0.5f));

        return v;
    }




    /*
    void Matrix::SetRotationMatrixEulerXYZ(const Vector3& v)
    {
        const float sy = Math::Sin(v.x);
        const float cy = Math::Cos(v.x);
        const float sp = Math::Sin(v.y);
        const float cp = Math::Cos(v.y);
        const float sr = Math::Sin(v.z);
        const float cr = Math::Cos(v.z);
        const float spsy = sp * sy;
        const float spcy = sp * cy;

        m44[0][0] = cr * cp;
        m44[0][1] = sr * cp;
        m44[0][2] = -sp;
        m44[0][3] = 0;
        m44[1][0] = cr * spsy - sr * cy;
        m44[1][1] = sr * spsy + cr * cy;
        m44[1][2] = cp * sy;
        m44[1][3] = 0;
        m44[2][0] = cr * spcy + sr * sy;
        m44[2][1] = sr * spcy - cr * sy;
        m44[2][2] = cp * cy;
        m44[2][3] = 0;
        m44[3][0] = 0;
        m44[3][1] = 0;
        m44[3][2] = 0;
        m44[3][3] = 1;
    }
    */


    // setup as scale matrix
    void Matrix::SetScaleMatrix(const AZ::Vector3& s)
    {
        TMAT(0, 0) = s.GetX();
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = s.GetY();
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = s.GetZ();
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = 0.0f;
        TMAT(3, 3) = 1.0f;
    }


    // setup as translation matrix
    void Matrix::SetTranslationMatrix(const AZ::Vector3& t)
    {
        TMAT(0, 0) = 1.0f;
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = 1.0f;
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = 1.0f;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = t.GetX();
        TMAT(3, 1) = t.GetY();
        TMAT(3, 2) = t.GetZ();
        TMAT(3, 3) = 1.0f;
    }


    // setup as rotation matrix
    void Matrix::SetRotationMatrixX(float angle)
    {
        const float s = Math::Sin(angle);
        const float c = Math::Cos(angle);

        TMAT(0, 0) = 1.0f;
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = c;
        TMAT(1, 2) = s;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = -s;
        TMAT(2, 2) = c;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = 0.0f;
        TMAT(3, 3) = 1.0f;
    }


    // setup as rotation matrix
    void Matrix::SetRotationMatrixY(float angle)
    {
        const float s = Math::Sin(angle);
        const float c = Math::Cos(angle);

        TMAT(0, 0) = c;
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = -s;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = 1.0f;
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = s;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = c;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = 0.0f;
        TMAT(3, 3) = 1.0f;
    }


    // setup as rotation matrix
    void Matrix::SetRotationMatrixZ(float angle)
    {
        const float s = Math::Sin(angle);
        const float c = Math::Cos(angle);

        TMAT(0, 0) = c;
        TMAT(0, 1) = s;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = -s;
        TMAT(1, 1) = c;
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = 1.0f;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = 0.0f;
        TMAT(3, 3) = 1.0f;
    }


    void Matrix::SetRotationMatrixEulerZYX(const AZ::Vector3& v)
    {
        *this = Matrix::RotationMatrixX(v.GetZ());
        this->MultMatrix4x3(Matrix::RotationMatrixY(v.GetY()));
        this->MultMatrix4x3(Matrix::RotationMatrixZ(v.GetX()));
    }



    void Matrix::SetRotationMatrixEulerXYZ(const AZ::Vector3& v)
    {
        *this = Matrix::RotationMatrixX(v.GetX());
        this->MultMatrix4x3(Matrix::RotationMatrixY(v.GetY()));
        this->MultMatrix4x3(Matrix::RotationMatrixZ(v.GetZ()));
    }



    void Matrix::SetRotationMatrixAxisAngle(const AZ::Vector3& axis, float angle)
    {
        const float length2 = axis.GetLengthSq();
        if (Math::Abs(length2) < 0.00001f)
        {
            Identity();
            return;
        }

        const AZ::Vector3 n = axis / Math::Sqrt(length2);
        const float s = Math::Sin(angle);
        const float c = Math::Cos(angle);
        const float k = 1.0f - c;
        const float xx = n.GetX() * n.GetX() * k + c;
        const float yy = n.GetY() * n.GetY() * k + c;
        const float zz = n.GetZ() * n.GetZ() * k + c;
        const float xy = n.GetX() * n.GetY() * k;
        const float yz = n.GetY() * n.GetZ() * k;
        const float zx = n.GetZ() * n.GetX() * k;
        const float xs = n.GetX() * s;
        const float ys = n.GetY() * s;
        const float zs = n.GetZ() * s;

        TMAT(0, 0) = xx;
        TMAT(0, 1) = xy + zs;
        TMAT(0, 2) = zx - ys;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = xy - zs;
        TMAT(1, 1) = yy;
        TMAT(1, 2) = yz + xs;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = zx + ys;
        TMAT(2, 1) = yz - xs;
        TMAT(2, 2) = zz;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = 0.0f;
        TMAT(3, 3) = 1.0f;
    }


    void Matrix::Scale3x3(const AZ::Vector3& scale)
    {
        TMAT(0, 0) *= scale.GetX();
        TMAT(0, 1) *= scale.GetY();
        TMAT(0, 2) *= scale.GetZ();

        TMAT(1, 0) *= scale.GetX();
        TMAT(1, 1) *= scale.GetY();
        TMAT(1, 2) *= scale.GetZ();

        TMAT(2, 0) *= scale.GetX();
        TMAT(2, 1) *= scale.GetY();
        TMAT(2, 2) *= scale.GetZ();
    }


    AZ::Vector3 Matrix::ExtractScale()
    {
        const AZ::Vector4 x = GetRow4D(0);
        const AZ::Vector4 y = GetRow4D(1);
        const AZ::Vector4 z = GetRow4D(2);
        const float lengthX = x.GetLength();
        const float lengthY = y.GetLength();
        const float lengthZ = z.GetLength();
        SetRow(0, x / lengthX);
        SetRow(1, y / lengthY);
        SetRow(2, z / lengthZ);

        return AZ::Vector3(lengthX, lengthY, lengthZ);
    }

    void Matrix::RotateX(float angle)
    {
        const float s = Math::Sin(angle);
        const float c = Math::Cos(angle);

        for (uint32 i = 0; i < 3; ++i)
        {
            const float x = TMAT(2, i);
            const float z = TMAT(1, i);
            TMAT(2, i) = x * c - z * s;
            TMAT(1, i) = x * s + z * c;
        }
    }



    void Matrix::RotateY(float angle)
    {
        const float s = Math::Sin(angle);
        const float c = Math::Cos(angle);

        for (uint32 i = 0; i < 3; ++i)
        {
            const float x = TMAT(0, i);
            const float z = TMAT(2, i);
            TMAT(0, i) = x * c - z * s;
            TMAT(2, i) = x * s + z * c;
        }
    }



    void Matrix::RotateZ(float angle)
    {
        const float s = Math::Sin(angle);
        const float c = Math::Cos(angle);

        for (uint32 i = 0; i < 3; ++i)
        {
            const float x = TMAT(1, i);
            const float z = TMAT(0, i);
            TMAT(1, i) = x * c - z * s;
            TMAT(0, i) = x * s + z * c;
        }
    }

    /*
    void Matrix::RotateXYZ(const float yaw, const float pitch, const float roll)
    {
        const float sy = Math::Sin(yaw);
        const float cy = Math::Cos(yaw);
        const float sp = Math::Sin(pitch);
        const float cp = Math::Cos(pitch);
        const float sr = Math::Sin(roll);
        const float cr = Math::Cos(roll);
        const float spsy = sp * sy;
        const float spcy = sp * cy;
        const float m00 = cr * cp;
        const float m01 = sr * cp;
        const float m02 = -sp;
        const float m10 = cr * spsy - sr * cy;
        const float m11 = sr * spsy + cr * cy;
        const float m12 = cp * sy;
        const float m20 = cr * spcy + sr * sy;
        const float m21 = sr * spcy - cr * sy;
        const float m22 = cp * cy;

        for ( int32 i=0; i<4; i++ )
        {
            const float x = m44[i][0];
            const float y = m44[i][1];
            const float z = m44[i][2];
            m44[i][0] = x * m00 + y * m10 + z * m20;
            m44[i][1] = x * m01 + y * m11 + z * m21;
            m44[i][2] = x * m02 + y * m12 + z * m22;
        }
    }
    */


    void Matrix::MultMatrix(const Matrix& right)
    {
    #if (AZ_TRAIT_USE_PLATFORM_SIMD_SSE && defined(MCORE_MATRIX_ROWMAJOR))
        const float* m = right.m16;
        const float* n = m16;
        float* t = this->m16;

        __m128 x0;
        __m128 x1;
        __m128 x2;
        __m128 x3;
        __m128 x4;
        __m128 x5;
        __m128 x6;
        __m128 x7;
        x0 = _mm_loadu_ps(&m[0]);
        x1 = _mm_loadu_ps(&m[4]);
        x2 = _mm_loadu_ps(&m[8]);
        x3 = _mm_loadu_ps(&m[12]);
        x4 = _mm_load_ps1(&n[0]);
        x5 = _mm_load_ps1(&n[1]);
        x6 = _mm_load_ps1(&n[2]);
        x7 = _mm_load_ps1(&n[3]);
        x4 = _mm_mul_ps(x4, x0);
        x5 = _mm_mul_ps(x5, x1);
        x6 = _mm_mul_ps(x6, x2);
        x7 = _mm_mul_ps(x7, x3);
        x4 = _mm_add_ps(x4, x5);
        x6 = _mm_add_ps(x6, x7);
        x4 = _mm_add_ps(x4, x6);
        x5 = _mm_load_ps1(&n[4]);
        x6 = _mm_load_ps1(&n[5]);
        x7 = _mm_load_ps1(&n[6]);
        x5 = _mm_mul_ps(x5, x0);
        x6 = _mm_mul_ps(x6, x1);
        x7 = _mm_mul_ps(x7, x2);
        x5 = _mm_add_ps(x5, x6);
        x5 = _mm_add_ps(x5, x7);
        x6 = _mm_load_ps1(&n[7]);
        x6 = _mm_mul_ps(x6, x3);
        x5 = _mm_add_ps(x5, x6);
        x6 = _mm_load_ps1(&n[8]);
        x7 = _mm_load_ps1(&n[9]);
        x6 = _mm_mul_ps(x6, x0);
        x7 = _mm_mul_ps(x7, x1);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[10]);
        x7 = _mm_mul_ps(x7, x2);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[11]);
        x7 = _mm_mul_ps(x7, x3);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[12]);
        x0 = _mm_mul_ps(x0, x7);
        x7 = _mm_load_ps1(&n[13]);
        x1 = _mm_mul_ps(x1, x7);
        x7 = _mm_load_ps1(&n[14]);
        x2 = _mm_mul_ps(x2, x7);
        x7 = _mm_load_ps1(&n[15]);
        x3 = _mm_mul_ps(x3, x7);
        x0 = _mm_add_ps(x0, x1);
        x2 = _mm_add_ps(x2, x3);
        x0 = _mm_add_ps(x0, x2);

        //store result
        _mm_storeu_ps(&t[0], x4);
        _mm_storeu_ps(&t[4], x5);
        _mm_storeu_ps(&t[8], x6);
        _mm_storeu_ps(&t[12], x0);
    #else
        float v[4];
        for (uint32 i = 0; i < 4; ++i)
        {
            v[0] = TMAT(i, 0);
            v[1] = TMAT(i, 1);
            v[2] = TMAT(i, 2);
            v[3] = TMAT(i, 3);
            TMAT(i, 0) = v[0] * MMAT(right, 0, 0) + v[1] * MMAT(right, 1, 0) + v[2] * MMAT(right, 2, 0) + v[3] * MMAT(right, 3, 0);
            TMAT(i, 1) = v[0] * MMAT(right, 0, 1) + v[1] * MMAT(right, 1, 1) + v[2] * MMAT(right, 2, 1) + v[3] * MMAT(right, 3, 1);
            TMAT(i, 2) = v[0] * MMAT(right, 0, 2) + v[1] * MMAT(right, 1, 2) + v[2] * MMAT(right, 2, 2) + v[3] * MMAT(right, 3, 2);
            TMAT(i, 3) = v[0] * MMAT(right, 0, 3) + v[1] * MMAT(right, 1, 3) + v[2] * MMAT(right, 2, 3) + v[3] * MMAT(right, 3, 3);
        }
    #endif
    }


    // init from position, rotation, scale and shear
    // use this to reconstruct a matrix that has been decomposed using the DecomposeQRGramSchmidt method
    void Matrix::InitFromPosRotScaleShear(const AZ::Vector3& pos, const AZ::Quaternion& rot, const AZ::Vector3& scale, const AZ::Vector3& shear)
    {
        // convert quat to matrix
        const float xx = rot.GetX() * rot.GetX();
        const float xy = rot.GetX() * rot.GetY(), yy = rot.GetY() * rot.GetY();
        const float xz = rot.GetX() * rot.GetZ(), yz = rot.GetY() * rot.GetZ(), zz = rot.GetZ() * rot.GetZ();
        const float xw = rot.GetX() * rot.GetW(), yw = rot.GetY() * rot.GetW(), zw = rot.GetZ() * rot.GetW(), ww = rot.GetW() * rot.GetW();
        TMAT(0, 0) = +xx - yy - zz + ww;
        TMAT(0, 1) = +xy + zw + xy + zw;
        TMAT(0, 2) = +xz - yw + xz - yw;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = +xy - zw + xy - zw;
        TMAT(1, 1) = -xx + yy - zz + ww;
        TMAT(1, 2) = +yz + xw + yz + xw;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = +xz + yw + xz + yw;
        TMAT(2, 1) = +yz - xw + yz - xw;
        TMAT(2, 2) = -xx - yy + zz + ww;
        TMAT(2, 3) = 0.0f;

        // scale
        TMAT(0, 0) *= scale.GetX();
        TMAT(0, 1) *= scale.GetY();
        TMAT(0, 2) *= scale.GetZ();
        TMAT(1, 0) *= scale.GetX();
        TMAT(1, 1) *= scale.GetY();
        TMAT(1, 2) *= scale.GetZ();
        TMAT(2, 0) *= scale.GetX();
        TMAT(2, 1) *= scale.GetY();
        TMAT(2, 2) *= scale.GetZ();

        // multiply with the shear matrix
        float v[3];
        v[0] = TMAT(0, 0);
        v[1] = TMAT(0, 1);
        v[2] = TMAT(0, 2);
        TMAT(0, 1) = v[0] * shear.GetX() + v[1];
        TMAT(0, 2) = v[0] * shear.GetY() + v[1] * shear.GetZ() + v[2];

        v[0] = TMAT(1, 0);
        v[1] = TMAT(1, 1);
        v[2] = TMAT(1, 2);
        TMAT(1, 1) = v[0] * shear.GetX() + v[1];
        TMAT(1, 2) = v[0] * shear.GetY() + v[1] * shear.GetZ() + v[2];

        v[0] = TMAT(2, 0);
        v[1] = TMAT(2, 1);
        v[2] = TMAT(2, 2);
        TMAT(2, 1) = v[0] * shear.GetX() + v[1];
        TMAT(2, 2) = v[0] * shear.GetY() + v[1] * shear.GetZ() + v[2];

        // translation
        TMAT(3, 0) = pos.GetX();
        TMAT(3, 1) = pos.GetY();
        TMAT(3, 2) = pos.GetZ();
        TMAT(3, 3) = 1.0f;
    }


    // init from position, rotation, scale, scale rotation
    // use this to reconstruct a matrix decomposed using the MatrixDecomposer class using polar decomposition
    void Matrix::InitFromPosRotScaleScaleRot(const AZ::Vector3& pos, const AZ::Quaternion& rot, const AZ::Vector3& scale, const AZ::Quaternion& scaleRot)
    {
        float xx = scaleRot.GetX() * scaleRot.GetX();
        float xy = scaleRot.GetX() * scaleRot.GetY(), yy = scaleRot.GetY() * scaleRot.GetY();
        float xz = scaleRot.GetX() * scaleRot.GetZ(), yz = scaleRot.GetY() * scaleRot.GetZ(), zz = scaleRot.GetZ() * scaleRot.GetZ();
        float xw = scaleRot.GetX() * scaleRot.GetW(), yw = scaleRot.GetY() * scaleRot.GetW(), zw = scaleRot.GetZ() * scaleRot.GetW(), ww = scaleRot.GetW() * scaleRot.GetW();

        // init on the inversed scale rotation
        TMAT(0, 0) = +xx - yy - zz + ww;
        TMAT(1, 0) = +xy + zw + xy + zw;
        TMAT(2, 0) = +xz - yw + xz - yw;                                                // translation part not initialized
        TMAT(0, 1) = +xy - zw + xy - zw;
        TMAT(1, 1) = -xx + yy - zz + ww;
        TMAT(2, 1) = +yz + xw + yz + xw;
        TMAT(0, 2) = +xz + yw + xz + yw;
        TMAT(1, 2) = +yz - xw + yz - xw;
        TMAT(2, 2) = -xx - yy + zz + ww;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 3) = 1.0f;

        // copy the 3x3 part into a temp buffer, so that we have the inverse scale rotation, before scaling applied to it
        float r33[3][3];
        uint32 i;
        for (i = 0; i < 3; ++i)
        {
#ifdef MCORE_MATRIX_ROWMAJOR
            r33[i][0] = TMAT(i, 0);
            r33[i][1] = TMAT(i, 1);
            r33[i][2] = TMAT(i, 2);
#else
            r33[0][i] = TMAT(i, 0);
            r33[1][i] = TMAT(i, 1);
            r33[2][i] = TMAT(i, 2);
#endif
        }

        // apply scaling
        TMAT(0, 0) *= scale.GetX();
        TMAT(0, 1) *= scale.GetY();
        TMAT(0, 2) *= scale.GetZ();
        TMAT(1, 0) *= scale.GetX();
        TMAT(1, 1) *= scale.GetY();
        TMAT(1, 2) *= scale.GetZ();
        TMAT(2, 0) *= scale.GetX();
        TMAT(2, 1) *= scale.GetY();
        TMAT(2, 2) *= scale.GetZ();

        // undo the scale rotation
        float v[3];
        for (i = 0; i < 3; ++i)
        {
            v[0] = TMAT(i, 0);
            v[1] = TMAT(i, 1);
            v[2] = TMAT(i, 2);

#ifdef MCORE_MATRIX_ROWMAJOR
            TMAT(i, 0) = v[0] * r33[0][0] + v[1] * r33[0][1] + v[2] * r33[0][2]; // transposed multiply
            TMAT(i, 1) = v[0] * r33[1][0] + v[1] * r33[1][1] + v[2] * r33[1][2];
            TMAT(i, 2) = v[0] * r33[2][0] + v[1] * r33[2][1] + v[2] * r33[2][2];
#else
            TMAT(i, 0) = v[0] * r33[0][0] + v[1] * r33[1][0] + v[2] * r33[2][0]; // transposed multiply
            TMAT(i, 1) = v[0] * r33[0][1] + v[1] * r33[1][1] + v[2] * r33[2][1];
            TMAT(i, 2) = v[0] * r33[0][2] + v[1] * r33[1][2] + v[2] * r33[2][2];
#endif
        }

        // apply regular rotation
        xx = rot.GetX() * rot.GetX();
        xy = rot.GetX() * rot.GetY();
        yy = rot.GetY() * rot.GetY();
        xz = rot.GetX() * rot.GetZ();
        yz = rot.GetY() * rot.GetZ();
        zz = rot.GetZ() * rot.GetZ();
        xw = rot.GetX() * rot.GetW();
        yw = rot.GetY() * rot.GetW();
        zw = rot.GetZ() * rot.GetW();
        ww = rot.GetW() * rot.GetW();

#ifdef MCORE_MATRIX_ROWMAJOR
        r33[0][0] = +xx - yy - zz + ww;
        r33[0][1] = +xy + zw + xy + zw;
        r33[0][2] = +xz - yw + xz - yw;
        r33[1][0] = +xy - zw + xy - zw;
        r33[1][1] = -xx + yy - zz + ww;
        r33[1][2] = +yz + xw + yz + xw;
        r33[2][0] = +xz + yw + xz + yw;
        r33[2][1] = +yz - xw + yz - xw;
        r33[2][2] = -xx - yy + zz + ww;
#else
        r33[0][0] = +xx - yy - zz + ww;
        r33[1][0] = +xy + zw + xy + zw;
        r33[2][0] = +xz - yw + xz - yw;
        r33[0][1] = +xy - zw + xy - zw;
        r33[1][1] = -xx + yy - zz + ww;
        r33[2][1] = +yz + xw + yz + xw;
        r33[0][2] = +xz + yw + xz + yw;
        r33[1][2] = +yz - xw + yz - xw;
        r33[2][2] = -xx - yy + zz + ww;
#endif

        // mult 3x3 matrix
        for (i = 0; i < 3; ++i)
        {
            v[0] = TMAT(i, 0);
            v[1] = TMAT(i, 1);
            v[2] = TMAT(i, 2);

#ifdef MCORE_MATRIX_ROWMAJOR
            TMAT(i, 0) = v[0] * r33[0][0] + v[1] * r33[1][0] + v[2] * r33[2][0];
            TMAT(i, 1) = v[0] * r33[0][1] + v[1] * r33[1][1] + v[2] * r33[2][1];
            TMAT(i, 2) = v[0] * r33[0][2] + v[1] * r33[1][2] + v[2] * r33[2][2];
#else
            TMAT(i, 0) = v[0] * r33[0][0] + v[1] * r33[0][1] + v[2] * r33[0][2];
            TMAT(i, 1) = v[0] * r33[1][0] + v[1] * r33[1][1] + v[2] * r33[1][2];
            TMAT(i, 2) = v[0] * r33[2][0] + v[1] * r33[2][1] + v[2] * r33[2][2];
#endif
        }

        // apply translation
        TMAT(3, 0) = pos.GetX();
        TMAT(3, 1) = pos.GetY();
        TMAT(3, 2) = pos.GetZ();
    }


    // init from pos/rot/scale
    void Matrix::InitFromPosRotScale(const AZ::Vector3& pos, const AZ::Quaternion& rot, const AZ::Vector3& scale)
    {
        // init on a scale + translation matrix
        TMAT(0, 0) = scale.GetX();
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = scale.GetY();
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = scale.GetZ();
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = pos.GetX();
        TMAT(3, 1) = pos.GetY();
        TMAT(3, 2) = pos.GetZ();
        TMAT(3, 3) = 1.0f;

        // multiply it with a rotation matrix built from the AZ::Quaternion
        // don't rotate the translation part
        const float xx = rot.GetX() * rot.GetX();
        const float xy = rot.GetX() * rot.GetY(), yy = rot.GetY() * rot.GetY();
        const float xz = rot.GetX() * rot.GetZ(), yz = rot.GetY() * rot.GetZ(), zz = rot.GetZ() * rot.GetZ();
        const float xw = rot.GetX() * rot.GetW(), yw = rot.GetY() * rot.GetW(), zw = rot.GetZ() * rot.GetW(), ww = rot.GetW() * rot.GetW();

        float r33[3][3];
    #ifdef MCORE_MATRIX_ROWMAJOR
        r33[0][0] = +xx - yy - zz + ww;
        r33[0][1] = +xy + zw + xy + zw;
        r33[0][2] = +xz - yw + xz - yw;
        r33[1][0] = +xy - zw + xy - zw;
        r33[1][1] = -xx + yy - zz + ww;
        r33[1][2] = +yz + xw + yz + xw;
        r33[2][0] = +xz + yw + xz + yw;
        r33[2][1] = +yz - xw + yz - xw;
        r33[2][2] = -xx - yy + zz + ww;
    #else
        r33[0][0] = +xx - yy - zz + ww;
        r33[1][0] = +xy + zw + xy + zw;
        r33[2][0] = +xz - yw + xz - yw;
        r33[0][1] = +xy - zw + xy - zw;
        r33[1][1] = -xx + yy - zz + ww;
        r33[2][1] = +yz + xw + yz + xw;
        r33[0][2] = +xz + yw + xz + yw;
        r33[1][2] = +yz - xw + yz - xw;
        r33[2][2] = -xx - yy + zz + ww;
    #endif

        // perform the matrix mul
        float v[3];
        for (uint32 i = 0; i < 3; ++i)
        {
            v[0] = TMAT(i, 0);
            v[1] = TMAT(i, 1);
            v[2] = TMAT(i, 2);

        #ifdef MCORE_MATRIX_ROWMAJOR
            TMAT(i, 0) = v[0] * r33[0][0] + v[1] * r33[1][0] + v[2] * r33[2][0];
            TMAT(i, 1) = v[0] * r33[0][1] + v[1] * r33[1][1] + v[2] * r33[2][1];
            TMAT(i, 2) = v[0] * r33[0][2] + v[1] * r33[1][2] + v[2] * r33[2][2];
        #else
            TMAT(i, 0) = v[0] * r33[0][0] + v[1] * r33[0][1] + v[2] * r33[0][2];
            TMAT(i, 1) = v[0] * r33[1][0] + v[1] * r33[1][1] + v[2] * r33[1][2];
            TMAT(i, 2) = v[0] * r33[2][0] + v[1] * r33[2][1] + v[2] * r33[2][2];
        #endif
        }
    }


    // init from pos/rot/scale with parent scale compensation
    void Matrix::InitFromNoScaleInherit(const AZ::Vector3& pos, const AZ::Quaternion& rot, const AZ::Vector3& scale, const AZ::Vector3& invParentScale)
    {
        // init on a scale + translation matrix
        TMAT(0, 0) = scale.GetX();
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = scale.GetY();
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = scale.GetZ();
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = pos.GetX();
        TMAT(3, 1) = pos.GetY();
        TMAT(3, 2) = pos.GetZ();
        TMAT(3, 3) = 1.0f;

        // multiply it with a rotation matrix built from the AZ::Quaternion
        // don't rotate the translation part
        const float xx = rot.GetX() * rot.GetX();
        const float xy = rot.GetX() * rot.GetY(), yy = rot.GetY() * rot.GetY();
        const float xz = rot.GetX() * rot.GetZ(), yz = rot.GetY() * rot.GetZ(), zz = rot.GetZ() * rot.GetZ();
        const float xw = rot.GetX() * rot.GetW(), yw = rot.GetY() * rot.GetW(), zw = rot.GetZ() * rot.GetW(), ww = rot.GetW() * rot.GetW();

        float r33[3][3];
    #ifdef MCORE_MATRIX_ROWMAJOR
        r33[0][0] = +xx - yy - zz + ww;
        r33[0][1] = +xy + zw + xy + zw;
        r33[0][2] = +xz - yw + xz - yw;
        r33[1][0] = +xy - zw + xy - zw;
        r33[1][1] = -xx + yy - zz + ww;
        r33[1][2] = +yz + xw + yz + xw;
        r33[2][0] = +xz + yw + xz + yw;
        r33[2][1] = +yz - xw + yz - xw;
        r33[2][2] = -xx - yy + zz + ww;
    #else
        r33[0][0] = +xx - yy - zz + ww;
        r33[1][0] = +xy + zw + xy + zw;
        r33[2][0] = +xz - yw + xz - yw;
        r33[0][1] = +xy - zw + xy - zw;
        r33[1][1] = -xx + yy - zz + ww;
        r33[2][1] = +yz + xw + yz + xw;
        r33[0][2] = +xz + yw + xz + yw;
        r33[1][2] = +yz - xw + yz - xw;
        r33[2][2] = -xx - yy + zz + ww;
    #endif

        // perform the matrix mul
        float v[3];
        for (uint32 i = 0; i < 3; ++i)
        {
            v[0] = TMAT(i, 0);
            v[1] = TMAT(i, 1);
            v[2] = TMAT(i, 2);

        #ifdef MCORE_MATRIX_ROWMAJOR
            TMAT(i, 0) = v[0] * r33[0][0] + v[1] * r33[1][0] + v[2] * r33[2][0];
            TMAT(i, 1) = v[0] * r33[0][1] + v[1] * r33[1][1] + v[2] * r33[2][1];
            TMAT(i, 2) = v[0] * r33[0][2] + v[1] * r33[1][2] + v[2] * r33[2][2];
        #else
            TMAT(i, 0) = v[0] * r33[0][0] + v[1] * r33[0][1] + v[2] * r33[0][2];
            TMAT(i, 1) = v[0] * r33[1][0] + v[1] * r33[1][1] + v[2] * r33[1][2];
            TMAT(i, 2) = v[0] * r33[2][0] + v[1] * r33[2][1] + v[2] * r33[2][2];
        #endif
        }

        // multiply this with the 3x3 scale inverse parent scale matrix
        TMAT(0, 0) *= invParentScale.GetX();
        TMAT(0, 1) *= invParentScale.GetY();
        TMAT(0, 2) *= invParentScale.GetZ();
        TMAT(1, 0) *= invParentScale.GetX();
        TMAT(1, 1) *= invParentScale.GetY();
        TMAT(1, 2) *= invParentScale.GetZ();
        TMAT(2, 0) *= invParentScale.GetX();
        TMAT(2, 1) *= invParentScale.GetY();
        TMAT(2, 2) *= invParentScale.GetZ();
    }


    void Matrix::InitFromPosRot(const AZ::Vector3& pos, const AZ::Quaternion& rot)
    {
        const float xx = rot.GetX() * rot.GetX();
        const float xy = rot.GetX() * rot.GetY(), yy = rot.GetY() * rot.GetY();
        const float xz = rot.GetX() * rot.GetZ(), yz = rot.GetY() * rot.GetZ(), zz = rot.GetZ() * rot.GetZ();
        const float xw = rot.GetX() * rot.GetW(), yw = rot.GetY() * rot.GetW(), zw = rot.GetZ() * rot.GetW(), ww = rot.GetW() * rot.GetW();

        TMAT(0, 0) = +xx - yy - zz + ww;
        TMAT(0, 1) = +xy + zw + xy + zw;
        TMAT(0, 2) = +xz - yw + xz - yw;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = +xy - zw + xy - zw;
        TMAT(1, 1) = -xx + yy - zz + ww;
        TMAT(1, 2) = +yz + xw + yz + xw;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = +xz + yw + xz + yw;
        TMAT(2, 1) = +yz - xw + yz - xw;
        TMAT(2, 2) = -xx - yy + zz + ww;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = pos.GetX();
        TMAT(3, 1) = pos.GetY();
        TMAT(3, 2) = pos.GetZ();
        TMAT(3, 3) = 1.0f;
    }

    /*
    // optimized routine for handling scale rotation, rotation, scale and translation
    void Matrix::Set(const AZ::Quaternion& scaleRot, const AZ::Quaternion& rotation, const Vector3& scale, const Vector3& translation)
    {
        float xx=scaleRot.x*scaleRot.x;
        float xy=scaleRot.x*scaleRot.y, yy=scaleRot.y*scaleRot.y;
        float xz=scaleRot.x*scaleRot.z, yz=scaleRot.y*scaleRot.z, zz=scaleRot.z*scaleRot.z;
        float xw=scaleRot.x*scaleRot.w, yw=scaleRot.y*scaleRot.w, zw=scaleRot.z*scaleRot.w, ww=scaleRot.w*scaleRot.w;

        // init on the inversed scale rotation
        TMAT(0,0) = +xx-yy-zz+ww;   TMAT(1,0) = +xy+zw+xy+zw;   TMAT(2,0) = +xz-yw+xz-yw;   // translation part not initialized
        TMAT(0,1) = +xy-zw+xy-zw;   TMAT(1,1) = -xx+yy-zz+ww;   TMAT(2,1) = +yz+xw+yz+xw;
        TMAT(0,2) = +xz+yw+xz+yw;   TMAT(1,2) = +yz-xw+yz-xw;   TMAT(2,2) = -xx-yy+zz+ww;
        TMAT(0,3) = 0.0f;           TMAT(1,3) = 0.0f;           TMAT(2,3) = 0.0f;           TMAT(3,3) = 1.0f;

        // copy the 3x3 part into a temp buffer, so that we have the inverse scale rotation, before scaling applied to it
        float r33[3][3];
        uint32 i;
        for (i=0; i<3; ++i)
        {
    #ifdef MCORE_MATRIX_ROWMAJOR
            r33[i][0] = TMAT(i,0);
            r33[i][1] = TMAT(i,1);
            r33[i][2] = TMAT(i,2);
    #else
            r33[0][i] = TMAT(i,0);
            r33[1][i] = TMAT(i,1);
            r33[2][i] = TMAT(i,2);
    #endif
        }

        // apply scaling
        TMAT(0,0) *= scale.x;
        TMAT(0,1) *= scale.y;
        TMAT(0,2) *= scale.z;
        TMAT(1,0) *= scale.x;
        TMAT(1,1) *= scale.y;
        TMAT(1,2) *= scale.z;
        TMAT(2,0) *= scale.x;
        TMAT(2,1) *= scale.y;
        TMAT(2,2) *= scale.z;

        // undo the scale rotation
        float v[3];
        for (i=0; i<3; ++i)
        {
            v[0] = TMAT(i,0);
            v[1] = TMAT(i,1);
            v[2] = TMAT(i,2);

    #ifdef MCORE_MATRIX_ROWMAJOR
            TMAT(i,0) = v[0]*r33[0][0] + v[1]*r33[0][1] + v[2]*r33[0][2];   // transposed multiply
            TMAT(i,1) = v[0]*r33[1][0] + v[1]*r33[1][1] + v[2]*r33[1][2];
            TMAT(i,2) = v[0]*r33[2][0] + v[1]*r33[2][1] + v[2]*r33[2][2];
    #else
            TMAT(i,0) = v[0]*r33[0][0] + v[1]*r33[1][0] + v[2]*r33[2][0];   // transposed multiply
            TMAT(i,1) = v[0]*r33[0][1] + v[1]*r33[1][1] + v[2]*r33[2][1];
            TMAT(i,2) = v[0]*r33[0][2] + v[1]*r33[1][2] + v[2]*r33[2][2];
    #endif
        }

        // apply regular rotation
        xx=rotation.x*rotation.x;
        xy=rotation.x*rotation.y; yy=rotation.y*rotation.y;
        xz=rotation.x*rotation.z; yz=rotation.y*rotation.z; zz=rotation.z*rotation.z;
        xw=rotation.x*rotation.w; yw=rotation.y*rotation.w; zw=rotation.z*rotation.w; ww=rotation.w*rotation.w;

    #ifdef MCORE_MATRIX_ROWMAJOR
        r33[0][0] = +xx-yy-zz+ww;   r33[0][1] = +xy+zw+xy+zw;   r33[0][2] = +xz-yw+xz-yw;
        r33[1][0] = +xy-zw+xy-zw;   r33[1][1] = -xx+yy-zz+ww;   r33[1][2] = +yz+xw+yz+xw;
        r33[2][0] = +xz+yw+xz+yw;   r33[2][1] = +yz-xw+yz-xw;   r33[2][2] = -xx-yy+zz+ww;
    #else
        r33[0][0] = +xx-yy-zz+ww;   r33[1][0] = +xy+zw+xy+zw;   r33[2][0] = +xz-yw+xz-yw;
        r33[0][1] = +xy-zw+xy-zw;   r33[1][1] = -xx+yy-zz+ww;   r33[2][1] = +yz+xw+yz+xw;
        r33[0][2] = +xz+yw+xz+yw;   r33[1][2] = +yz-xw+yz-xw;   r33[2][2] = -xx-yy+zz+ww;
    #endif

        // mult 3x3 matrix
        for (i=0; i<3; ++i)
        {
            v[0] = TMAT(i,0);
            v[1] = TMAT(i,1);
            v[2] = TMAT(i,2);

    #ifdef MCORE_MATRIX_ROWMAJOR
            TMAT(i,0) = v[0]*r33[0][0] + v[1]*r33[1][0] + v[2]*r33[2][0];
            TMAT(i,1) = v[0]*r33[0][1] + v[1]*r33[1][1] + v[2]*r33[2][1];
            TMAT(i,2) = v[0]*r33[0][2] + v[1]*r33[1][2] + v[2]*r33[2][2];
    #else
            TMAT(i,0) = v[0]*r33[0][0] + v[1]*r33[0][1] + v[2]*r33[0][2];
            TMAT(i,1) = v[0]*r33[1][0] + v[1]*r33[1][1] + v[2]*r33[1][2];
            TMAT(i,2) = v[0]*r33[2][0] + v[1]*r33[2][1] + v[2]*r33[2][2];
    #endif
        }

        // apply translation
        TMAT(3,0) = translation.x;
        TMAT(3,1) = translation.y;
        TMAT(3,2) = translation.z;
    }
    */


    void Matrix::MultMatrix4x3(const Matrix& right)
    {
    #if (AZ_TRAIT_USE_PLATFORM_SIMD_SSE && defined(MCORE_MATRIX_ROWMAJOR))
        const float* m = right.m16;
        const float* n = m16;
        float* t = this->m16;

        __m128 x0;
        __m128 x1;
        __m128 x2;
        __m128 x3;
        __m128 x4;
        __m128 x5;
        __m128 x6;
        __m128 x7;
        x0 = _mm_loadu_ps(&m[0]);
        x1 = _mm_loadu_ps(&m[4]);
        x2 = _mm_loadu_ps(&m[8]);
        x3 = _mm_loadu_ps(&m[12]);
        x4 = _mm_load_ps1(&n[0]);
        x5 = _mm_load_ps1(&n[1]);
        x6 = _mm_load_ps1(&n[2]);
        x7 = _mm_load_ps1(&n[3]);
        x4 = _mm_mul_ps(x4, x0);
        x5 = _mm_mul_ps(x5, x1);
        x6 = _mm_mul_ps(x6, x2);
        x7 = _mm_mul_ps(x7, x3);
        x4 = _mm_add_ps(x4, x5);
        x6 = _mm_add_ps(x6, x7);
        x4 = _mm_add_ps(x4, x6);
        x5 = _mm_load_ps1(&n[4]);
        x6 = _mm_load_ps1(&n[5]);
        x7 = _mm_load_ps1(&n[6]);
        x5 = _mm_mul_ps(x5, x0);
        x6 = _mm_mul_ps(x6, x1);
        x7 = _mm_mul_ps(x7, x2);
        x5 = _mm_add_ps(x5, x6);
        x5 = _mm_add_ps(x5, x7);
        x6 = _mm_load_ps1(&n[7]);
        x6 = _mm_mul_ps(x6, x3);
        x5 = _mm_add_ps(x5, x6);
        x6 = _mm_load_ps1(&n[8]);
        x7 = _mm_load_ps1(&n[9]);
        x6 = _mm_mul_ps(x6, x0);
        x7 = _mm_mul_ps(x7, x1);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[10]);
        x7 = _mm_mul_ps(x7, x2);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[11]);
        x7 = _mm_mul_ps(x7, x3);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[12]);
        x0 = _mm_mul_ps(x0, x7);
        x7 = _mm_load_ps1(&n[13]);
        x1 = _mm_mul_ps(x1, x7);
        x7 = _mm_load_ps1(&n[14]);
        x2 = _mm_mul_ps(x2, x7);
        x7 = _mm_load_ps1(&n[15]);
        x3 = _mm_mul_ps(x3, x7);
        x0 = _mm_add_ps(x0, x1);
        x2 = _mm_add_ps(x2, x3);
        x0 = _mm_add_ps(x0, x2);

        //store result
        _mm_storeu_ps(&t[0], x4);
        _mm_storeu_ps(&t[4], x5);
        _mm_storeu_ps(&t[8], x6);
        _mm_storeu_ps(&t[12], x0);
    #else
        float v[3];

        for (uint32 i = 0; i < 4; ++i)
        {
            v[0] = TMAT(i, 0);
            v[1] = TMAT(i, 1);
            v[2] = TMAT(i, 2);
            TMAT(i, 0) = v[0] * MMAT(right, 0, 0) + v[1] * MMAT(right, 1, 0) + v[2] * MMAT(right, 2, 0);
            TMAT(i, 1) = v[0] * MMAT(right, 0, 1) + v[1] * MMAT(right, 1, 1) + v[2] * MMAT(right, 2, 1);
            TMAT(i, 2) = v[0] * MMAT(right, 0, 2) + v[1] * MMAT(right, 1, 2) + v[2] * MMAT(right, 2, 2);
        }

        TMAT(3, 0) += MMAT(right, 3, 0);
        TMAT(3, 1) += MMAT(right, 3, 1);
        TMAT(3, 2) += MMAT(right, 3, 2);
    #endif
    }


    // *this = left * right
    void Matrix::MultMatrix(const Matrix& left, const Matrix& right)
    {
    #if (AZ_TRAIT_USE_PLATFORM_SIMD_SSE && defined(MCORE_MATRIX_ROWMAJOR))
        const float* m = right.m16;
        const float* n = left.m16;
        float* t = this->m16;

        __m128 x0;
        __m128 x1;
        __m128 x2;
        __m128 x3;
        __m128 x4;
        __m128 x5;
        __m128 x6;
        __m128 x7;
        x0 = _mm_loadu_ps(&m[0]);
        x1 = _mm_loadu_ps(&m[4]);
        x2 = _mm_loadu_ps(&m[8]);
        x3 = _mm_loadu_ps(&m[12]);
        x4 = _mm_load_ps1(&n[0]);
        x5 = _mm_load_ps1(&n[1]);
        x6 = _mm_load_ps1(&n[2]);
        x7 = _mm_load_ps1(&n[3]);
        x4 = _mm_mul_ps(x4, x0);
        x5 = _mm_mul_ps(x5, x1);
        x6 = _mm_mul_ps(x6, x2);
        x7 = _mm_mul_ps(x7, x3);
        x4 = _mm_add_ps(x4, x5);
        x6 = _mm_add_ps(x6, x7);
        x4 = _mm_add_ps(x4, x6);
        x5 = _mm_load_ps1(&n[4]);
        x6 = _mm_load_ps1(&n[5]);
        x7 = _mm_load_ps1(&n[6]);
        x5 = _mm_mul_ps(x5, x0);
        x6 = _mm_mul_ps(x6, x1);
        x7 = _mm_mul_ps(x7, x2);
        x5 = _mm_add_ps(x5, x6);
        x5 = _mm_add_ps(x5, x7);
        x6 = _mm_load_ps1(&n[7]);
        x6 = _mm_mul_ps(x6, x3);
        x5 = _mm_add_ps(x5, x6);
        x6 = _mm_load_ps1(&n[8]);
        x7 = _mm_load_ps1(&n[9]);
        x6 = _mm_mul_ps(x6, x0);
        x7 = _mm_mul_ps(x7, x1);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[10]);
        x7 = _mm_mul_ps(x7, x2);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[11]);
        x7 = _mm_mul_ps(x7, x3);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[12]);
        x0 = _mm_mul_ps(x0, x7);
        x7 = _mm_load_ps1(&n[13]);
        x1 = _mm_mul_ps(x1, x7);
        x7 = _mm_load_ps1(&n[14]);
        x2 = _mm_mul_ps(x2, x7);
        x7 = _mm_load_ps1(&n[15]);
        x3 = _mm_mul_ps(x3, x7);
        x0 = _mm_add_ps(x0, x1);
        x2 = _mm_add_ps(x2, x3);
        x0 = _mm_add_ps(x0, x2);

        //store result
        _mm_storeu_ps(&t[0], x4);
        _mm_storeu_ps(&t[4], x5);
        _mm_storeu_ps(&t[8], x6);
        _mm_storeu_ps(&t[12], x0);
    #else
        float v[4];
        for (uint32 i = 0; i < 4; ++i)
        {
            v[0] = MMAT(left, i, 0);
            v[1] = MMAT(left, i, 1);
            v[2] = MMAT(left, i, 2);
            v[3] = MMAT(left, i, 3);
            TMAT(i, 0) = v[0] * MMAT(right, 0, 0) + v[1] * MMAT(right, 1, 0) + v[2] * MMAT(right, 2, 0) + v[3] * MMAT(right, 3, 0);
            TMAT(i, 1) = v[0] * MMAT(right, 0, 1) + v[1] * MMAT(right, 1, 1) + v[2] * MMAT(right, 2, 1) + v[3] * MMAT(right, 3, 1);
            TMAT(i, 2) = v[0] * MMAT(right, 0, 2) + v[1] * MMAT(right, 1, 2) + v[2] * MMAT(right, 2, 2) + v[3] * MMAT(right, 3, 2);
            TMAT(i, 3) = v[0] * MMAT(right, 0, 3) + v[1] * MMAT(right, 1, 3) + v[2] * MMAT(right, 2, 3) + v[3] * MMAT(right, 3, 3);
        }
    #endif
    }



    void Matrix::MultMatrix4x3(const Matrix& left, const Matrix& right)
    {
    #if (AZ_TRAIT_USE_PLATFORM_SIMD_SSE && defined(MCORE_MATRIX_ROWMAJOR))
        const float* m = right.m16;
        const float* n = left.m16;
        float* t = this->m16;

        __m128 x0;
        __m128 x1;
        __m128 x2;
        __m128 x3;
        __m128 x4;
        __m128 x5;
        __m128 x6;
        __m128 x7;
        x0 = _mm_loadu_ps(&m[0]);
        x1 = _mm_loadu_ps(&m[4]);
        x2 = _mm_loadu_ps(&m[8]);
        x3 = _mm_loadu_ps(&m[12]);
        x4 = _mm_load_ps1(&n[0]);
        x5 = _mm_load_ps1(&n[1]);
        x6 = _mm_load_ps1(&n[2]);
        x7 = _mm_load_ps1(&n[3]);
        x4 = _mm_mul_ps(x4, x0);
        x5 = _mm_mul_ps(x5, x1);
        x6 = _mm_mul_ps(x6, x2);
        x7 = _mm_mul_ps(x7, x3);
        x4 = _mm_add_ps(x4, x5);
        x6 = _mm_add_ps(x6, x7);
        x4 = _mm_add_ps(x4, x6);
        x5 = _mm_load_ps1(&n[4]);
        x6 = _mm_load_ps1(&n[5]);
        x7 = _mm_load_ps1(&n[6]);
        x5 = _mm_mul_ps(x5, x0);
        x6 = _mm_mul_ps(x6, x1);
        x7 = _mm_mul_ps(x7, x2);
        x5 = _mm_add_ps(x5, x6);
        x5 = _mm_add_ps(x5, x7);
        x6 = _mm_load_ps1(&n[7]);
        x6 = _mm_mul_ps(x6, x3);
        x5 = _mm_add_ps(x5, x6);
        x6 = _mm_load_ps1(&n[8]);
        x7 = _mm_load_ps1(&n[9]);
        x6 = _mm_mul_ps(x6, x0);
        x7 = _mm_mul_ps(x7, x1);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[10]);
        x7 = _mm_mul_ps(x7, x2);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[11]);
        x7 = _mm_mul_ps(x7, x3);
        x6 = _mm_add_ps(x6, x7);
        x7 = _mm_load_ps1(&n[12]);
        x0 = _mm_mul_ps(x0, x7);
        x7 = _mm_load_ps1(&n[13]);
        x1 = _mm_mul_ps(x1, x7);
        x7 = _mm_load_ps1(&n[14]);
        x2 = _mm_mul_ps(x2, x7);
        x7 = _mm_load_ps1(&n[15]);
        x3 = _mm_mul_ps(x3, x7);
        x0 = _mm_add_ps(x0, x1);
        x2 = _mm_add_ps(x2, x3);
        x0 = _mm_add_ps(x0, x2);

        //store result
        _mm_storeu_ps(&t[0], x4);
        _mm_storeu_ps(&t[4], x5);
        _mm_storeu_ps(&t[8], x6);
        _mm_storeu_ps(&t[12], x0);
    #else
        float v[3];
        for (uint32 i = 0; i < 4; ++i)
        {
            v[0] = MMAT(left, i, 0);
            v[1] = MMAT(left, i, 1);
            v[2] = MMAT(left, i, 2);
            TMAT(i, 0) = v[0] * MMAT(right, 0, 0) + v[1] * MMAT(right, 1, 0) + v[2] * MMAT(right, 2, 0);
            TMAT(i, 1) = v[0] * MMAT(right, 0, 1) + v[1] * MMAT(right, 1, 1) + v[2] * MMAT(right, 2, 1);
            TMAT(i, 2) = v[0] * MMAT(right, 0, 2) + v[1] * MMAT(right, 1, 2) + v[2] * MMAT(right, 2, 2);
        }

        TMAT(0, 3) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 3) = 1.0f;
        TMAT(3, 0) += MMAT(right, 3, 0);
        TMAT(3, 1) += MMAT(right, 3, 1);
        TMAT(3, 2) += MMAT(right, 3, 2);
    #endif
    }


    void Matrix::MultMatrix3x3(const Matrix& right)
    {
        float v[3];
        for (uint32 i = 0; i < 4; ++i)
        {
            v[0] = TMAT(i, 0);
            v[1] = TMAT(i, 1);
            v[2] = TMAT(i, 2);
            TMAT(i, 0) = v[0] * MMAT(right, 0, 0) + v[1] * MMAT(right, 1, 0) + v[2] * MMAT(right, 2, 0);
            TMAT(i, 1) = v[0] * MMAT(right, 0, 1) + v[1] * MMAT(right, 1, 1) + v[2] * MMAT(right, 2, 1);
            TMAT(i, 2) = v[0] * MMAT(right, 0, 2) + v[1] * MMAT(right, 1, 2) + v[2] * MMAT(right, 2, 2);
        }
    }


    void Matrix::Transpose()
    {
        Matrix v;

        MMAT(v, 0, 0) = TMAT(0, 0);
        MMAT(v, 0, 1) = TMAT(1, 0);
        MMAT(v, 0, 2) = TMAT(2, 0);
        MMAT(v, 0, 3) = TMAT(3, 0);
        MMAT(v, 1, 0) = TMAT(0, 1);
        MMAT(v, 1, 1) = TMAT(1, 1);
        MMAT(v, 1, 2) = TMAT(2, 1);
        MMAT(v, 1, 3) = TMAT(3, 1);
        MMAT(v, 2, 0) = TMAT(0, 2);
        MMAT(v, 2, 1) = TMAT(1, 2);
        MMAT(v, 2, 2) = TMAT(2, 2);
        MMAT(v, 2, 3) = TMAT(3, 2);
        MMAT(v, 3, 0) = TMAT(0, 3);
        MMAT(v, 3, 1) = TMAT(1, 3);
        MMAT(v, 3, 2) = TMAT(2, 3);
        MMAT(v, 3, 3) = TMAT(3, 3);

        *this = v;
    }


    void Matrix::TransposeTranslation()
    {
        AZ::Vector3 temp;

        temp.SetX(TMAT(3, 0));
        temp.SetY(TMAT(3, 1));
        temp.SetZ(TMAT(3, 2));

        TMAT(3, 0) = TMAT(0, 3);
        TMAT(3, 1) = TMAT(1, 3);
        TMAT(3, 2) = TMAT(2, 3);

        TMAT(0, 3) = temp.GetX();
        TMAT(1, 3) = temp.GetY();
        TMAT(2, 3) = temp.GetZ();
    }



    void Matrix::Adjoint()
    {
        Matrix v;

        MMAT(v, 0, 0) = TMAT(1, 1) * TMAT(2, 2) - TMAT(1, 2) * TMAT(2, 1);
        MMAT(v, 0, 1) = TMAT(2, 1) * TMAT(0, 2) - TMAT(2, 2) * TMAT(0, 1);
        MMAT(v, 0, 2) = TMAT(0, 1) * TMAT(1, 2) - TMAT(0, 2) * TMAT(1, 1);
        MMAT(v, 0, 3) = TMAT(0, 3);
        MMAT(v, 1, 0) = TMAT(1, 2) * TMAT(2, 0) - TMAT(1, 0) * TMAT(2, 2);
        MMAT(v, 1, 1) = TMAT(2, 2) * TMAT(0, 0) - TMAT(2, 0) * TMAT(0, 2);
        MMAT(v, 1, 2) = TMAT(0, 2) * TMAT(1, 0) - TMAT(0, 0) * TMAT(1, 2);
        MMAT(v, 1, 3) = TMAT(1, 3);
        MMAT(v, 2, 0) = TMAT(1, 0) * TMAT(2, 1) - TMAT(1, 1) * TMAT(2, 0);
        MMAT(v, 2, 1) = TMAT(2, 0) * TMAT(0, 1) - TMAT(2, 1) * TMAT(0, 0);
        MMAT(v, 2, 2) = TMAT(0, 0) * TMAT(1, 1) - TMAT(0, 1) * TMAT(1, 0);
        MMAT(v, 2, 3) = TMAT(2, 3);
        MMAT(v, 3, 0) = -(TMAT(0, 0) * TMAT(3, 0) + TMAT(1, 0) * TMAT(3, 1) + TMAT(2, 0) * TMAT(3, 2));
        MMAT(v, 3, 1) = -(TMAT(0, 1) * TMAT(3, 0) + TMAT(1, 1) * TMAT(3, 1) + TMAT(2, 1) * TMAT(3, 2));
        MMAT(v, 3, 2) = -(TMAT(0, 2) * TMAT(3, 0) + TMAT(1, 2) * TMAT(3, 1) + TMAT(2, 2) * TMAT(3, 2));
        MMAT(v, 3, 3) = TMAT(3, 3);

        *this = v;
    }



    AZ::Vector3 Matrix::InverseRot(const AZ::Vector3& v)
    {
        Matrix m(*this);
        m.Inverse();
        m.SetTranslation(0.0f, 0.0f, 0.0f);
        return v * m;
    }



    void Matrix::Inverse()
    {
        Matrix v;

        const float s = 1.0f / CalcDeterminant();
        MMAT(v, 0, 0) = (TMAT(1, 1) * TMAT(2, 2) - TMAT(1, 2) * TMAT(2, 1)) * s;
        MMAT(v, 0, 1) = (TMAT(2, 1) * TMAT(0, 2) - TMAT(2, 2) * TMAT(0, 1)) * s;
        MMAT(v, 0, 2) = (TMAT(0, 1) * TMAT(1, 2) - TMAT(0, 2) * TMAT(1, 1)) * s;
        MMAT(v, 0, 3) = TMAT(0, 3);
        MMAT(v, 1, 0) = (TMAT(1, 2) * TMAT(2, 0) - TMAT(1, 0) * TMAT(2, 2)) * s;
        MMAT(v, 1, 1) = (TMAT(2, 2) * TMAT(0, 0) - TMAT(2, 0) * TMAT(0, 2)) * s;
        MMAT(v, 1, 2) = (TMAT(0, 2) * TMAT(1, 0) - TMAT(0, 0) * TMAT(1, 2)) * s;
        MMAT(v, 1, 3) = TMAT(1, 3);
        MMAT(v, 2, 0) = (TMAT(1, 0) * TMAT(2, 1) - TMAT(1, 1) * TMAT(2, 0)) * s;
        MMAT(v, 2, 1) = (TMAT(2, 0) * TMAT(0, 1) - TMAT(2, 1) * TMAT(0, 0)) * s;
        MMAT(v, 2, 2) = (TMAT(0, 0) * TMAT(1, 1) - TMAT(0, 1) * TMAT(1, 0)) * s;
        MMAT(v, 2, 3) = TMAT(2, 3);
        MMAT(v, 3, 0) = -(MMAT(v, 0, 0) * TMAT(3, 0) + MMAT(v, 1, 0) * TMAT(3, 1) + MMAT(v, 2, 0) * TMAT(3, 2));
        MMAT(v, 3, 1) = -(MMAT(v, 0, 1) * TMAT(3, 0) + MMAT(v, 1, 1) * TMAT(3, 1) + MMAT(v, 2, 1) * TMAT(3, 2));
        MMAT(v, 3, 2) = -(MMAT(v, 0, 2) * TMAT(3, 0) + MMAT(v, 1, 2) * TMAT(3, 1) + MMAT(v, 2, 2) * TMAT(3, 2));
        MMAT(v, 3, 3) = TMAT(3, 3);

        *this = v;
    }



    void Matrix::OrthoNormalize()
    {
        AZ::Vector3 x = GetRight();
        AZ::Vector3 y = GetUp();
        //Vector3 z = GetForward();

        x.Normalize();
        y -= x * x.Dot(y);
        y.Normalize();
        AZ::Vector3 z = x.Cross(y);

        SetRight(x);
        SetUp(y);
        SetForward(z);
    }



    void Matrix::Mirror(const Matrix& transform, const PlaneEq& plane)
    {
        // components
        AZ::Vector3 x = transform.GetRight();
        AZ::Vector3 y = transform.GetForward();
        AZ::Vector3 z = transform.GetUp();
        AZ::Vector3 t = transform.GetTranslation();
        AZ::Vector3 n = plane.GetNormal();
        AZ::Vector3 n2 = n * -2.0f;
        float   d = plane.GetDist();

        // mirror translation
        AZ::Vector3 mt = t + n2 * (t.Dot(n) - d);

        // mirror x rotation
        x += t;
        x += n2 * (x.Dot(n) - d);
        x -= mt;

        // mirror y rotation
        y += t;
        y += n2 * (y.Dot(n) - d);
        y -= mt;

        // mirror z rotation
        z += t;
        z += n2 * (z.Dot(n) - d);
        z -= mt;

        // write result
        SetRight(x);
        SetForward(y);
        SetUp(z);
        SetTranslation(mt);

        TMAT(0, 3) = 0;
        TMAT(1, 3) = 0;
        TMAT(2, 3) = 0;
        TMAT(3, 3) = 1;
    }


    void Matrix::LookAt(const AZ::Vector3& view, const AZ::Vector3& target, const AZ::Vector3& up)
    {
        const AZ::Vector3 z = (target - view).GetNormalized();
        const AZ::Vector3 x = (up.Cross(z)).GetNormalized();
        const AZ::Vector3 y = z.Cross(x);

        TMAT(0, 0) = x.GetX();
        TMAT(0, 1) = y.GetX();
        TMAT(0, 2) = z.GetX();
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = x.GetY();
        TMAT(1, 1) = y.GetY();
        TMAT(1, 2) = z.GetY();
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = x.GetZ();
        TMAT(2, 1) = y.GetZ();
        TMAT(2, 2) = z.GetZ();
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = -x.Dot(view);
        TMAT(3, 1) = -y.Dot(view);
        TMAT(3, 2) = -z.Dot(view);
        TMAT(3, 3) = 1.0f;

        // DirectX:
        //  zaxis = normal(cameraTarget - cameraPosition)
        //  xaxis = normal(cross(cameraUpVector, zaxis))
        //  yaxis = cross(zaxis, xaxis)
        //  xaxis.x           yaxis.x           zaxis.x          0
        //  xaxis.y           yaxis.y           zaxis.y          0
        //  xaxis.z           yaxis.z           zaxis.z          0
        //  -dot(xaxis, cameraPosition)  -dot(yaxis, cameraPosition)  -dot(zaxis, cameraPosition)  1
    }


    void Matrix::LookAtRH(const AZ::Vector3& view, const AZ::Vector3& target, const AZ::Vector3& up)
    {
        const AZ::Vector3 z = (view - target).GetNormalized();
        const AZ::Vector3 x = (up.Cross(z)).GetNormalized();
        const AZ::Vector3 y = z.Cross(x);

        TMAT(0, 0) = x.GetX();
        TMAT(0, 1) = y.GetX();
        TMAT(0, 2) = z.GetX();
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = x.GetY();
        TMAT(1, 1) = y.GetY();
        TMAT(1, 2) = z.GetY();
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = x.GetZ();
        TMAT(2, 1) = y.GetZ();
        TMAT(2, 2) = z.GetZ();
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = -x.Dot(view);
        TMAT(3, 1) = -y.Dot(view);
        TMAT(3, 2) = -z.Dot(view);
        TMAT(3, 3) = 1.0f;

        // DirectX:
        //  zaxis = normal(cameraPosition - cameraTarget)
        //  xaxis = normal(cross(cameraUpVector, zaxis))
        //  yaxis = cross(zaxis, xaxis)
        //  xaxis.x           yaxis.x           zaxis.x          0
        //  xaxis.y           yaxis.y           zaxis.y          0
        //  xaxis.z           yaxis.z           zaxis.z          0
        //  -dot(xaxis, cameraPosition)  -dot(yaxis, cameraPosition)  -dot(zaxis, cameraPosition)  1
    }

    // ortho projection matrix
    void Matrix::OrthoOffCenter(float left, float right, float top, float bottom, float znear, float zfar)
    {
        TMAT(0, 0) = 2.0f / (right - left);
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = 2.0f / (top - bottom);
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = 1.0f / (zfar - znear);
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = (left + right) / (left - right);
        TMAT(3, 1) = (top + bottom) / (bottom - top);
        TMAT(3, 2) = znear / (znear - zfar);
        TMAT(3, 3) = 1.0f;

        // DirectX:
        //  2/(right-l)           0                         0                                  0
        //  0                     2/(top-bottom)            0                                  0
        //  0                     0                         1/(zfarPlane-znearPlane)           0
        //  (l+right)/(l-right)  (top+bottom)/(bottom-top)  znearPlane/(znearPlane-zfarPlane)  1
    }


    // ortho projection matrix
    void Matrix::OrthoOffCenterRH(float left, float right, float top, float bottom, float znear, float zfar)
    {
        TMAT(0, 0) = 2.0f / (right - left);
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = 2.0f / (top - bottom);
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = 1.0f / (znear - zfar);
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = (left + right) / (left - right);
        TMAT(3, 1) = (top + bottom) / (bottom - top);
        TMAT(3, 2) = znear / (znear - zfar);
        TMAT(3, 3) = 1.0f;

        // DirectX:
        //  2/(right-left)         0                         0                                  0
        //  0                      2/(top-bottom)            0                                  0
        //  0                      0                         1/(znearPlane-zfarPlane)           0
        //  (l+right)/(l-rright)  (top+bottom)/(bottom-top)  znearPlane/(znearPlane-zfarPlane)  1
    }


    // ortho projection matrix
    void Matrix::Ortho(float left, float right, float top, float bottom, float znear, float zfar)
    {
        TMAT(0, 0) = 2.0f / (right - left);
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = 2.0f / (top - bottom);
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = 1.0f / (zfar - znear);
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = znear / (znear - zfar);
        TMAT(3, 3) = 1.0f;

        // DirectX:
        //  2/width    0         0                                  0
        //  0          2/height  0                                  0
        //  0          0         1/(zfarPlane-znearPlane)           0
        //  0          0         znearPlane/(znearPlane-zfarPlane)  1
    }


    // ortho projection matrix, right handed
    void Matrix::OrthoRH(float left, float right, float top, float bottom, float znear, float zfar)
    {
        TMAT(0, 0) = 2.0f / (right - left);
        TMAT(0, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = 0.0f;
        TMAT(1, 1) = 2.0f / (top - bottom);
        TMAT(1, 2) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = 0.0f;
        TMAT(2, 1) = 0.0f;
        TMAT(2, 2) = 1.0f / (znear - zfar);
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = znear / (znear - zfar);
        TMAT(3, 3) = 1.0f;

        // DirectX:
        //  2/width  0         0                                  0
        //  0        2/height  0                                  0
        //  0        0         1/(znearPlane-zfarPlane)           0
        //  0        0         znearPlane/(znearPlane-zfarPlane)  1
    }


    // frustum matrix
    void Matrix::Frustum(float left, float right, float top, float bottom, float znear, float zfar)
    {
        TMAT(0, 0) = 2.0f * znear / (right - left);
        TMAT(1, 0) = 0.0f;
        TMAT(2, 0) = (right + left) / (right - left);
        TMAT(3, 0) = 0.0f;
        TMAT(0, 1) = 0.0f;
        TMAT(1, 1) = 2.0f * znear / (top - bottom);
        TMAT(2, 1) = (top + bottom) / (top - bottom);
        TMAT(3, 1) = 0.0f;
        TMAT(0, 2) = 0.0f;
        TMAT(1, 2) = 0.0f;
        TMAT(2, 2) = (zfar + znear) / (zfar - znear);
        TMAT(3, 2) = 2.0f * zfar * znear / (zfar - znear);
        TMAT(0, 3) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 3) = -1.0f;
        TMAT(3, 3) = 0.0f;
    }


    // setup perspective projection matrix
    void Matrix::Perspective(float fov, float aspect, float zNear, float zFar)
    {
        const float yScale = 1.0f / Math::Tan(fov * 0.5f);
        const float xScale = yScale / aspect;
        const float d = zFar / (zFar - zNear);

        MCore::MemSet(m44, 0, 16 * sizeof(float));
        TMAT(0, 0) = xScale;
        TMAT(1, 1) = yScale;
        TMAT(2, 2) = d;
        TMAT(2, 3) = 1.0f;
        TMAT(3, 2) = -zNear * d;
    }


    // setup perspective projection matrix, right handed
    void Matrix::PerspectiveRH(float fov, float aspect, float zNear, float zFar)
    {
        const float yScale = 1.0f / Math::Tan(fov * 0.5f);
        const float xScale = yScale / aspect;
        const float d = zFar / (zNear - zFar);

        MCore::MemSet(m44, 0, 16 * sizeof(float));
        TMAT(0, 0) = xScale;
        TMAT(1, 1) = yScale;
        TMAT(2, 2) = d;
        TMAT(2, 3) = -1.0f;
        TMAT(3, 2) = zNear * d;
    }


    // check if the matrix is symmetric or not
    bool Matrix::CheckIfIsSymmetric(float tolerance) const
    {
        // if no tolerance check is needed
        if (MCore::Math::IsFloatZero(tolerance))
        {
            if (TMAT(1, 0) != TMAT(0, 1))
            {
                return false;
            }
            if (TMAT(2, 0) != TMAT(0, 2))
            {
                return false;
            }
            if (TMAT(2, 1) != TMAT(1, 2))
            {
                return false;
            }
            if (TMAT(3, 0) != TMAT(0, 3))
            {
                return false;
            }
            if (TMAT(3, 1) != TMAT(1, 3))
            {
                return false;
            }
            if (TMAT(3, 2) != TMAT(2, 3))
            {
                return false;
            }
        }
        else // tolerance check needed
        {
            if (Math::Abs(TMAT(1, 0) - TMAT(0, 1)) > tolerance)
            {
                return false;
            }
            if (Math::Abs(TMAT(2, 0) - TMAT(0, 2)) > tolerance)
            {
                return false;
            }
            if (Math::Abs(TMAT(2, 1) - TMAT(1, 2)) > tolerance)
            {
                return false;
            }
            if (Math::Abs(TMAT(3, 0) - TMAT(0, 3)) > tolerance)
            {
                return false;
            }
            if (Math::Abs(TMAT(3, 1) - TMAT(1, 3)) > tolerance)
            {
                return false;
            }
            if (Math::Abs(TMAT(3, 2) - TMAT(2, 3)) > tolerance)
            {
                return false;
            }
        }

        // yeah, we have a symmetric matrix here
        return true;
    }


    // check if this matrix is a diagonal matrix or not.
    bool Matrix::CheckIfIsDiagonal(float tolerance) const
    {
        if (tolerance <= Math::epsilon)
        {
            // for all entries
            for (uint32 y = 0; y < 4; ++y)
            {
                for (uint32 x = 0; x < 4; ++x)
                {
                    // if we are on the diagonal
                    if (x == y)
                    {
                        if (TMAT(y, x) == 0)
                        {
                            return false;               // if this entry on the diagonal is 0, we have no diagonal matrix
                        }
                    }
                    else // we are not on the diagonal
                    if (TMAT(y, x) != 0)
                    {
                        return false;                   // if the entry isn't equal to 0, it isn't a diagonal matrix
                    }
                }
            }
        }
        else
        {
            // for all entries
            for (uint32 y = 0; y < 4; ++y)
            {
                for (uint32 x = 0; x < 4; ++x)
                {
                    // if we are on the diagonal
                    if (x == y)
                    {
                        if (Math::Abs(TMAT(y, x)) < tolerance)
                        {
                            return false; // if this entry on the diagonal is 0, we have no diagonal matrix
                        }
                    }
                    else // we are not on the diagonal
                    {
                        if (Math::Abs(TMAT(y, x)) > tolerance)
                        {
                            return false; // if the entry isn't equal to 0, it isn't a diagonal matrix
                        }
                    }
                }
            }
        }

        // yeaaah, we have a diagonal matrix here
        return true;
    }


    // prints the matrix into the logfile or debug output, using MCore::LOG()
    void Matrix::Log() const
    {
        MCore::LogDetailedInfo("");
        MCore::LogDetailedInfo("(%.8f, %.8f, %.8f, %.8f)", m16[0], m16[1], m16[2], m16[3]);
        MCore::LogDetailedInfo("(%.8f, %.8f, %.8f, %.8f)", m16[4], m16[5], m16[6], m16[7]);
        MCore::LogDetailedInfo("(%.8f, %.8f, %.8f, %.8f)", m16[8], m16[9], m16[10], m16[11]);
        MCore::LogDetailedInfo("(%.8f, %.8f, %.8f, %.8f)", m16[12], m16[13], m16[14], m16[15]);
        MCore::LogDetailedInfo("");
    }


    // check if the matrix is orthogonal or not
    bool Matrix::CheckIfIsOrthogonal(float tolerance) const
    {
        // get the matrix vectors
        AZ::Vector3 right   = GetRight();
        AZ::Vector3 up      = GetUp();
        AZ::Vector3 forward = GetForward();

        // check if the vectors form an orthonormal set
        if (Math::Abs(right.Dot(up))      > tolerance)
        {
            return false;
        }
        if (Math::Abs(right.Dot(forward)) > tolerance)
        {
            return false;
        }
        if (Math::Abs(forward.Dot(up))    > tolerance)
        {
            return false;
        }

        // the vector set is not orthonormal, so the matrix is not an orthogonal one
        return true;
    }


    // check if the matrix is an identity matrix or not
    bool Matrix::CheckIfIsIdentity(float tolerance) const
    {
        // for all entries
        for (uint32 y = 0; y < 4; ++y)
        {
            for (uint32 x = 0; x < 4; ++x)
            {
                // if we are on the diagonal
                if (x == y)
                {
                    if (Math::Abs(1.0f - TMAT(y, x)) > tolerance)
                    {
                        return false; // if this entry on the diagonal not 1, we have no identity matrix
                    }
                }
                else // we are not on the diagonal
                {
                    if (Math::Abs(TMAT(y, x)) > tolerance)
                    {
                        return false; // if the entry isn't equal to 0, it isn't an identity matrix
                    }
                }
            }
        }

        // yup, we have an identity matrix here :)
        return true;
    }


    // calculate the handedness of the matrix
    float Matrix::CalcHandedness() const
    {
        // get the matrix vectors
        AZ::Vector3 right   = GetRight();
        AZ::Vector3 up      = GetUp();
        AZ::Vector3 forward = GetForward();

        // calculate the handedness (negative result means left handed, positive means right handed)
        return (right.Cross(up)).Dot(forward);
    }


    // check if the matrix is right handed or not
    bool Matrix::CheckIfIsRightHanded() const
    {
        return (CalcHandedness() <= 0.0f);
    }


    // check if the matrix is right handed or not

    bool Matrix::CheckIfIsLeftHanded() const
    {
        return (CalcHandedness() > 0.0f);
    }


    // check if this matrix is a pure rotation matrix or not
    bool Matrix::CheckIfIsPureRotationMatrix(float tolerance) const
    {
        return (Math::Abs(1.0f - CalcDeterminant()) < tolerance);
    }


    // check if the matrix is reflected (mirrored) or not
    bool Matrix::CheckIfIsReflective() const
    {
        float determinant = CalcDeterminant();
        return (determinant < 0.0f);
        //return ((determinant > (-1.0 - tolerance)) && (determinant < (-1.0 + tolerance)));    // if the determinant is near -1, it will reflect
    }


    // calculate the inverse transpose
    void Matrix::InverseTranspose()
    {
        Inverse();
        Transpose();
    }


    // return the inverse transposed version of this matrix
    Matrix Matrix::InverseTransposed() const
    {
        Matrix result(*this);
        result.InverseTranspose();
        return result;
    }


    // normalize a matrix
    void Matrix::Normalize()
    {
        // get the current vectors
        AZ::Vector3 right   = GetRight();
        AZ::Vector3 up      = GetUp();
        AZ::Vector3 forward = GetForward();

        // normalize them
        right.Normalize();
        up.Normalize();
        forward.Normalize();

        // update them again with the normalized versions
        SetRight(right);
        SetUp(up);
        SetForward(forward);
    }


    // creates a shear matrix
    void Matrix::SetShearMatrix(const AZ::Vector3& s)
    {
        TMAT(0, 0) = 1;
        TMAT(0, 1) = s.GetX();
        TMAT(0, 2) = s.GetY();
        TMAT(0, 3) = 0;
        TMAT(1, 0) = 0;
        TMAT(1, 1) = 1;
        TMAT(1, 2) = s.GetZ();
        TMAT(1, 3) = 0;
        TMAT(2, 0) = 0;
        TMAT(2, 1) = 0;
        TMAT(2, 2) = 1;
        TMAT(2, 3) = 0;
        TMAT(3, 0) = 0;
        TMAT(3, 1) = 0;
        TMAT(3, 2) = 0;
        TMAT(3, 3) = 1;
    }



    void Matrix::SetRotationMatrix(const AZ::Quaternion& rotation)
    {
        const float xx = rotation.GetX() * rotation.GetX();
        const float xy = rotation.GetX() * rotation.GetY(), yy = rotation.GetY() * rotation.GetY();
        const float xz = rotation.GetX() * rotation.GetZ(), yz = rotation.GetY() * rotation.GetZ(), zz = rotation.GetZ() * rotation.GetZ();
        const float xw = rotation.GetX() * rotation.GetW(), yw = rotation.GetY() * rotation.GetW(), zw = rotation.GetZ() * rotation.GetW(), ww = rotation.GetW() * rotation.GetW();

        TMAT(0, 0) = +xx - yy - zz + ww;
        TMAT(0, 1) = +xy + zw + xy + zw;
        TMAT(0, 2) = +xz - yw + xz - yw;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = +xy - zw + xy - zw;
        TMAT(1, 1) = -xx + yy - zz + ww;
        TMAT(1, 2) = +yz + xw + yz + xw;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = +xz + yw + xz + yw;
        TMAT(2, 1) = +yz - xw + yz - xw;
        TMAT(2, 2) = -xx - yy + zz + ww;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = 0.0f;
        TMAT(3, 3) = 1.0f;
    }



    // simple decompose a matrix into translation and rotation
    void Matrix::Decompose(AZ::Vector3* outTranslation, AZ::Quaternion* outRotation) const
    {
        // make a copy of the matrix
        Matrix mat(*this);

        // normalize the basis vectors
        mat.SetRight(SafeNormalize(mat.GetRight()));
        mat.SetUp(SafeNormalize(mat.GetUp()));
        mat.SetForward(SafeNormalize(mat.GetForward()));

        // extract the translation from the matrix
        *outTranslation = mat.GetTranslation();

        // convert the normalized 3x3 rotation part into a AZ::Quaternion
        *outRotation = MCore::MCoreMatrixToQuaternion(*this);
    }



    // calculate a rotation matrix from two vectors
    void Matrix::SetRotationMatrixTwoVectors(const AZ::Vector3& from, const AZ::Vector3& to)
    {
        // calculate intermediate values
        const float lengths = SafeLength(to) * SafeLength(from);
        const float D = (lengths > Math::epsilon) ? 1.0f / lengths : 0.0f;
        const float C = (to.GetX() * from.GetX() + to.GetY() * from.GetY() + to.GetZ() * from.GetZ()) * D;
        const float vzwy = (to.GetY() * from.GetZ()) - (to.GetZ() * from.GetY());
        const float wxuz = (to.GetZ() * from.GetX()) - (to.GetX() * from.GetZ());
        const float uyvx = (to.GetX() * from.GetY()) - (to.GetY() * from.GetX());
        const float A = vzwy * vzwy + wxuz * wxuz + uyvx * uyvx;

        // return identity if the cross product of the two vectors is small
        if (A < Math::epsilon)
        {
            Identity();
            return;
        }

        // set the components of the rotation matrix
        const float t = (1.0f - C) / A;
        TMAT(0, 0) = t * vzwy * vzwy + C;
        TMAT(1, 1) = t * wxuz * wxuz + C;
        TMAT(2, 2) = t * uyvx * uyvx + C;
        TMAT(3, 3) = 1.0f;
        TMAT(0, 1) = t * vzwy * wxuz + D * uyvx;
        TMAT(0, 2) = t * vzwy * uyvx - D * wxuz;
        TMAT(1, 2) = t * wxuz * uyvx + D * vzwy;
        TMAT(1, 0) = t * vzwy * wxuz - D * uyvx;
        TMAT(2, 0) = t * vzwy * uyvx + D * wxuz;
        TMAT(2, 1) = t * wxuz * uyvx - D * vzwy;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = 0.0f;
    }



    // output: x=pitch, y=yaw, z=roll
    // reconstruction: roll*pitch*yaw (zxy)
    AZ::Vector3 Matrix::CalcPitchYawRoll() const
    {
        const float pitch = Math::ASin(-TMAT(2, 1));
        const float cosPitch = Math::Cos(pitch);
        const float threshold = 0.0001f;
        float roll;
        float yaw;

        if (cosPitch > threshold)
        {
            roll = Math::ATan2(TMAT(0, 1), TMAT(1, 1));
            yaw  = Math::ATan2(TMAT(2, 0), TMAT(2, 2));
        }
        else
        {
            roll = Math::ATan2(-TMAT(1, 0), TMAT(0, 0));
            yaw  = 0.0f;
        }

        return AZ::Vector3(pitch, yaw, roll);
    }



    // init the matrix from a yaw/pitch/roll angle set
    void Matrix::SetRotationMatrixPitchYawRoll(float pitch, float yaw, float roll)
    {
        const float cosX = Math::Cos(pitch);
        const float cosY = Math::Cos(yaw);
        const float cosZ = Math::Cos(roll);
        const float sinX = Math::Sin(pitch);
        const float sinY = Math::Sin(yaw);
        const float sinZ = Math::Sin(roll);

        TMAT(0, 0) = cosZ * cosY + sinZ * sinX * sinY;
        TMAT(0, 1) = sinZ * cosX;
        TMAT(0, 2) = cosZ * -sinY + sinZ * sinX * cosY;
        TMAT(0, 3) = 0.0f;
        TMAT(1, 0) = -sinZ * cosY + cosZ * sinX * sinY;
        TMAT(1, 1) = cosZ * cosX;
        TMAT(1, 2) = sinZ * sinY + cosZ * sinX * cosY;
        TMAT(1, 3) = 0.0f;
        TMAT(2, 0) = cosX * sinY;
        TMAT(2, 1) = -sinX;
        TMAT(2, 2) = cosX * cosY;
        TMAT(2, 3) = 0.0f;
        TMAT(3, 0) = 0.0f;
        TMAT(3, 1) = 0.0f;
        TMAT(3, 2) = 0.0f;
        TMAT(3, 3) = 1.0f;
    }


    //
    void Matrix::DecomposeQRGramSchmidt(AZ::Vector3& translation, AZ::Quaternion& rot, AZ::Vector3& scale, AZ::Vector3& shear) const
    {
        Matrix rotMatrix;
        DecomposeQRGramSchmidt(translation, rotMatrix, scale, shear);
        rot = MCore::MCoreMatrixToQuaternion(*this);
    }

    //
    void Matrix::DecomposeQRGramSchmidt(AZ::Vector3& translation, AZ::Quaternion& rot, AZ::Vector3& scale) const
    {
        Matrix rotMatrix;
        DecomposeQRGramSchmidt(translation, rotMatrix, scale);
        rot = MCore::MCoreMatrixToQuaternion(rotMatrix);
    }


    //
    void Matrix::DecomposeQRGramSchmidt(AZ::Vector3& translation, AZ::Quaternion& rot) const
    {
        Matrix rotMatrix;
        DecomposeQRGramSchmidt(translation, rotMatrix);
        rot = MCore::MCoreMatrixToQuaternion(rotMatrix);
    }


    //
    void Matrix::DecomposeQRGramSchmidt(AZ::Vector3& translation, Matrix& rot, AZ::Vector3& scale, AZ::Vector3& shear) const
    {
        // build orthogonal matrix Q
        float invLength = Math::InvSqrt(TMAT(0, 0) * TMAT(0, 0) + TMAT(1, 0) * TMAT(1, 0) + TMAT(2, 0) * TMAT(2, 0));
        MMAT(rot, 0, 0) = TMAT(0, 0) * invLength;
        MMAT(rot, 1, 0) = TMAT(1, 0) * invLength;
        MMAT(rot, 2, 0) = TMAT(2, 0) * invLength;

        float fDot = MMAT(rot, 0, 0) * TMAT(0, 1) + MMAT(rot, 1, 0) * TMAT(1, 1) + MMAT(rot, 2, 0) * TMAT(2, 1);
        MMAT(rot, 0, 1) = TMAT(0, 1) - fDot * MMAT(rot, 0, 0);
        MMAT(rot, 1, 1) = TMAT(1, 1) - fDot * MMAT(rot, 1, 0);
        MMAT(rot, 2, 1) = TMAT(2, 1) - fDot * MMAT(rot, 2, 0);
        invLength = Math::InvSqrt(MMAT(rot, 0, 1) * MMAT(rot, 0, 1) + MMAT(rot, 1, 1) * MMAT(rot, 1, 1) + MMAT(rot, 2, 1) * MMAT(rot, 2, 1));
        MMAT(rot, 0, 1) *= invLength;
        MMAT(rot, 1, 1) *= invLength;
        MMAT(rot, 2, 1) *= invLength;

        fDot = MMAT(rot, 0, 0) * TMAT(0, 2) + MMAT(rot, 1, 0) * TMAT(1, 2) + MMAT(rot, 2, 0) * TMAT(2, 2);
        MMAT(rot, 0, 2) = TMAT(0, 2) - fDot * MMAT(rot, 0, 0);
        MMAT(rot, 1, 2) = TMAT(1, 2) - fDot * MMAT(rot, 1, 0);
        MMAT(rot, 2, 2) = TMAT(2, 2) - fDot * MMAT(rot, 2, 0);
        fDot = MMAT(rot, 0, 1) * TMAT(0, 2) + MMAT(rot, 1, 1) * TMAT(1, 2) + MMAT(rot, 2, 1) * TMAT(2, 2);
        MMAT(rot, 0, 2) -= fDot * MMAT(rot, 0, 1);
        MMAT(rot, 1, 2) -= fDot * MMAT(rot, 1, 1);
        MMAT(rot, 2, 2) -= fDot * MMAT(rot, 2, 1);
        invLength = Math::InvSqrt(MMAT(rot, 0, 2) * MMAT(rot, 0, 2) + MMAT(rot, 1, 2) * MMAT(rot, 1, 2) + MMAT(rot, 2, 2) * MMAT(rot, 2, 2));
        MMAT(rot, 0, 2) *= invLength;
        MMAT(rot, 1, 2) *= invLength;
        MMAT(rot, 2, 2) *= invLength;

        // guarantee that orthogonal matrix has determinant 1 (no reflections)
        float fDet =    MMAT(rot, 0, 0) * MMAT(rot, 1, 1) * MMAT(rot, 2, 2) + MMAT(rot, 0, 1) * MMAT(rot, 1, 2) * MMAT(rot, 2, 0) +
            MMAT(rot, 0, 2) * MMAT(rot, 1, 0) * MMAT(rot, 2, 1) - MMAT(rot, 0, 2) * MMAT(rot, 1, 1) * MMAT(rot, 2, 0) -
            MMAT(rot, 0, 1) * MMAT(rot, 1, 0) * MMAT(rot, 2, 2) - MMAT(rot, 0, 0) * MMAT(rot, 1, 2) * MMAT(rot, 2, 1);

        if (fDet < 0.0f)
        {
            for (uint32 r = 0; r < 3; ++r)
            {
                for (uint32 c = 0; c < 3; ++c)
                {
                    MMAT(rot, r, c) = -MMAT(rot, r, c);
                }
            }
        }

        // build "right" matrix R
        Matrix R;
        MMAT(R, 0, 0) = MMAT(rot, 0, 0) * TMAT(0, 0) + MMAT(rot, 1, 0) * TMAT(1, 0) + MMAT(rot, 2, 0) * TMAT(2, 0);
        MMAT(R, 0, 1) = MMAT(rot, 0, 0) * TMAT(0, 1) + MMAT(rot, 1, 0) * TMAT(1, 1) + MMAT(rot, 2, 0) * TMAT(2, 1);
        MMAT(R, 1, 1) = MMAT(rot, 0, 1) * TMAT(0, 1) + MMAT(rot, 1, 1) * TMAT(1, 1) + MMAT(rot, 2, 1) * TMAT(2, 1);
        MMAT(R, 0, 2) = MMAT(rot, 0, 0) * TMAT(0, 2) + MMAT(rot, 1, 0) * TMAT(1, 2) + MMAT(rot, 2, 0) * TMAT(2, 2);
        MMAT(R, 1, 2) = MMAT(rot, 0, 1) * TMAT(0, 2) + MMAT(rot, 1, 1) * TMAT(1, 2) + MMAT(rot, 2, 1) * TMAT(2, 2);
        MMAT(R, 2, 2) = MMAT(rot, 0, 2) * TMAT(0, 2) + MMAT(rot, 1, 2) * TMAT(1, 2) + MMAT(rot, 2, 2) * TMAT(2, 2);

        // the scaling component
        scale.SetX(MMAT(R, 0, 0));
        scale.SetY(MMAT(R, 1, 1));
        scale.SetZ(MMAT(R, 2, 2));

        // the shear component
        const float invScaleX = 1.0f / scale.GetX();
        shear.SetX(MMAT(R, 0, 1) * invScaleX);
        shear.SetY(MMAT(R, 0, 2) * invScaleX);
        shear.SetZ(MMAT(R, 1, 2) / scale.GetY());

        translation = GetTranslation();
    }


    //
    void Matrix::DecomposeQRGramSchmidt(AZ::Vector3& translation, Matrix& rot, AZ::Vector3& scale) const
    {
        // build orthogonal matrix Q
        float invLength = Math::InvSqrt(TMAT(0, 0) * TMAT(0, 0) + TMAT(1, 0) * TMAT(1, 0) + TMAT(2, 0) * TMAT(2, 0));
        MMAT(rot, 0, 0) = TMAT(0, 0) * invLength;
        MMAT(rot, 1, 0) = TMAT(1, 0) * invLength;
        MMAT(rot, 2, 0) = TMAT(2, 0) * invLength;

        float fDot = MMAT(rot, 0, 0) * TMAT(0, 1) + MMAT(rot, 1, 0) * TMAT(1, 1) + MMAT(rot, 2, 0) * TMAT(2, 1);
        MMAT(rot, 0, 1) = TMAT(0, 1) - fDot * MMAT(rot, 0, 0);
        MMAT(rot, 1, 1) = TMAT(1, 1) - fDot * MMAT(rot, 1, 0);
        MMAT(rot, 2, 1) = TMAT(2, 1) - fDot * MMAT(rot, 2, 0);
        invLength = Math::InvSqrt(MMAT(rot, 0, 1) * MMAT(rot, 0, 1) + MMAT(rot, 1, 1) * MMAT(rot, 1, 1) + MMAT(rot, 2, 1) * MMAT(rot, 2, 1));
        MMAT(rot, 0, 1) *= invLength;
        MMAT(rot, 1, 1) *= invLength;
        MMAT(rot, 2, 1) *= invLength;

        fDot = MMAT(rot, 0, 0) * TMAT(0, 2) + MMAT(rot, 1, 0) * TMAT(1, 2) + MMAT(rot, 2, 0) * TMAT(2, 2);
        MMAT(rot, 0, 2) = TMAT(0, 2) - fDot * MMAT(rot, 0, 0);
        MMAT(rot, 1, 2) = TMAT(1, 2) - fDot * MMAT(rot, 1, 0);
        MMAT(rot, 2, 2) = TMAT(2, 2) - fDot * MMAT(rot, 2, 0);
        fDot = MMAT(rot, 0, 1) * TMAT(0, 2) + MMAT(rot, 1, 1) * TMAT(1, 2) + MMAT(rot, 2, 1) * TMAT(2, 2);
        MMAT(rot, 0, 2) -= fDot * MMAT(rot, 0, 1);
        MMAT(rot, 1, 2) -= fDot * MMAT(rot, 1, 1);
        MMAT(rot, 2, 2) -= fDot * MMAT(rot, 2, 1);
        invLength = Math::InvSqrt(MMAT(rot, 0, 2) * MMAT(rot, 0, 2) + MMAT(rot, 1, 2) * MMAT(rot, 1, 2) + MMAT(rot, 2, 2) * MMAT(rot, 2, 2));
        MMAT(rot, 0, 2) *= invLength;
        MMAT(rot, 1, 2) *= invLength;
        MMAT(rot, 2, 2) *= invLength;

        // guarantee that orthogonal matrix has determinant 1 (no reflections)
        float fDet =    MMAT(rot, 0, 0) * MMAT(rot, 1, 1) * MMAT(rot, 2, 2) + MMAT(rot, 0, 1) * MMAT(rot, 1, 2) * MMAT(rot, 2, 0) +
            MMAT(rot, 0, 2) * MMAT(rot, 1, 0) * MMAT(rot, 2, 1) - MMAT(rot, 0, 2) * MMAT(rot, 1, 1) * MMAT(rot, 2, 0) -
            MMAT(rot, 0, 1) * MMAT(rot, 1, 0) * MMAT(rot, 2, 2) - MMAT(rot, 0, 0) * MMAT(rot, 1, 2) * MMAT(rot, 2, 1);

        if (fDet < 0.0f)
        {
            for (uint32 r = 0; r < 3; ++r)
            {
                for (uint32 c = 0; c < 3; ++c)
                {
                    MMAT(rot, r, c) = -MMAT(rot, r, c);
                }
            }
        }

        // build "right" matrix R
        Matrix R;
        MMAT(R, 0, 0) = MMAT(rot, 0, 0) * TMAT(0, 0) + MMAT(rot, 1, 0) * TMAT(1, 0) + MMAT(rot, 2, 0) * TMAT(2, 0);
        MMAT(R, 0, 1) = MMAT(rot, 0, 0) * TMAT(0, 1) + MMAT(rot, 1, 0) * TMAT(1, 1) + MMAT(rot, 2, 0) * TMAT(2, 1);
        MMAT(R, 1, 1) = MMAT(rot, 0, 1) * TMAT(0, 1) + MMAT(rot, 1, 1) * TMAT(1, 1) + MMAT(rot, 2, 1) * TMAT(2, 1);
        MMAT(R, 0, 2) = MMAT(rot, 0, 0) * TMAT(0, 2) + MMAT(rot, 1, 0) * TMAT(1, 2) + MMAT(rot, 2, 0) * TMAT(2, 2);
        MMAT(R, 1, 2) = MMAT(rot, 0, 1) * TMAT(0, 2) + MMAT(rot, 1, 1) * TMAT(1, 2) + MMAT(rot, 2, 1) * TMAT(2, 2);
        MMAT(R, 2, 2) = MMAT(rot, 0, 2) * TMAT(0, 2) + MMAT(rot, 1, 2) * TMAT(1, 2) + MMAT(rot, 2, 2) * TMAT(2, 2);

        // the scaling component
        scale.SetX(MMAT(R, 0, 0));
        scale.SetY(MMAT(R, 1, 1));
        scale.SetZ(MMAT(R, 2, 2));

        translation = GetTranslation();
    }


    // decompose into translation and rotation
    void Matrix::DecomposeQRGramSchmidt(AZ::Vector3& translation, Matrix& rot) const
    {
        // build orthogonal matrix Q
        float invLength = Math::InvSqrt(TMAT(0, 0) * TMAT(0, 0) + TMAT(1, 0) * TMAT(1, 0) + TMAT(2, 0) * TMAT(2, 0));
        MMAT(rot, 0, 0) = TMAT(0, 0) * invLength;
        MMAT(rot, 1, 0) = TMAT(1, 0) * invLength;
        MMAT(rot, 2, 0) = TMAT(2, 0) * invLength;

        float fDot = MMAT(rot, 0, 0) * TMAT(0, 1) + MMAT(rot, 1, 0) * TMAT(1, 1) + MMAT(rot, 2, 0) * TMAT(2, 1);
        MMAT(rot, 0, 1) = TMAT(0, 1) - fDot * MMAT(rot, 0, 0);
        MMAT(rot, 1, 1) = TMAT(1, 1) - fDot * MMAT(rot, 1, 0);
        MMAT(rot, 2, 1) = TMAT(2, 1) - fDot * MMAT(rot, 2, 0);
        invLength = Math::InvSqrt(MMAT(rot, 0, 1) * MMAT(rot, 0, 1) + MMAT(rot, 1, 1) * MMAT(rot, 1, 1) + MMAT(rot, 2, 1) * MMAT(rot, 2, 1));
        MMAT(rot, 0, 1) *= invLength;
        MMAT(rot, 1, 1) *= invLength;
        MMAT(rot, 2, 1) *= invLength;

        fDot = MMAT(rot, 0, 0) * TMAT(0, 2) + MMAT(rot, 1, 0) * TMAT(1, 2) + MMAT(rot, 2, 0) * TMAT(2, 2);
        MMAT(rot, 0, 2) = TMAT(0, 2) - fDot * MMAT(rot, 0, 0);
        MMAT(rot, 1, 2) = TMAT(1, 2) - fDot * MMAT(rot, 1, 0);
        MMAT(rot, 2, 2) = TMAT(2, 2) - fDot * MMAT(rot, 2, 0);
        fDot = MMAT(rot, 0, 1) * TMAT(0, 2) + MMAT(rot, 1, 1) * TMAT(1, 2) + MMAT(rot, 2, 1) * TMAT(2, 2);
        MMAT(rot, 0, 2) -= fDot * MMAT(rot, 0, 1);
        MMAT(rot, 1, 2) -= fDot * MMAT(rot, 1, 1);
        MMAT(rot, 2, 2) -= fDot * MMAT(rot, 2, 1);
        invLength = Math::InvSqrt(MMAT(rot, 0, 2) * MMAT(rot, 0, 2) + MMAT(rot, 1, 2) * MMAT(rot, 1, 2) + MMAT(rot, 2, 2) * MMAT(rot, 2, 2));
        MMAT(rot, 0, 2) *= invLength;
        MMAT(rot, 1, 2) *= invLength;
        MMAT(rot, 2, 2) *= invLength;

        // guarantee that orthogonal matrix has determinant 1 (no reflections)
        float fDet =    MMAT(rot, 0, 0) * MMAT(rot, 1, 1) * MMAT(rot, 2, 2) + MMAT(rot, 0, 1) * MMAT(rot, 1, 2) * MMAT(rot, 2, 0) +
            MMAT(rot, 0, 2) * MMAT(rot, 1, 0) * MMAT(rot, 2, 1) - MMAT(rot, 0, 2) * MMAT(rot, 1, 1) * MMAT(rot, 2, 0) -
            MMAT(rot, 0, 1) * MMAT(rot, 1, 0) * MMAT(rot, 2, 2) - MMAT(rot, 0, 0) * MMAT(rot, 1, 2) * MMAT(rot, 2, 1);

        if (fDet < 0.0f)
        {
            for (uint32 r = 0; r < 3; ++r)
            {
                for (uint32 c = 0; c < 3; ++c)
                {
                    MMAT(rot, r, c) = -MMAT(rot, r, c);
                }
            }
        }

        translation = GetTranslation();
    }

    /*
    // init from pos/rot/scale/shear
    void Matrix::Set(const Vector3& translation, const AZ::Quaternion& rotation, const Vector3& scale, const Vector3& shear)
    {
        // convert quat to matrix
        const float xx=rotation.x*rotation.x;
        const float xy=rotation.x*rotation.y, yy=rotation.y*rotation.y;
        const float xz=rotation.x*rotation.z, yz=rotation.y*rotation.z, zz=rotation.z*rotation.z;
        const float xw=rotation.x*rotation.w, yw=rotation.y*rotation.w, zw=rotation.z*rotation.w, ww=rotation.w*rotation.w;
        TMAT(0,0) = +xx-yy-zz+ww;   TMAT(0,1) = +xy+zw+xy+zw;   TMAT(0,2) = +xz-yw+xz-yw;   TMAT(0,3) = 0.0f;
        TMAT(1,0) = +xy-zw+xy-zw;   TMAT(1,1) = -xx+yy-zz+ww;   TMAT(1,2) = +yz+xw+yz+xw;   TMAT(1,3) = 0.0f;
        TMAT(2,0) = +xz+yw+xz+yw;   TMAT(2,1) = +yz-xw+yz-xw;   TMAT(2,2) = -xx-yy+zz+ww;   TMAT(2,3) = 0.0f;
        TMAT(3,0) = translation.x;  TMAT(3,1) = translation.y;  TMAT(3,2) = translation.z;  TMAT(3,3) = 1.0f;

        // scale
        TMAT(0,0) *= scale.x;
        TMAT(0,1) *= scale.y;
        TMAT(0,2) *= scale.z;
        TMAT(1,0) *= scale.x;
        TMAT(1,1) *= scale.y;
        TMAT(1,2) *= scale.z;
        TMAT(2,0) *= scale.x;
        TMAT(2,1) *= scale.y;
        TMAT(2,2) *= scale.z;

        // multiply with the shear matrix
        float v[3];
        v[0] = TMAT(0,0);
        v[1] = TMAT(0,1);
        v[2] = TMAT(0,2);
        TMAT(0,1) = v[0]*shear.x + v[1];
        TMAT(0,2) = v[0]*shear.y + v[1]*shear.z + v[2];

        v[0] = TMAT(1,0);
        v[1] = TMAT(1,1);
        v[2] = TMAT(1,2);
        TMAT(1,1) = v[0]*shear.x + v[1];
        TMAT(1,2) = v[0]*shear.y + v[1]*shear.z + v[2];

        v[0] = TMAT(2,0);
        v[1] = TMAT(2,1);
        v[2] = TMAT(2,2);
        TMAT(2,1) = v[0]*shear.x + v[1];
        TMAT(2,2) = v[0]*shear.y + v[1]*shear.z + v[2];

        // translation
        TMAT(3,0) = translation.x;
        TMAT(3,1) = translation.y;
        TMAT(3,2) = translation.z;
        TMAT(3,3) = 1.0f;
    }
    */

    //-------------------------------------------------------
    /*
    // decompose using QR decomposition (householder)
    void Matrix::DecomposeQRHouseHolder(Vector3& outTranslation, AZ::Quaternion& outRotation, Vector3& outScale, Vector3& outShear)
    {
        // extract translation
        outTranslation = GetTranslation();
        SetTranslation( Vector3(0.0f, 0.0f, 0.0f) );

        // decompose into the two matrices first
        Matrix Q;
        Matrix R;
        DecomposeQRHouseHolder(Q, R);
        SetTranslation( outTranslation );

        // extract scale
        outScale.Set( MMAT(R,0,0), MMAT(R,1,1), MMAT(R,2,2) );

        // extract shear
        const float invScaleX = 1.0f / outScale.x;          // TODO: handle 0 scale?
        outShear.x = MMAT(R, 0, 1) * invScaleX;
        outShear.y = MMAT(R, 0, 2) * invScaleX;
        outShear.z = MMAT(R, 1, 2) / outScale.y;

        // convert the rotation into a AZ::Quaternion
        outRotation.FromMatrix( Q );
    }



    // decompose using QR decomposition
    void Matrix::DecomposeQRHouseHolder(Vector3& outTranslation, AZ::Quaternion& outRotation, Vector3& outScale)
    {
        // extract translation
        outTranslation = GetTranslation();
        SetTranslation( Vector3(0.0f, 0.0f, 0.0f) );

        // decompose into the two matrices first
        Matrix Q;
        Matrix R;
        DecomposeQRHouseHolder(Q, R);
        SetTranslation( outTranslation );

        // extract scale
        outScale.Set( MMAT(R,0,0), MMAT(R,1,1), MMAT(R,2,2) );

        // convert the rotation into a AZ::Quaternion
        outRotation.FromMatrix( Q );
    }


    // decompose using QR decomposition
    void Matrix::DecomposeQRHouseHolder(Vector3& outTranslation, AZ::Quaternion& outRotation)
    {
        // extract translation
        outTranslation = GetTranslation();
        SetTranslation( Vector3(0.0f, 0.0f, 0.0f) );

        // decompose into the two matrices first
        Matrix Q;
        Matrix R;
        DecomposeQRHouseHolder(Q, R);
        SetTranslation( outTranslation );

        // convert the rotation into a AZ::Quaternion
        outRotation.FromMatrix( Q );
    }


    // decompose into Q (rotation) and R (scale/shear/translation) matrices
    void Matrix::DecomposeQRHouseHolder(Matrix& Q, Matrix& R)
    {
        float mag;
        float alpha;
        Vector4 u;
        Vector4 v;
        Matrix P;
        Matrix I;

        I.Identity();
        P.Identity();

        Q.Identity();
        R = *this;

        for (uint32 i=0; i<4; i++)
        {
            u.Zero();
            v.Zero();

            mag = 0.0f;
            for (uint32 j=i; j<4; ++j)
            {
                u[j] = MMAT(R, j, i);
                mag += u[j] * u[j];
            }

            mag = Math::SafeSqrt(mag);
            alpha = u[i] < 0 ? mag : -mag;

            mag = 0.0f;
            for (uint32 j=i; j<4; ++j)
            {
                v[j] = (j == i) ? u[j] + alpha : u[j];
                mag += v[j] * v[j];
            }

            mag = Math::SafeSqrt(mag);
            if (mag < Math::epsilon)
                continue;

            const float invMag = 1.0f / mag;
            for (uint32 j=i; j<4; j++)
                v[j] *= invMag;

            //P = I - (v * v.Transpose()) * 2.0;
            P = I - OuterProduct(v, v) * 2.0f;

            //R = P * R;
            //Q = Q * P;
            R.MultMatrix(P, R);
            Q.MultMatrix(P);
        }
    }
    //-------------------------------------------------------
    */

    // basically does (vecA * vecB.Transposed()) and results in a 4x4 matrix
    Matrix Matrix::OuterProduct(const AZ::Vector4& column, const AZ::Vector4& row)
    {
        Matrix result;

        for (uint32 r = 0; r < 4; ++r)
        {
            for (uint32 c = 0; c < 4; ++c)
            {
                MMAT(result, r, c) = column.GetElement(r) * row.GetElement(c);
            }
        }

        return result;
    }
}   // namespace MCore
