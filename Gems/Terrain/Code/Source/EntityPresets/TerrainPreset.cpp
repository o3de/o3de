/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EntityPresets/TerrainPreset.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntityAPIBus.h>
#include <AzToolsFramework/PropertyTreeEditor/PropertyTreeEditor.h>

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/UI/EntityPresets/ActionManagerIdentifiers/EntityPresetsIdentifiers.h>
#include <AzToolsFramework/UI/EntityPresets/PresetRequirements.h>

namespace Terrain
{
    namespace TerrainPreset
    {
        namespace
        {
            using EntityType = AzToolsFramework::EditorComponentAPIRequests::EntityType;

            // -- The shape of the terrain this builds ----------------------------------
            // A 1km square at the origin with 100m of height range and gentle noise.

            constexpr float TerrainSizeMetres = 1024.0f;
            constexpr float TerrainHeightMetres = 100.0f;

            //! Height range of the terrain world (Terrain World), separate from the spawner's box.
            constexpr float WorldMinHeight = 0.0f;
            constexpr float WorldMaxHeight = 1024.0f;

            //! Metres between height samples; 1 is the engine default.
            constexpr float HeightQueryResolution = 1.0f;

            //! Noise frequency. Low values give broad rolling hills.
            constexpr float NoiseFrequency = 0.01f;

            //! Side length of the vegetation area, kept small so planting is quick.
            constexpr float VegetationAreaMetres = 128.0f;

            //! Vegetation::SnapMode::Center, as a number to avoid depending on the Vegetation gem.
            constexpr AZ::s64 SectorPointSnapModeCentre = 1;

            // -- Property values -------------------------------------------------------
            // PropertyTreeEditor only converts from double and s64, so values go through these two types
            // whatever the property's real type is.

            AZStd::any RealValue(const double value)
            {
                return AZStd::any(value);
            }

            AZStd::any IntegerValue(const AZ::s64 value)
            {
                return AZStd::any(value);
            }

            // -- Component plumbing ----------------------------------------------------

            //! Resolve a component by display name, trying each entity type in turn. Level components such
            //! as Terrain World are only registered under Level.
            AZ::Uuid FindComponentTypeId(const char* componentName, const AZStd::vector<EntityType>& categories)
            {
                for (const EntityType category : categories)
                {
                    AZStd::vector<AZ::Uuid> typeIds;
                    AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                        typeIds, &AzToolsFramework::EditorComponentAPIRequests::FindComponentTypeIdsByEntityType,
                        AZStd::vector<AZStd::string>{ componentName }, category);

                    if (!typeIds.empty() && !typeIds.front().IsNull())
                    {
                        return typeIds.front();
                    }
                }

                return AZ::Uuid::CreateNull();
            }

            //! Game first, with Level as a fallback.
            AZ::Uuid FindEntityComponentTypeId(const char* componentName)
            {
                return FindComponentTypeId(componentName, { EntityType::Game, EntityType::Level });
            }

            AZ::Uuid FindLevelComponentTypeId(const char* componentName)
            {
                return FindComponentTypeId(componentName, { EntityType::Level, EntityType::Game });
            }

            //! Add a component and hand back the instance, so its properties can be set.
            AZ::EntityComponentIdPair AddComponentOfType(
                const AZ::EntityId entityId, const char* componentName, const AZ::Uuid typeId)
            {
                if (typeId.IsNull())
                {
                    AZ_Warning(
                        "EntityPresets", false,
                        "Terrain preset needs a '%s' component, which is not registered. Is the gem that "
                        "provides it enabled for this project?",
                        componentName);
                    return AZ::EntityComponentIdPair();
                }

                AzToolsFramework::EditorComponentAPIRequests::AddComponentsOutcome outcome =
                    AZ::Failure(AZStd::string());
                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                    outcome, &AzToolsFramework::EditorComponentAPIRequests::AddComponentOfType, entityId, typeId);

                if (!outcome.IsSuccess() || outcome.GetValue().empty())
                {
                    AZ_Warning(
                        "EntityPresets", false, "Terrain preset could not add '%s': %s", componentName,
                        outcome.IsSuccess() ? "no component returned" : outcome.GetError().c_str());
                    return AZ::EntityComponentIdPair();
                }

                return outcome.GetValue().front();
            }

            AZ::EntityComponentIdPair AddComponent(const AZ::EntityId entityId, const char* componentName)
            {
                return AddComponentOfType(entityId, componentName, FindEntityComponentTypeId(componentName));
            }

            //! Add a component unless the entity already has one (the heightfield colliders pull each other in).
            void AddComponentIfMissing(const AZ::EntityId entityId, const char* componentName)
            {
                const AZ::Uuid typeId = FindEntityComponentTypeId(componentName);
                if (typeId.IsNull())
                {
                    AZ_Warning(
                        "EntityPresets", false,
                        "Terrain preset needs a '%s' component, which is not registered. Is the gem that "
                        "provides it enabled for this project?",
                        componentName);
                    return;
                }

                bool alreadyPresent = false;
                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                    alreadyPresent, &AzToolsFramework::EditorComponentAPIRequests::HasComponentOfType, entityId,
                    typeId);

                if (!alreadyPresent)
                {
                    AddComponentOfType(entityId, componentName, typeId);
                }
            }

            bool SetProperty(
                const AZ::EntityComponentIdPair& component, const char* propertyPath, const AZStd::any& value)
            {
                if (!component.GetEntityId().IsValid())
                {
                    return false;
                }

                AzToolsFramework::EditorComponentAPIRequests::PropertyOutcome outcome =
                    AZ::Failure(AZStd::string());
                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                    outcome, &AzToolsFramework::EditorComponentAPIRequests::SetComponentProperty, component,
                    AZStd::string_view(propertyPath), value);

                AZ_Warning(
                    "EntityPresets", outcome.IsSuccess(), "Terrain preset could not set '%s': %s", propertyPath,
                    outcome.IsSuccess() ? "" : outcome.GetError().c_str());

                return outcome.IsSuccess();
            }

            //! Append an entity id to a container property, which SetComponentProperty cannot do.
            bool AppendToContainer(
                const AZ::EntityComponentIdPair& component, const char* propertyPath, const AZ::EntityId value)
            {
                if (!component.GetEntityId().IsValid())
                {
                    return false;
                }

                AzToolsFramework::EditorComponentAPIRequests::PropertyTreeOutcome treeOutcome =
                    AZ::Failure(AZStd::string());
                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                    treeOutcome, &AzToolsFramework::EditorComponentAPIRequests::BuildComponentPropertyTreeEditor,
                    component);

                if (!treeOutcome.IsSuccess())
                {
                    AZ_Warning(
                        "EntityPresets", false, "Terrain preset could not inspect '%s': %s", propertyPath,
                        treeOutcome.GetError().c_str());
                    return false;
                }

                AzToolsFramework::PropertyTreeEditor tree = treeOutcome.GetValue();
                const auto appendOutcome = tree.AppendContainerItem(propertyPath, AZStd::any(value));

                if (appendOutcome.IsSuccess())
                {
                    return true;
                }

                // List the component's real property paths to help fix a wrong one.
                AZ_Warning(
                    "EntityPresets", false, "Terrain preset could not fill '%s': %s. This component offers:",
                    propertyPath, appendOutcome.GetError().c_str());

                AZStd::vector<AZStd::string> available;
                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                    available, &AzToolsFramework::EditorComponentAPIRequests::BuildComponentPropertyList,
                    component);

                for ([[maybe_unused]] const AZStd::string& path : available)
                {
                    AZ_Warning("EntityPresets", false, "    %s", path.c_str());
                }

                return false;
            }

            //! Re-enable a component so it picks up a container filled after it was added; otherwise the
            //! gradient list is ignored and the terrain stays invisible.
            void ForceRefresh(const AZ::EntityComponentIdPair& component)
            {
                if (!component.GetEntityId().IsValid())
                {
                    return;
                }

                bool refreshed = false;
                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                    refreshed, &AzToolsFramework::EditorComponentAPIRequests::EnableComponents,
                    AZStd::vector<AZ::EntityComponentIdPair>{ component });

                AZ_Warning(
                    "EntityPresets", refreshed,
                    "Terrain preset could not refresh a gradient list. If nothing appears, disable and "
                    "re-enable that component on the new entity.");
            }

            AZ::EntityId CreateEntity(const char* name, const AZ::EntityId parentId)
            {
                AZ::EntityId entityId;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                    entityId, &AzToolsFramework::ToolsApplicationRequests::CreateNewEntity, parentId);

                if (entityId.IsValid())
                {
                    AzToolsFramework::EditorEntityAPIBus::Event(
                        entityId, &AzToolsFramework::EditorEntityAPIRequests::SetName, AZStd::string(name));
                }

                return entityId;
            }

            //! The variant's name as the menu shows it.
            const char* VariantName(const Variant variant)
            {
                switch (variant)
                {
                case Variant::Landscape:
                    return "Landscape Terrain";
                case Variant::LandscapeWithVegetation:
                    return "Landscape Terrain + Vegetation";
                case Variant::Simple:
                default:
                    return "Simple Terrain";
                }
            }

            AZ::EntityId LevelEntityId()
            {
                AZ::EntityId levelEntityId;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
                    levelEntityId, &AzToolsFramework::ToolsApplicationRequests::GetCurrentLevelEntityId);
                return levelEntityId;
            }

            //! Add a level component, unless the level already has one.
            //! @return The component, or an invalid pair if it was already present or unavailable.
            AZ::EntityComponentIdPair AddLevelComponent(const AZ::EntityId levelEntityId, const char* componentName)
            {
                const AZ::Uuid typeId = FindLevelComponentTypeId(componentName);
                if (typeId.IsNull())
                {
                    AZ_Warning(
                        "EntityPresets", false,
                        "Terrain preset needs the level component '%s', which is not registered. Is the "
                        "gem that provides it enabled?",
                        componentName);
                    return AZ::EntityComponentIdPair();
                }

                bool alreadyPresent = false;
                AzToolsFramework::EditorComponentAPIBus::BroadcastResult(
                    alreadyPresent, &AzToolsFramework::EditorComponentAPIRequests::HasComponentOfType,
                    levelEntityId, typeId);

                if (alreadyPresent)
                {
                    return AZ::EntityComponentIdPair();
                }

                return AddComponentOfType(levelEntityId, componentName, typeId);
            }

            //! Terrain does not render without these level components.
            void EnsureTerrainLevelComponents()
            {
                const AZ::EntityId levelEntityId = LevelEntityId();
                if (!levelEntityId.IsValid())
                {
                    AZ_Warning(
                        "EntityPresets", false,
                        "Terrain preset found no level entity - open a level before creating terrain.");
                    return;
                }

                // The height range lives on Terrain World; the spawner's box only sets where terrain exists.
                if (const AZ::EntityComponentIdPair world = AddLevelComponent(levelEntityId, "Terrain World");
                    world.GetEntityId().IsValid())
                {
                    SetProperty(world, "Configuration|Min Height", RealValue(WorldMinHeight));
                    SetProperty(world, "Configuration|Max Height", RealValue(WorldMaxHeight));
                    SetProperty(
                        world, "Configuration|Height Query Resolution (m)", RealValue(HeightQueryResolution));
                }

                AddLevelComponent(levelEntityId, "Terrain World Renderer");
            }

            void EnsureVegetationLevelComponents()
            {
                const AZ::EntityId levelEntityId = LevelEntityId();
                if (!levelEntityId.IsValid())
                {
                    return;
                }

                if (const AZ::EntityComponentIdPair settings =
                        AddLevelComponent(levelEntityId, "Vegetation System Settings");
                    settings.GetEntityId().IsValid())
                {
                    // Snap to sector centres so instance positions stay stable.
                    SetProperty(
                        settings, "Configuration|Area System Settings|Sector Point Snap Mode",
                        IntegerValue(SectorPointSnapModeCentre));
                }
            }

            //! The noise that drives terrain height: FastNoise Gradient if its gem is enabled, otherwise
            //! Perlin Noise Gradient from GradientSignal, which has the same Frequency property.
            AZ::EntityComponentIdPair AddNoiseGradient(const AZ::EntityId entityId)
            {
                const char* const noiseComponents[] = { "FastNoise Gradient", "Perlin Noise Gradient" };

                for (const char* componentName : noiseComponents)
                {
                    const AZ::Uuid typeId = FindEntityComponentTypeId(componentName);
                    if (typeId.IsNull())
                    {
                        continue;
                    }

                    AZ_Warning(
                        "EntityPresets", componentName == noiseComponents[0],
                        "FastNoise Gradient is not available, so the terrain's height is coming from "
                        "'%s' instead. Enable the FastNoise gem for the intended result.",
                        componentName);

                    return AddComponentOfType(entityId, componentName, typeId);
                }

                AZ_Warning(
                    "EntityPresets", false,
                    "Terrain preset found no noise gradient component - neither FastNoise Gradient nor "
                    "Perlin Noise Gradient is registered. The terrain will be flat.");

                return AZ::EntityComponentIdPair();
            }

            //! The vegetation area: a box, a spawner, and a list of things to plant.
            void CreateVegetationArea(const AZ::EntityId parentId)
            {
                const AZ::EntityId vegetationId = CreateEntity("Vegetation", parentId);
                if (!vegetationId.IsValid())
                {
                    return;
                }

                const AZ::EntityComponentIdPair box = AddComponent(vegetationId, "Box Shape");
                AddComponent(vegetationId, "Vegetation Layer Spawner");
                AddComponent(vegetationId, "Vegetation Asset List");

                SetProperty(
                    box, "Box Shape|Box Configuration|Dimensions",
                    AZStd::any(AZ::Vector3(
                        VegetationAreaMetres, VegetationAreaMetres, TerrainHeightMetres)));

                // The asset list is left empty; there is no sensible default plant.
                AZ_TracePrintf(
                    "EntityPresets",
                    "Vegetation area created. Add a mesh or prefab to its Vegetation Asset List to see "
                    "anything planted.\n");
            }
        } // namespace

        AZ::EntityId Create(const Variant variant)
        {
            bool wantsLandscapeCanvas = variant != Variant::Simple;
            bool wantsVegetation = variant == Variant::LandscapeWithVegetation;
            bool wantsPhysics = true;

            using AzToolsFramework::EntityPresets::RequiredComponent;

            // Required components, with the gem each comes from. Without these nothing is built.
            const AZStd::vector<RequiredComponent> core = {
                { "Terrain Layer Spawner", "Terrain" },
                { "Terrain Height Gradient List", "Terrain" },
                { "Terrain World", "Terrain" },
                { "Axis Aligned Box Shape", "LmbrCentral" },
                { "Shape Reference", "LmbrCentral" },
                { "Gradient Transform Modifier", "GradientSignal" },
            };

            if (const auto missing = AzToolsFramework::EntityPresets::MissingComponents(core); !missing.empty())
            {
                for ([[maybe_unused]] const RequiredComponent& component : missing)
                {
                    AZ_Warning(
                        "EntityPresets", false, "Terrain preset needs '%s' from the %s gem, which is not available.",
                        component.m_componentName.c_str(), component.m_gemName.c_str());
                }

                AzToolsFramework::EntityPresets::ReportMissingRequirements(VariantName(variant), missing);
                return AZ::EntityId();
            }

            // Optional layers. Missing ones are dropped after asking the user once.
            AZStd::vector<RequiredComponent> reduced;
            AZStd::vector<AZStd::string> lost;

            if (wantsLandscapeCanvas)
            {
                if (const auto missing = AzToolsFramework::EntityPresets::MissingComponents(
                        { { "Landscape Canvas", "LandscapeCanvas" } });
                    !missing.empty())
                {
                    reduced.insert(reduced.end(), missing.begin(), missing.end());
                    lost.push_back("the node graph");
                    wantsLandscapeCanvas = false;
                }
            }

            if (const auto missing = AzToolsFramework::EntityPresets::MissingComponents(
                    { { "Terrain Physics Heightfield Collider", "Terrain" },
                      { "PhysX Heightfield Collider", "PhysX5" } });
                !missing.empty())
            {
                reduced.insert(reduced.end(), missing.begin(), missing.end());
                lost.push_back("collision");
                wantsPhysics = false;
            }

            if (wantsVegetation)
            {
                if (const auto missing = AzToolsFramework::EntityPresets::MissingComponents(
                        { { "Vegetation Layer Spawner", "Vegetation" },
                          { "Vegetation Asset List", "Vegetation" },
                          { "Vegetation System Settings", "Vegetation" } });
                    !missing.empty())
                {
                    reduced.insert(reduced.end(), missing.begin(), missing.end());
                    lost.push_back("vegetation");
                    wantsVegetation = false;
                }
            }

            if (!reduced.empty())
            {
                AZStd::string reduction = "without ";
                for (size_t index = 0; index < lost.size(); ++index)
                {
                    if (index > 0)
                    {
                        reduction += (index + 1 == lost.size()) ? " or " : ", ";
                    }
                    reduction += lost[index];
                }

                for ([[maybe_unused]] const RequiredComponent& component : reduced)
                {
                    AZ_Warning(
                        "EntityPresets", false, "Terrain preset is leaving out '%s' from the %s gem.",
                        component.m_componentName.c_str(), component.m_gemName.c_str());
                }

                if (!AzToolsFramework::EntityPresets::ConfirmReducedSetup(VariantName(variant), reduction, reduced))
                {
                    return AZ::EntityId();
                }
            }


            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::BeginUndoBatch, "Create Terrain");

            EnsureTerrainLevelComponents();
            if (wantsVegetation)
            {
                EnsureVegetationLevelComponents();
            }

            // -- The graph root, for the Landscape Canvas variants ---------------------
            // A Landscape Canvas graph is an entity with the Landscape Canvas component; its descendants
            // are the nodes. Parenting the terrain under one makes it open as a graph.
            AZ::EntityId graphRootId;
            if (wantsLandscapeCanvas)
            {
                graphRootId = CreateEntity("Landscape Terrain", AZ::EntityId());
                if (graphRootId.IsValid())
                {
                    // Availability was checked up front, so a failure here has another cause.
                    const AZ::EntityComponentIdPair canvas = AddComponent(graphRootId, "Landscape Canvas");

                    AZ_Warning(
                        "EntityPresets", canvas.GetEntityId().IsValid(),
                        "The terrain was built, but the Landscape Canvas component could not be added, "
                        "so it will not open as a graph.");
                }
            }

            // -- The spawner: a box saying where terrain exists ------------------------
            const AZ::EntityId spawnerId = CreateEntity("Terrain", graphRootId);
            if (!spawnerId.IsValid())
            {
                AZ_Warning("EntityPresets", false, "Terrain preset could not create its entity.");
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                    &AzToolsFramework::ToolsApplicationRequests::EndUndoBatch);
                return AZ::EntityId();
            }

            const AZ::EntityComponentIdPair boxShape = AddComponent(spawnerId, "Axis Aligned Box Shape");
            AddComponent(spawnerId, "Terrain Layer Spawner");
            const AZ::EntityComponentIdPair heightGradientList =
                AddComponent(spawnerId, "Terrain Height Gradient List");

            // Both colliders or neither: Terrain Physics Heightfield Collider provides the heightfield,
            // PhysX Heightfield Collider puts it in the physics scene.
            if (wantsPhysics)
            {
                AddComponentIfMissing(spawnerId, "Terrain Physics Heightfield Collider");
                AddComponentIfMissing(spawnerId, "PhysX Heightfield Collider");
            }

            SetProperty(
                boxShape, "Axis Aligned Box Shape|Box Configuration|Dimensions",
                AZStd::any(AZ::Vector3(TerrainSizeMetres, TerrainSizeMetres, TerrainHeightMetres)));

            // -- The height provider: noise, shaped to the spawner's box ---------------
            // A child of the spawner, so the two move together.
            const AZ::EntityId heightProviderId = CreateEntity("Terrain Height", spawnerId);
            if (heightProviderId.IsValid())
            {
                const AZ::EntityComponentIdPair shapeReference = AddComponent(heightProviderId, "Shape Reference");
                AddComponent(heightProviderId, "Gradient Transform Modifier");
                const AZ::EntityComponentIdPair noise = AddNoiseGradient(heightProviderId);

                // Sample the noise over the spawner's box...
                SetProperty(shapeReference, "Configuration|Shape Entity Id", AZStd::any(spawnerId));
                SetProperty(noise, "Configuration|Frequency", RealValue(NoiseFrequency));

                // ...and feed it to the spawner. Without this the terrain has no height data and renders nothing.
                AppendToContainer(heightGradientList, "Configuration|Gradient Entities", heightProviderId);
                ForceRefresh(heightGradientList);
            }

            if (wantsVegetation)
            {
                CreateVegetationArea(spawnerId);
            }

            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities,
                AzToolsFramework::EntityIdList{ graphRootId.IsValid() ? graphRootId : spawnerId });

            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                &AzToolsFramework::ToolsApplicationRequests::EndUndoBatch);

            return spawnerId;
        }

        // -- Editor menu registration --------------------------------
        // Registered like any gem-supplied entry, against the identifiers AzToolsFramework publishes.

        namespace
        {
            //! Unchanged from when this lived in AzToolsFramework, so existing hotkeys still work.
            constexpr AZStd::string_view TerrainMenuIdentifier = "o3de.menu.editor.entityPresets.terrain";

            struct TerrainVariantAction
            {
                const char* m_identifier;
                const char* m_name;
                const char* m_description;
                Variant m_variant;
            };

            //! Ordered from least to most.
            constexpr TerrainVariantAction TerrainVariants[] = {
                { "o3de.action.editor.entityPreset.terrain.simple", "Simple Terrain",
                  "A terrain region with noise driving its height. The quickest way to have ground.",
                  Variant::Simple },
                { "o3de.action.editor.entityPreset.terrain.landscape", "Landscape Terrain",
                  "The same terrain under a Landscape Canvas entity, so the whole setup opens as a "
                  "node graph.",
                  Variant::Landscape },
                { "o3de.action.editor.entityPreset.terrain.vegetation", "Landscape Terrain + Vegetation",
                  "Landscape terrain plus a vegetation area and the level's vegetation settings. Add a "
                  "mesh to its Vegetation Asset List to see anything planted.",
                  Variant::LandscapeWithVegetation },
            };
        } // namespace

        void RegisterActions()
        {
            auto* actionManager = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
            if (actionManager == nullptr)
            {
                return;
            }

            for (const TerrainVariantAction& terrain : TerrainVariants)
            {
                if (actionManager->IsActionRegistered(terrain.m_identifier))
                {
                    continue;
                }

                AzToolsFramework::ActionProperties properties;
                properties.m_name = terrain.m_name;
                properties.m_description = terrain.m_description;
                properties.m_category = "Entity Presets";

                const Variant variant = terrain.m_variant;
                actionManager->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier, terrain.m_identifier, properties,
                    [variant]() { Create(variant); });
            }
        }

        void RegisterMenus()
        {
            auto* menuManager = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
            if (menuManager == nullptr || menuManager->IsMenuRegistered(TerrainMenuIdentifier))
            {
                return;
            }

            AzToolsFramework::MenuProperties properties;
            properties.m_name = "Terrain";
            menuManager->RegisterMenu(TerrainMenuIdentifier, properties);
        }

        void BindMenus()
        {
            auto* menuManager = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
            if (menuManager == nullptr)
            {
                return;
            }

            int sortKey = 0;
            for (const TerrainVariantAction& terrain : TerrainVariants)
            {
                sortKey += 100;
                menuManager->AddActionToMenu(TerrainMenuIdentifier, terrain.m_identifier, sortKey);
            }

            // The root menu exists by now: all menu registration hooks run before any binding hook.
            menuManager->AddSubMenuToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, TerrainMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsGemSortKeyStart);
        }
    } // namespace TerrainPreset
} // namespace Terrain
