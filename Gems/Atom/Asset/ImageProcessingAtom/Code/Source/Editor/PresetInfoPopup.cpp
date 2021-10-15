/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PresetInfoPopup.h"
#include <Source/Editor/ui_PresetInfoPopup.h>
#include <BuilderSettings/PresetSettings.h>
#include <QLabel>
#include <QString>

namespace ImageProcessingAtomEditor
{
    using namespace ImageProcessingAtom;
    // Help functions to convert enum to strings
    static const char* RGBWeightToString(RGBWeight weight)
    {
        static const char* RGBWeightNames[] = { "Uniform", "Luminance", "CIEXYZ" };
        AZ_Assert(weight <= RGBWeight::ciexyz, "Invalid RGBWeight!");
        return RGBWeightNames[(int)weight];
    }

    static const char* ColorSpaceToString(ColorSpace colorSpace)
    {
        static const char* colorSpaceNames[] = { "Linear", "sRGB", "Auto" };
        AZ_Assert(colorSpace <= ColorSpace::autoSelect, "Invalid ColorSpace!");
        return colorSpaceNames[(int)colorSpace];
    }

    static const char* MipGenTypeToString(MipGenType mipGenType)
    {
        static const char* mipGenTypeNames[] = { "Point", "Box", "Triangle", "Quadratic", "Gaussian", "BlackmanHarris", "KaiserSinc" };
        AZ_Assert(mipGenType <= MipGenType::kaiserSinc, "Invalid MipGenType!");
        return mipGenTypeNames[(int)mipGenType];
    }

    static const char* CubemapFilterTypeToString(CubemapFilterType cubemapFilterType)
    {
        static const char* cubemapFilterTypeNames[] = { "Disc", "Cone", "Cosine", "Gaussian", "CosinePower", "GGX" };
        AZ_Assert(cubemapFilterType <= CubemapFilterType::ggx, "Invalid CubemapFilterType!");
        return cubemapFilterTypeNames[(int)cubemapFilterType];
    }

    PresetInfoPopup::PresetInfoPopup(const PresetSettings* presetSettings, QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDialog(parent, Qt::Dialog | Qt::Popup)
        , m_ui(new Ui::PresetInfoPopup)
    {
        m_ui->setupUi(this);
        RefreshPresetInfoLabel(presetSettings);
    }

    PresetInfoPopup::~PresetInfoPopup()
    {
    }
    void PresetInfoPopup::RefreshPresetInfoLabel(const PresetSettings* presetSettings)
    {
        QString presetInfoText = "";
        if (!presetSettings)
        {
            presetInfoText = "Invalid Preset!";
            m_ui->infoLabel->setText(presetInfoText);
            return;
        }

        presetInfoText += QString("UUID: %1\n").arg(presetSettings->m_uuid.ToString<AZStd::string>().c_str());
        presetInfoText += QString("Name: %1\n").arg(presetSettings->m_name.GetCStr());
        presetInfoText += QString("Generate IBL Only: %1\n").arg(presetSettings->m_generateIBLOnly ? "True" : "False");
        presetInfoText += QString("RGB Weight: %1\n").arg(RGBWeightToString(presetSettings->m_rgbWeight));
        presetInfoText += QString("Source ColorSpace: %1\n").arg(ColorSpaceToString(presetSettings->m_srcColorSpace));
        presetInfoText += QString("Destination ColorSpace: %1\n").arg(ColorSpaceToString(presetSettings->m_destColorSpace));
        presetInfoText += QString("FileMasks: ");
        int i = 0;
        for (auto& mask : presetSettings->m_fileMasks)
        {
            presetInfoText += i > 0 ? ", " : "";
            presetInfoText += mask.c_str();
            i++;
        }
        presetInfoText += "\n";
        presetInfoText += QString("Suppress Engine Reduce: %1\n").arg(presetSettings->m_suppressEngineReduce ? "True" : "False");
        presetInfoText += QString("Discard Alpha: %1\n").arg(presetSettings->m_discardAlpha ? "True" : "False");
        presetInfoText += QString("Is Color Chart: %1\n").arg(presetSettings->m_isColorChart ? "True" : "False");
        presetInfoText += QString("High Pass Mip: %1\n").arg(presetSettings->m_highPassMip);
        presetInfoText += QString("Gloss From Normal: %1\n").arg(presetSettings->m_glossFromNormals);
        presetInfoText += QString("Use Legacy Gloss: %1\n").arg(presetSettings->m_isLegacyGloss ? "True" : "False");
        presetInfoText += QString("Mip Re-normalize: %1\n").arg(presetSettings->m_isMipRenormalize ? "True" : "False");
        presetInfoText += QString("Resident Mips Number: %1\n").arg(presetSettings->m_numResidentMips);
        presetInfoText += QString("Swizzle: %1\n").arg(presetSettings->m_swizzle.c_str());
        if (presetSettings->m_cubemapSetting)
        {
            presetInfoText += QString("[Cubemap Settings]\n");
            presetInfoText += QString("Filter: %1\n").arg(CubemapFilterTypeToString(presetSettings->m_cubemapSetting->m_filter));
            presetInfoText += QString("Angle: %1\n").arg(presetSettings->m_cubemapSetting->m_angle);
            presetInfoText += QString("Mip Angle: %1\n").arg(presetSettings->m_cubemapSetting->m_mipAngle);
            presetInfoText += QString("Mip Slope: %1\n").arg(presetSettings->m_cubemapSetting->m_mipSlope);
            presetInfoText += QString("Edge Fixup: %1\n").arg(presetSettings->m_cubemapSetting->m_edgeFixup);
            presetInfoText += QString("Generate IBL Specular: %1\n").arg(presetSettings->m_cubemapSetting->m_generateIBLSpecular ? "True" : "False");
            presetInfoText += QString("IBL Specular Preset: %1\n").arg(presetSettings->m_cubemapSetting->m_iblSpecularPreset.GetCStr());
            presetInfoText += QString("Generate IBL Diffuse: %1\n").arg(presetSettings->m_cubemapSetting->m_generateIBLDiffuse ? "True" : "False");
            presetInfoText += QString("IBL Diffuse Preset: %1\n").arg(presetSettings->m_cubemapSetting->m_iblDiffusePreset.GetCStr());
            presetInfoText += QString("Requires Convolve: %1\n").arg(presetSettings->m_cubemapSetting->m_requiresConvolve ? "True" : "False");
            presetInfoText += QString("SubId: %1\n").arg(presetSettings->m_cubemapSetting->m_subId);
        }

        if (presetSettings->m_mipmapSetting)
        {
            presetInfoText += QString("[MipMapSetting]\n");
            presetInfoText += QString("Type: %1\n").arg(MipGenTypeToString(presetSettings->m_mipmapSetting->m_type));
        }

        m_ui->infoLabel->setText(presetInfoText);
    }
}//namespace ImageProcessingAtomEditor
#include <Source/Editor/moc_PresetInfoPopup.cpp>
