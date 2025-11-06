/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "core/math/VectorX.h"
#include "core/math/MatrixX.h"

namespace SimuCore {
    class SimplexNoise {
    public:
        static Matrix4 SampleSimplexNoise(const Vector3& input);   // out 4 * Vector3
        static Matrix4 JacobianSimplexNoise(const Vector3& input); // out 3 * Vector4
        static Vector3 RandomPCG16(const Vector3& vec);
        static Vector4 SimplexSmooth(const Matrix4& offsetToCell);       // in 4 * Vector3
        static Matrix4 SimplexDSmooth(const Matrix4& offsetToCell);      // out 3 * Vector4, in 4 * Vector3
    };
} // namespace SimuCore
