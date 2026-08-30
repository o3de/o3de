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

    //! An enum shaped exactly like the ones presets are used to drive.
    //!
    //! Atom's Light type is `enum class LightType : uint8_t` and the Vegetation gem's sector point
    //! snap mode is `enum class SnapMode : AZ::u8`. Both were being set with an AZStd::any holding
    //! an s32, and both silently did nothing, because PropertyTreeEditor::HandleTypeConversion
    //! accepts only double and s64 as *source* types - the narrower types appear only in its
    //! destination list. An s32 lands only when the property is literally an s32, in which case the
    //! types match exactly and no conversion is attempted at all.
    //!
    //! The existing PropertyTreeEditor tests have no enum property, which is a large part of why
    //! this went unnoticed on both sides.
    enum class EntityPresetsTestEnum : AZ::u8
    {
        First = 0,
        Second,
        Third
    };

    //! One property of every kind a preset can set, reflected the way a real component reflects
    //! its own - serialize fields plus an Edit Context - because that is what a preset addresses.
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

            // PropertyTreeEditor addresses properties by their *Edit Context* display names - that
            // is where a path like "Controller|Configuration|Light type" comes from - and
            // PopulateNodeMap skips outright any node that has no edit metadata. Reflecting
            // serialize fields alone leaves the tree empty, so every SetProperty fails on a missing
            // path, for reasons that have nothing to do with the type conversion under test.
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
                // Deliberately a plain Default handler with no EnumAttributes: the conversion under
                // test has to work on the type itself, not on edit-context decoration a component
                // may or may not have bothered to add.
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

            // Without this the user settings component tries to save on shutdown, and that file is
            // shared engine-wide, so parallel test runs can collide on it.
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

        //! Set a property the way EntityPresets::Create does: build the value, convert it, hand the
        //! any to PropertyTreeEditor. Going through ToAny rather than constructing the any directly
        //! is the point - that conversion is what the bug lived in.
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

    // -- The regression this whole file exists for -------------------------------------

    TEST_F(EntityPresetsTests, IntValueReachesAnEnumProperty)
    {
        EntityPresetsTestConfig config;
        PropertyTreeEditor tree(&config, AZ::AzTypeInfo<EntityPresetsTestConfig>::Uuid());

        const auto outcome = SetThroughToAny(tree, "My Enum", IntValue(2));

        // Before the fix this failed with "value type cannot be converted to the property's type"
        // and the enum kept its default, with nothing said about it above trace level.
        EXPECT_TRUE(outcome.IsSuccess());
        EXPECT_EQ(config.m_enum, EntityPresetsTestEnum::Third);
    }

    TEST_F(EntityPresetsTests, IntValueReachesAnIntProperty)
    {
        EntityPresetsTestConfig config;
        PropertyTreeEditor tree(&config, AZ::AzTypeInfo<EntityPresetsTestConfig>::Uuid());

        // The case that always worked, and by working made the s32 look correct. PostFX's layer
        // category is a plain int, so it was the one that got tested by hand.
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

        // Stricter than the tests above and deliberately so: it names the one type that works
        // rather than only checking the outcome. If HandleTypeConversion ever widens the set of
        // source types it accepts, this is the test that should be relaxed - not the ones above,
        // which describe behaviour rather than mechanism.
        EXPECT_EQ(converted.type(), azrtti_typeid<AZ::s64>());
    }

    // -- The JSON format, which existing project files depend on -----------------------

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

        // Flat, not nested: reflection on its own would have put the value in its own object, and
        // every preset file written before the serialization rewrite would have stopped loading.
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
        // Written by hand rather than by the serializer, because the point is to prove that a file
        // produced by the old Qt-based writer still reads. If the format ever has to change, this
        // is the test that should force a migration path to be written first.
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

        // The asset case matters separately: it is the one where the type name decides how the
        // string is treated, and a preset that reads it as a plain string would set a model path
        // into a property expecting an AssetId.
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

        // Both of these describe where a preset came from at runtime. Writing them would let a
        // user preset claim to be a built-in, or to belong to a gem that never shipped it.
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
    // -- The file format, as opposed to the serializer ---------------------------------
    //
    // Every test above works on in-memory documents through AZ::JsonSerialization. The code that
    // actually reads a gem's Presets/<name>.entitypresets.json does not: it goes through
    // AZ::JsonSerializationUtils::LoadObjectFromFile, which demands a
    // Type/Version/ClassName/ClassData header before it hands anything to the serializer.
    //
    // Nothing covered that, and the gap is not theoretical: a preset file written in the shape the
    // tests above would lead you to write is valid JSON, is accepted by the serializer, and is
    // still refused by the loader - so an entire gem's presets fail to appear, quietly.

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
        // The exact mistake the test above exists to prevent, pinned from the other side. This is
        // the shape PresetsFileFromBeforeTheRewriteStillLoads hands straight to
        // AZ::JsonSerialization::Load, which is why writing a gem file this way looks reasonable
        // right up until nothing shows up in the menu.
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
        // The project's file and a gem's files are meant to be the same shape, so a preset can be
        // moved between them by copying it and a gem can ship exactly what the editor wrote. That
        // is a claim about two functions agreeing - SaveUser writes through SaveObjectToFile, the
        // loader reads through LoadObjectFromFile - and nothing else holds them to it.
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

        // The settings SaveUser uses: defaults kept, so the file is spelled out in full rather
        // than relying on the reader knowing what was left out.
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

        // levelComponents is the newest part of the format, and a round trip that dropped it would
        // lose the level singleton a preset depends on - visible only as "the preset does nothing".
        ASSERT_EQ(read.m_presets[0].m_levelComponents.size(), 1);
        EXPECT_STREQ(
            read.m_presets[0].m_levelComponents[0].m_componentName.c_str(), "Vegetation System Settings");
    }
} // namespace UnitTest
