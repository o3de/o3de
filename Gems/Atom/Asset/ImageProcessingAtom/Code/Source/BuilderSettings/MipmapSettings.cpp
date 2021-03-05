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
