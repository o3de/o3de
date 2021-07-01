/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/JSON/rapidjson.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <native/InternalBuilders/SettingsRegistryBuilder.h>
#include <native/tests/AssetProcessorTest.h>

namespace AssetProcessor
{
    class SettingsRegistryBuilderTest
        : public AssetProcessorTest
    {
    };

    // These tests are done relative to "TestValues" because the Settings Registry adds runtime information for
    // anything that is merged in.


    TEST_F(SettingsRegistryBuilderTest, SettingsExporter_ExportRegistryToJson_ProducesIdenticalJsonToRegularWriter)
    {
        static constexpr char json[] =
            R"( {
                    "TestValues":
                    {
                        "BoolTrue": true,
                        "BoolFalse": false,
                        "Integer": 42,
                        "Double": 42.0,
                        "String": "hello",
                        "Array": [ null, true, false, 42, 42.0, "hello", { "Field": 42 }, [ 42, 42.0 ] ]
                    }
                })";

        rapidjson::Document document;
        document.Parse(json);
        ASSERT_FALSE(document.HasParseError());
        rapidjson::StringBuffer jsonOutputBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(jsonOutputBuffer);
        document.FindMember("TestValues")->value.Accept(writer);

        AZ::SettingsRegistryImpl registry;
        ASSERT_TRUE(registry.MergeSettings(json, AZ::SettingsRegistryInterface::Format::JsonMergePatch));
        rapidjson::StringBuffer registryOutputBuffer;
        AZStd::vector<AZStd::string> excludes;
        SettingsRegistryBuilder::SettingsExporter exporter(registryOutputBuffer, excludes);
        registry.Visit(exporter, "/TestValues");
        ASSERT_TRUE(exporter.Finalize());

        EXPECT_EQ(jsonOutputBuffer.GetLength(), registryOutputBuffer.GetLength());
        EXPECT_STREQ(jsonOutputBuffer.GetString(), registryOutputBuffer.GetString());
    }

    TEST_F(SettingsRegistryBuilderTest, SettingsExporter_FilterOutSection_FieldNotInOutput)
    {
        static constexpr char json[] =
            R"( {
                    "TestValues":
                    {
                        "A":
                        {
                            "B":
                            {
                                "X": 42
                            },
                            "C": true
                        }
                    }
                })";

        AZ::SettingsRegistryImpl registry;
        ASSERT_TRUE(registry.MergeSettings(json, AZ::SettingsRegistryInterface::Format::JsonMergePatch));
        rapidjson::StringBuffer registryOutputBuffer;
        AZStd::vector<AZStd::string> excludes;
        excludes.push_back("/TestValues/A/B");
        SettingsRegistryBuilder::SettingsExporter exporter(registryOutputBuffer, excludes);
        registry.Visit(exporter, "/TestValues");
        ASSERT_TRUE(exporter.Finalize());

        rapidjson::Document document;
        document.Parse(registryOutputBuffer.GetString(), registryOutputBuffer.GetLength());
        ASSERT_FALSE(document.HasParseError());

        auto it = document.FindMember("A");
        ASSERT_NE(document.MemberEnd(), it);
        EXPECT_EQ(it->value.MemberEnd(), it->value.FindMember("B"));
        EXPECT_NE(it->value.MemberEnd(), it->value.FindMember("C"));
    }

    TEST_F(SettingsRegistryBuilderTest, SettingsExporter_ExportRegistryWithNull_NullIsSerialized)
    {
        static constexpr char json[] =
            R"( [
                    { "op": "add", "path": "/TestValues", "value": { "Null": null } }
                ])";

        AZ::SettingsRegistryImpl registry;
        ASSERT_TRUE(registry.MergeSettings(json, AZ::SettingsRegistryInterface::Format::JsonPatch));
        rapidjson::StringBuffer registryOutputBuffer;
        AZStd::vector<AZStd::string> excludes;
        SettingsRegistryBuilder::SettingsExporter exporter(registryOutputBuffer, excludes);
        registry.Visit(exporter, "/TestValues");
        ASSERT_TRUE(exporter.Finalize());

        rapidjson::Document document;
        document.Parse(registryOutputBuffer.GetString(), registryOutputBuffer.GetLength());
        ASSERT_FALSE(document.HasParseError());

        auto it = document.FindMember("Null");
        ASSERT_NE(document.MemberEnd(), it);
        EXPECT_TRUE(it->value.IsNull());
    }

    TEST_F(SettingsRegistryBuilderTest, SettingsExporter_ExportCanBeReused_SecondExportWorksCorrectly)
    {
        static constexpr char jsonFirst[] =
            R"( {
                    "TestValues": { "FirstPass" : 1 }
                })";
        static constexpr char jsonSecond[] =
            R"( {
                    "TestValues": { "SecondPass" : 1 }
                })";

        AZ::SettingsRegistryImpl registryFirst;
        ASSERT_TRUE(registryFirst.MergeSettings(jsonFirst, AZ::SettingsRegistryInterface::Format::JsonMergePatch));
        AZ::SettingsRegistryImpl registrySecond;
        ASSERT_TRUE(registrySecond.MergeSettings(jsonSecond, AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        rapidjson::StringBuffer registryOutputBuffer;
        AZStd::vector<AZStd::string> excludes;
        SettingsRegistryBuilder::SettingsExporter exporter(registryOutputBuffer, excludes);
        registryFirst.Visit(exporter, "/TestValues");
        ASSERT_TRUE(exporter.Finalize());
        registryOutputBuffer.Clear();
        exporter.Reset(registryOutputBuffer);
        registrySecond.Visit(exporter, "/TestValues");

        rapidjson::Document document;
        document.Parse(jsonSecond);
        ASSERT_FALSE(document.HasParseError());
        rapidjson::StringBuffer jsonOutputBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(jsonOutputBuffer);
        document.FindMember("TestValues")->value.Accept(writer);

        EXPECT_EQ(jsonOutputBuffer.GetLength(), registryOutputBuffer.GetLength());
        EXPECT_STREQ(jsonOutputBuffer.GetString(), registryOutputBuffer.GetString());
    }
} // namespace AssetProcessor
