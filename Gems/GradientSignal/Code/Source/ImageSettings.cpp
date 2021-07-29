/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/ImageSettings.h>

namespace GradientSignal
{
    void ImageSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<ImageSettings, AZ::Data::AssetData>()
                ->Version(1, &ImageSettings::VersionConverter)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("ShouldProcess", &ImageSettings::m_shouldProcess)
                ->Field("UseR", &ImageSettings::m_useR)
                ->Field("UseG", &ImageSettings::m_useG)
                ->Field("UseB", &ImageSettings::m_useB)
                ->Field("RGBTransformation", &ImageSettings::m_rgbTransform)
                ->Field("UseA", &ImageSettings::m_useA)
                ->Field("AlphaTransformation", &ImageSettings::m_alphaTransform)
                ->Field("ExportFormat", &ImageSettings::m_format)
                ->Field("AutoScale", &ImageSettings::m_autoScale)
                ->Field("RangeMin", &ImageSettings::m_scaleRangeMin)
                ->Field("RangeMax", &ImageSettings::m_scaleRangeMax)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<ImageSettings>(
                    "Image Settings", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSettings::m_shouldProcess, "Should Process", "")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSettings::m_useR, "R", "Should the R channel be used for transforming?")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSettings::m_useG, "G", "Should the G channel be used for transforming?")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSettings::m_useB, "B", "Should the B channel be used for transforming?")

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageSettings::m_rgbTransform, "RGB Transformation", "The transform to apply to the active channels.")
                    ->EnumAttribute(ChannelExportTransform::AVERAGE, "Average")
                    ->EnumAttribute(ChannelExportTransform::MIN, "Min")
                    ->EnumAttribute(ChannelExportTransform::MAX, "Max")
                    ->EnumAttribute(ChannelExportTransform::TERRARIUM, "Terrarium")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSettings::m_useA, "A", "Should the A channel be used for transforming?")

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageSettings::m_alphaTransform, "Alpha Transformation", "The alpha transformation to apply to the result of the RGB transformation.")
                    ->EnumAttribute(AlphaExportTransform::MULTIPLY, "Multiply")
                    ->EnumAttribute(AlphaExportTransform::ADD, "Add")
                    ->EnumAttribute(AlphaExportTransform::SUBTRACT, "Subtract")

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ImageSettings::m_format, "Output Format", "The pixel format to output the asset as.")
                    ->EnumAttribute(ExportFormat::U8, "U8: 8-bit unsigned int")
                    ->EnumAttribute(ExportFormat::U16, "U16: 16-bit unsigned int")
                    ->EnumAttribute(ExportFormat::U32, "U32: 32-bit unsigned int")
                    ->EnumAttribute(ExportFormat::F32, "F32: 32-bit float")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSettings::m_autoScale, "Auto Scale", "Automatically scale based on the minimum and maximum values in the asset.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSettings::m_scaleRangeMin, "Range Minimum", "The minimum range each value is scaled against when transforming between output types.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &ImageSettings::m_scaleRangeMax, "Range Maximum", "The maximum range each value is scaled against when transforming between output types.")
                    ;
            }
        }
    }

    bool ImageSettings::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 1)
        {
            {
                int index = classElement.AddElement<bool>(context, "UseR");

                AZ::SerializeContext::DataElementNode& channelElem = classElement.GetSubElement(index);
                channelElem.SetData<bool>(context, true);
            }

            {
                int index = classElement.AddElement<bool>(context, "UseG");

                AZ::SerializeContext::DataElementNode& channelElem = classElement.GetSubElement(index);
                channelElem.SetData<bool>(context, false);
            }

            {
                int index = classElement.AddElement<bool>(context, "UseB");

                AZ::SerializeContext::DataElementNode& channelElem = classElement.GetSubElement(index);
                channelElem.SetData<bool>(context, false);
            }

            {
                int trsIndex = classElement.AddElement<ChannelExportTransform>(context, "RGBTransformation");

                AZ::SerializeContext::DataElementNode& transform = classElement.GetSubElement(trsIndex);
                transform.SetData<ChannelExportTransform>(context, ChannelExportTransform::MAX);
            }

            {
                int index = classElement.AddElement<bool>(context, "UseA");

                AZ::SerializeContext::DataElementNode& channelElem = classElement.GetSubElement(index);
                channelElem.SetData<bool>(context, false);
            }

            {
                int alphaIndex = classElement.AddElement<AlphaExportTransform>(context, "AlphaTransformation");

                AZ::SerializeContext::DataElementNode& alph = classElement.GetSubElement(alphaIndex);
                alph.SetData<AlphaExportTransform>(context, AlphaExportTransform::MULTIPLY);
            }
            
            {
                int expIndex = classElement.AddElement<ExportFormat>(context, "ExportFormat");

                AZ::SerializeContext::DataElementNode& exp = classElement.GetSubElement(expIndex);
                exp.SetData<ExportFormat>(context, ExportFormat::U8);
            }

            {
                int index = classElement.AddElement<bool>(context, "AutoScale");

                AZ::SerializeContext::DataElementNode& node = classElement.GetSubElement(index);
                node.SetData<bool>(context, true);
            }

            {
                int index = classElement.AddElement<float>(context, "RangeMin");

                AZ::SerializeContext::DataElementNode& node = classElement.GetSubElement(index);
                node.SetData<float>(context, 0.0f);
            }

            {
                int index = classElement.AddElement<float>(context, "RangeMax");

                AZ::SerializeContext::DataElementNode& node = classElement.GetSubElement(index);
                node.SetData<float>(context, 255.0f);
            }
        }

        return true;
    }

} // namespace GradientSignal
