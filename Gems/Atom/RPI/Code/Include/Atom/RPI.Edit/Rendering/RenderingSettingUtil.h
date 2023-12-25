/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
//  Rendering Settings
namespace RenderingSetting
{
    template<typename ObjectType>
    bool LoadFromFile(const AZStd::string& path, ObjectType& objectData)
    {
        objectData = ObjectType();

        auto loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(path);
        if (!loadOutcome.IsSuccess())
        {
            AZ_Error("AZ::JsonSerializationUtils", false, "%s", loadOutcome.GetError().c_str());
            return false;
        }

        rapidjson::Document& document = loadOutcome.GetValue();

        AZ::JsonDeserializerSettings jsonSettings;
        AZ::RPI::JsonReportingHelper reportingHelper;
        reportingHelper.Attach(jsonSettings);

        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Load(objectData, document, jsonSettings);
        if (result.GetOutcome() > AZ::JsonSerializationResult::Outcomes::PartialDefaults || reportingHelper.ErrorsReported())
        {
            AZ_Error("AZ::JsonSerializationUtils", false, "Failed to load object from JSON file: %s", path.c_str());
            return false;
        }
        return true;
    }

    template<typename ObjectType>
    bool SaveToFile(const AZStd::string& path, const ObjectType& objectData)
    {
        rapidjson::Document document;
        document.SetObject();

        AZ::JsonSerializerSettings settings;
        settings.m_keepDefaults = true;
        AZ::RPI::JsonReportingHelper reportingHelper;
        reportingHelper.Attach(settings);

        AZ::JsonSerialization::Store(document, document.GetAllocator(), objectData, settings);
        if (reportingHelper.ErrorsReported())
        {
            AZ_Error("AZ::RPI::JsonUtils", false, "Failed to write object data to JSON document: %s", path.c_str());
            return false;
        }

        AZ::JsonSerializationUtils::WriteJsonFile(document, path);
        if (reportingHelper.ErrorsReported())
        {
            AZ_Error("AZ::RPI::JsonUtils", false, "Failed to write JSON document to file: %s", path.c_str());
            return false;
        }

        return true;
    }
}
