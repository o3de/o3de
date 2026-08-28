/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EntityPresets/EntityPresets.h>

#include <AzToolsFramework/Entity/EntityPresets/EntityPresetsSerializer.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/conversions.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntityAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerRequestBus.h>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        namespace
        {
            constexpr const char* PresetsFileName = "EntityPresets.json";
            //! Registry/ is a standard project folder that is normally committed, which is the
            //! point - presets should travel with the project.
            constexpr const char* PresetsFolder = "Registry";

            //! Where a gem keeps the presets it ships. Its own folder rather than Registry/,
            //! because these are content the gem author authored, not machine-written settings.
            constexpr const char* GemPresetsFolder = "Presets";

            //! Only files named <something>.entitypresets.json are read. A bare *.json filter
            //! would claim every JSON file a gem happens to keep in that folder, so the suffix is
            //! what makes the folder shareable rather than owned by this feature.
            constexpr const char* GemPresetsFileFilter = "*.entitypresets.json";

            //! Cached so menus and the panel are looking at the same list, and so the file is not
            //! re-read every time the context menu opens.
            AZStd::vector<Preset>& UserStorage()
            {
                static AZStd::vector<Preset> presets;
                return presets;
            }

            AZStd::vector<Preset>& GemStorage()
            {
                static AZStd::vector<Preset> presets;
                return presets;
            }

            AZStd::vector<Preset>& AllStorage()
            {
                static AZStd::vector<Preset> presets;
                return presets;
            }

            bool& LoadedFlag()
            {
                static bool loaded = false;
                return loaded;
            }

            void RebuildAll()
            {
                AZStd::vector<Preset>& all = AllStorage();
                all.clear();
                all.reserve(BuiltIn().size() + GemStorage().size() + UserStorage().size());

                for (const Preset& preset : BuiltIn())
                {
                    all.push_back(preset);
                }
                for (const Preset& preset : GemStorage())
                {
                    all.push_back(preset);
                }
                for (const Preset& preset : UserStorage())
                {
                    all.push_back(preset);
                }
            }

            // -- File IO ---------------------------------------------------------------

            //! Read one presets file.
            //!
            //! A missing file is not an error - a project that has never had a preset saved simply
            //! has none - so it is reported separately from a file that exists but will not parse.
            //! @return False only when the file is there and unreadable, which is worth telling the
            //! user about because their presets are not going to appear.
            bool ReadPresetFile(const AZStd::string& path, PresetFile& out)
            {
                auto* fileIO = AZ::IO::FileIOBase::GetInstance();
                if (fileIO == nullptr || !fileIO->Exists(path.c_str()))
                {
                    return true;
                }

                const auto loaded = AZ::JsonSerializationUtils::LoadObjectFromFile(out, path);
                if (!loaded.IsSuccess())
                {
                    AZ_Warning(
                        "EntityPresets", false, "Could not read presets from '%s': %s", path.c_str(),
                        loaded.GetError().c_str());
                    return false;
                }

                return true;
            }

            //! Read every *.entitypresets.json under a gem's Presets folder.
            //!
            //! One file may hold many presets, in the same shape as the project's own file, so a gem
            //! can either group them or keep one per file - whichever suits it.
            void LoadPresetsFromFolder(
                const AZStd::string& folderPath, const AZStd::string& gemName, AZStd::vector<Preset>& out)
            {
                auto* fileIO = AZ::IO::FileIOBase::GetInstance();
                if (fileIO == nullptr || !fileIO->IsDirectory(folderPath.c_str()))
                {
                    return;
                }

                fileIO->FindFiles(
                    folderPath.c_str(), GemPresetsFileFilter,
                    [&gemName, &out](const char* filePath)
                    {
                        PresetFile file;
                        if (ReadPresetFile(AZStd::string(filePath), file))
                        {
                            for (Preset& preset : file.m_presets)
                            {
                                if (preset.m_name.empty())
                                {
                                    continue;
                                }

                                // Marked as not-editable and tagged with its origin. Both matter in
                                // the manager: one greys out Edit, the other explains why.
                                preset.m_readOnly = true;
                                preset.m_sourceGem = gemName;
                                out.push_back(AZStd::move(preset));
                            }
                        }

                        return true;
                    });
            }

            // -- Asset lookup ----------------------------------------------------------

            //! Everything after the last separator, lowercased - what the file is called, with no
            //! regard for where it lives.
            AZStd::string FileNameOf(AZStd::string path)
            {
                const size_t separator = path.find_last_of("/\\");
                if (separator != AZStd::string::npos)
                {
                    path = path.substr(separator + 1);
                }

                AZStd::to_lower(path.begin(), path.end());
                return path;
            }

            //! Find a product asset by file name, for when its path no longer resolves.
            //!
            //! Engine assets get moved and renamed between versions - "objects/groudplane/" became
            //! "objects/groundplane/" when that long-standing typo was fixed - and a preset that
            //! names one by path breaks silently when they do: the property is left at its default
            //! and you get an entity with no model on it. Searching the catalogue by file name
            //! recovers from that without the preset having to know what moved where.
            //!
            //! Only a unique match is accepted. Several products can share a file name, and picking
            //! one arbitrarily would put the wrong model on the entity - worse than putting none on
            //! it, because it looks like it worked.
            //!
            //! Cached including misses, since the scan walks the whole catalogue and a preset that
            //! cannot be resolved once will not resolve on the next click either.
            AZ::Data::AssetId FindAssetByFileName(const AZStd::string& path)
            {
                static AZStd::unordered_map<AZStd::string, AZ::Data::AssetId> cache;

                const AZStd::string fileName = FileNameOf(path);
                if (const auto found = cache.find(fileName); found != cache.end())
                {
                    return found->second;
                }

                AZ::Data::AssetId match;
                size_t matchCount = 0;

                AZ::Data::AssetCatalogRequestBus::Broadcast(
                    &AZ::Data::AssetCatalogRequests::EnumerateAssets, nullptr,
                    [&fileName, &match, &matchCount](const AZ::Data::AssetId id, const AZ::Data::AssetInfo& info)
                    {
                        if (FileNameOf(info.m_relativePath) == fileName)
                        {
                            ++matchCount;
                            match = id;
                        }
                    },
                    nullptr);

                if (matchCount != 1)
                {
                    match = AZ::Data::AssetId();
                }

                cache[fileName] = match;
                return match;
            }

            // -- Creation helpers ------------------------------------------------------

            //! Component display name to type id. Worth caching: the lookup walks every registered
            //! component, and building a rig clicks through several presets in a row.
            AZ::Uuid FindComponentTypeId(const AZStd::string& componentName)
            {
                static AZStd::unordered_map<AZStd::string, AZ::Uuid> cache;

                const auto found = cache.find(componentName);
                if (found != cache.end())
                {
                    return found->second;
                }

                AZStd::vector<AZ::Uuid> typeIds;
                EditorComponentAPIBus::BroadcastResult(
                    typeIds, &EditorComponentAPIRequests::FindComponentTypeIdsByEntityType,
                    AZStd::vector<AZStd::string>{ componentName },
                    EditorComponentAPIRequests::EntityType::Game);

                const AZ::Uuid typeId =
                    (typeIds.empty() || typeIds.front().IsNull()) ? AZ::Uuid::CreateNull() : typeIds.front();
                cache[componentName] = typeId;
                return typeId;
            }

            //! Set one property, and if that fails say what the component actually offers.
            //!
            //! Property paths are spelled by the Edit Context and do drift between engine
            //! versions, so a silent failure here is the difference between "my preset is broken"
            //! and "the path is now Controller|Configuration|Mesh Asset".
            void ApplyProperty(const AZ::EntityComponentIdPair& component, const PropertyAssignment& assignment)
            {
                bool resolved = true;
                const AZStd::any value = assignment.m_value.ToAny(resolved);

                if (!resolved)
                {
                    // Asset not in the catalogue. Leaving the property at its default is better
                    // than writing a null asset over it.
                    return;
                }

                EditorComponentAPIRequests::PropertyOutcome outcome = AZ::Failure(AZStd::string());
                EditorComponentAPIBus::BroadcastResult(
                    outcome, &EditorComponentAPIRequests::SetComponentProperty, component, assignment.m_path,
                    value);

                if (outcome.IsSuccess())
                {
                    return;
                }

                AZ_Warning(
                    "EntityPresets", false, "Preset could not set '%s'. Properties this component offers:",
                    assignment.m_path.c_str());

                AZStd::vector<AZStd::string> available;
                EditorComponentAPIBus::BroadcastResult(
                    available, &EditorComponentAPIRequests::BuildComponentPropertyList, component);

                for (const AZStd::string& path : available)
                {
                    AZ_Warning("EntityPresets", false, "    %s", path.c_str());
                }
            }
            //! Level components are registered against the Level entity type rather than Game, so
            //! the ordinary lookup does not find them. Game is tried too, because a few components
            //! are offered for both.
            AZ::Uuid FindLevelComponentTypeId(const AZStd::string& componentName)
            {
                static AZStd::unordered_map<AZStd::string, AZ::Uuid> cache;

                if (const auto found = cache.find(componentName); found != cache.end())
                {
                    return found->second;
                }

                AZ::Uuid typeId = AZ::Uuid::CreateNull();
                for (const EditorComponentAPIRequests::EntityType entityType :
                     { EditorComponentAPIRequests::EntityType::Level, EditorComponentAPIRequests::EntityType::Game })
                {
                    AZStd::vector<AZ::Uuid> typeIds;
                    EditorComponentAPIBus::BroadcastResult(
                        typeIds, &EditorComponentAPIRequests::FindComponentTypeIdsByEntityType,
                        AZStd::vector<AZStd::string>{ componentName }, entityType);

                    if (!typeIds.empty() && !typeIds.front().IsNull())
                    {
                        typeId = typeIds.front();
                        break;
                    }
                }

                cache[componentName] = typeId;
                return typeId;
            }

            //! Put the preset's level components on the level entity, skipping any already there.
            void EnsureLevelComponents(const Preset& preset)
            {
                if (preset.m_levelComponents.empty())
                {
                    return;
                }

                AZ::EntityId levelEntityId;
                ToolsApplicationRequestBus::BroadcastResult(
                    levelEntityId, &ToolsApplicationRequests::GetCurrentLevelEntityId);

                if (!levelEntityId.IsValid())
                {
                    AZ_Warning(
                        "EntityPresets", false,
                        "Preset '%s' needs components on the level entity, but no level is open.",
                        preset.m_name.c_str());
                    return;
                }

                for (const ComponentSpec& component : preset.m_levelComponents)
                {
                    const AZ::Uuid typeId = FindLevelComponentTypeId(component.m_componentName);
                    if (typeId.IsNull())
                    {
                        AZ_Warning(
                            "EntityPresets", false,
                            "Preset '%s' wants the level component '%s', which is not registered - is "
                            "the gem that provides it enabled?",
                            preset.m_name.c_str(), component.m_componentName.c_str());
                        continue;
                    }

                    bool alreadyPresent = false;
                    EditorComponentAPIBus::BroadcastResult(
                        alreadyPresent, &EditorComponentAPIRequests::HasComponentOfType, levelEntityId, typeId);

                    if (alreadyPresent)
                    {
                        continue;
                    }

                    EditorComponentAPIRequests::AddComponentsOutcome outcome = AZ::Failure(AZStd::string());
                    EditorComponentAPIBus::BroadcastResult(
                        outcome, &EditorComponentAPIRequests::AddComponentOfType, levelEntityId, typeId);

                    if (!outcome.IsSuccess() || outcome.GetValue().empty())
                    {
                        AZ_Warning(
                            "EntityPresets", false, "Preset '%s' could not add level component '%s': %s",
                            preset.m_name.c_str(), component.m_componentName.c_str(),
                            outcome.IsSuccess() ? "no component returned" : outcome.GetError().c_str());
                        continue;
                    }

                    const AZ::EntityComponentIdPair added = outcome.GetValue().front();
                    for (const PropertyAssignment& assignment : component.m_properties)
                    {
                        ApplyProperty(added, assignment);
                    }
                }
            }
        } // namespace

        void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                // PropertyValue and PropertyAssignment are registered without their fields on
                // purpose. JsonPropertyAssignmentSerializer writes both of them itself, as one flat
                // object, and reflecting the fields as well would only offer a second, conflicting
                // description of the same data. The types still have to be known so that the
                // containers above them can be described.
                serializeContext->Class<PropertyValue>()->Version(1);
                serializeContext->Class<PropertyAssignment>()->Version(1);

                serializeContext->Class<ComponentSpec>()
                    ->Version(1)
                    ->Field("component", &ComponentSpec::m_componentName)
                    ->Field("properties", &ComponentSpec::m_properties);

                // m_readOnly and m_sourceGem are absent by design - see the header.
                serializeContext->Class<Preset>()
                    ->Version(1)
                    ->Field("name", &Preset::m_name)
                    ->Field("description", &Preset::m_description)
                    ->Field("category", &Preset::m_category)
                    ->Field("components", &Preset::m_components)
                    ->Field("levelComponents", &Preset::m_levelComponents);

                serializeContext->Class<PresetFile>()->Version(1)->Field("presets", &PresetFile::m_presets);
            }

            if (auto* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
            {
                jsonContext->Serializer<JsonPropertyAssignmentSerializer>()->HandlesType<PropertyAssignment>();
            }
        }

        AZStd::any PropertyValue::ToAny(bool& resolved) const
        {
            resolved = true;

            switch (m_type)
            {
            case Type::Bool:
                return AZStd::any(m_bool);
            case Type::Double:
                return AZStd::any(m_double);
            case Type::String:
                return AZStd::any(m_string);
            case Type::AssetPath:
                {
                    // Several spellings are tried because the product path convention changed:
                    // models used to be "name.azmodel" and are now "name.fbx.azmodel", and a
                    // preset written against either should keep working.
                    const AZStd::string candidates[] = {
                        m_string,
                        [this]()
                        {
                            AZStd::string legacy = m_string;
                            const size_t suffix = legacy.find(".fbx.azmodel");
                            if (suffix != AZStd::string::npos)
                            {
                                legacy.replace(suffix, AZStd::string(".fbx.azmodel").size(), ".azmodel");
                            }
                            return legacy;
                        }(),
                    };

                    for (const AZStd::string& candidate : candidates)
                    {
                        AZ::Data::AssetId assetId;
                        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                            assetId, &AZ::Data::AssetCatalogRequests::GetAssetIdByPath, candidate.c_str(),
                            AZ::Data::s_invalidAssetType, false);

                        if (assetId.IsValid())
                        {
                            return AZStd::any(assetId);
                        }
                    }

                    // The path did not resolve in any of its spellings. Before giving up, look the
                    // file name up in the catalogue - it is almost always the folder that moved,
                    // not the asset that vanished.
                    if (const AZ::Data::AssetId byFileName = FindAssetByFileName(m_string);
                        byFileName.IsValid())
                    {
                        AZ_TracePrintf(
                            "EntityPresets",
                            "Preset asset '%s' was not at that path, but a file of that name exists "
                            "in the catalogue and has been used instead. The preset's path is out of "
                            "date and should be corrected.\n",
                            m_string.c_str());

                        return AZStd::any(byFileName);
                    }

                    AZ_Warning(
                        "EntityPresets", false,
                        "Preset asset not found in the catalogue: '%s'. Leaving the property at its "
                        "default - the model may not have been processed yet.",
                        m_string.c_str());

                    resolved = false;
                    return AZStd::any();
                }
            case Type::Int:
            default:
                // As s64, because that is what PropertyTreeEditor::HandleTypeConversion accepts as
                // a *source* type. It takes only double and s64 and converts down to whatever the
                // property really is - u32, s32, u16, u8 and so on. Those narrower types appear
                // only in its destination list.
                //
                // Handing it an s32 therefore works by accident and only when the property is
                // literally reflected as s32: fromType == toType short-circuits and no conversion
                // is attempted. Anything else fails, including every enum. Light type is
                // `enum class LightType : uint8_t`, so it needs the conversion and an s32 silently
                // loses. PostFX's layer category is a plain int, which is why that one appeared to
                // work and made the s32 look correct.
                return AZStd::any(m_int);
            }
        }

        const AZStd::vector<Preset>& FromGems()
        {
            if (!LoadedFlag())
            {
                Reload();
            }
            return GemStorage();
        }

        const AZStd::vector<Preset>& User()
        {
            if (!LoadedFlag())
            {
                Reload();
            }
            return UserStorage();
        }

        const AZStd::vector<Preset>& All()
        {
            if (!LoadedFlag())
            {
                Reload();
            }
            return AllStorage();
        }

        AZStd::string UserPresetsPath()
        {
            const AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
            if (projectPath.empty())
            {
                return {};
            }

            return (AZ::IO::Path(projectPath) / PresetsFolder / PresetsFileName)
                .LexicallyNormal()
                .AsPosix();
        }

        void Reload()
        {
            LoadedFlag() = true;
            UserStorage().clear();
            GemStorage().clear();

            // VisitActiveGems hands back name and path for each *enabled* gem, so a gem that is
            // registered but switched off contributes nothing - which is what you would expect,
            // since its components would not be there to build with either.
            if (auto* registry = AZ::SettingsRegistry::Get())
            {
                AZ::SettingsRegistryMergeUtils::VisitActiveGems(
                    *registry,
                    [](AZStd::string_view gemName, AZStd::string_view gemPath)
                    {
                        if (gemPath.empty())
                        {
                            return;
                        }

                        const AZStd::string folder =
                            (AZ::IO::Path(gemPath) / GemPresetsFolder).LexicallyNormal().AsPosix();

                        LoadPresetsFromFolder(folder, AZStd::string(gemName), GemStorage());
                    });
            }

            if (const AZStd::string path = UserPresetsPath(); !path.empty())
            {
                PresetFile file;
                if (ReadPresetFile(path, file))
                {
                    for (Preset& preset : file.m_presets)
                    {
                        if (!preset.m_name.empty())
                        {
                            UserStorage().push_back(AZStd::move(preset));
                        }
                    }
                }
            }

            RebuildAll();
        }

        bool SaveUser(const AZStd::vector<Preset>& presets)
        {
            const AZStd::string path = UserPresetsPath();
            if (path.empty())
            {
                AZ_Warning("EntityPresets", false, "No project path, so presets cannot be saved.");
                return false;
            }

            if (auto* fileIO = AZ::IO::FileIOBase::GetInstance())
            {
                // StringAsPosix rather than AsPosix: ParentPath hands back a PathView, which only
                // offers the explicitly-named conversions.
                const AZStd::string folder = AZ::IO::Path(path).ParentPath().StringAsPosix();
                fileIO->CreatePath(folder.c_str());
            }

            PresetFile file;
            file.m_presets = presets;

            // Written with defaults kept, so every preset is spelled out in full. The file is meant
            // to be read, diffed and hand-edited, and a half-written entry that relies on knowing
            // what the defaults are is worse to work with than a verbose one.
            AZ::JsonSerializerSettings settings;
            settings.m_keepDefaults = true;

            // Typed rather than a bare nullptr: the default object is the same template parameter
            // as the value being written, so a literal nullptr gives the compiler nothing to
            // deduce ObjectType from.
            const PresetFile* noDefault = nullptr;

            const auto saved = AZ::JsonSerializationUtils::SaveObjectToFile(&file, path, noDefault, &settings);
            if (!saved.IsSuccess())
            {
                AZ_Warning(
                    "EntityPresets", false, "Could not write '%s': %s", path.c_str(), saved.GetError().c_str());
                return false;
            }

            UserStorage() = presets;
            LoadedFlag() = true;
            RebuildAll();

            return true;
        }

        AZ::EntityId Create(const Preset& preset)
        {
            EntityIdList selected;
            ToolsApplicationRequestBus::BroadcastResult(
                selected, &ToolsApplicationRequests::GetSelectedEntities);

            // Exactly one selected entity becomes the parent. With none, or several, there is no
            // unambiguous parent to pick, so it goes to the root.
            const AZ::EntityId parentId = selected.size() == 1 ? selected.front() : AZ::EntityId();

            ToolsApplicationRequestBus::Broadcast(
                &ToolsApplicationRequests::BeginUndoBatch,
                (AZStd::string("Create Preset: ") + preset.m_name).c_str());

            // Before the entity, because a level singleton is a prerequisite for it rather than a
            // part of it - and inside the undo batch, so one Ctrl+Z takes both back.
            EnsureLevelComponents(preset);

            // Put it where the user is looking rather than at the origin. The interaction
            // position accounts for the context menu and cursor, so a preset picked from a right
            // click lands under the cursor; it falls back to the viewport centre otherwise.
            AZ::Vector3 position = AZ::Vector3::CreateZero();
            EditorRequestBus::BroadcastResult(
                position, &EditorRequestBus::Events::GetWorldPositionAtViewportInteraction);

            AZ::EntityId entityId;
            ToolsApplicationRequestBus::BroadcastResult(
                entityId, &ToolsApplicationRequests::CreateNewEntityAtPosition, position, parentId);

            if (entityId.IsValid())
            {
                EditorEntityAPIBus::Event(entityId, &EditorEntityAPIRequests::SetName, preset.m_name);

                ToolsApplicationRequestBus::Broadcast(
                    &ToolsApplicationRequests::SetSelectedEntities, EntityIdList{ entityId });

                for (const ComponentSpec& component : preset.m_components)
                {
                    const AZ::Uuid typeId = FindComponentTypeId(component.m_componentName);
                    if (typeId.IsNull())
                    {
                        AZ_Warning(
                            "EntityPresets", false,
                            "Preset '%s' wants a '%s' component, which is not registered - is the gem "
                            "that provides it enabled?",
                            preset.m_name.c_str(), component.m_componentName.c_str());
                        continue;
                    }

                    EditorComponentAPIRequests::AddComponentsOutcome outcome = AZ::Failure(AZStd::string());
                    EditorComponentAPIBus::BroadcastResult(
                        outcome, &EditorComponentAPIRequests::AddComponentOfType, entityId, typeId);

                    if (!outcome.IsSuccess() || outcome.GetValue().empty())
                    {
                        AZ_Warning(
                            "EntityPresets", false, "Preset '%s' could not add '%s': %s",
                            preset.m_name.c_str(), component.m_componentName.c_str(),
                            outcome.IsSuccess() ? "no component returned" : outcome.GetError().c_str());
                        continue;
                    }

                    const AZ::EntityComponentIdPair added = outcome.GetValue().front();
                    for (const PropertyAssignment& assignment : component.m_properties)
                    {
                        ApplyProperty(added, assignment);
                    }
                }

                // A component whose required services are not met is added *pending* rather than
                // rejected: the entity looks right in the outliner, the component sits greyed out
                // in the inspector, and creation reports success. Nothing else reports it, which
                // is how presets missing a prerequisite went unnoticed for as long as they did.
                //
                // Warn rather than repair. Which component satisfies a service is a judgement -
                // several shapes provide ShapeService - so guessing would sometimes assemble
                // something the preset author did not mean.
                AZ::Entity::ComponentArrayType pending;
                EditorPendingCompositionRequestBus::Event(
                    entityId, &EditorPendingCompositionRequests::GetPendingComponents, pending);

                for (const AZ::Component* pendingComponent : pending)
                {
                    AZ_Warning(
                        "EntityPresets", false,
                        "Preset '%s' left '%s' inactive: it requires a component the preset does not "
                        "add. Add the missing component to the preset.",
                        preset.m_name.c_str(), GetFriendlyComponentName(pendingComponent).c_str());
                }
            }
            else
            {
                AZ_Warning(
                    "EntityPresets", false, "Preset '%s' could not create an entity.", preset.m_name.c_str());
            }

            ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequests::EndUndoBatch);

            // Open the outliner's rename box on the new entity. A preset's name says what the
            // entity is made of, hardly ever what this particular one is for, so it gets renamed
            // almost every time - this saves hunting it down in the outliner and double clicking.
            //
            // Deliberately after the batch closes: renaming is its own undoable step, and starting
            // it inside the batch would fold the typed name into the creation, so one Ctrl+Z would
            // undo both and a second would be needed to get back to before the preset.
            if (entityId.IsValid())
            {
                EntityOutlinerRequestBus::Broadcast(
                    &EntityOutlinerRequests::TriggerRenameEntityUi, entityId);
            }

            return entityId;
        }
    } // namespace EntityPresets
} // namespace AzToolsFramework
