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
            //
            // A 1km square starting at the origin, 100m of vertical range, with gentle noise for
            // height. Big enough to be a landscape rather than a test patch, small enough that the
            // heightfield does not take a noticeable moment to build.

            constexpr float TerrainSizeMetres = 1024.0f;
            constexpr float TerrainHeightMetres = 100.0f;

            //! Vertical extent of the terrain *world*, which is a separate thing from the box: the
            //! box says where terrain exists, this says what range of heights can be represented.
            constexpr float WorldMinHeight = 0.0f;
            constexpr float WorldMaxHeight = 1024.0f;

            //! Metres between height samples. 1 is the engine default and a reasonable balance;
            //! lower is more detail and more memory.
            constexpr float HeightQueryResolution = 1.0f;

            //! Noise frequency. Low values give broad rolling hills; high values give a rough
            //! surface that reads as noise rather than terrain.
            constexpr float NoiseFrequency = 0.01f;

            //! Side length of the vegetation area. Smaller than the terrain on purpose - planting
            //! a full square kilometre on first click would take a noticeable while.
            constexpr float VegetationAreaMetres = 128.0f;

            //! Snap vegetation instances to sector centres rather than corners.
            //!
            //! Written as a bare number because the enum belongs to the Vegetation gem, which this
            //! file deliberately does not depend on. Vegetation::SnapMode is
            //! `enum class SnapMode : AZ::u8 { Corner = 0, Center }`.
            constexpr AZ::s64 SectorPointSnapModeCentre = 1;

            // -- Property values -------------------------------------------------------
            //
            // PropertyTreeEditor::HandleTypeConversion accepts exactly two source types - double
            // and s64 - and converts down to whatever the property really is: float, u32, s32,
            // u16, u8 and so on. Those narrower types appear only in its *destination* list.
            //
            // So an any holding a float reaches a float property, but only because the types match
            // exactly and no conversion is attempted; against a double or an enum property it
            // fails and the value is silently dropped. The same trap cost the entity presets their
            // light type, which is `enum class LightType : uint8_t` and was being handed an s32.
            //
            // Going through these two helpers means no call site has to know what the property's
            // real type is, which is just as well - it is not visible from here, and it changes.

            AZStd::any RealValue(const double value)
            {
                return AZStd::any(value);
            }

            AZStd::any IntegerValue(const AZ::s64 value)
            {
                return AZStd::any(value);
            }

            // -- Component plumbing ----------------------------------------------------

            //! Resolve a component by display name, trying each category in turn.
            //!
            //! The category matters and is easy to get wrong. Components declare which Add
            //! Component menu they belong to, and level components such as Terrain World appear
            //! *only* under Level - asking for them as Game components returns nothing at all,
            //! which looks exactly like the owning gem being disabled.
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

            //! Components that go on an ordinary entity. Level is tried as a fallback because the
            //! categories are advisory and a component may be reachable from either.
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

            //! Add a component unless the entity already has one.
            //!
            //! Needed for components that pull each other in. The two heightfield colliders depend
            //! on one another and Landscape Canvas adds them as a pair, so depending on how the
            //! entity was built one may already be there - and asking for it again just logs a
            //! failure that means nothing.
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

            //! Append an entity id to a container property.
            //!
            //! SetComponentProperty cannot do this - it replaces a value, and a gradient list is a
            //! *container* of entity ids. Containers are only reachable through a
            //! PropertyTreeEditor, which has to be built for the component first.
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

                // Print the component's real property paths. Container schemas differ between
                // components that look alike, and a path that is merely *wrong* fails in exactly
                // the same way as one that is unsupported - this is the difference between a
                // one-line fix and guesswork.
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

            //! Toggle a component off and on so it re-reads a container it was given after it was
            //! added. Without this the gradient list is populated but not acted on, and the
            //! terrain stays invisible - which looks exactly like the preset having failed.
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

            //! The variant's name as the menu spells it, so a dialog names what was clicked.
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

            //! Terrain will not render at all without these, and they are easy to forget because
            //! they live on the level rather than on anything you selected.
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

                // The world extents are configured here, on the Terrain World component; the box on
                // the spawner only decides *where* terrain exists within them.
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
                    // Snap instances to sector centres. Without it, instance positions shift as the
                    // sector grid moves and counts drift - the automated tests set this for exactly
                    // that reason, and it makes hand-checking a planted area far less confusing.
                    SetProperty(
                        settings, "Configuration|Area System Settings|Sector Point Snap Mode",
                        IntegerValue(SectorPointSnapModeCentre));
                }
            }

            //! The noise that drives terrain height.
            //!
            //! FastNoise Gradient is what this setup is tuned around, but it comes from the
            //! FastNoise gem, which is not always enabled. Perlin Noise Gradient is part of
            //! GradientSignal - which the Gradient Transform Modifier alongside it already requires,
            //! so if the rest of the height provider built at all, Perlin is present. Both expose
            //! Configuration|Frequency and both default to 1.0, so the configured value carries over
            //! unchanged; the resulting shape differs in character but is still terrain.
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

                    // Pointer comparison against the first element, not a string compare - these are
                    // the same array entries, so identity is the question being asked.
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

                // The asset list is deliberately left empty. There is no sensible default plant to
                // pick, and guessing at an engine asset path would break the moment it moved.
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

            // Everything this builds, and the gem each piece comes from, split by whether the
            // terrain is still worth having without it. The old check covered Terrain and
            // Vegetation and let the other four gems fail silently one component at a time.
            //
            // Components are asked about rather than gems: a gem can be enabled and still have
            // registered nothing usable, and it is the components that decide whether this builds.
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
                for (const RequiredComponent& component : missing)
                {
                    AZ_Warning(
                        "EntityPresets", false, "Terrain preset needs '%s' from the %s gem, which is not available.",
                        component.m_componentName.c_str(), component.m_gemName.c_str());
                }

                AzToolsFramework::EntityPresets::ReportMissingRequirements(VariantName(variant), missing);
                return AZ::EntityId();
            }

            // Layers on top. Each is dropped on its own, and the user is asked once about all of
            // them together rather than told afterwards - it is their call whether a terrain with
            // no collision is what they wanted.
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

                for (const RequiredComponent& component : reduced)
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
            //
            // A Landscape Canvas "graph" is not a file - it is an entity carrying the Landscape
            // Canvas component, whose descendants' components become the nodes. So making the
            // terrain openable as a graph is a matter of parenting it under such an entity, and
            // needs no dependency on the Landscape Canvas gem at all.
            AZ::EntityId graphRootId;
            if (wantsLandscapeCanvas)
            {
                graphRootId = CreateEntity("Landscape Terrain", AZ::EntityId());
                if (graphRootId.IsValid())
                {
                    // Availability was settled in the preflight, so a failure here is something
                    // else - worth saying, but not worth blaming the gem for.
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

            // Both halves of the collider. Terrain Physics Heightfield Collider turns the terrain
            // into heightfield data; PhysX Heightfield Collider is what actually puts that data
            // into the physics scene. One without the other is terrain you fall straight through -
            // so it is both or neither, and the preflight already settled which.
            if (wantsPhysics)
            {
                AddComponentIfMissing(spawnerId, "Terrain Physics Heightfield Collider");
                AddComponentIfMissing(spawnerId, "PhysX Heightfield Collider");
            }

            SetProperty(
                boxShape, "Axis Aligned Box Shape|Box Configuration|Dimensions",
                AZStd::any(AZ::Vector3(TerrainSizeMetres, TerrainSizeMetres, TerrainHeightMetres)));

            // -- The height provider: noise, shaped to the spawner's box ---------------
            //
            // A child of the spawner so the two travel together, and so the setup reads as one
            // thing in the outliner rather than two unrelated entities.
            const AZ::EntityId heightProviderId = CreateEntity("Terrain Height", spawnerId);
            if (heightProviderId.IsValid())
            {
                const AZ::EntityComponentIdPair shapeReference = AddComponent(heightProviderId, "Shape Reference");
                AddComponent(heightProviderId, "Gradient Transform Modifier");
                const AZ::EntityComponentIdPair noise = AddNoiseGradient(heightProviderId);

                // Point the gradient's shape back at the spawner's box, so the noise is sampled
                // across exactly the region the terrain covers.
                SetProperty(shapeReference, "Configuration|Shape Entity Id", AZStd::any(spawnerId));
                SetProperty(noise, "Configuration|Frequency", RealValue(NoiseFrequency));

                // ...and point the spawner's lists at the gradient. This is the connection that
                // makes terrain actually appear; without it there is a region with no data in it,
                // which renders as nothing.
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
        //
        // Terrain reaches the "Create Preset" menu the way any gem would: it registers its own
        // actions and its own submenu, then attaches that submenu to the identifier
        // AzToolsFramework publishes. Nothing about it is special-cased on the framework side.

        namespace
        {
            //! Kept identical to the identifiers this used when it lived in AzToolsFramework, so a
            //! hotkey a user had already bound to one of these survives the move.
            constexpr AZStd::string_view TerrainMenuIdentifier = "o3de.menu.editor.entityPresets.terrain";

            struct TerrainVariantAction
            {
                const char* m_identifier;
                const char* m_name;
                const char* m_description;
                Variant m_variant;
            };

            //! Ordered as a progression - each builds on the one above - so the list reads as
            //! "how much do I want" rather than as three unrelated options.
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

            // Safe without any ordering arrangement: the root menu is registered during the menu
            // registration hook, and every handler completes that hook before any handler reaches
            // this one.
            menuManager->AddSubMenuToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, TerrainMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsGemSortKeyStart);
        }
    } // namespace TerrainPreset
} // namespace Terrain
