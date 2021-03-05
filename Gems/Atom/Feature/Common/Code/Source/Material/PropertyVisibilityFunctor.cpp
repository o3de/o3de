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

#include "PropertyVisibilityFunctor.h"

namespace AZ
{
    namespace Render
    {
        void PropertyVisibilityFunctor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<Action>()
                    ->Version(1)
                    ->Field("triggerProperty", &Action::m_triggerPropertyIndex)
                    ->Field("triggerValue", &Action::m_triggerValue)
                    ->Field("visibility", &Action::m_visibility)
                    ;
                serializeContext->Class<PropertyVisibilityFunctor, RPI::MaterialFunctor>()
                    ->Version(1)
                    ->Field("actions",            &PropertyVisibilityFunctor::m_actions)
                    ->Field("affectedProperties", &PropertyVisibilityFunctor::m_affectedProperties)
                    ;
            }
        }

        void PropertyVisibilityFunctor::Process(EditorContext& context)
        {
            bool visibilityApplied = false;
            RPI::MaterialPropertyVisibility lastAppliedVisibility;

            for (const auto& action : m_actions)
            {
                bool willSetVisibility = false;
                if (action.m_triggerValue.Is<bool>() || action.m_triggerValue.Is<int32_t>() || action.m_triggerValue.Is<uint32_t>())
                {
                    willSetVisibility = action.m_triggerValue == context.GetMaterialPropertyValue(action.m_triggerPropertyIndex);
                }
                else if (action.m_triggerValue.Is<float>())
                {
                    willSetVisibility = AZ::IsClose(action.m_triggerValue.GetValue<float>(),
                        context.GetMaterialPropertyValue<float>(action.m_triggerPropertyIndex),
                        std::numeric_limits<float>::epsilon());
                }
                else // for types Vector2, Vector3, Vector4, Color, Image
                {
                    AZ_Error("PropertyVisibilityFunctor", false, "Unsupported property data type as an enable property.");
                }

                if (willSetVisibility)
                {
                    visibilityApplied = true;
                    lastAppliedVisibility = action.m_visibility;
                }
            }

            if (visibilityApplied)
            {
                for (const auto& propertyIndex : m_affectedProperties)
                {
                    context.SetMaterialPropertyVisibility(propertyIndex, lastAppliedVisibility);
                }
            }
        }

    } // namespace Render
} // namespace AZ
