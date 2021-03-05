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

// Macros below are of the form:
// PARAM(NAME, MEMBER_NAME, DEFAULT_VALUE, ...)

AZ_GFX_BOOL_PARAM(Enabled, m_enabled, false)

AZ_GFX_COMMON_PARAM(Data::Asset<RPI::AnyAsset>, ColorGradingLut, m_colorGradingLut, {})

AZ_GFX_COMMON_PARAM(AZ::Render::ShaperPresetType, ShaperPresetType, m_shaperPresetType, AZ::Render::ShaperPresetType::Log2_48_nits)

AZ_GFX_FLOAT_PARAM(ColorGradingLutIntensity, m_colorGradingLutIntensity, 1.0)

AZ_GFX_FLOAT_PARAM(ColorGradingLutOverride, m_colorGradingLutOverride, 1.0)
