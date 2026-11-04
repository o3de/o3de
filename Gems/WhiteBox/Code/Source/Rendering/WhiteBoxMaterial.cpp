/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxMaterial.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Translation/TranslationDef.h>

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
                editContext->Class<WhiteBoxMaterial>(
                    QT_TRANSLATE_NOOP("WhiteBox", "White Box Material"),
                    QT_TRANSLATE_NOOP("WhiteBox", "White Box material editing"))
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Color, &WhiteBoxMaterial::m_tint,
                        QT_TRANSLATE_NOOP("WhiteBox", "Tint"),
                        QT_TRANSLATE_NOOP("WhiteBox", "The tint colour to use for the material."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &WhiteBoxMaterial::m_useTexture,
                        QT_TRANSLATE_NOOP("WhiteBox", "Use Texture"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Use the material's texture."))
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &WhiteBoxMaterial::m_visible,
                        QT_TRANSLATE_NOOP("WhiteBox", "Visible"),
                        QT_TRANSLATE_NOOP("WhiteBox", "Material is visible in game mode."));
            }
        }
    }
} // namespace WhiteBox
