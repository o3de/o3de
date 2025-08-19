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
    inline Quaternion::Quaternion()
    {
        detail::Calculator<MemType>::SetValue(value, 0, 0, 0, 1);
    }

    inline Quaternion::Quaternion(float qx, float qy, float qz, float qw)
    {
        detail::Calculator<MemType>::SetValue(value, qx, qy, qz, qw);
    }

    inline Quaternion::Quaternion(const Vector3& axis, float angle)
    {
        Vector3 tmpAxis = Vector3(axis.x, axis.y, axis.z);
        (void)tmpAxis.Normalize();
        float length = tmpAxis.Length();
        if (length > 0.0f) {
            float halfAngle = angle / 2.f;
            float s = sin(halfAngle) / length;
            float c = cos(halfAngle);
            Vector4 v1(tmpAxis.x, tmpAxis.y, tmpAxis.z, 1.f);
            Vector4 v2(s, s, s, c);
            value = detail::Calculator<MemType>::Mul(v1.value, v2.value);
        } else {
            detail::Calculator<MemType>::SetValue(value, 0, 0, 0, 1);
        }
    }

    inline Quaternion::Quaternion(const Quaternion& q)
    {
        detail::Calculator<MemType>::SetValue(value, q.x, q.y, q.z, q.w);
    }

    inline Quaternion& Quaternion::operator=(const Quaternion& q)
    {
        detail::Calculator<MemType>::SetValue(value, q.x, q.y, q.z, q.w);
        return *this;
    }

    inline Quaternion& Quaternion::operator*=(const Quaternion& q)
    {
        Quaternion a(*this);
        Quaternion b(q);

        w = a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
        x = a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
        y = a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z;
        z = a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x;
        return *this;
    }

    inline Quaternion& Quaternion::operator+=(const Quaternion& q)
    {
        detail::Calculator<MemType>::SetValue(value, x + q.x, y + q.y, z + q.z, w + q.w);
        return *this;
    }

    inline Quaternion Quaternion::operator*(const float scale) const
    {
        return Quaternion(x * scale, y * scale, z * scale, w * scale);
    }

    inline Quaternion Quaternion::operator+(const Quaternion& q) const
    {
        return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
    }

    inline Quaternion Quaternion::operator-(const Quaternion& q) const
    {
        return Quaternion(x - q.x, y - q.y, z - q.z, w - q.w);
    }

    inline Quaternion Quaternion::operator-() const
    {
        return Quaternion(-x, -y, -z, -w);
    }

    inline bool Quaternion::operator==(const Quaternion& q) const
    {
        return (Math::Equal(this->w, q.w) &&
                Math::Equal(this->x, q.x) &&
                Math::Equal(this->y, q.y) &&
                Math::Equal(this->z, q.z));
    }

    inline bool Quaternion::operator!=(const Quaternion& q) const
    {
        return !(*this == q);
    }

    inline Vector3 Quaternion::operator*(const Vector3& v) const
    {
        Vector3 qv(x, y, z);
        Vector3 v1(Cross(qv, v));
        Vector3 v2(Cross(qv, v1));
        return v + ((v1 * w) + v2) * 2.f;
    }
    
    inline Quaternion& Quaternion::Normalize()
    {
        float lenSquared = w * w + x * x + y * y + z * z;
        if (lenSquared < Math::EPSLON) {
            detail::Calculator<MemType>::SetValue(value, 0, 0, 0, 1);
            return *this;
        }

        value = detail::Calculator<MemType>::Normalize(value);
        return *this;
    }

    inline void Quaternion::FromMatrix3(const Matrix3& m)
    {
        float traceM = m[0][0] + m[1][1] + m[2][2];
        if (traceM > 0.0f) {
            float root = std::sqrt(traceM + 1.0f);
            w = 0.5f * root;
            root = 0.5f / root;
            x = (m[1][2] - m[2][1]) * root;
            y = (m[2][0] - m[0][2]) * root;
            z = (m[0][1] - m[1][0]) * root;
        } else {
            static size_t iNext[3] = { 1, 2, 0 };
            size_t i = 0;
            if (m[1][1] > m[0][0]) {
                i = 1;
            }
            if (m[2][2] > m[i][i]) {
                i = 2;
            }
            size_t j = iNext[i];
            size_t k = iNext[j];

            float root = std::sqrt(m[i][i] - m[j][j] - m[k][k] + 1.0f);
            float* tempQua[3] = { &x, &y, &z };
            *tempQua[i] = 0.5f * root;
            root = 0.5f / root;
            w = (m[j][k] - m[k][j]) * root;
            *tempQua[j] = (m[i][j] + m[j][i]) * root;
            *tempQua[k] = (m[i][k] + m[k][i]) * root;
        }
        (void)Normalize();
    }

    inline Matrix3 Quaternion::ToMatrix3() const
    {
        Matrix3 matrix;
        float x2 = 2.0f * x;  // 2x
        float y2 = 2.0f * y;
        float z2 = 2.0f * z;
        float w2x = w * x2;  // 2wx
        float w2y = w * y2;
        float w2z = w * z2;
        float x2x = x * x2;
        float x2y = x * y2;
        float x2z = x * z2;
        float y2y = y * y2;
        float y2z = y * z2;
        float z2z = z * z2;

        matrix[0][0] = 1.0f - y2y - z2z;
        matrix[1][0] = x2y - w2z;
        matrix[2][0] = x2z + w2y;
        matrix[0][1] = x2y + w2z;
        matrix[1][1] = 1.0f - x2x - z2z;
        matrix[2][1] = y2z - w2x;
        matrix[0][2] = x2z - w2y;
        matrix[1][2] = y2z + w2x;
        matrix[2][2] = 1.0f - x2x - y2y;
        return matrix;
    }

    inline Matrix4 Quaternion::ToMatrix() const
    {
        Matrix4 matrix;
        float x2 = 2.0f * x;  // 2x
        float y2 = 2.0f * y;
        float z2 = 2.0f * z;
        float w2x = w * x2;   // 2wx
        float w2y = w * y2;
        float w2z = w * z2;
        float x2x = x * x2;
        float x2y = x * y2;
        float x2z = x * z2;
        float y2y = y * y2;
        float y2z = y * z2;
        float z2z = z * z2;

        matrix[0][0] = 1.0f - y2y - z2z;
        matrix[1][0] = x2y - w2z;
        matrix[2][0] = x2z + w2y;
        matrix[3][0] = 0.0f;

        matrix[0][1] = x2y + w2z;
        matrix[1][1] = 1.0f - x2x - z2z;
        matrix[2][1] = y2z - w2x;
        matrix[3][1] = 0.0f;

        matrix[0][2] = x2z - w2y;
        matrix[1][2] = y2z + w2x;
        matrix[2][2] = 1.0f - x2x - y2y;
        matrix[3][2] = 0.0f;

        matrix[0][3] = 0.0f;
        matrix[1][3] = 0.0f;
        matrix[2][3] = 0.0f;
        matrix[3][3] = 1.0f;
        return matrix;
    }

    inline Quaternion Quaternion::GetConjugate() const
    {
        Quaternion result;
        result.value = detail::Calculator<MemType>::GetConjugate(value);
        return result;
    }

    inline Quaternion Quaternion::Inverse() const
    {
        float lenSquared = w * w + x * x + y * y + z * z;
        if (lenSquared > 0.0f) {
            return Quaternion(-x / lenSquared, -y / lenSquared, -z / lenSquared, w / lenSquared);
        } else {
            return Quaternion();
        }
    }

    inline void Quaternion::Slerp(const Quaternion& right, float factor)
    {
        // NB: left is *this
        if (factor >= 1.f) {
            (*this) = right;
            return;
        } else if (factor <= 0.f) {
            return;
        }

        // Compute the cosine of the angle between this and target.
        float dot = x * right.x + y * right.y + z * right.z + w * right.w;

        // If the dot product is negative, slerp won't take
        // the shorter path. Fix by reversing one quaternion.
        if (dot < 0.f) {
            (*this) = (*this) * -1.0f;
            dot = -dot;
        }

        // If the inputs are too close, only linearly interpolate the result.
        if (dot > 0.9999999f) { // dot threshold
            (*this) += (right - (*this)) * factor; // q0 + t * (q1 - q0)
            (void)Normalize();
            return;
        }

        double angle0 = std::acos(dot);
        double angle1 = angle0 * factor;
        double sinAngle0 = std::sin(angle0);
        double sinAngle1 = std::sin(angle1);
        double s0 = std::cos(angle1) - dot * sinAngle1 / sinAngle0;
        double s1 = sinAngle1 / sinAngle0;

        (*this) = (*this) * static_cast<float>(s0);
        Quaternion v1(right);
        v1 = v1 * static_cast<float>(s1);
        (*this) = (*this) + v1;
        (void)Normalize();
    }

    inline Vector3 Quaternion::RotateVector3(const Vector3& v) const
    {
        Quaternion p(v.x, v.y, v.z, 0.0f);
        p *= this->Inverse();
        Quaternion pp = Quaternion(*this);
        pp *= p;
        return Vector3(pp.x, pp.y, pp.z);
    }

    inline Vector3 Quaternion::RotateVector3(const Quaternion& q, const Vector3& v)
    {
        Quaternion p(v.x, v.y, v.z, 0.0f);
        p *= q.Inverse();
        Quaternion pp = Quaternion(q);
        pp *= p;
        return Vector3(pp.x, pp.y, pp.z);
    }
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif
