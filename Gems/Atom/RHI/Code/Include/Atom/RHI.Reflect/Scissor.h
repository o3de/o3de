/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <limits>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    struct Scissor
    {
        AZ_TYPE_INFO(Scissor, "{A0D8D250-59DB-4940-93B4-92C0FA6911CC}");
        static void Reflect(AZ::ReflectContext* context);

        Scissor() = default;
        Scissor(int32_t minX, int32_t minY, int32_t maxX, int32_t maxY);

        Scissor GetScaled(
            float normalizedMinX,
            float normalizedMinY,
            float normalizedMaxY,
            float normalizedMaxX) const;

        static Scissor CreateNull();

        bool IsNull() const;

        static const int32_t DefaultScissorMin = 0;
        static const int32_t DefaultScissorMax = std::numeric_limits<int32_t>::max();

        int32_t m_minX = DefaultScissorMin;
        int32_t m_minY = DefaultScissorMin;
        int32_t m_maxX = DefaultScissorMax;
        int32_t m_maxY = DefaultScissorMax;
    };
}
