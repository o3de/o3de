/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "core/math/Noise.h"

namespace SimuCore {
    Matrix4 SimplexNoise::SampleSimplexNoise(const Vector3& input)
    {
        float skew3DFactor = 1.0f / 3.0f;
        float unskew3DFactor = 1.0f / 6.0f;
        Vector3 cellSkewPos = input + input.Dot(Vector3{ skew3DFactor });
        cellSkewPos = Vector3{std::floor(cellSkewPos.x), std::floor(cellSkewPos.y), std::floor(cellSkewPos.z)};
        Vector3 cellPos = cellSkewPos - cellSkewPos.Dot(Vector3{ unskew3DFactor });
        Vector3 offsetToCell = input - cellPos;
        Vector3 order1 = Vector3 {
            offsetToCell.y <= offsetToCell.x ? 1.0f : 0.0f,
            offsetToCell.z <= offsetToCell.y ? 1.0f : 0.0f,
            offsetToCell.x <= offsetToCell.z ? 1.0f : 0.0f
        };
        Vector3 order2 = Vector3 {
            offsetToCell.z <= offsetToCell.x ? 1.0f : 0.0f,
            offsetToCell.x <= offsetToCell.y ? 1.0f : 0.0f,
            offsetToCell.y <= offsetToCell.z ? 1.0f : 0.0f
        };
        Vector3 offset1 = order1.ComponentMin(order2) - unskew3DFactor;
        Vector3 offset2 = order1.ComponentMax(order2) - 2.0f * unskew3DFactor;
        Vector3 offset3 = Vector3(1.0f - 3.0f * unskew3DFactor);
        return Matrix4 {
            Vector4{cellPos},
            Vector4{cellPos + offset1},
            Vector4{cellPos + offset2},
            Vector4{cellPos + offset3}
        }; // 4 * Vector3
    }

    Matrix4 SimplexNoise::JacobianSimplexNoise(const Vector3& input)
    {
        Matrix4 simplexNoise = SampleSimplexNoise(input); // 4 * Vector3
        Matrix4 offsetToCell;                             // 4 * Vector3
        Matrix4 gvec[3];                                  // 3 * 4 * Vector3
        Matrix4 grad;                                     // 3 * Vector4
        const int GRADIENT_MASK[3] = { 0x8000, 0x4000, 0x2000 };
        const Vector3 GRADIENT_SCALE{ 1.0f / 0x4000, 1.0f / 0x2000, 1.0f / 0x1000 };
        for (uint32_t i = 0; i < 4; i++) {
            offsetToCell.m[i] = input - Vector3(simplexNoise.m[i]);
            Vector3 rand = RandomPCG16(6.0f * Vector3(simplexNoise.m[i]) + Vector3{ 0.5f });
            gvec[0].m[i] = Vector3(
                static_cast<float>(static_cast<int>(rand.x) & GRADIENT_MASK[0]),
                static_cast<float>(static_cast<int>(rand.x) & GRADIENT_MASK[1]),
                static_cast<float>(static_cast<int>(rand.x) & GRADIENT_MASK[2])
            ) * GRADIENT_SCALE - 1.0f;
            gvec[1].m[i] = Vector3(
                static_cast<float>(static_cast<int>(rand.y) & GRADIENT_MASK[0]),
                static_cast<float>(static_cast<int>(rand.y) & GRADIENT_MASK[1]),
                static_cast<float>(static_cast<int>(rand.y) & GRADIENT_MASK[2])
            ) * GRADIENT_SCALE - 1.0f;
            gvec[2].m[i] = Vector3(
                static_cast<float>(static_cast<int>(rand.z) & GRADIENT_MASK[0]),
                static_cast<float>(static_cast<int>(rand.z) & GRADIENT_MASK[1]),
                static_cast<float>(static_cast<int>(rand.z) & GRADIENT_MASK[2])
            ) * GRADIENT_SCALE - 1.0f;
            grad.m[0][i] = Vector3(gvec[0].m[i]).Dot(Vector3(offsetToCell.m[i]));
            grad.m[1][i] = Vector3(gvec[1].m[i]).Dot(Vector3(offsetToCell.m[i]));
            grad.m[2][i] = Vector3(gvec[2].m[i]).Dot(Vector3(offsetToCell.m[i]));
        }
        Vector4 smoothOffset = SimplexSmooth(offsetToCell);
        Matrix4 dSmoothOffset = SimplexDSmooth(offsetToCell);
        return Matrix4 {
            Vector4(Vector3(smoothOffset * gvec[0] + dSmoothOffset * grad.m[0]), smoothOffset.Dot(grad.m[0])),
            Vector4(Vector3(smoothOffset * gvec[1] + dSmoothOffset * grad.m[1]), smoothOffset.Dot(grad.m[1])),
            Vector4(Vector3(smoothOffset * gvec[2] + dSmoothOffset * grad.m[2]), smoothOffset.Dot(grad.m[2])),
            Vector4(0.0f)
        }; // 3 * Vector4
    }

    Vector3 SimplexNoise::RandomPCG16(const Vector3& vec)
    {
        uint32_t x = static_cast<uint32_t>(vec.x) * 1664525u + 1013904223u;
        uint32_t y = static_cast<uint32_t>(vec.y) * 1664525u + 1013904223u;
        uint32_t z = static_cast<uint32_t>(vec.z) * 1664525u + 1013904223u;
        x += y * z;
        y += z * x;
        z += x * y;
        x += y * z;
        y += z * x;
        z += x * y;
        return Vector3 {
            static_cast<float>(x >> 16u),
            static_cast<float>(y >> 16u),
            static_cast<float>(z >> 16u)
        };
    }

    Vector4 SimplexNoise::SimplexSmooth(const Matrix4& offsetToCell) // 4 * Vector3
    {
        float scale = 1024.0f / 375.0f;
        Vector4 d = Vector4 {
            Vector3(offsetToCell.m[0]).Dot(Vector3(offsetToCell.m[0])),
            Vector3(offsetToCell.m[1]).Dot(Vector3(offsetToCell.m[1])),
            Vector3(offsetToCell.m[2]).Dot(Vector3(offsetToCell.m[2])),
            Vector3(offsetToCell.m[3]).Dot(Vector3(offsetToCell.m[3])),
        };
        Vector4 s = Vector4 (
            Math::Clamp(2.0f * d.x, 0.0f, 1.0f),
            Math::Clamp(2.0f * d.y, 0.0f, 1.0f),
            Math::Clamp(2.0f * d.z, 0.0f, 1.0f),
            Math::Clamp(2.0f * d.w, 0.0f, 1.0f)
        );
        s = scale * (Vector4(1.0f) + s * (Vector4(-3.0f) + s * (Vector4(3.0f) - s)));
        return s;
    }

    Matrix4 SimplexNoise::SimplexDSmooth(const Matrix4& offsetToCell) // out 3 * Vector4, in 4 * Vector3
    {
        float scale = 1024.0f / 375.0f;
        Vector4 d = Vector4 {
            Vector3(offsetToCell.m[0]).Dot(Vector3(offsetToCell.m[0])),
            Vector3(offsetToCell.m[1]).Dot(Vector3(offsetToCell.m[1])),
            Vector3(offsetToCell.m[2]).Dot(Vector3(offsetToCell.m[2])),
            Vector3(offsetToCell.m[3]).Dot(Vector3(offsetToCell.m[3])),
        };
        Vector4 s = Vector4(
            Math::Clamp(2.0f * d.x, 0.0f, 1.0f),
            Math::Clamp(2.0f * d.y, 0.0f, 1.0f),
            Math::Clamp(2.0f * d.z, 0.0f, 1.0f),
            Math::Clamp(2.0f * d.w, 0.0f, 1.0f)
        );
        s = scale * (Vector4(-12.0f) + s * (Vector4(24.0f) - 12.0f * s));
        return Matrix4(
            s * Vector4(offsetToCell.m[0][0], offsetToCell.m[1][0], offsetToCell.m[2][0], offsetToCell.m[3][0]),
            s * Vector4(offsetToCell.m[0][1], offsetToCell.m[1][1], offsetToCell.m[2][1], offsetToCell.m[3][1]),
            s * Vector4(offsetToCell.m[0][2], offsetToCell.m[1][2], offsetToCell.m[2][2], offsetToCell.m[3][2]),
            Vector4(0.0f)
        );
    }
}
