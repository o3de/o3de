/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <BuilderSettings/MipmapSettings.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Translation/TranslationDef.h>

namespace ImageProcessingAtom
{
    bool MipmapSettings::operator!=(const MipmapSettings& other) const
    {
        return !(*this == other);
    }

    bool MipmapSettings::operator==(const MipmapSettings& other) const
    {
        return m_type == other.m_type;
    }

    void MipmapSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<MipmapSettings>()
                ->Version(1)
                ->Field("MipGenType", &MipmapSettings::m_type);

            AZ::EditContext* editContext = serialize->GetEditContext();
            if (editContext)
            {
                editContext->Class<MipmapSettings>(QT_TRANSLATE_NOOP("Atom::Asset", "Mipmap Setting"), "")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MipmapSettings::m_type, QT_TRANSLATE_NOOP("Atom::Asset", "Type"), "")
                    ->EnumAttribute(MipGenType::point, QT_TRANSLATE_NOOP("Atom::Asset", "Point"))
                    ->EnumAttribute(MipGenType::box, QT_TRANSLATE_NOOP("Atom::Asset", "Average"))
                    ->EnumAttribute(MipGenType::triangle, QT_TRANSLATE_NOOP("Atom::Asset", "Linear"))
                    ->EnumAttribute(MipGenType::quadratic, QT_TRANSLATE_NOOP("Atom::Asset", "Bilinear"))
                    ->EnumAttribute(MipGenType::gaussian, QT_TRANSLATE_NOOP("Atom::Asset", "Gaussian"))
                    ->EnumAttribute(MipGenType::blackmanHarris, QT_TRANSLATE_NOOP("Atom::Asset", "BlackmanHarris"))
                    ->EnumAttribute(MipGenType::kaiserSinc, QT_TRANSLATE_NOOP("Atom::Asset", "KaiserSinc"))
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                ;
            }
        }
    }
} // namespace ImageProcessingAtom
