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
#include "GemDescription.h"

using Gems::GemDescription;
using Gems::ModuleDefinition;

class DescriptionTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {

    }

    /// Helper to parse json text to a GemDescription result
    static AZ::Outcome<GemDescription, AZStd::string> CreateFromString(const char* text)
    {
        rapidjson::Document document;
        document.Parse(text);
        EXPECT_FALSE(document.HasParseError());

        return GemDescription::CreateFromJson(document, "", "");
    }

    /// Helper to parse json text (that should compile) to a GemDescription
    static GemDescription ParseString(const char* text)
    {
        auto result = CreateFromString(text);
        EXPECT_TRUE(result.IsSuccess());

        return AZStd::move(result.GetValue());
    }
};

// Helper asserts
#define EXPECT_MODULE_COUNT(desc, expectedCount)            EXPECT_EQ(expectedCount, desc.GetModules().size());
#define EXPECT_MODULE_TYPE_COUNT(desc, type, expectedCount) EXPECT_EQ(expectedCount, desc.GetModulesOfType(ModuleDefinition::Type::type).size())

////////////////////////////////////////////////////////////////////////
// Success tests
////////////////////////////////////////////////////////////////////////

// Test for parsing V3 Gem Descriptions
TEST_F(DescriptionTest, ParseJson_V3_GameModule)
{
    static const char* description = R"JSON(
{
    "GemFormatVersion": 3,
    "Uuid": "ff06785f7145416b9d46fde39098cb0c",
    "Name": "LmbrCentral",
    "Version": "0.1.0",
    "LinkType": "Dynamic",
    "Summary": "Required LmbrCentral Engine Gem.",
    "Tags": ["Untagged"],
    "IconPath": "preview.png",
    "IsRequired": true
}
)JSON";

    GemDescription desc = ParseString(description);

    EXPECT_MODULE_COUNT(desc, 1);

    EXPECT_MODULE_TYPE_COUNT(desc, GameModule, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, EditorModule, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, StaticLib, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Builder, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Standalone, 0);
}

// Test for parsing V3 Gem Descriptions with an editor module
TEST_F(DescriptionTest, ParseJson_V3_EditorModule)
{
    static const char* description = R"JSON(
{
    "GemFormatVersion": 3,
    "Uuid": "ff06785f7145416b9d46fde39098cb0c",
    "Name": "LmbrCentral",
    "Version": "0.1.0",
    "LinkType": "Dynamic",
    "Summary": "Required LmbrCentral Engine Gem.",
    "Tags": ["Untagged"],
    "IconPath": "preview.png",
    "EditorModule": true,
    "IsRequired": true
}
)JSON";

    GemDescription desc = ParseString(description);

    EXPECT_MODULE_COUNT(desc, 2);

    EXPECT_MODULE_TYPE_COUNT(desc, GameModule, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, EditorModule, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, StaticLib, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Builder, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Standalone, 0);
}

// Test for parsing V4 Gem Descriptions with a game module
TEST_F(DescriptionTest, ParseJson_V4_GameModule)
{
    static const char* description = R"JSON(
{
    "GemFormatVersion": 4,
    "Uuid": "f910686b6725452fbfc4671f95f733c6",
    "Name": "Camera",
    "Version": "0.1.0",
    "DisplayName": "Camera",
    "Tags": ["Camera"],
    "Summary": "The Camera Gem includes a basic camera component that defines a frustum for runtime rendering.",
    "IconPath": "preview.png",
    "Modules": [
        {
            "Type": "GameModule"
        }
    ]
}
)JSON";

    GemDescription desc = ParseString(description);

    EXPECT_MODULE_COUNT(desc, 1);

    EXPECT_MODULE_TYPE_COUNT(desc, GameModule, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, EditorModule, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, StaticLib, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Builder, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Standalone, 0);
}

// Test for parsing V4 Gem Descriptions with an editor module
TEST_F(DescriptionTest, ParseJson_V4_EditorModule)
{
    static const char* description = R"JSON(
{
    "GemFormatVersion": 4,
    "Uuid": "f910686b6725452fbfc4671f95f733c6",
    "Name": "Camera",
    "Version": "0.1.0",
    "DisplayName": "Camera",
    "Tags": ["Camera"],
    "Summary": "The Camera Gem includes a basic camera component that defines a frustum for runtime rendering.",
    "IconPath": "preview.png",
    "Modules": [
        {
            "Name": "Editor",
            "Type": "EditorModule"
        }
    ]
}
)JSON";

    GemDescription desc = ParseString(description);

    EXPECT_MODULE_COUNT(desc, 1);

    EXPECT_MODULE_TYPE_COUNT(desc, GameModule, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, EditorModule, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, StaticLib, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Builder, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Standalone, 0);
}

// Test for parsing V4 Gem Descriptions with a game and an editor module (that extends the editor module
TEST_F(DescriptionTest, ParseJson_V4_EditorModuleExtends)
{
    static const char* description = R"JSON(
{
    "GemFormatVersion": 4,
    "Uuid": "f910686b6725452fbfc4671f95f733c6",
    "Name": "Camera",
    "Version": "0.1.0",
    "DisplayName": "Camera",
    "Tags": ["Camera"],
    "Summary": "The Camera Gem includes a basic camera component that defines a frustum for runtime rendering.",
    "IconPath": "preview.png",
    "Modules": [
        {
            "Type": "GameModule"
        },
        {
            "Name": "Editor",
            "Type": "EditorModule",
            "Extends": "GameModule"
        }
    ]
}
)JSON";

    GemDescription desc = ParseString(description);

    EXPECT_MODULE_COUNT(desc, 2);

    EXPECT_MODULE_TYPE_COUNT(desc, GameModule, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, EditorModule, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, StaticLib, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Builder, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Standalone, 0);
}

// Test for parsing V4 Gem Descriptions with a static lib
TEST_F(DescriptionTest, ParseJson_V4_StaticLib)
{
    static const char* description = R"JSON(
{
    "GemFormatVersion": 4,
    "Uuid": "f910686b6725452fbfc4671f95f733c6",
    "Name": "Camera",
    "Version": "0.1.0",
    "DisplayName": "Camera",
    "Tags": ["Camera"],
    "Summary": "The Camera Gem includes a basic camera component that defines a frustum for runtime rendering.",
    "IconPath": "preview.png",
    "Modules": [
        {
            "Name": "CameraHelper",
            "Type": "StaticLib"
        }
    ]
}
)JSON";

    GemDescription desc = ParseString(description);

    EXPECT_MODULE_COUNT(desc, 1);

    EXPECT_MODULE_TYPE_COUNT(desc, GameModule, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, EditorModule, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, StaticLib, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, Builder, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Standalone, 0);
}

// Test for parsing V4 Gem Descriptions with a static lib
TEST_F(DescriptionTest, ParseJson_V4_Standalone)
{
    static const char* description = R"JSON(
{
    "GemFormatVersion": 4,
    "Uuid": "f910686b6725452fbfc4671f95f733c6",
    "Name": "Camera",
    "Version": "0.1.0",
    "DisplayName": "Camera",
    "Tags": ["Camera"],
    "Summary": "The Camera Gem includes a basic camera component that defines a frustum for runtime rendering.",
    "IconPath": "preview.png",
    "Modules": [
        {
            "Name": "CameraHelper",
            "Type": "Standalone"
        }
    ]
}
)JSON";

    GemDescription desc = ParseString(description);

    EXPECT_MODULE_COUNT(desc, 1);

    EXPECT_MODULE_TYPE_COUNT(desc, GameModule, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, EditorModule, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, StaticLib, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Builder, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Standalone, 1);
}

// Test for parsing V4 Gem Descriptions with a builder module
TEST_F(DescriptionTest, ParseJson_V4_BuilderModule)
{
    static const char* description = R"JSON(
{
    "GemFormatVersion": 4,
    "Uuid": "f910686b6725452fbfc4671f95f733c6",
    "Name": "Camera",
    "Version": "0.1.0",
    "DisplayName": "Camera",
    "Tags": ["Camera"],
    "Summary": "The Camera Gem includes a basic camera component that defines a frustum for runtime rendering.",
    "IconPath": "preview.png",
    "Modules": [
        {
            "Name": "CameraBuilder",
            "Type": "Builder"
        }
    ]
}
)JSON";

    GemDescription desc = ParseString(description);

    EXPECT_MODULE_COUNT(desc, 1);

    EXPECT_MODULE_TYPE_COUNT(desc, GameModule, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, EditorModule, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, StaticLib, 0);
    EXPECT_MODULE_TYPE_COUNT(desc, Builder, 1);
    EXPECT_MODULE_TYPE_COUNT(desc, Standalone, 0);
}

////////////////////////////////////////////////////////////////////////
// Failure tests
////////////////////////////////////////////////////////////////////////

// Test for parsing V4 Gem Descriptions with an invalid extends
TEST_F(DescriptionTest, ParseJson_V4_ExtendsNonExistantModule)
{
    static const char* description = R"JSON(
{
    "GemFormatVersion": 4,
    "Uuid": "f910686b6725452fbfc4671f95f733c6",
    "Name": "Camera",
    "Version": "0.1.0",
    "DisplayName": "Camera",
    "Tags": ["Camera"],
    "Summary": "The Camera Gem includes a basic camera component that defines a frustum for runtime rendering.",
    "IconPath": "preview.png",
    "Modules": [
        {
            "Type": "GameModule"
        },
        {
            "Name": "Editor",
            "Type": "EditorModule",
            "Extends": "ModuleThatDoesntExist"
        }
    ]
}
)JSON";

    EXPECT_FALSE(CreateFromString(description).IsSuccess());
}

// Test for parsing V4 Gem Descriptions with an invalid extends
TEST_F(DescriptionTest, ParseJson_V4_ExtendsStaticLib)
{
    static const char* description = R"JSON(
{
    "GemFormatVersion": 4,
    "Uuid": "f910686b6725452fbfc4671f95f733c6",
    "Name": "Camera",
    "Version": "0.1.0",
    "DisplayName": "Camera",
    "Tags": ["Camera"],
    "Summary": "The Camera Gem includes a basic camera component that defines a frustum for runtime rendering.",
    "IconPath": "preview.png",
    "Modules": [
        {
            "Name": "CameraHelper",
            "Type": "StaticLib"
        },
        {
            "Name": "Editor",
            "Type": "EditorModule",
            "Extends": "CameraHelper"
        }
    ]
}
)JSON";

    EXPECT_FALSE(CreateFromString(description).IsSuccess());
}