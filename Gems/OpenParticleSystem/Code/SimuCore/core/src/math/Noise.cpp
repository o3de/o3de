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
        Vector3 cellSkewPos = input + Vector3(input.Dot(Vector3(skew3DFactor)));
        cellSkewPos = Vector3{std::floor(cellSkewPos.GetX()), std::floor(cellSkewPos.GetY()), std::floor(cellSkewPos.GetZ())};
        Vector3 cellPos = cellSkewPos - Vector3(cellSkewPos.Dot(Vector3(unskew3DFactor)));
        Vector3 offsetToCell = input - cellPos;
        Vector3 order1 = Vector3 {
            offsetToCell.GetY() <= offsetToCell.GetX() ? 1.0f : 0.0f,
            offsetToCell.GetZ() <= offsetToCell.GetY() ? 1.0f : 0.0f,
            offsetToCell.GetX() <= offsetToCell.GetZ() ? 1.0f : 0.0f
        };
        Vector3 order2 = Vector3 {
            offsetToCell.GetZ() <= offsetToCell.GetX() ? 1.0f : 0.0f,
            offsetToCell.GetX() <= offsetToCell.GetY() ? 1.0f : 0.0f,
            offsetToCell.GetY() <= offsetToCell.GetZ() ? 1.0f : 0.0f
        };
        Vector3 offset1 = order1.GetMin(order2) - Vector3(unskew3DFactor);
        Vector3 offset2 = order1.GetMax(order2) - 2.0f * Vector3(unskew3DFactor);
        Vector3 offset3 = Vector3(1.0f - 3.0f * unskew3DFactor);
        return Matrix4::CreateFromRows(
            Vector4{cellPos},
            Vector4{cellPos + offset1},
            Vector4{cellPos + offset2},
            Vector4{cellPos + offset3}
        ); // 4 * Vector3
    }

    Matrix4 SimplexNoise::JacobianSimplexNoise(const Vector3& input)
    {
        Matrix4 simplexNoise = SampleSimplexNoise(input); // 4 * Vector3
        Matrix4 offsetToCell;                             // 4 * Vector3
        Matrix4 gvec[3];                                  // 3 * 4 * Vector3
        Matrix4 grad;                                     // 3 * Vector4
        const int GRADIENT_MASK[3] = { 0x8000, 0x4000, 0x2000 };
        const Vector3 GRADIENT_SCALE{ 1.0f / 0x4000, 1.0f / 0x2000, 1.0f / 0x1000 };
        for (AZ::u32 i = 0; i < 4; i++) {
            offsetToCell.SetRow(i, AZ::Vector4(input) - simplexNoise.GetRow(i));
            Vector3 rand = RandomPCG16(6.0f * Vector3(simplexNoise.GetRow(i)) + AZ::Vector3{ 0.5f });
            gvec[0].SetRow(i, AZ::Vector3(
                static_cast<float>(static_cast<int>(rand.GetX()) & GRADIENT_MASK[0]),
                static_cast<float>(static_cast<int>(rand.GetY()) & GRADIENT_MASK[1]),
                static_cast<float>(static_cast<int>(rand.GetZ()) & GRADIENT_MASK[2])
            ) * GRADIENT_SCALE - AZ::Vector3(1.0f));
            gvec[1].SetRow(i, AZ::Vector3(
                static_cast<float>(static_cast<int>(rand.GetX()) & GRADIENT_MASK[0]),
                static_cast<float>(static_cast<int>(rand.GetY()) & GRADIENT_MASK[1]),
                static_cast<float>(static_cast<int>(rand.GetZ()) & GRADIENT_MASK[2])
            ) * GRADIENT_SCALE - AZ::Vector3(1.0f));
            gvec[2].SetRow(i, AZ::Vector3(
                static_cast<float>(static_cast<int>(rand.GetX()) & GRADIENT_MASK[0]),
                static_cast<float>(static_cast<int>(rand.GetY()) & GRADIENT_MASK[1]),
                static_cast<float>(static_cast<int>(rand.GetZ()) & GRADIENT_MASK[2])
            ) * GRADIENT_SCALE - AZ::Vector3(1.0f));
        
            grad.SetColumn(i,
                 AZ::Vector3(
                 gvec[0].GetRow(i).Dot(offsetToCell.GetRow(i)),
                 gvec[1].GetRow(i).Dot(offsetToCell.GetRow(i)),
                 gvec[2].GetRow(i).Dot(offsetToCell.GetRow(i)))
            );
        }
        Vector4 smoothOffset = SimplexSmooth(offsetToCell);
        Matrix4 dSmoothOffset = SimplexDSmooth(offsetToCell);
        return Matrix4::CreateFromRows(
            Vector4(Vector3(smoothOffset * gvec[0] + dSmoothOffset * grad.GetRow(0)), smoothOffset.Dot(grad.GetRow(0))),
            Vector4(Vector3(smoothOffset * gvec[1] + dSmoothOffset * grad.GetRow(1)), smoothOffset.Dot(grad.GetRow(1))),
            Vector4(Vector3(smoothOffset * gvec[2] + dSmoothOffset * grad.GetRow(2)), smoothOffset.Dot(grad.GetRow(2))),
            Vector4(0.0f)
        ); // 3 * Vector4
    }

    Vector3 SimplexNoise::RandomPCG16(const Vector3& vec)
    {
        AZ::u32 x = static_cast<AZ::u32>(vec.GetX()) * 1664525u + 1013904223u;
        AZ::u32 y = static_cast<AZ::u32>(vec.GetY()) * 1664525u + 1013904223u;
        AZ::u32 z = static_cast<AZ::u32>(vec.GetZ()) * 1664525u + 1013904223u;
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
            Vector3(offsetToCell.GetRow(0)).Dot(Vector3(offsetToCell.GetRow(0))),
            Vector3(offsetToCell.GetRow(1)).Dot(Vector3(offsetToCell.GetRow(1))),
            Vector3(offsetToCell.GetRow(2)).Dot(Vector3(offsetToCell.GetRow(2))),
            Vector3(offsetToCell.GetRow(3)).Dot(Vector3(offsetToCell.GetRow(3))),
        };
        Vector4 s = Vector4 (
            AZ::GetClamp(2.0f * d.GetX(), 0.0f, 1.0f),
            AZ::GetClamp(2.0f * d.GetY(), 0.0f, 1.0f),
            AZ::GetClamp(2.0f * d.GetZ(), 0.0f, 1.0f),
            AZ::GetClamp(2.0f * d.GetW(), 0.0f, 1.0f)
        );
        s = scale * (Vector4(1.0f) + s * (Vector4(-3.0f) + s * (Vector4(3.0f) - s)));
        return s;
    }

    Matrix4 SimplexNoise::SimplexDSmooth(const Matrix4& offsetToCell) // out 3 * Vector4, in 4 * Vector3
    {
        float scale = 1024.0f / 375.0f;
        Vector4 d = Vector4 {
            Vector3(offsetToCell.GetRow(0)).Dot(Vector3(offsetToCell.GetRow(0))),
            Vector3(offsetToCell.GetRow(1)).Dot(Vector3(offsetToCell.GetRow(1))),
            Vector3(offsetToCell.GetRow(2)).Dot(Vector3(offsetToCell.GetRow(2))),
            Vector3(offsetToCell.GetRow(3)).Dot(Vector3(offsetToCell.GetRow(3))),
        };
        Vector4 s = Vector4(
            AZ::GetClamp(2.0f * d.GetX(), 0.0f, 1.0f),
            AZ::GetClamp(2.0f * d.GetY(), 0.0f, 1.0f),
            AZ::GetClamp(2.0f * d.GetZ(), 0.0f, 1.0f),
            AZ::GetClamp(2.0f * d.GetW(), 0.0f, 1.0f)
        );
        s = scale * (Vector4(-12.0f) + s * (Vector4(24.0f) - 12.0f * s));
        return Matrix4::CreateFromRows(
            s * Vector4(offsetToCell.GetRow(0).GetX(), offsetToCell.GetRow(1).GetX(), offsetToCell.GetRow(2).GetX(), offsetToCell.GetRow(3).GetX()),
            s * Vector4(offsetToCell.GetRow(0).GetY(), offsetToCell.GetRow(1).GetY(), offsetToCell.GetRow(2).GetY(), offsetToCell.GetRow(3).GetY()),
            s * Vector4(offsetToCell.GetRow(0).GetZ(), offsetToCell.GetRow(1).GetZ(), offsetToCell.GetRow(2).GetZ(), offsetToCell.GetRow(3).GetZ()),
            Vector4(0.0f)
        );
    }
}
