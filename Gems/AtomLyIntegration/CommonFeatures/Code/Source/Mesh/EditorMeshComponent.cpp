/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Mesh/EditorMeshComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentConstants.h>

namespace AZ
{
    namespace Render
    {
        void EditorMeshComponent::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);
            EditorMeshStats::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<EditorMeshStats>();

                serializeContext->Class<EditorMeshComponent, BaseClass>()
                    ->Version(2, ConvertToEditorRenderComponentAdapter<1>)
                    ->Field("addMaterialComponentFlag", &EditorMeshComponent::m_addMaterialComponentFlag)
                    ->Field("meshStats", &EditorMeshComponent::m_stats)
                    ;

                // This shouldn't be registered here, but is required to make a vector from EditorMeshComponentTypeId. This can be removed when one of the following happens:
                // - The generic type for AZStd::vector<AZ::Uuid> is registered in a more generic place
                // - EditorLevelComponentAPIComponent has a version of AddComponentsOfType that takes a single Uuid instead of a vector
                serializeContext->RegisterGenericType<AZStd::vector<AZ::Uuid>>();

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorMeshComponent>(
                        "Mesh", "The mesh component is the primary method of adding visual geometry to entities")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Category, "Atom")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Mesh.svg")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Mesh.svg")
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/atom/mesh/")
                            ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<RPI::ModelAsset>::Uuid())
                        ->DataElement(AZ::Edit::UIHandlers::Button, &EditorMeshComponent::m_addMaterialComponentFlag, "Add Material Component", "Add Material Component")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                            ->Attribute(AZ::Edit::Attributes::ButtonText, "Add Material Component")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorMeshComponent::AddEditorMaterialComponent)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &EditorMeshComponent::GetEditorMaterialComponentVisibility)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EditorMeshComponent::m_stats, "Mesh Stats", "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ;

                    editContext->Class<MeshComponentController>(
                        "MeshComponentController", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentController::m_configuration, "Configuration", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ;

                    editContext->Class<MeshComponentConfig>(
                        "MeshComponentConfig", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentConfig::m_modelAsset, "Mesh Asset", "Mesh asset reference")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MeshComponentConfig::m_sortKey, "Sort Key", "Transparent meshes are drawn by sort key then depth. Used this to force certain transparent meshes to draw before or after others.")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::IsAssetSet)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MeshComponentConfig::m_excludeFromReflectionCubeMaps, "Exclude from reflection cubemaps", "Mesh will not be visible in baked reflection probe cubemaps")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &MeshComponentConfig::m_useForwardPassIblSpecular, "Use Forward Pass IBL Specular",
                                "Renders IBL specular reflections in the forward pass, using only the most influential probe (based on the position of the entity) and the global IBL cubemap.  Can reduce rendering costs, but only recommended for static objects that are affected by at most one reflection probe.")
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::ValuesOnly)
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MeshComponentConfig::m_lodType, "Lod Type", "Lod Method.")
                                ->EnumAttribute(RPI::Cullable::LodType::Default, "Default")
                                ->EnumAttribute(RPI::Cullable::LodType::ScreenCoverage, "Screen Coverage")
                                ->EnumAttribute(RPI::Cullable::LodType::SpecificLod, "Specific Lod")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::IsAssetSet)
                                ->Attribute(AZ::Edit::Attributes::ChangeNotify, Edit::PropertyRefreshLevels::EntireTree)
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Lod Configuration")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::ShowLodConfig)
                            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MeshComponentConfig::m_lodOverride, "Lod Override", "Allows the rendered LOD to be overridden instead of being calculated automatically.")
                                ->Attribute(AZ::Edit::Attributes::EnumValues, &MeshComponentConfig::GetLodOverrideValues)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::LodTypeIsSpecificLOD)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshComponentConfig::m_minimumScreenCoverage, "Minimum Screen Coverage", "Minimum proportion of screen area an entitiy takes up, after that the entitiy is culled.")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                                ->Attribute(AZ::Edit::Attributes::Suffix, " percent")
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::LodTypeIsScreenCoverage)
                            ->DataElement(AZ::Edit::UIHandlers::Slider, &MeshComponentConfig::m_qualityDecayRate, "Quality Decay Rate",
                                "Rate at which mesh quality decays (0 -> always stay highest quality, 1 -> quality falls off to lowest quality immediately).")
                                ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                                ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                                ->Attribute(AZ::Edit::Attributes::Visibility, &MeshComponentConfig::LodTypeIsScreenCoverage)
                        ;
                }
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->ConstantProperty("EditorMeshComponentTypeId", BehaviorConstant(Uuid(EditorMeshComponentTypeId)))
                    ->Attribute(AZ::Script::Attributes::Module, "render")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);

                behaviorContext->Class<EditorMeshComponent>()->RequestBus("RenderMeshComponentRequestBus");
            }
        }

        EditorMeshComponent::EditorMeshComponent(const MeshComponentConfig& config)
            : BaseClass(config)
        {
        }

        void EditorMeshComponent::Activate()
        {
            BaseClass::Activate();
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
        }

        void EditorMeshComponent::Deactivate()
        {
            MeshComponentNotificationBus::Handler::BusDisconnect();
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
            AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        void EditorMeshComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            m_controller.SetModelAssetId(assetId);
        }

        AZ::Aabb EditorMeshComponent::GetEditorSelectionBoundsViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo)
        {
            return m_controller.GetWorldBounds();
        }

        bool EditorMeshComponent::EditorSelectionIntersectRayViewport(
            [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, const AZ::Vector3& src,
            const AZ::Vector3& dir, float& distance)
        {
            if (!m_controller.GetModel())
            {
                return false;
            }

            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

            float t;
            AZ::Vector3 ignoreNormal;
            constexpr float rayLength = 1000.0f;
            if (m_controller.GetModel()->RayIntersection(transform, nonUniformScale, src, dir * rayLength, t, ignoreNormal))
            {
                distance = rayLength * t;
                return true;
            }

            return false;
        }

        bool EditorMeshComponent::SupportsEditorRayIntersect()
        {
            return true;
        }

        void EditorMeshComponent::DisplayEntityViewport(
            const AzFramework::ViewportInfo&, AzFramework::DebugDisplayRequests& debugDisplay)
        {
            if (!IsSelected())
            {
                return;
            }

            const AZ::Aabb localAabb = m_controller.GetLocalBounds();
            if (!localAabb.IsValid())
            {
                return;
            }

            AZ::Transform worldTM;
            AZ::TransformBus::EventResult(worldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

            debugDisplay.PushMatrix(worldTM);

            debugDisplay.DrawWireBox(localAabb.GetMin(), localAabb.GetMax());

            debugDisplay.PopMatrix();
        }

        AZ::Crc32 EditorMeshComponent::AddEditorMaterialComponent()
        {
            const AZStd::vector<AZ::EntityId> entityList = { GetEntityId() };
            const AZ::ComponentTypeList componentsToAdd = { AZ::Uuid(AZ::Render::EditorMaterialComponentTypeId) };

            AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome outcome =
                AZ::Failure(AZStd::string("Failed to add AZ::Render::EditorMaterialComponentTypeId"));
            AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(outcome, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entityList, componentsToAdd);
            return Edit::PropertyRefreshLevels::EntireTree;
        }

        bool EditorMeshComponent::HasEditorMaterialComponent() const
        {
            return GetEntity() && GetEntity()->FindComponent(AZ::Uuid(AZ::Render::EditorMaterialComponentTypeId)) != nullptr;
        }

        AZ::u32 EditorMeshComponent::GetEditorMaterialComponentVisibility() const
        {
            return HasEditorMaterialComponent() ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
        }

        void EditorMeshComponent::OnModelReady(const Data::Asset<RPI::ModelAsset>& /*modelAsset*/, const Data::Instance<RPI::Model>& /*model*/)
        {
            const auto& lodAssets = m_controller.GetConfiguration().m_modelAsset->GetLodAssets();
            m_stats.m_meshStatsForLod.clear();
            m_stats.m_meshStatsForLod.reserve(lodAssets.size());
            for (const auto& lodAsset : lodAssets)
            {
                EditorMeshStatsForLod stats;
                const auto& meshes = lodAsset->GetMeshes();
                stats.m_meshCount = static_cast<AZ::u32>(meshes.size());
                for (const auto& mesh : meshes)
                {
                    stats.m_vertCount += mesh.GetVertexCount();
                    stats.m_triCount += mesh.GetIndexCount() / 3;
                }
                m_stats.m_meshStatsForLod.emplace_back(AZStd::move(stats));
            }

            // Refresh the tree when the model loads to update UI based on the model.
            AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
                &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay,
                AzToolsFramework::Refresh_EntireTree);
        }

        AZ::u32 EditorMeshComponent::OnConfigurationChanged()
        {
            // temp variable is needed to hold reference to m_modelAsset while it's being loaded.
            // Otherwise it gets released in Deactivate function, and instantly re-activating the component
            // places it in a bad state, which happens in OnConfigurationChanged base function.
            // This is a bug with AssetManager [LYN-2249]
            auto temp = m_controller.m_configuration.m_modelAsset;

            m_stats.m_meshStatsForLod.swap({});
            SetDirty();

            return BaseClass::OnConfigurationChanged();
        }

        void EditorMeshComponent::OnEntityVisibilityChanged(bool visibility)
        {
            m_controller.SetVisibility(visibility);
        }

        bool EditorMeshComponent::ShouldActivateController() const
        {
            // By default, components using the EditorRenderComponentAdapter will only activate if the component is visible
            // Since the mesh component handles visibility changes by not rendering the mesh, rather than deactivating the component entirely,
            // it can be activated even if it is not visible
            return true;
        }
    } // namespace Render
} // namespace AZ
