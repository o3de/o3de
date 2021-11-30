/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

MCORE_INLINE Matrix::Matrix(const Matrix& m)
{
    MCore::MemCopy(m_m16, m.m_m16, sizeof(Matrix));
}


MCORE_INLINE void Matrix::operator = (const Matrix& right)
{
    MCore::MemCopy(m_m16, right.m_m16, sizeof(Matrix));
}



MCORE_INLINE void Matrix::SetRight(float xx, float xy, float xz)
{
    TMAT(0, 0) = xx;
    TMAT(0, 1) = xy;
    TMAT(0, 2) = xz;
}



MCORE_INLINE void Matrix::SetUp(float yx, float yy, float yz)
{
    TMAT(1, 0) = yx;
    TMAT(1, 1) = yy;
    TMAT(1, 2) = yz;
}



MCORE_INLINE void Matrix::SetForward(float zx, float zy, float zz)
{
    TMAT(2, 0) = zx;
    TMAT(2, 1) = zy;
    TMAT(2, 2) = zz;
}



MCORE_INLINE void Matrix::SetTranslation(float tx, float ty, float tz)
{
    TMAT(3, 0) = tx;
    TMAT(3, 1) = ty;
    TMAT(3, 2) = tz;
}



MCORE_INLINE void Matrix::SetRight(const AZ::Vector3& x)
{
    TMAT(0, 0) = x.GetX();
    TMAT(0, 1) = x.GetY();
    TMAT(0, 2) = x.GetZ();
}



MCORE_INLINE void Matrix::SetUp(const AZ::Vector3& y)
{
    TMAT(1, 0) = y.GetX();
    TMAT(1, 1) = y.GetY();
    TMAT(1, 2) = y.GetZ();
}



MCORE_INLINE void Matrix::SetForward(const AZ::Vector3& z)
{
    TMAT(2, 0) = z.GetX();
    TMAT(2, 1) = z.GetY();
    TMAT(2, 2) = z.GetZ();
}



MCORE_INLINE void Matrix::SetTranslation(const AZ::Vector3& t)
{
    TMAT(3, 0) = t.GetX();
    TMAT(3, 1) = t.GetY();
    TMAT(3, 2) = t.GetZ();
}



MCORE_INLINE AZ::Vector3 Matrix::GetRight() const
{
    //return *reinterpret_cast<Vector3*>( m16/* + 0*/);
    return AZ::Vector3(TMAT(0, 0), TMAT(0, 1), TMAT(0, 2));
}



MCORE_INLINE AZ::Vector3 Matrix::GetForward() const
{
    //  return *reinterpret_cast<Vector3*>(m16+4);
    return AZ::Vector3(TMAT(1, 0), TMAT(1, 1), TMAT(1, 2));
}



MCORE_INLINE AZ::Vector3 Matrix::GetUp() const
{
    //  return *reinterpret_cast<Vector3*>(m16+8);
    return AZ::Vector3(TMAT(2, 0), TMAT(2, 1), TMAT(2, 2));
}



MCORE_INLINE AZ::Vector3 Matrix::GetTranslation() const
{
    //return *reinterpret_cast<Vector3*>(m16+12);
    return AZ::Vector3(TMAT(3, 0), TMAT(3, 1), TMAT(3, 2));
}



MCORE_INLINE AZ::Vector3 Matrix::Mul3x3(const AZ::Vector3& v) const
{
    return AZ::Vector3(
        v.GetX() * TMAT(0, 0) + v.GetY() * TMAT(1, 0) + v.GetZ() * TMAT(2, 0),
        v.GetX() * TMAT(0, 1) + v.GetY() * TMAT(1, 1) + v.GetZ() * TMAT(2, 1),
        v.GetX() * TMAT(0, 2) + v.GetY() * TMAT(1, 2) + v.GetZ() * TMAT(2, 2));
}



MCORE_INLINE void operator *= (AZ::Vector3& v, const Matrix& m)
{
    v = AZ::Vector3(
        v.GetX() * MMAT(m, 0, 0) + v.GetY() * MMAT(m, 1, 0) + v.GetZ() * MMAT(m, 2, 0) + MMAT(m, 3, 0),
        v.GetX() * MMAT(m, 0, 1) + v.GetY() * MMAT(m, 1, 1) + v.GetZ() * MMAT(m, 2, 1) + MMAT(m, 3, 1),
        v.GetX() * MMAT(m, 0, 2) + v.GetY() * MMAT(m, 1, 2) + v.GetZ() * MMAT(m, 2, 2) + MMAT(m, 3, 2));
}



MCORE_INLINE void operator *= (AZ::Vector4& v, const Matrix& m)
{
    v = AZ::Vector4(
            v.GetX() * MMAT(m, 0, 0) + v.GetY() * MMAT(m, 1, 0) + v.GetZ() * MMAT(m, 2, 0) + v.GetW() * MMAT(m, 3, 0),
            v.GetX() * MMAT(m, 0, 1) + v.GetY() * MMAT(m, 1, 1) + v.GetZ() * MMAT(m, 2, 1) + v.GetW() * MMAT(m, 3, 1),
            v.GetX() * MMAT(m, 0, 2) + v.GetY() * MMAT(m, 1, 2) + v.GetZ() * MMAT(m, 2, 2) + v.GetW() * MMAT(m, 3, 2),
            v.GetX() * MMAT(m, 0, 3) + v.GetY() * MMAT(m, 1, 3) + v.GetZ() * MMAT(m, 2, 3) + v.GetW() * MMAT(m, 3, 3));
}



MCORE_INLINE AZ::Vector3 operator * (const AZ::Vector3& v, const Matrix& m)
{
    return AZ::Vector3(
        v.GetX() * MMAT(m, 0, 0) + v.GetY() * MMAT(m, 1, 0) + v.GetZ() * MMAT(m, 2, 0) + MMAT(m, 3, 0),
        v.GetX() * MMAT(m, 0, 1) + v.GetY() * MMAT(m, 1, 1) + v.GetZ() * MMAT(m, 2, 1) + MMAT(m, 3, 1),
        v.GetX() * MMAT(m, 0, 2) + v.GetY() * MMAT(m, 1, 2) + v.GetZ() * MMAT(m, 2, 2) + MMAT(m, 3, 2));
}




// skin a vertex position
MCORE_INLINE void Matrix::Skin4x3(const AZ::Vector3& in, AZ::Vector3& out, float weight)
{
    out.Set(
        out.GetX() + (in.GetX() * TMAT(0, 0) + in.GetY() * TMAT(1, 0) + in.GetZ() * TMAT(2, 0) + TMAT(3, 0)) * weight,
        out.GetY() + (in.GetX() * TMAT(0, 1) + in.GetY() * TMAT(1, 1) + in.GetZ() * TMAT(2, 1) + TMAT(3, 1)) * weight,
        out.GetZ() + (in.GetX() * TMAT(0, 2) + in.GetY() * TMAT(1, 2) + in.GetZ() * TMAT(2, 2) + TMAT(3, 2)) * weight
    );
}


// skin a position and normal
MCORE_INLINE void Matrix::Skin(const AZ::Vector3* inPos, const AZ::Vector3* inNormal, AZ::Vector3* outPos, AZ::Vector3* outNormal, float weight)
{
    const float mat00 = TMAT(0, 0);
    const float mat10 = TMAT(1, 0);
    const float mat20 = TMAT(2, 0);
    const float mat30 = TMAT(3, 0);
    const float mat01 = TMAT(0, 1);
    const float mat11 = TMAT(1, 1);
    const float mat21 = TMAT(2, 1);
    const float mat31 = TMAT(3, 1);
    const float mat02 = TMAT(0, 2);
    const float mat12 = TMAT(1, 2);
    const float mat22 = TMAT(2, 2);
    const float mat32 = TMAT(3, 2);

    outPos->Set(
        outPos->GetX() + (inPos->GetX() * mat00 + inPos->GetY() * mat10 + inPos->GetZ() * mat20 + mat30) * weight,
        outPos->GetY() + (inPos->GetX() * mat01 + inPos->GetY() * mat11 + inPos->GetZ() * mat21 + mat31) * weight,
        outPos->GetZ() + (inPos->GetX() * mat02 + inPos->GetY() * mat12 + inPos->GetZ() * mat22 + mat32) * weight
    );

    outNormal->Set(
        outNormal->GetX() + (inNormal->GetX() * mat00 + inNormal->GetY() * mat10 + inNormal->GetZ() * mat20) * weight,
        outNormal->GetY() + (inNormal->GetX() * mat01 + inNormal->GetY() * mat11 + inNormal->GetZ() * mat21) * weight,
        outNormal->GetZ() + (inNormal->GetX() * mat02 + inNormal->GetY() * mat12 + inNormal->GetZ() * mat22) * weight
    );
}


// skin a position, normal, and tangent
MCORE_INLINE void Matrix::Skin(const AZ::Vector3* inPos, const AZ::Vector3* inNormal, const AZ::Vector4* inTangent, AZ::Vector3* outPos, AZ::Vector3* outNormal, AZ::Vector4* outTangent, float weight)
{
    const float mat00 = TMAT(0, 0);
    const float mat10 = TMAT(1, 0);
    const float mat20 = TMAT(2, 0);
    const float mat30 = TMAT(3, 0);
    const float mat01 = TMAT(0, 1);
    const float mat11 = TMAT(1, 1);
    const float mat21 = TMAT(2, 1);
    const float mat31 = TMAT(3, 1);
    const float mat02 = TMAT(0, 2);
    const float mat12 = TMAT(1, 2);
    const float mat22 = TMAT(2, 2);
    const float mat32 = TMAT(3, 2);

    outPos->Set(
        outPos->GetX() + (inPos->GetX() * mat00 + inPos->GetY() * mat10 + inPos->GetZ() * mat20 + mat30) * weight,
        outPos->GetY() + (inPos->GetX() * mat01 + inPos->GetY() * mat11 + inPos->GetZ() * mat21 + mat31) * weight,
        outPos->GetZ() + (inPos->GetX() * mat02 + inPos->GetY() * mat12 + inPos->GetZ() * mat22 + mat32) * weight
    );

    outNormal->Set(
        outNormal->GetX() + (inNormal->GetX() * mat00 + inNormal->GetY() * mat10 + inNormal->GetZ() * mat20) * weight,
        outNormal->GetY() + (inNormal->GetX() * mat01 + inNormal->GetY() * mat11 + inNormal->GetZ() * mat21) * weight,
        outNormal->GetZ() + (inNormal->GetX() * mat02 + inNormal->GetY() * mat12 + inNormal->GetZ() * mat22) * weight
    );

    outTangent->Set(
        outTangent->GetX() + (inTangent->GetX() * mat00 + inTangent->GetY() * mat10 + inTangent->GetZ() * mat20) * weight,
        outTangent->GetY() + (inTangent->GetX() * mat01 + inTangent->GetY() * mat11 + inTangent->GetZ() * mat21) * weight,
        outTangent->GetZ() + (inTangent->GetX() * mat02 + inTangent->GetY() * mat12 + inTangent->GetZ() * mat22) * weight,
        inTangent->GetW()
    );
}

// skin a position, normal, and tangent and bitangent
MCORE_INLINE void Matrix::Skin(const AZ::Vector3* inPos, const AZ::Vector3* inNormal, const AZ::Vector4* inTangent, const AZ::Vector3* inBitangent, AZ::Vector3* outPos, AZ::Vector3* outNormal, AZ::Vector4* outTangent, AZ::Vector3* outBitangent, float weight)
{
    const float mat00 = TMAT(0, 0);
    const float mat10 = TMAT(1, 0);
    const float mat20 = TMAT(2, 0);
    const float mat30 = TMAT(3, 0);
    const float mat01 = TMAT(0, 1);
    const float mat11 = TMAT(1, 1);
    const float mat21 = TMAT(2, 1);
    const float mat31 = TMAT(3, 1);
    const float mat02 = TMAT(0, 2);
    const float mat12 = TMAT(1, 2);
    const float mat22 = TMAT(2, 2);
    const float mat32 = TMAT(3, 2);

    outPos->Set(
        outPos->GetX() + (inPos->GetX() * mat00 + inPos->GetY() * mat10 + inPos->GetZ() * mat20 + mat30) * weight,
        outPos->GetY() + (inPos->GetX() * mat01 + inPos->GetY() * mat11 + inPos->GetZ() * mat21 + mat31) * weight,
        outPos->GetZ() + (inPos->GetX() * mat02 + inPos->GetY() * mat12 + inPos->GetZ() * mat22 + mat32) * weight
    );

    outNormal->Set(
        outNormal->GetX() + (inNormal->GetX() * mat00 + inNormal->GetY() * mat10 + inNormal->GetZ() * mat20) * weight,
        outNormal->GetY() + (inNormal->GetX() * mat01 + inNormal->GetY() * mat11 + inNormal->GetZ() * mat21) * weight,
        outNormal->GetZ() + (inNormal->GetX() * mat02 + inNormal->GetY() * mat12 + inNormal->GetZ() * mat22) * weight
    );

    outTangent->Set(
        outTangent->GetX() + (inTangent->GetX() * mat00 + inTangent->GetY() * mat10 + inTangent->GetZ() * mat20) * weight,
        outTangent->GetY() + (inTangent->GetX() * mat01 + inTangent->GetY() * mat11 + inTangent->GetZ() * mat21) * weight,
        outTangent->GetZ() + (inTangent->GetX() * mat02 + inTangent->GetY() * mat12 + inTangent->GetZ() * mat22) * weight,
        inTangent->GetW()
    );

    outBitangent->Set(
        outBitangent->GetX() + (inBitangent->GetX() * mat00 + inBitangent->GetY() * mat10 + inBitangent->GetZ() * mat20) * weight,
        outBitangent->GetY() + (inBitangent->GetX() * mat01 + inBitangent->GetY() * mat11 + inBitangent->GetZ() * mat21) * weight,
        outBitangent->GetZ() + (inBitangent->GetX() * mat02 + inBitangent->GetY() * mat12 + inBitangent->GetZ() * mat22) * weight
    );
}


// skin a normal
MCORE_INLINE void Matrix::Skin3x3(const AZ::Vector3& in, AZ::Vector3& out, float weight)
{
    out.Set(
        out.GetX() + (in.GetX() * TMAT(0, 0) + in.GetY() * TMAT(1, 0) + in.GetZ() * TMAT(2, 0)) * weight,
        out.GetY() + (in.GetX() * TMAT(0, 1) + in.GetY() * TMAT(1, 1) + in.GetZ() * TMAT(2, 1)) * weight,
        out.GetZ() + (in.GetX() * TMAT(0, 2) + in.GetY() * TMAT(1, 2) + in.GetZ() * TMAT(2, 2)) * weight
    );
}


// multiply by a float
MCORE_INLINE Matrix& Matrix::operator *= (float value)
{
    for (uint32 i = 0; i < 16; ++i)
    {
        m_m16[i] *= value;
    }

    return *this;
}


// scale (uniform)
MCORE_INLINE void Matrix::Scale(const AZ::Vector3& scale)
{
    for (uint32 i = 0; i < 4; ++i)
    {
        TMAT(i, 0) *= scale.GetX();
        TMAT(i, 1) *= scale.GetY();
        TMAT(i, 2) *= scale.GetZ();
    }
}


// returns a normalized version of this matrix
Matrix Matrix::Normalized() const
{
    Matrix result(*this);
    result.Normalize();
    return result;
}

