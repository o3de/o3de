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
#include <AzTest/AzTest.h>
#include "ProjectSettings.h"
#include "GemDescription.h"

using Gems::ProjectSettings;
using Gems::ProjectGemSpecifier;
using AzFramework::Specifier;
using Gems::GemVersion;

class ProjectSettingsTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_registry = aznew Gems::GemRegistry();
        m_errCreationFailed                     = "Failed to create a ProjectSettings instance.";
        m_errNonEmptyEnabledGems                = "EnabledGems should start empty.";
        m_errEnableGemFailed                    = "Failed to enable valid Gem Spec.";
        m_errIsGemEnabledFailed                 = "Failed to accurately determine if a gem was enabled or not";
        m_errDescriptionParseFailed             = "Failed to parse valid Description: ";
        m_errInvalidDescriptionParseSucceeded   = "Parsing of invalid Description succeeded.";
    }

    void TearDown() override
    {
        delete m_registry;
    }

    enum GenerateJsonFlags : AZ::u8
    {
        GJF_IncludeID       = 1 << 0,
        GJF_IncludeVersion  = 1 << 1,
        GJF_IncludePath     = 1 << 2,
        GJF_All             = GJF_IncludeID | GJF_IncludeVersion | GJF_IncludePath,
    };

    rapidjson::Document ParseFromString(AZStd::string document)
    {
        rapidjson::Document root(rapidjson::kObjectType);
        root.Parse(document.c_str());
        return root;
    }

    void GenerateJson(rapidjson::Document& rootObj, AZ::Uuid id, GemVersion version, const AZStd::string& path, AZ::u8 flags)
    {
        rootObj.SetObject();
        rootObj.AddMember(GPF_TAG_LIST_FORMAT_VERSION, GEMS_PROJECT_FILE_VERSION, rootObj.GetAllocator());

        rapidjson::Value gemsArray(rapidjson::kArrayType);

        if (flags != 0)
        {
            // build a gem
            rapidjson::Value gemObj(rapidjson::kObjectType);

            if (flags & GJF_IncludeID)
            {
                char idstr[UUID_STR_BUF_LEN];
                id.ToString(idstr, UUID_STR_BUF_LEN, false, false);
                rapidjson::Value v(idstr, rootObj.GetAllocator());
                gemObj.AddMember(GPF_TAG_UUID, v.Move(), rootObj.GetAllocator());
            }

            if (flags & GJF_IncludeVersion)
            {
                rapidjson::Value v(version.ToString().c_str(), rootObj.GetAllocator());
                gemObj.AddMember(GPF_TAG_VERSION, v.Move(), rootObj.GetAllocator());
            }

            if (flags & GJF_IncludePath)
            {
                rapidjson::Value v(path.c_str(), rootObj.GetAllocator());
                gemObj.AddMember(GPF_TAG_PATH, v.Move(), rootObj.GetAllocator());
            }

            gemsArray.PushBack(gemObj.Move(), rootObj.GetAllocator()); // copy into array
        }

        rootObj.AddMember(GPF_TAG_GEM_ARRAY, gemsArray.Move(), rootObj.GetAllocator()); // copy into root object
    }

    Gems::GemRegistry* m_registry;
    const char* m_errCreationFailed;
    const char* m_errNonEmptyEnabledGems;
    const char* m_errEnableGemFailed;
    const char* m_errIsGemEnabledFailed;
    const char* m_errDescriptionParseFailed;
    const char* m_errInvalidDescriptionParseSucceeded;
};

TEST_F(ProjectSettingsTest, CreateAndDestroyTest)
{
    Gems::IProjectSettings* ps = m_registry->CreateProjectSettings();
    EXPECT_NE(ps, nullptr)                                  << m_errCreationFailed;
    m_registry->DestroyProjectSettings(ps);
}

TEST_F(ProjectSettingsTest, EnableDisableTest)
{
    ProjectSettings ps {
        m_registry
    };
    AZ::Uuid id = AZ::Uuid::CreateRandom();
    GemVersion v1 {
        1, 0, 0
    };
    AZStd::string path {
        "some\\path"
    };
    ProjectGemSpecifier s0 {
        id, v1, path
    };

    EXPECT_TRUE(ps.EnableGem(s0))                           << m_errEnableGemFailed;
    EXPECT_TRUE(ps.IsGemEnabled(s0))                        << m_errEnableGemFailed;

    EXPECT_TRUE(ps.DisableGem(s0))                          << m_errEnableGemFailed;
    EXPECT_FALSE(ps.IsGemEnabled(s0))                       << m_errEnableGemFailed;
}

TEST_F(ProjectSettingsTest, IsEnabledTest)
{
    ProjectSettings ps {
        m_registry
    };
    ProjectGemSpecifier enabledGemSpecifier {
        AZ::Uuid::CreateRandom(), GemVersion {
            1, 0, 0
        }, "some\\path"
    };

    EXPECT_TRUE(ps.EnableGem(enabledGemSpecifier))          << m_errEnableGemFailed;

    // Compare 1.0.0 to 1.0.0
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "<1.0.0" }))  << m_errIsGemEnabledFailed;
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "<=1.0.0" }))  << m_errIsGemEnabledFailed;
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "==1.0.0" }))  << m_errIsGemEnabledFailed;
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">=1.0.0" }))  << m_errIsGemEnabledFailed;
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">1.0.0" }))  << m_errIsGemEnabledFailed;

    // Compare 1.0.0 to 1.0.1
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "<1.0.1" }))   << m_errIsGemEnabledFailed;
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "<=1.0.1" }))  << m_errIsGemEnabledFailed;
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "==1.0.1" })) << m_errIsGemEnabledFailed;
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">=1.0.1" })) << m_errIsGemEnabledFailed;
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">1.0.1" }))  << m_errIsGemEnabledFailed;

    // Compare 1.0.0 to 0.1.1
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "<0.1.1" }))  << m_errIsGemEnabledFailed;
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "<=0.1.1" })) << m_errIsGemEnabledFailed;
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "==0.1.1" })) << m_errIsGemEnabledFailed;
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">=0.1.1" }))  << m_errIsGemEnabledFailed;
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">0.1.1" }))   << m_errIsGemEnabledFailed;

    // Test ranges around the enabled gem
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">=0.1.0", "<=1.1.0" }))   << m_errIsGemEnabledFailed;
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "<=1.1.0", ">=0.1.0" }))   << m_errIsGemEnabledFailed;
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "~>1.0.0" }))              << m_errIsGemEnabledFailed;
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "~>1.0" }))                << m_errIsGemEnabledFailed;

    // Test ranges at or above the enabled gem
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">=1.0.0", "<=1.1.0" }))   << m_errIsGemEnabledFailed;
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">1.0.0", "<=1.1.0" }))   << m_errIsGemEnabledFailed;
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "~>1.1.0" }))             << m_errIsGemEnabledFailed;

    //// Test ranges at or below the enabled gem
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">=0.1.0", "<1.0.0" }))   << m_errIsGemEnabledFailed;
    EXPECT_TRUE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { ">=0.1.0", "<=1.0.0" }))   << m_errIsGemEnabledFailed;
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "~>0.1.0" }))             << m_errIsGemEnabledFailed;
    EXPECT_FALSE(ps.IsGemEnabled(enabledGemSpecifier.m_id, { "~>0.1" }))               << m_errIsGemEnabledFailed;
}

TEST_F(ProjectSettingsTest, ParseTest)
{
    ProjectSettings ps {
        m_registry
    };
    AZ::Uuid id = AZ::Uuid::CreateRandom();
    GemVersion v1 {
        1, 0, 0
    };
    AZStd::string path("Some\\Path");

    rapidjson::Document json(rapidjson::kObjectType);
    GenerateJson(json, id, v1, path, GJF_All);
    AZ::Outcome<void, AZStd::string> outcome = ps.ParseGemsJson(json);
    ASSERT_TRUE(outcome.IsSuccess())                << m_errDescriptionParseFailed << outcome.GetError().c_str();
    auto gemMap = ps.GetGems();
    EXPECT_EQ(gemMap.size(), 1)                     << m_errDescriptionParseFailed << outcome.GetError().c_str();
    EXPECT_EQ(gemMap.begin()->second.m_id, id)      << m_errDescriptionParseFailed << outcome.GetError().c_str();
    EXPECT_EQ(gemMap.begin()->second.m_version, v1) << m_errDescriptionParseFailed << outcome.GetError().c_str();

    GenerateJson(json, id, v1, path, GJF_IncludeVersion | GJF_IncludePath);
    EXPECT_FALSE(ps.ParseGemsJson(json))                << m_errInvalidDescriptionParseSucceeded;

    GenerateJson(json, id, v1, path, GJF_IncludeID | GJF_IncludePath);
    EXPECT_FALSE(ps.ParseGemsJson(json))                << m_errInvalidDescriptionParseSucceeded;

    GenerateJson(json, id, v1, path, GJF_IncludeVersion | GJF_IncludeID);
    EXPECT_FALSE(ps.ParseGemsJson(json))                << m_errInvalidDescriptionParseSucceeded;

    GenerateJson(json, id, v1, path, GJF_IncludeID | GJF_IncludeVersion | GJF_IncludePath);
    EXPECT_TRUE(ps.ParseGemsJson(json))                 << m_errDescriptionParseFailed;
}

TEST_F(ProjectSettingsTest, SaveTest)
{
    ProjectSettings ps {
        m_registry
    };
    AZ::Uuid id = AZ::Uuid::CreateRandom();
    char idStr[UUID_STR_BUF_LEN];
    id.ToString(idStr, UUID_STR_BUF_LEN, false, false);
    GemVersion v1 {
        1, 0, 0
    };
    AZStd::string path("Some\\Path");

    rapidjson::Document json(rapidjson::kObjectType);
    GenerateJson(json, id, v1, path, GJF_All);
    ASSERT_TRUE(ps.ParseGemsJson(json))                             << m_errDescriptionParseFailed;
    EXPECT_TRUE(json.HasMember(GPF_TAG_LIST_FORMAT_VERSION))    << m_errDescriptionParseFailed;
    EXPECT_TRUE(json[GPF_TAG_LIST_FORMAT_VERSION].IsInt())      << m_errDescriptionParseFailed;
    EXPECT_EQ(json[GPF_TAG_LIST_FORMAT_VERSION].GetInt(), GEMS_PROJECT_FILE_VERSION) << m_errDescriptionParseFailed;
    ASSERT_TRUE(json.HasMember(GPF_TAG_GEM_ARRAY))              << m_errDescriptionParseFailed;
    ASSERT_TRUE(json[GPF_TAG_GEM_ARRAY].IsArray())              << m_errDescriptionParseFailed;
    ASSERT_EQ(json[GPF_TAG_GEM_ARRAY].Size(), 1)                << m_errDescriptionParseFailed;
    EXPECT_STRCASEEQ(json[GPF_TAG_GEM_ARRAY][0][GPF_TAG_UUID].GetString(), idStr) << m_errDescriptionParseFailed;
    EXPECT_STRCASEEQ(json[GPF_TAG_GEM_ARRAY][0][GPF_TAG_VERSION].GetString(), v1.ToString().c_str()) << m_errDescriptionParseFailed;
    EXPECT_STRCASEEQ(json[GPF_TAG_GEM_ARRAY][0][GPF_TAG_PATH].GetString(), path.c_str()) << m_errDescriptionParseFailed;

    ps.DisableGem({ id, v1 });
    json = ps.GetJsonRepresentation();
    ASSERT_EQ(json[GPF_TAG_GEM_ARRAY].Size(), 0);
}
