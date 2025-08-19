/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace SimuCore {
    inline Matrix4::Matrix4(const Vector4& v1, const Vector4& v2, const Vector4& v3, const Vector4& v4)
        : m{ v1, v2, v3, v4 }
    {
    }

    inline Matrix4::Matrix4(const Matrix3& mat)
    {
        m[0][0] = mat.m[0][0];
        m[0][1] = mat.m[0][1];
        m[0][2] = mat.m[0][2];
        m[0][3] = 0.f;

        m[1][0] = mat.m[1][0];
        m[1][1] = mat.m[1][1];
        m[1][2] = mat.m[1][2];
        m[1][3] = 0.f;

        m[2][0] = mat.m[2][0];
        m[2][1] = mat.m[2][1];
        m[2][2] = mat.m[2][2];
        m[2][3] = 0.f;

        m[3][0] = 0.f;
        m[3][1] = 0.f;
        m[3][2] = 0.f;
        m[3][3] = 1.f;
    }

    inline Matrix4& Matrix4::operator+=(const Matrix4& v)
    {
        m[0] += v.m[0];
        m[1] += v.m[1];
        m[2] += v.m[2];
        m[3] += v.m[3];
        return *this;
    }

    inline Matrix4& Matrix4::operator-=(const Matrix4& v)
    {
        m[0] -= v.m[0];
        m[1] -= v.m[1];
        m[2] -= v.m[2];
        m[3] -= v.m[3];
        return *this;
    }

    inline Matrix4& Matrix4::operator*=(float v)
    {
        m[0] *= v;
        m[1] *= v;
        m[2] *= v;
        m[3] *= v;
        return *this;
    }

    inline Matrix4& Matrix4::operator/=(float v)
    {
        m[0] /= v;
        m[1] /= v;
        m[2] /= v;
        m[3] /= v;
        return *this;
    }

    inline Vector4& Matrix4::operator[](size_t index)
    {
        return m[index];
    }

    inline const Vector4& Matrix4::operator[](size_t index) const
    {
        return m[index];
    }

    inline Matrix4 Matrix4::Inverse() const
    {
        float coe00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
        float coe02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
        float coe03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

        float coe04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
        float coe06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
        float coe07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

        float coe08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
        float coe10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
        float coe11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

        float coe12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
        float coe14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
        float coe15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

        float coe16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
        float coe18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
        float coe19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

        float coe20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
        float coe22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
        float coe23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

        Vector4 fac0(coe00, coe00, coe02, coe03);
        Vector4 fac1(coe04, coe04, coe06, coe07);
        Vector4 fac2(coe08, coe08, coe10, coe11);
        Vector4 fac3(coe12, coe12, coe14, coe15);
        Vector4 fac4(coe16, coe16, coe18, coe19);
        Vector4 fac5(coe20, coe20, coe22, coe23);

        Vector4 vec0(m[1][0], m[0][0], m[0][0], m[0][0]);
        Vector4 vec1(m[1][1], m[0][1], m[0][1], m[0][1]);
        Vector4 vec2(m[1][2], m[0][2], m[0][2], m[0][2]);
        Vector4 vec3(m[1][3], m[0][3], m[0][3], m[0][3]);

        Vector4 inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
        Vector4 inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
        Vector4 inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
        Vector4 inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

        Vector4 signA(+1, -1, +1, -1);
        Vector4 signB(-1, +1, -1, +1);
        Matrix4 inverse(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

        Vector4 row0(inverse[0][0], inverse[1][0], inverse[2][0], inverse[3][0]);

        Vector4 dot0(m[0] * row0);
        float dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

        float inverseDet = 1.f / dot1;

        return inverse * inverseDet;
    }

    inline Matrix4 Matrix4::Transpose() const
    {
        Matrix4 transpose;
        for (size_t x = 0; x < 4; x++) {
            for (size_t y = 0; y < 4; y++) {
                transpose[x][y] = m[y][x];
            }
        }
        return transpose;
    }

    inline Matrix4 Matrix4::operator*(const Matrix4& v) const
    {
        Matrix4 result;
        result.m[0] = m[0] * v.m[0].x + m[1] * v.m[0].y + m[2] * v.m[0].z + m[3] * v.m[0].w;
        result.m[1] = m[0] * v.m[1].x + m[1] * v.m[1].y + m[2] * v.m[1].z + m[3] * v.m[1].w;
        result.m[2] = m[0] * v.m[2].x + m[1] * v.m[2].y + m[2] * v.m[2].z + m[3] * v.m[2].w;
        result.m[3] = m[0] * v.m[3].x + m[1] * v.m[3].y + m[2] * v.m[3].z + m[3] * v.m[3].w;
        return result;
    }

    inline Vector4 Matrix4::operator*(const Vector4& v) const
    {
        auto result = m[0] * v.x;
        result += m[1] * v.y;
        result += m[2] * v.z;
        result += m[3] * v.w;
        return result;
    }

    inline Matrix3::Matrix3(const Vector3& v1, const Vector3& v2, const Vector3& v3)
        : m{ v1, v2, v3 }
    {
    }

    inline Matrix3::Matrix3(const Matrix4& mat)
        : m{ Vector3(mat.m[0]), Vector3(mat.m[1]), Vector3(mat.m[2]) }
    {
    }

    inline Matrix3& Matrix3::operator+=(const Matrix3& v)
    {
        m[0] += v.m[0];
        m[1] += v.m[1];
        m[2] += v.m[2];
        return *this;
    }

    inline Matrix3& Matrix3::operator-=(const Matrix3& v)
    {
        m[0] -= v.m[0];
        m[1] -= v.m[1];
        m[2] -= v.m[2];
        return *this;
    }
    inline Matrix3& Matrix3::operator*=(float v)
    {
        m[0] *= v;
        m[1] *= v;
        m[2] *= v;
        return *this;
    }
    inline Matrix3& Matrix3::operator/=(float v)
    {
        m[0] /= v;
        m[1] /= v;
        m[2] /= v;
        return *this;
    }

    inline Matrix3 Matrix3::operator*(const Matrix3& v) const
    {
        Matrix3 result;
        result.m[0] = m[0] * v.m[0].x + m[1] * v.m[0].y + m[2] * v.m[0].z;
        result.m[1] = m[0] * v.m[1].x + m[1] * v.m[1].y + m[2] * v.m[1].z;
        result.m[2] = m[0] * v.m[2].x + m[1] * v.m[2].y + m[2] * v.m[2].z;
        return result;
    }

    inline Vector3 Matrix3::operator*(const Vector3& v) const
    {
        auto result = m[0] * v.x;
        result += m[1] * v.y;
        result += m[2] * v.z;
        return result;
    }

    inline Vector3& Matrix3::operator[](size_t index)
    {
        return m[index];
    }

    inline const Vector3& Matrix3::operator[](size_t index) const
    {
        return m[index];
    }

    inline float Matrix3::Determinant() const
    {
        float d = m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) +
                  m[1][0] * (m[2][1] * m[0][2] - m[0][1] * m[2][2]) +
                  m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2]);
        return d;
    }

    inline void Matrix3::FromAxisRadian(const Vector3& direction)
    {
        Matrix3 xMatrix = Matrix3(Vector3(0.f), Vector3(0.f), Vector3(0.f));
        Matrix3 yMatrix = Matrix3(Vector3(0.f), Vector3(0.f), Vector3(0.f));
        Matrix3 zMatrix = Matrix3(Vector3(0.f), Vector3(0.f), Vector3(0.f));
        Vector3 vectorX = Vector3(1.f, 0.f, 0.f);
        Vector3 vectorY = Vector3(0.f, 1.f, 0.f);
        Vector3 vectorZ = Vector3(0.f, 0.f, 1.f);
        xMatrix.FromAxisRadian(vectorX, Math::AngleToRadians(direction.x));
        yMatrix.FromAxisRadian(vectorY, Math::AngleToRadians(direction.y));
        zMatrix.FromAxisRadian(vectorZ, Math::AngleToRadians(direction.z));
        *this = zMatrix * (yMatrix * xMatrix);
    }

    inline void Matrix3::FromAxisRadian(const Vector3& axis, const float& radian)
    {
        Vector3 tmpAxis = Vector3(axis.x, axis.y, axis.z);
        (void)tmpAxis.Normalize();
        float cosTheta = cos(radian);
        float oneMinCos = 1 - cosTheta;
        float sinTheta = sin(radian);
        float x2 = tmpAxis.x * tmpAxis.x;
        float y2 = tmpAxis.y * tmpAxis.y;
        float z2 = tmpAxis.z * tmpAxis.z;
        float xyo = tmpAxis.x * tmpAxis.y * oneMinCos;
        float xzo = tmpAxis.x * tmpAxis.z * oneMinCos;
        float yzo = tmpAxis.y * tmpAxis.z * oneMinCos;
        float xs = tmpAxis.x * sinTheta;
        float ys = tmpAxis.y * sinTheta;
        float zs = tmpAxis.z * sinTheta;

        m[0] = Vector3(cosTheta + x2 * oneMinCos, xyo - zs, xzo + ys);
        m[1] = Vector3(xyo + zs, cosTheta + y2 * oneMinCos, yzo - xs);
        m[2] = Vector3(xzo - ys, yzo + xs, cosTheta + z2 * oneMinCos);
    }
}
