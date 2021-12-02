/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <Atom/RPI.Edit/Common/JsonFileLoadContext.h>

namespace UnitTest
{
    //! Contains the results of a JSON test operation
    struct JsonTestResult
    {
        struct Report
        {
            AZ::JsonSerializationResult::ResultCode resultCode{AZ::JsonSerializationResult::Tasks::Clear};
            AZStd::string message;
        };

        //! The ResultCode that was returned by the JSON serializer.
        AZ::JsonSerializationResult::ResultCode m_jsonResultCode{AZ::JsonSerializationResult::Tasks::Clear};

        //! Contains the set of all reports from the JSON serializer, grouped by json path.
        AZStd::map<AZStd::string/*json path*/, AZStd::vector<Report>> m_reports;

        //! Returns true if any json field was reported with the given outcome
        bool ContainsOutcome(AZ::JsonSerializationResult::Outcomes outcome);

        //! Returns true if a message was reported for a specific json field path.
        //! @jsonFieldPath - A json path like "/students/3/firstName"
        //! @messageSubstring - A substring to find in any message that was reported for the jsonFieldPath
        bool ContainsMessage(AZStd::string_view jsonFieldPath, AZStd::string_view messageSubstring);
    };

    //! Uses JsonSerialization to load JSON data into a reflected object
    template<typename T>
    JsonTestResult LoadTestDataFromJson(T& object, rapidjson::Value& json, AZ::RPI::JsonFileLoadContext* jsonFileLoadContext = nullptr)
    {
        JsonTestResult result;

        AZ::JsonDeserializerSettings settings;

        if (jsonFileLoadContext)
        {
            settings.m_metadata.Add(*jsonFileLoadContext);
        }
        
        settings.m_reporting = [&result](AZStd::string_view message, AZ::JsonSerializationResult::ResultCode resultCode, AZStd::string_view path)
        {
            JsonTestResult::Report report;
            report.message = message;
            report.resultCode = resultCode;
            result.m_reports[path].emplace_back(AZStd::move(report));

            return resultCode;
        };
        
        result.m_jsonResultCode = AZ::JsonSerialization::Load(object, json, settings);

        return result;
    }

    //! Uses JsonSerialization to load JSON data from a string into a reflected object
    template<typename T>
    JsonTestResult LoadTestDataFromJson(T& object, AZStd::string_view jsonText, AZ::RPI::JsonFileLoadContext* jsonFileLoadContext = nullptr)
    {
        auto parseResult = AZ::JsonSerializationUtils::ReadJsonString(jsonText);

        EXPECT_TRUE(parseResult.IsSuccess()) << parseResult.GetError().c_str();
        if (parseResult.IsSuccess())
        {
            return LoadTestDataFromJson(object, parseResult.GetValue(), jsonFileLoadContext);
        }
        else
        {
            JsonTestResult result;
            return result;
        }
    }

    //! Uses JsonSerialization to store a reflected object into a JSON string
    template<typename T>
    JsonTestResult StoreTestDataToJson(const T& object, AZStd::string& jsonText)
    {
        JsonTestResult result;

        AZ::JsonSerializerSettings serializerSettings;

        serializerSettings.m_reporting = [&result](AZStd::string_view message, AZ::JsonSerializationResult::ResultCode resultCode, AZStd::string_view path)
        {
            JsonTestResult::Report report;
            report.message = message;
            report.resultCode = resultCode;
            result.m_reports[path].emplace_back(AZStd::move(report));

            return resultCode;
        };

        rapidjson::Document jsonDocument;
        result.m_jsonResultCode = AZ::JsonSerialization::Store(jsonDocument, jsonDocument.GetAllocator(), object, serializerSettings);

        AZ::JsonSerializationUtils::WriteJsonSettings writeSettings;
        writeSettings.m_maxDecimalPlaces = 5;
        auto writeResult = AZ::JsonSerializationUtils::WriteJsonString(jsonDocument, jsonText, writeSettings);

        EXPECT_TRUE(writeResult.IsSuccess());

        return result;
    }

    //! Compares JSON strings, ignoring whitespace and casing.
    void ExpectSimilarJson(AZStd::string_view a, AZStd::string_view b);
}
