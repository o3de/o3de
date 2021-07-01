/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImageProcessing_precompiled.h"
#include <BuilderSettings/MipmapSettings.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

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
                editContext->Class<MipmapSettings>("Mipmap Setting", "")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MipmapSettings::m_type, "Type", "")
                    ->EnumAttribute(MipGenType::point, "Point")
                    ->EnumAttribute(MipGenType::box, "Average")
                    ->EnumAttribute(MipGenType::triangle, "Linear")
                    ->EnumAttribute(MipGenType::quadratic, "Bilinear")
                    ->EnumAttribute(MipGenType::gaussian, "Gaussian")
                    ->EnumAttribute(MipGenType::blackmanHarris, "BlackmanHarris")
                    ->EnumAttribute(MipGenType::kaiserSinc, "KaiserSinc")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                ;
            }
        }
    }
} // namespace ImageProcessingAtom
