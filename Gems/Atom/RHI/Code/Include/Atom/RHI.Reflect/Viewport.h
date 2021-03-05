/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    class ReflectContext;

    namespace RHI
    {
        struct Viewport
        {
            AZ_TYPE_INFO(Viewport, "{69160593-B7C3-4E94-A397-CC0A34567698}");
            static void Reflect(AZ::ReflectContext* context);

            Viewport() = default;
            Viewport(
                float minX,
                float maxX,
                float minY,
                float maxY,
                float minZ = 0.0f,
                float maxZ = 1.0f);

            Viewport GetScaled(
                float normalizedMinX,
                float normalizedMaxX,
                float normalizedMinY,
                float normalizedMaxY,
                float normalizedMinZ = 0.0f,
                float normalizedMaxZ = 1.0f) const;

            static Viewport CreateNull();

            bool IsNull() const;

            float m_minX = 0.0f;
            float m_maxX = 0.0f;
            float m_minY = 0.0f;
            float m_maxY = 0.0f;
            float m_minZ = 0.0f;
            float m_maxZ = 1.0f;
        };
    } // namespace RHI
} // namespace AZ