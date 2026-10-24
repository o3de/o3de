/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/JSON/document.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/std/any.h>

#include <AzTest/AzTest.h>
#include <AzTest/Utils.h>

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EntityPresets/EntityPresets.h>
#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

namespace UnitTest
{
    using namespace AzToolsFramework;

    //! An enum with a narrow underlying type, like Atom's LightType. PropertyTreeEditor only
    //! converts from double and s64, so presets must hand it an s64 to reach such a property.
    enum class EntityPresetsTestEnum : AZ::u8
    {
        First = 0,
        Second,
        Third
    };

    //! One property of every kind a preset can set, reflected like a real component.
    struct EntityPresetsTestConfig
    {
        AZ_TYPE_INFO(EntityPresetsTestConfig, "{6E4C97A5-2E0D-4F91-9B3C-1D5A7F8E2B44}");

        static void Reflect(AZ::SerializeContext* context)
        {
            context->Class<EntityPresetsTestConfig>()
                ->Version(1)
                ->Field("My Bool", &EntityPresetsTestConfig::m_bool)
                ->Field("My Int", &EntityPresetsTestConfig::m_int)
                ->Field("My Float", &EntityPresetsTestConfig::m_float)
                ->Field("My String", &EntityPresetsTestConfig::m_string)
                ->Field("My Enum", &EntityPresetsTestConfig::m_enum);

            // PropertyTreeEditor finds properties by their Edit Context names and skips nodes without
            // edit metadata, so an Edit Context is required.
            AZ::EditContext* editContext = context->GetEditContext();
            if (editContext == nullptr)
            {
                editContext = context->CreateEditContext();
            }

            editContext->Class<EntityPresetsTestConfig>("Entity Presets Test Config", "")
                ->DataElement(AZ::Edit::UIHandlers::Default, &EntityPresetsTestConfig::m_bool, "My Bool", "")
                ->DataElement(AZ::Edit::UIHandlers::Default, &EntityPresetsTestConfig::m_int, "My Int", "")
                ->DataElement(AZ::Edit::UIHandlers::Default, &EntityPresetsTestConfig::m_float, "My Float", "")
                ->DataElement(AZ::Edit::UIHandlers::Default, &EntityPresetsTestConfig::m_string, "My String", "")
                // No EnumAttributes: the conversion has to work on the type itself.
                ->DataElement(AZ::Edit::UIHandlers::Default, &EntityPresetsTestConfig::m_enum, "My Enum", "");
        }

        bool m_bool = false;
        int m_int = 0;
        float m_float = 0.0f;
        AZStd::string m_string;
        EntityPresetsTestEnum m_enum = EntityPresetsTestEnum::First;
    };

    class EntityPresetsTests : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(AzFramework::Application::Descriptor(), startupParameters);

            // Keep the shared user settings file from being saved, so parallel test runs do not collide.
            AZ::UserSettingsComponentRequestBus::Broadcast(
                &AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            AZ::ComponentApplicationBus::BroadcastResult(
                m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            EntityPresetsTestConfig::Reflect(m_serializeContext);
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        //! Set a property the way EntityPresets::Create does, through PropertyValue::ToAny.
        static PropertyTreeEditor::PropertyAccessOutcome SetThroughToAny(
            PropertyTreeEditor& tree, const char* path, const EntityPresets::PropertyValue& value)
        {
            bool resolved = false;
            const AZStd::any converted = value.ToAny(resolved);
            EXPECT_TRUE(resolved);

            return tree.SetProperty(path, converted);
        }

        static EntityPresets::PropertyValue IntValue(const AZ::s64 value)
        {
            EntityPresets::PropertyValue property;
            property.m_type = EntityPresets::PropertyValue::Type::Int;
            property.m_int = value;
            return property;
        }

        ToolsTestApplication m_app{ "EntityPresetsTests" };
        AZ::SerializeContext* m_serializeContext = nullptr;
    };

    // -- Type conversion ---------------------------------------------------------------

    TEST_F(EntityPresetsTests, IntValueReachesAnEnumProperty)
    {
        EntityPresetsTestConfig config;
        PropertyTreeEditor tree(&config, AZ::AzTypeInfo<EntityPresetsTestConfig>::Uuid());

        const auto outcome = SetThroughToAny(tree, "My Enum", IntValue(2));

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(config.m_enum, EntityPresetsTestEnum::Third);
    }

    TEST_F(EntityPresetsTests, IntValueReachesAnIntProperty)
    {
        EntityPresetsTestConfig config;
        PropertyTreeEditor tree(&config, AZ::AzTypeInfo<EntityPresetsTestConfig>::Uuid());

        const auto outcome = SetThroughToAny(tree, "My Int", IntValue(5000000));

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(config.m_int, 5000000);
    }

    TEST_F(EntityPresetsTests, DoubleValueReachesAFloatProperty)
    {
        EntityPresetsTestConfig config;
        PropertyTreeEditor tree(&config, AZ::AzTypeInfo<EntityPresetsTestConfig>::Uuid());

        EntityPresets::PropertyValue value;
        value.m_type = EntityPresets::PropertyValue::Type::Double;
        value.m_double = 0.25;

        const auto outcome = SetThroughToAny(tree, "My Float", value);

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_FLOAT_EQ(config.m_float, 0.25f);
    }

    TEST_F(EntityPresetsTests, BoolValueReachesABoolProperty)
    {
        EntityPresetsTestConfig config;
        PropertyTreeEditor tree(&config, AZ::AzTypeInfo<EntityPresetsTestConfig>::Uuid());

        EntityPresets::PropertyValue value;
        value.m_type = EntityPresets::PropertyValue::Type::Bool;
        value.m_bool = true;

        const auto outcome = SetThroughToAny(tree, "My Bool", value);

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_TRUE(config.m_bool);
    }

    TEST_F(EntityPresetsTests, StringValueReachesAStringProperty)
    {
        EntityPresetsTestConfig config;
        PropertyTreeEditor tree(&config, AZ::AzTypeInfo<EntityPresetsTestConfig>::Uuid());

        EntityPresets::PropertyValue value;
        value.m_type = EntityPresets::PropertyValue::Type::String;
        value.m_string = "a string";

        const auto outcome = SetThroughToAny(tree, "My String", value);

        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_STREQ(config.m_string.c_str(), "a string");
    }

    TEST_F(EntityPresetsTests, IntValueConvertsToSignedSixtyFourBit)
    {
        bool resolved = false;
        const AZStd::any converted = IntValue(1).ToAny(resolved);

        EXPECT_TRUE(resolved);

        // Names the one integer type that converts to every integer property.
        EXPECT_EQ(converted.type(), azrtti_typeid<AZ::s64>());
    }

    // -- The JSON format ----------------------------------------------------------------

    TEST_F(EntityPresetsTests, PropertyAssignmentIsWrittenFlat)
    {
        EntityPresets::PresetFile file;

        EntityPresets::Preset preset;
        preset.m_name = "Point Light (Sphere)";
        preset.m_category = "Lights";

        EntityPresets::ComponentSpec component;
        component.m_componentName = "Light";
        component.m_properties.push_back(
            EntityPresets::PropertyAssignment{ "Controller|Configuration|Light type", IntValue(1) });

        preset.m_components.push_back(component);
        file.m_presets.push_back(preset);

        rapidjson::Document document;
        AZ::JsonSerializerSettings settings;
        settings.m_keepDefaults = true;

        const auto result =
            AZ::JsonSerialization::Store(document, document.GetAllocator(), file, settings);
        ASSERT_NE(result.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);

        const rapidjson::Value& assignment =
            document["presets"][0]["components"][0]["properties"][0];

        // Flat, not nested.
        ASSERT_TRUE(assignment.IsObject());
        EXPECT_TRUE(assignment["path"].IsString());
        EXPECT_TRUE(assignment["type"].IsString());
        EXPECT_STREQ(assignment["type"].GetString(), "int");
        EXPECT_TRUE(assignment["value"].IsNumber());
        EXPECT_EQ(assignment["value"].GetInt64(), 1);
        EXPECT_FALSE(assignment.HasMember("m_value"));
    }

    TEST_F(EntityPresetsTests, PresetsRoundTripThroughJson)
    {
        EntityPresets::PresetFile written;

        EntityPresets::Preset preset;
        preset.m_name = "Exposure Volume";
        preset.m_category = "PostFX";

        EntityPresets::ComponentSpec component;
        component.m_componentName = "PostFX Layer";
        component.m_properties.push_back(EntityPresets::PropertyAssignment{
            "Controller|Configuration|Layer Category", IntValue(5000000) });

        preset.m_components.push_back(component);
        written.m_presets.push_back(preset);

        rapidjson::Document document;
        AZ::JsonSerializerSettings storeSettings;
        storeSettings.m_keepDefaults = true;

        const auto stored =
            AZ::JsonSerialization::Store(document, document.GetAllocator(), written, storeSettings);
        ASSERT_NE(stored.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);

        EntityPresets::PresetFile read;
        const auto loaded = AZ::JsonSerialization::Load(read, document);
        ASSERT_NE(loaded.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);

        ASSERT_EQ(read.m_presets.size(), 1);
        EXPECT_STREQ(read.m_presets[0].m_name.c_str(), "Exposure Volume");
        EXPECT_STREQ(read.m_presets[0].m_category.c_str(), "PostFX");

        ASSERT_EQ(read.m_presets[0].m_components.size(), 1);
        const EntityPresets::ComponentSpec& readComponent = read.m_presets[0].m_components[0];
        EXPECT_STREQ(readComponent.m_componentName.c_str(), "PostFX Layer");

        ASSERT_EQ(readComponent.m_properties.size(), 1);
        EXPECT_STREQ(readComponent.m_properties[0].m_path.c_str(), "Controller|Configuration|Layer Category");
        EXPECT_EQ(readComponent.m_properties[0].m_value.m_type, EntityPresets::PropertyValue::Type::Int);
        EXPECT_EQ(readComponent.m_properties[0].m_value.m_int, 5000000);
    }

    TEST_F(EntityPresetsTests, PresetsFileFromBeforeTheRewriteStillLoads)
    {
        // Written by hand, to prove an existing project file still loads.
        constexpr const char* legacy = R"JSON(
        {
            "presets": [
                {
                    "name": "Quad Light",
                    "category": "Lights",
                    "components": [
                        {
                            "component": "Light",
                            "properties": [
                                {
                                    "type": "int",
                                    "value": 4,
                                    "path": "Controller|Configuration|Light type"
                                }
                            ]
                        }
                    ]
                },
                {
                    "name": "Shaderball",
                    "category": "Meshes",
                    "components": [
                        {
                            "component": "Mesh",
                            "properties": [
                                {
                                    "type": "asset",
                                    "value": "materialeditor/viewportmodels/shaderball.fbx.azmodel",
                                    "path": "Controller|Configuration|Model Asset"
                                }
                            ]
                        }
                    ]
                }
            ]
        })JSON";

        const auto parsed = AZ::JsonSerializationUtils::ReadJsonString(legacy);
        ASSERT_TRUE(parsed.IsSuccess());

        EntityPresets::PresetFile file;
        const auto loaded = AZ::JsonSerialization::Load(file, parsed.GetValue());
        ASSERT_NE(loaded.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);

        ASSERT_EQ(file.m_presets.size(), 2);

        EXPECT_STREQ(file.m_presets[0].m_name.c_str(), "Quad Light");
        ASSERT_EQ(file.m_presets[0].m_components.size(), 1);
        ASSERT_EQ(file.m_presets[0].m_components[0].m_properties.size(), 1);
        EXPECT_EQ(file.m_presets[0].m_components[0].m_properties[0].m_value.m_int, 4);

        // For assets the type decides how the string is read.
        const EntityPresets::PropertyAssignment& mesh = file.m_presets[1].m_components[0].m_properties[0];
        EXPECT_EQ(mesh.m_value.m_type, EntityPresets::PropertyValue::Type::AssetPath);
        EXPECT_STREQ(mesh.m_value.m_string.c_str(), "materialeditor/viewportmodels/shaderball.fbx.azmodel");
    }

    TEST_F(EntityPresetsTests, ProvenanceIsNotWrittenToFile)
    {
        EntityPresets::PresetFile file;

        EntityPresets::Preset preset;
        preset.m_name = "From A Gem";
        preset.m_category = "Lights";

        // Runtime-only fields must not be written.
        preset.m_readOnly = true;
        preset.m_sourceGem = "SomeGem";

        file.m_presets.push_back(preset);

        rapidjson::Document document;
        AZ::JsonSerializerSettings settings;
        settings.m_keepDefaults = true;

        const auto result =
            AZ::JsonSerialization::Store(document, document.GetAllocator(), file, settings);
        ASSERT_NE(result.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);

        const rapidjson::Value& written = document["presets"][0];
        EXPECT_FALSE(written.HasMember("builtIn"));
        EXPECT_FALSE(written.HasMember("sourceGem"));
    }
    // -- Files on disk -------------------------------------------------------------------
    //
    // Gem files are read with LoadObjectFromFile, which requires the JsonSerialization header.

    TEST_F(EntityPresetsTests, GemPresetFileLoadsFromDisk)
    {
        constexpr const char* gemShaped = R"JSON(
        {
            "Type": "JsonSerialization",
            "Version": 1,
            "ClassName": "PresetFile",
            "ClassData": {
                "presets": [
                    {
                        "name": "Box Shape",
                        "category": "Shapes",
                        "components": [
                            { "component": "Box Shape", "properties": [] }
                        ]
                    },
                    {
                        "name": "Quad Light",
                        "category": "Lights",
                        "components": [
                            {
                                "component": "Light",
                                "properties": [
                                    {
                                        "path": "Controller|Configuration|Light type",
                                        "type": "int",
                                        "value": 4
                                    }
                                ]
                            }
                        ]
                    }
                ]
            }
        })JSON";

        AZ::Test::ScopedAutoTempDirectory tempDirectory;
        const auto filePath = AZ::Test::CreateTestFile(tempDirectory, "shapes.entitypresets.json", gemShaped);
        ASSERT_TRUE(filePath.has_value());

        EntityPresets::PresetFile file;
        const auto loaded =
            AZ::JsonSerializationUtils::LoadObjectFromFile(file, AZStd::string(filePath->c_str()));

        ASSERT_TRUE(loaded.IsSuccess()) << loaded.GetError().c_str();
        ASSERT_EQ(file.m_presets.size(), 2);

        EXPECT_STREQ(file.m_presets[0].m_name.c_str(), "Box Shape");
        EXPECT_STREQ(file.m_presets[0].m_category.c_str(), "Shapes");

        ASSERT_EQ(file.m_presets[1].m_components.size(), 1);
        ASSERT_EQ(file.m_presets[1].m_components[0].m_properties.size(), 1);
        EXPECT_EQ(file.m_presets[1].m_components[0].m_properties[0].m_value.m_int, 4);
    }

    TEST_F(EntityPresetsTests, PresetFileWithoutTheSerializationHeaderIsRefused)
    {
        constexpr const char* headerless = R"JSON(
        {
            "presets": [
                {
                    "name": "Box Shape",
                    "category": "Shapes",
                    "components": [ { "component": "Box Shape" } ]
                }
            ]
        })JSON";

        AZ::Test::ScopedAutoTempDirectory tempDirectory;
        const auto filePath =
            AZ::Test::CreateTestFile(tempDirectory, "headerless.entitypresets.json", headerless);
        ASSERT_TRUE(filePath.has_value());

        EntityPresets::PresetFile file;
        const auto loaded =
            AZ::JsonSerializationUtils::LoadObjectFromFile(file, AZStd::string(filePath->c_str()));

        EXPECT_FALSE(loaded.IsSuccess());
        EXPECT_TRUE(file.m_presets.empty());
    }

    TEST_F(EntityPresetsTests, WhatTheEditorWritesTheLoaderReads)
    {
        // What SaveUser writes, LoadObjectFromFile must read: project and gem files share one shape.
        EntityPresets::PresetFile written;

        EntityPresets::Preset preset;
        preset.m_name = "Exposure Volume";
        preset.m_description = "Exposure control that applies only inside the box.";
        preset.m_category = "PostFX";

        EntityPresets::ComponentSpec shape;
        shape.m_componentName = "Box Shape";
        preset.m_components.push_back(shape);

        EntityPresets::ComponentSpec layer;
        layer.m_componentName = "PostFX Layer";
        layer.m_properties.push_back(EntityPresets::PropertyAssignment{
            "Controller|Configuration|Layer Category", IntValue(5000000) });
        preset.m_components.push_back(layer);

        EntityPresets::ComponentSpec levelSettings;
        levelSettings.m_componentName = "Vegetation System Settings";
        preset.m_levelComponents.push_back(levelSettings);

        written.m_presets.push_back(preset);

        AZ::Test::ScopedAutoTempDirectory tempDirectory;
        const AZ::IO::Path filePath = tempDirectory.Resolve("EntityPresets.json");

        // The settings SaveUser uses.
        AZ::JsonSerializerSettings serializerSettings;
        serializerSettings.m_keepDefaults = true;
        const EntityPresets::PresetFile* noDefault = nullptr;

        const auto saved = AZ::JsonSerializationUtils::SaveObjectToFile(
            &written, filePath.Native(), noDefault, &serializerSettings);
        ASSERT_TRUE(saved.IsSuccess()) << saved.GetError().c_str();

        EntityPresets::PresetFile read;
        const auto loaded = AZ::JsonSerializationUtils::LoadObjectFromFile(read, filePath.Native());
        ASSERT_TRUE(loaded.IsSuccess()) << loaded.GetError().c_str();

        ASSERT_EQ(read.m_presets.size(), 1);
        EXPECT_STREQ(read.m_presets[0].m_name.c_str(), "Exposure Volume");
        EXPECT_STREQ(read.m_presets[0].m_description.c_str(), preset.m_description.c_str());

        ASSERT_EQ(read.m_presets[0].m_components.size(), 2);
        ASSERT_EQ(read.m_presets[0].m_components[1].m_properties.size(), 1);
        EXPECT_EQ(read.m_presets[0].m_components[1].m_properties[0].m_value.m_int, 5000000);

        ASSERT_EQ(read.m_presets[0].m_levelComponents.size(), 1);
        EXPECT_STREQ(
            read.m_presets[0].m_levelComponents[0].m_componentName.c_str(), "Vegetation System Settings");
    }
} // namespace UnitTest
