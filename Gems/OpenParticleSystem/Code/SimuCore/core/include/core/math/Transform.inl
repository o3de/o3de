/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef _MSC_VER
#pragma warning(push)
#endif

namespace SimuCore {
    inline Transform::Transform(const Vector3& trans, const Quaternion& rot, const Vector3& s)
        : translation(trans),
          rotation(rot),
          scale(s)
    {
    }

    inline Transform Transform::operator*(const Transform& rhs) const
    {
        Transform result;
        result.rotation = rotation * rhs.rotation;
        result.scale = scale * rhs.scale;
        result.translation = TransformPoint(rhs.translation);
        return result;
    }

    inline Transform& Transform::operator*=(const Transform& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    inline bool Transform::operator==(const Transform& rhs) const
    {
        return (translation == rhs.translation && scale == rhs.scale) && rotation == rhs.rotation;
    }

    inline bool Transform::operator!=(const Transform& rhs) const
    {
        return !operator==(rhs);
    }

    /**
     *       | r00sx  r01sy  r02sz   px  |   | 1  0  0  px |   | r00  r01  r02  0 |   | sx  0   0   0  |
     *   M = | r10sx  r11sy  r12sz   py  | = | 0  1  0  py | * | r10  r11  r12  0 | * | 0   sy  0   0  |
     *       | r20sx  r21sy  r22sz   pz  |   | 0  0  1  pz |   | r20  r21  r22  0 |   | 0   0   sz  0  |
     *       |   0      0       0    1   |   | 0  0  0  1  |   |  0    0    0   1 |   | 0   0   0   1  |
     *
     *       | r00sx  r01sy  r02sz |   | r00  r01  r02 |   | sx  0   0  |
     *   M = | r10sx  r11sy  r12sz | = | r10  r11  r12 | * | 0   sy  0  | = R * S
     *       | r20sx  r21sy  r22sz |   | r20  r21  r22 |   | 0   0   sz |
     *
     *   R is orthogonal and S is diagonal.
     *
     *   M = [ M0 M1 M2 ]     R = [ R0 R1 R2 ]
     *
     *   R0 = M0 / |M0|
     *
     *   R1 = M1 - ( R0 * M1 ) * R0
     *   R1 = R1 / |R1|
     *
     *   R2 = M2 - ( R0 * M2 ) * R0 - ( R1 * M2 ) * R1
     *   R2 = R2 / |R2|
     *
     *   because Ri * Rj = 0
     *                     | R0 * M0   R0 * M1   R0 * M2 |
     *   so  S = R-1 * M = |    0      R1 * M1   R1 * M2 |
     *                     |    0         0      R2 * M2 |
     */
    inline void Transform::FromMatrix(const Matrix4& trans)
    {
        translation.x = trans[3][0];
        translation.y = trans[3][1];
        translation.z = trans[3][2];

        Matrix3 rotMatrix;

        // R0
        float fInv = std::sqrt(trans[0][0] * trans[0][0] + trans[0][1] * trans[0][1] + trans[0][2] * trans[0][2]);
        rotMatrix[0][0] = trans[0][0] / fInv;
        rotMatrix[0][1] = trans[0][1] / fInv;
        rotMatrix[0][2] = trans[0][2] / fInv;

        // R1
        float fDot = rotMatrix[0][0] * trans[1][0] + rotMatrix[0][1] * trans[1][1] + rotMatrix[0][2] * trans[1][2];
        rotMatrix[1][0] = trans[1][0] - fDot * rotMatrix[0][0];
        rotMatrix[1][1] = trans[1][1] - fDot * rotMatrix[0][1];
        rotMatrix[1][2] = trans[1][2] - fDot * rotMatrix[0][2];
        fInv = std::sqrt(
            rotMatrix[1][0] * rotMatrix[1][0] + rotMatrix[1][1] * rotMatrix[1][1] + rotMatrix[1][2] * rotMatrix[1][2]);
        rotMatrix[1][0] = rotMatrix[1][0] / fInv;
        rotMatrix[1][1] = rotMatrix[1][1] / fInv;
        rotMatrix[1][2] = rotMatrix[1][2] / fInv;

        // R2
        fDot = rotMatrix[0][0] * trans[2][0] + rotMatrix[0][1] * trans[2][1] + rotMatrix[0][2] * trans[2][2];
        rotMatrix[2][0] = trans[2][0] - fDot * rotMatrix[0][0];
        rotMatrix[2][1] = trans[2][1] - fDot * rotMatrix[0][1];
        rotMatrix[2][2] = trans[2][2] - fDot * rotMatrix[0][2];
        fDot = rotMatrix[1][0] * trans[2][0] + rotMatrix[1][1] * trans[2][1] + rotMatrix[1][2] * trans[2][2];

        rotMatrix[2][0] -= fDot * rotMatrix[1][0];
        rotMatrix[2][1] -= fDot * rotMatrix[1][1];
        rotMatrix[2][2] -= fDot * rotMatrix[1][2];
        fInv = std::sqrt(
            rotMatrix[2][0] * rotMatrix[2][0] + rotMatrix[2][1] * rotMatrix[2][1] + rotMatrix[2][2] * rotMatrix[2][2]);
        rotMatrix[2][0] = rotMatrix[2][0] / fInv;
        rotMatrix[2][1] = rotMatrix[2][1] / fInv;
        rotMatrix[2][2] = rotMatrix[2][2] / fInv;

        if (rotMatrix.Determinant() < 0.0f) {
            for (size_t i = 0; i < 3; i++) {
                for (size_t j = 0; j < 3; j++) {
                    rotMatrix[i][j] = -rotMatrix[i][j];
                }
            }
        }

        rotation.FromMatrix3(rotMatrix);

        scale.x = rotMatrix[0][0] * trans[0][0] + rotMatrix[0][1] * trans[0][1] + rotMatrix[0][2] * trans[0][2];
        scale.y = rotMatrix[1][0] * trans[1][0] + rotMatrix[1][1] * trans[1][1] + rotMatrix[1][2] * trans[1][2];
        scale.z = rotMatrix[2][0] * trans[2][0] + rotMatrix[2][1] * trans[2][1] + rotMatrix[2][2] * trans[2][2];
    }

    inline Matrix4 Transform::ToMatrix() const
    {
        Matrix4 trans = MAT4_IDENTITY;
        trans[3][0] = translation.x;
        trans[3][1] = translation.y;
        trans[3][2] = translation.z;
        trans[3][3] = 1.0f;

        Matrix4 rot = MAT4_IDENTITY;
        rot = rotation.ToMatrix();

        Matrix4 sc = MAT4_IDENTITY;
        sc[0][0] = scale.x;
        sc[1][1] = scale.y;
        sc[2][2] = scale.z;
        sc[3][3] = 1.0f;

        return trans * rot * sc;
    }

    inline Transform Transform::Inverse() const
    {
        Transform out;
        out.rotation = rotation.GetConjugate();
        out.scale = Vector3(1.0f) / scale;
        Vector3 tempScale = {-out.scale.x, -out.scale.y, -out.scale.z};
        out.translation = tempScale * (out.rotation * (translation));
        return out;
    }

    inline Vector3 Transform::TransformPoint(const Vector3& rhs) const
    {
        return rotation * ((scale * rhs)) + translation;
    }

    inline Vector3 Transform::TransformVector(const Vector3& rhs) const
    {
        return rotation * ((scale * rhs));
    }

    inline const Vector3& Transform::GetTranslation() const
    {
        return translation;
    }

    inline const Quaternion& Transform::GetRotation() const
    {
        return rotation;
    }

    inline const Vector3& Transform::GetUniformScale() const
    {
        return scale;
    }

    inline void Transform::LookAt(const Vector3& start, const Vector3& target, const Vector3& yAxisUp)
    {
        Vector3 w = target - start;
        (void)w.Normalize();
        Vector3 normUp = yAxisUp;
        (void)normUp.Normalize();
        if (std::abs(w.Dot(normUp)) > ALMOST_ONE) {
            normUp = { normUp.z, normUp.x, normUp.y };
        }
        Vector3 u = w.Cross(normUp);
        (void)u.Normalize();
        Vector3 v = u.Cross(w);
        (void)v.Normalize();

        Matrix4 mat{
            Vector4(u.x, v.x, -w.x, 0.0f),
            Vector4(u.y, v.y, -w.y, 0.0f),
            Vector4(u.z, v.z, -w.z, 0.0f),
            Vector4(-u.Dot(start), -v.Dot(start), w.Dot(start), 1.0f),
        };
        Matrix4 mInverse = mat.Inverse();

        Vector3 sc = scale;
        FromMatrix(mInverse);
        scale = sc;
    }
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif
