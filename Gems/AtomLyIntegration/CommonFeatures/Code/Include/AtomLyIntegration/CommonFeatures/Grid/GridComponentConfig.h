/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace Render
    {
        //! Common settings for GridComponents and EditorGridComponent.
        class GridComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(GridComponentConfig, "{D1E357DF-6CCC-43C4-81F1-6B85C2E06A59}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(GridComponentConfig, SystemAllocator);

            static void Reflect(ReflectContext* context);

            float m_gridSize = 32.0f;
            AZ::Color m_axisColor = AZ::Color(0.0f, 0.0f, 1.0f, 1.0f);

            float m_primarySpacing = 1.0f;
            AZ::Color m_primaryColor = AZ::Color(0.25f, 0.25f, 0.25f, 1.0f);

            float m_secondarySpacing = 0.25f;
            AZ::Color m_secondaryColor = AZ::Color(0.5f, 0.5f, 0.5f, 1.0f);
        };
    } // namespace Render
} // namespace AZ
