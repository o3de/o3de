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
            AZ_CLASS_ALLOCATOR(GridComponentConfig, SystemAllocator, 0);

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