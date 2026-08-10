/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/EntityPresets/TerrainPreset.h>

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

namespace AzToolsFramework
{
    namespace TerrainPreset
    {
        namespace
        {
            using EntityType = EditorComponentAPIRequests::EntityType;

            // ── The shape of the terrain this builds ──────────────────────────────────
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

            // ── Property values ───────────────────────────────────────────────────────
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

            // ── Component plumbing ────────────────────────────────────────────────────

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
                    EditorComponentAPIBus::BroadcastResult(
                        typeIds, &EditorComponentAPIRequests::FindComponentTypeIdsByEntityType,
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

                EditorComponentAPIRequests::AddComponentsOutcome outcome =
                    AZ::Failure(AZStd::string());
                EditorComponentAPIBus::BroadcastResult(
                    outcome, &EditorComponentAPIRequests::AddComponentOfType, entityId, typeId);

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
                EditorComponentAPIBus::BroadcastResult(
                    alreadyPresent, &EditorComponentAPIRequests::HasComponentOfType, entityId,
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

                EditorComponentAPIRequests::PropertyOutcome outcome =
                    AZ::Failure(AZStd::string());
                EditorComponentAPIBus::BroadcastResult(
                    outcome, &EditorComponentAPIRequests::SetComponentProperty, component,
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

                EditorComponentAPIRequests::PropertyTreeOutcome treeOutcome =
                    AZ::Failure(AZStd::string());
                EditorComponentAPIBus::BroadcastResult(
                    treeOutcome, &EditorComponentAPIRequests::BuildComponentPropertyTreeEditor,
                    component);

                if (!treeOutcome.IsSuccess())
                {
                    AZ_Warning(
                        "EntityPresets", false, "Terrain preset could not inspect '%s': %s", propertyPath,
                        treeOutcome.GetError().c_str());
                    return false;
                }

                PropertyTreeEditor tree = treeOutcome.GetValue();
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
                EditorComponentAPIBus::BroadcastResult(
                    available, &EditorComponentAPIRequests::BuildComponentPropertyList,
                    component);

                for (const AZStd::string& path : available)
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
                EditorComponentAPIBus::BroadcastResult(
                    refreshed, &EditorComponentAPIRequests::EnableComponents,
                    AZStd::vector<AZ::EntityComponentIdPair>{ component });

                AZ_Warning(
                    "EntityPresets", refreshed,
                    "Terrain preset could not refresh a gradient list. If nothing appears, disable and "
                    "re-enable that component on the new entity.");
            }

            AZ::EntityId CreateEntity(const char* name, const AZ::EntityId parentId)
            {
                AZ::EntityId entityId;
                ToolsApplicationRequestBus::BroadcastResult(
                    entityId, &ToolsApplicationRequests::CreateNewEntity, parentId);

                if (entityId.IsValid())
                {
                    EditorEntityAPIBus::Event(
                        entityId, &EditorEntityAPIRequests::SetName, AZStd::string(name));
                }

                return entityId;
            }

            AZ::EntityId LevelEntityId()
            {
                AZ::EntityId levelEntityId;
                ToolsApplicationRequestBus::BroadcastResult(
                    levelEntityId, &ToolsApplicationRequests::GetCurrentLevelEntityId);
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
                EditorComponentAPIBus::BroadcastResult(
                    alreadyPresent, &EditorComponentAPIRequests::HasComponentOfType,
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
            const bool wantsLandscapeCanvas = variant != Variant::Simple;
            bool wantsVegetation = variant == Variant::LandscapeWithVegetation;

            // Landscape Canvas and FastNoise are genuinely optional - the setup degrades to
            // something usable without them. The Terrain gem is not: without it there is no
            // spawner, no height gradient list and no world component, so pressing on would create
            // a hierarchy of empty entities, add nothing to any of them, log a warning per missing
            // component, and leave the mess behind for the user to delete. Check before touching
            // anything, and before opening an undo batch there would be nothing to put in.
            if (FindEntityComponentTypeId("Terrain Layer Spawner").IsNull())
            {
                AZ_Warning(
                    "EntityPresets", false,
                    "Terrain preset needs the Terrain gem, which is not enabled for this project. "
                    "Enable it in Project Manager and restart the Editor. Nothing has been created.");

                return AZ::EntityId();
            }

            // Vegetation is a layer on top rather than a prerequisite, so a missing gem downgrades
            // the request instead of refusing it - the terrain is still worth having.
            if (wantsVegetation && FindEntityComponentTypeId("Vegetation Layer Spawner").IsNull())
            {
                AZ_Warning(
                    "EntityPresets", false,
                    "Terrain preset cannot add vegetation without the Vegetation gem, which is not "
                    "enabled for this project. Building the terrain without it.");

                wantsVegetation = false;
            }

            ToolsApplicationRequestBus::Broadcast(
                &ToolsApplicationRequests::BeginUndoBatch, "Create Terrain");

            EnsureTerrainLevelComponents();
            if (wantsVegetation)
            {
                EnsureVegetationLevelComponents();
            }

            // ── The graph root, for the Landscape Canvas variants ─────────────────────
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
                    const AZ::EntityComponentIdPair canvas = AddComponent(graphRootId, "Landscape Canvas");

                    AZ_Warning(
                        "EntityPresets", canvas.GetEntityId().IsValid(),
                        "The terrain was built, but without a Landscape Canvas component it will not open "
                        "as a graph. Enable the Landscape Canvas gem for this project.");
                }
            }

            // ── The spawner: a box saying where terrain exists ────────────────────────
            const AZ::EntityId spawnerId = CreateEntity("Terrain", graphRootId);
            if (!spawnerId.IsValid())
            {
                AZ_Warning("EntityPresets", false, "Terrain preset could not create its entity.");
                ToolsApplicationRequestBus::Broadcast(
                    &ToolsApplicationRequests::EndUndoBatch);
                return AZ::EntityId();
            }

            const AZ::EntityComponentIdPair boxShape = AddComponent(spawnerId, "Axis Aligned Box Shape");
            AddComponent(spawnerId, "Terrain Layer Spawner");
            const AZ::EntityComponentIdPair heightGradientList =
                AddComponent(spawnerId, "Terrain Height Gradient List");

            // Both halves of the collider. Terrain Physics Heightfield Collider turns the terrain
            // into heightfield data; PhysX Heightfield Collider is what actually puts that data
            // into the physics scene. One without the other is terrain you fall straight through.
            AddComponentIfMissing(spawnerId, "Terrain Physics Heightfield Collider");
            AddComponentIfMissing(spawnerId, "PhysX Heightfield Collider");

            SetProperty(
                boxShape, "Axis Aligned Box Shape|Box Configuration|Dimensions",
                AZStd::any(AZ::Vector3(TerrainSizeMetres, TerrainSizeMetres, TerrainHeightMetres)));

            // ── The height provider: noise, shaped to the spawner's box ───────────────
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

            ToolsApplicationRequestBus::Broadcast(
                &ToolsApplicationRequests::SetSelectedEntities,
                EntityIdList{ graphRootId.IsValid() ? graphRootId : spawnerId });

            ToolsApplicationRequestBus::Broadcast(
                &ToolsApplicationRequests::EndUndoBatch);

            return spawnerId;
        }
    } // namespace TerrainPreset
} // namespace AzToolsFramework
