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

#include "WhiteBox_precompiled.h"

#include "WhiteBoxMaterial.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace WhiteBox
{
    void WhiteBoxMaterial::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<WhiteBoxMaterial>()
                ->Version(1)
                ->Field("Tint", &WhiteBoxMaterial::m_tint)
                ->Field("UseTexture", &WhiteBoxMaterial::m_useTexture)
                ->Field("Visible", &WhiteBoxMaterial::m_visible);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<WhiteBoxMaterial>("White Box Material", "White Box material editing")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Color, &WhiteBoxMaterial::m_tint, "Tint",
                        "The tint colour to use for the material.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &WhiteBoxMaterial::m_useTexture, "Use Texture",
                        "Use the material's texture.")
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &WhiteBoxMaterial::m_visible, "Visible",
                        "Material is visible in game mode.");
            }
        }
    }
} // namespace WhiteBox
