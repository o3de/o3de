/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RPI.Edit/Common/ColorUtils.h>

namespace AZ
{
    namespace RPI
    {
        namespace ColorUtils
        {
            AzToolsFramework::ColorEditorConfiguration GetLinearRgbEditorConfig()
            {
                enum ColorSpace : uint32_t
                {
                    LinearSRGB,
                    SRGB
                };
                
                AzToolsFramework::ColorEditorConfiguration configuration;
                configuration.m_colorPickerDialogConfiguration = AzQtComponents::ColorPicker::Configuration::RGB;

                //[GFX TODO][ATOM-4462] Allow the materialtype to set the m_propertyColorSpace for each property

                configuration.m_propertyColorSpaceId = ColorSpace::LinearSRGB;
                configuration.m_colorPickerDialogColorSpaceId = ColorSpace::SRGB;
                configuration.m_colorSwatchColorSpaceId = ColorSpace::SRGB;

                configuration.m_colorSpaceNames[ColorSpace::LinearSRGB] = "Linear sRGB";
                configuration.m_colorSpaceNames[ColorSpace::SRGB] = "sRGB";

                configuration.m_transformColorCallback = [](const AZ::Color& color, uint32_t fromColorSpaceId, uint32_t toColorSpaceId)
                {
                    //[GFX TODO][ATOM-4436] Change this to use the central TransformColor utility function after it's added
                    if (fromColorSpaceId == toColorSpaceId)
                    {
                        return color;
                    }
                    else if (fromColorSpaceId == ColorSpace::LinearSRGB && toColorSpaceId == ColorSpace::SRGB)
                    {
                        return color.LinearToGamma();
                    }
                    else if (fromColorSpaceId == ColorSpace::SRGB && toColorSpaceId == ColorSpace::LinearSRGB)
                    {
                        return color.GammaToLinear();
                    }
                    else
                    {
                        AZ_Error("ColorEditorConfiguration", false, "Invalid color space combination");
                        return color;
                    }
                };

                return configuration;
            }

        } // namespace ColorPropertyEditorConfigurations
    } // namespace RPI
} // namespace AZ
