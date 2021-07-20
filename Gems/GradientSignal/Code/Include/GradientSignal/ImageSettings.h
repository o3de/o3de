/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace GradientSignal
{
    enum class ChannelExportTransform : AZ::u8
    {
        AVERAGE,
        MIN,
        MAX,
        TERRARIUM
    };

    enum class AlphaExportTransform : AZ::u8
    {
        MULTIPLY,
        ADD,
        SUBTRACT
    };

    enum class ExportFormat : AZ::u8
    {
        U8,
        U16,
        U32,
        F32
    };

    enum ChannelMask : AZ::u8
    {
        R = 0x01,
        G = 0x02,
        B = 0x04,
        A = 0x08
    };

    class ImageSettings
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(ImageSettings, "{B36FEB5C-41B6-4B58-A212-21EF5AEF523C}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ImageSettings, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        bool m_shouldProcess = true;

        ChannelExportTransform m_rgbTransform = ChannelExportTransform::MAX;
        AlphaExportTransform m_alphaTransform = AlphaExportTransform::MULTIPLY;
        ExportFormat m_format = ExportFormat::U8;

        bool m_useR = true;
        bool m_useG = false;
        bool m_useB = false;
        bool m_useA = false;

        bool m_autoScale = true;

        float m_scaleRangeMin = 0.0f;
        float m_scaleRangeMax = 255.0f;
    };
}// namespace GradientSignal
