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
            //! Inside the project, so user presets can be committed with it.
            constexpr const char* PresetsFolder = "Registry";

            //! Folder at a gem's root that holds the presets it ships.
            constexpr const char* GemPresetsFolder = "Presets";

            //! The suffix lets a gem keep other JSON files in the same folder.
            constexpr const char* GemPresetsFileFilter = "*.entitypresets.json";

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

            //! Read one presets file. A missing file is not an error.
            //! @return False only when the file exists but cannot be read.
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

            //! Read every *.entitypresets.json in a gem's Presets folder.
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

                                preset.m_readOnly = true;
                                preset.m_sourceGem = gemName;
                                out.push_back(AZStd::move(preset));
                            }
                        }

                        return true;
                    });
            }

            // -- Asset lookup ----------------------------------------------------------

            //! The lowercased file name of a path.
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

            //! Find a product asset by file name, for when its path no longer resolves (for example
            //! after an engine folder was renamed).
            //!
            //! Only a unique match is accepted, since picking one of several would silently put the
            //! wrong asset on the entity. Results are cached, misses included.
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

            //! Component display name to type id, cached.
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

            //! Set one property. On failure, log the paths the component does offer.
            void ApplyProperty(const AZ::EntityComponentIdPair& component, const PropertyAssignment& assignment)
            {
                bool resolved = true;
                const AZStd::any value = assignment.m_value.ToAny(resolved);

                if (!resolved)
                {
                    // Asset not in the catalog: keep the property's default rather than writing a null asset.
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

                for ([[maybe_unused]] const AZStd::string& path : available)
                {
                    AZ_Warning("EntityPresets", false, "    %s", path.c_str());
                }
            }
            //! Like FindComponentTypeId, but tries the Level entity type first, since level
            //! components are not registered as Game components.
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

            //! Add the preset's level components to the level entity, skipping any already there.
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
                // No fields: JsonPropertyAssignmentSerializer reads and writes both types itself.
                serializeContext->Class<PropertyValue>()->Version(1);
                serializeContext->Class<PropertyAssignment>()->Version(1);

                serializeContext->Class<ComponentSpec>()
                    ->Version(1)
                    ->Field("component", &ComponentSpec::m_componentName)
                    ->Field("properties", &ComponentSpec::m_properties);

                // m_readOnly and m_sourceGem are runtime-only.
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
                    // Models used to be "name.azmodel" and are now "name.fbx.azmodel"; try both.
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

                    // Fall back to the file name, in case the folder moved.
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
                // As s64: PropertyTreeEditor only converts from s64 and double, so a narrower type
                // reaches only a property of exactly that type and fails for enums.
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

            // VisitActiveGems only visits enabled gems.
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
                const AZStd::string folder = AZ::IO::Path(path).ParentPath().StringAsPosix();
                fileIO->CreatePath(folder.c_str());
            }

            PresetFile file;
            file.m_presets = presets;

            // Defaults are kept so the file is complete and easy to edit by hand.
            AZ::JsonSerializerSettings settings;
            settings.m_keepDefaults = true;

            // Typed so SaveObjectToFile can deduce its template parameter.
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

            // Parent to the selection only when it is unambiguous.
            const AZ::EntityId parentId = selected.size() == 1 ? selected.front() : AZ::EntityId();

            ToolsApplicationRequestBus::Broadcast(
                &ToolsApplicationRequests::BeginUndoBatch,
                (AZStd::string("Create Preset: ") + preset.m_name).c_str());

            // Inside the batch, so one undo removes these too.
            EnsureLevelComponents(preset);

            // Under the cursor when opened from a context menu, otherwise the viewport centre.
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

                // A component with unmet required services is added pending, which otherwise goes
                // unnoticed. Warn rather than guess which component would satisfy it.
                AZ::Entity::ComponentArrayType pending;
                EditorPendingCompositionRequestBus::Event(
                    entityId, &EditorPendingCompositionRequests::GetPendingComponents, pending);

                for ([[maybe_unused]] const AZ::Component* pendingComponent : pending)
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

            // Start renaming the new entity. After the batch, so the rename is its own undo step.
            if (entityId.IsValid())
            {
                EntityOutlinerRequestBus::Broadcast(
                    &EntityOutlinerRequests::TriggerRenameEntityUi, entityId);
            }

            return entityId;
        }
    } // namespace EntityPresets
} // namespace AzToolsFramework
