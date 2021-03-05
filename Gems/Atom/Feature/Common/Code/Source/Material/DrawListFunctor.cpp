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

#include "DrawListFunctor.h"
#include <Atom/RPI.Public/Material/Material.h>

namespace AZ
{
    namespace Render
    {
        void DrawListFunctor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DrawListFunctor, RPI::MaterialFunctor>()
                    ->Version(1)
                    ->Field("triggerPropertyIndex", &DrawListFunctor::m_triggerPropertyIndex)
                    ->Field("triggerValue", &DrawListFunctor::m_triggerValue)
                    ->Field("shaderIndex", &DrawListFunctor::m_shaderItemIndex)
                    ->Field("drawList", &DrawListFunctor::m_drawListName)
                    ;
            }
        }

        void DrawListFunctor::Process(RuntimeContext& context)
        {
            using namespace RPI;

            bool enable = false;

            if (m_triggerValue.Is<bool>() || m_triggerValue.Is<int32_t>() || m_triggerValue.Is<uint32_t>())
            {
                enable = m_triggerValue == context.GetMaterialPropertyValue(m_triggerPropertyIndex);
            }
            else if (m_triggerValue.Is<float>())
            {
                enable = AZ::IsClose(m_triggerValue.GetValue<float>(),
                    context.GetMaterialPropertyValue<float>(m_triggerPropertyIndex),
                    std::numeric_limits<float>::epsilon());
            }
            else // for types Vector2, Vector3, Vector4, Color, MaterialAsset::ImageBinding
            {
                AZ_Error("DrawListFunctor", false, "Unsupported property data type as an enable property.");
            }

            if (enable)
            {
                context.SetShaderDrawListTagOverride(m_shaderItemIndex, m_drawListName);
            }
            else
            {
                context.SetShaderDrawListTagOverride(m_shaderItemIndex, AZ::Name());
            }
        }

    } // namespace Render
} // namespace AZ
