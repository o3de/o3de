/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiDynamicContentDatabase.h"
#include <ISystem.h>
#include <AzCore/JSON/reader.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Archive/IArchive.h>

namespace LyShineExamples
{
    UiDynamicContentDatabase::UiDynamicContentDatabase()
    {
        memset(m_documentParsed, 0, sizeof(m_documentParsed));

        UiDynamicContentDatabaseBus::Handler::BusConnect();
    }


    UiDynamicContentDatabase::~UiDynamicContentDatabase()
    {
        UiDynamicContentDatabaseBus::Handler::BusDisconnect();
    }

    int UiDynamicContentDatabase::GetNumColors(ColorType colorType)
    {
        if (!m_documentParsed[colorType])
        {
            return 0;
        }

        const rapidjson::Value& colors = m_document[colorType]["colors"];
        return colors.Size();
    }

    AZ::Color UiDynamicContentDatabase::GetColor(ColorType colorType, int index)
    {
        AZ::Color color(0.0f, 0.0f, 0.0f, 1.0f);

        if (!m_documentParsed[colorType])
        {
            return color;
        }

        if (index < GetNumColors(colorType))
        {
            const rapidjson::Value& jsonColors = m_document[colorType]["colors"];
            const rapidjson::Value& jsonColor = jsonColors[index]["color"];

            color.Set(jsonColor[0].GetInt() / 255.0f, jsonColor[1].GetInt() / 255.0f, jsonColor[2].GetInt() / 255.0f, 1.0f);
        }

        return color;
    }

    AZStd::string UiDynamicContentDatabase::GetColorName(ColorType colorType, int index)
    {
        AZStd::string colorName;

        if (!m_documentParsed[colorType])
        {
            return colorName;
        }

        if (index < GetNumColors(colorType))
        {
            const rapidjson::Value& colors = m_document[colorType]["colors"];
            const rapidjson::Value& name = colors[index]["name"];

            colorName = name.GetString();
        }

        return colorName;
    }

    AZStd::string UiDynamicContentDatabase::GetColorPrice(ColorType colorType, int index)
    {
        AZStd::string colorPrice;

        if (!m_documentParsed[colorType])
        {
            return "";
        }

        if (index < GetNumColors(colorType))
        {
            const rapidjson::Value& colors = m_document[colorType]["colors"];
            const rapidjson::Value& price = colors[index]["price"];

            colorPrice = price.GetString();
        }

        return colorPrice;
    }

    void UiDynamicContentDatabase::Refresh(ColorType colorType, const AZStd::string& filePath)
    {
        AZ::IO::HandleType readHandle = gEnv->pCryPak->FOpen(filePath.c_str(), "rt");

        if (readHandle == AZ::IO::InvalidHandle)
        {
            return;
        }

        size_t fileSize = gEnv->pCryPak->FGetSize(readHandle);
        if (fileSize > 0)
        {
            AZStd::string fileBuf;
            fileBuf.resize(fileSize);

            gEnv->pCryPak->FRead(fileBuf.data(), fileSize, readHandle);

            m_documentParsed[colorType] = false;

            rapidjson::ParseResult parseResult = m_document[colorType].Parse(fileBuf.data());
            if (!parseResult)
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "Failed to parse content due to '%s' at offset %zd.\n",
                    rapidjson::GetParseError_En(parseResult.Code()), parseResult.Offset());
            }
            else if (!m_document[colorType].IsObject())
            {
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING,
                    "Expected an object at the root.");
            }
            else
            {
                m_documentParsed[colorType] = true;
            }

        }
        gEnv->pCryPak->FClose(readHandle);
    }

    void UiDynamicContentDatabase::Reflect(AZ::ReflectContext* context)
    {
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Enum<(int)UiDynamicContentDatabaseInterface::ColorType::FreeColors>("eUiDynamicContentDBColorType_Free")
                ->Enum<(int)UiDynamicContentDatabaseInterface::ColorType::PaidColors>("eUiDynamicContentDBColorType_Paid");

            behaviorContext->EBus<UiDynamicContentDatabaseBus>("UiDynamicContentDatabaseBus")
                ->Event("GetNumColors", &UiDynamicContentDatabaseBus::Events::GetNumColors)
                ->Event("GetColor", &UiDynamicContentDatabaseBus::Events::GetColor)
                ->Event("GetColorName", &UiDynamicContentDatabaseBus::Events::GetColorName)
                ->Event("GetColorPrice", &UiDynamicContentDatabaseBus::Events::GetColorPrice)
                ->Event("Refresh", &UiDynamicContentDatabaseBus::Events::Refresh);
        }
    }
} // namespace LYGame
