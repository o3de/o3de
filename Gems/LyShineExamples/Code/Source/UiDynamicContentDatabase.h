/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/JSON/document.h>
#include <AzCore/std/string/string.h>
#include <LyShineExamples/UiDynamicContentDatabaseBus.h>

namespace LyShineExamples
{
    class UiDynamicContentDatabase
        : protected UiDynamicContentDatabaseBus::Handler
    {
    public: // member functions
        UiDynamicContentDatabase();
        ~UiDynamicContentDatabase();

        // UiDynamicContentDatabaseBus interface implementation
        int GetNumColors(ColorType colorType) override;
        AZ::Color GetColor(ColorType colorType, int index) override;
        AZStd::string GetColorName(ColorType colorType, int index) override;
        AZStd::string GetColorPrice(ColorType colorType, int index) override;
        void Refresh(ColorType colorType, const AZStd::string& filePath) override;
        // ~UiDynamicContentDatabaseBus

    public: // static member functions

        static void Reflect(AZ::ReflectContext* context);

    private: // member functions
        AZ_DISABLE_COPY_MOVE(UiDynamicContentDatabase);

    private: // data
        rapidjson::Document m_document[UiDynamicContentDatabaseInterface::ColorType::NumColorTypes];
        bool m_documentParsed[UiDynamicContentDatabaseInterface::ColorType::NumColorTypes];
    };
} // namespace LYGame
