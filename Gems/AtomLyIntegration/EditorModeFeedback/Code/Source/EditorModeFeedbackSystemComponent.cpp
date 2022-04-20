/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/EditorModeFeedbackParentPass.h>
#include <Pass/EditorModeDesaturationPass.h>
#include <Pass/EditorModeTintPass.h>
#include <Pass/EditorModeBlurPass.h>
#include <Pass/EditorModeOutlinePass.h>

#include <EditorModeFeedbackSystemComponent.h>
#include <EditorModeFeedbackFeatureProcessor.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Model/ModelLodUtils.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/Utils/Utils.h>

namespace AZ
{
    namespace Render
    {
        // Builds the mesh draw packets for the specified model.
        static AZStd::vector<RPI::MeshDrawPacket> BuildMeshDrawPackets(
            const RPI::ModelLodIndex& modelLodIndex,
            const Data::Asset<RPI::ModelAsset>& modelAsset,
            const Data::Instance<RPI::Material>& material,
            const Data::Instance<RPI::ShaderResourceGroup>& meshObjectSrg)
        {
            AZStd::vector<RPI::MeshDrawPacket> meshDrawPackets;
            const Data::Asset<RPI::ModelLodAsset>& modelLodAsset = modelAsset->GetLodAssets()[modelLodIndex.m_index];
            RPI::ModelLod& modelLod = *RPI::ModelLod::FindOrCreate(modelLodAsset, modelAsset).get();
        
            for (auto i = 0; i < modelLod.GetMeshes().size(); i++)
            {
                RPI::MeshDrawPacket drawPacket(modelLod, i, material, meshObjectSrg);
                meshDrawPackets.emplace_back(drawPacket);
            }
        
            return meshDrawPackets;
        }

        // Creates the mask shader resource group for the specified mesh.
        static Data::Instance<RPI::ShaderResourceGroup> CreateMaskShaderResourceGroup(
            const MeshFeatureProcessorInterface* featureProcessor,
            const MeshFeatureProcessorInterface::MeshHandle& meshHandle,
            Data::Instance<RPI::Material> maskMaterial)
        {
            const auto& shaderAsset = maskMaterial->GetAsset()->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
            const auto& objectSrgLayout = maskMaterial->GetAsset()->GetObjectSrgLayout();
            const auto maskMeshObjectSrg = RPI::ShaderResourceGroup::Create(shaderAsset, objectSrgLayout->GetName());

            // Set the object id so the correct MVP matrices can be selected in the shader
            const auto objectId = featureProcessor->GetObjectId(meshHandle).GetIndex();
            RHI::ShaderInputNameIndex objectIdIndex = "m_objectId";
            maskMeshObjectSrg->SetConstant(objectIdIndex, objectId);
            maskMeshObjectSrg->Compile();

            return maskMeshObjectSrg;
        }

        // Gets the view for the specified scene.
        static const RPI::ViewPtr GetViewFromScene(const RPI::Scene* scene)
        {
            const auto viewportContextRequests = RPI::ViewportContextRequests::Get();
            const auto viewportContext = viewportContextRequests->GetViewportContextByScene(scene);
            const RPI::ViewPtr viewPtr = viewportContext->GetDefaultView();
            return viewPtr;
        }

        // Creates the material for the mask pass shader.
        static Data::Instance<RPI::Material> CreateMaskMaterial()
        {
            const AZStd::string path = "shaders/editormodemask.azmaterial";
            const auto materialAsset = GetAssetFromPath<RPI::MaterialAsset>(path, Data::AssetLoadBehavior::PreLoad, true);
            const auto maskMaterial = RPI::Material::FindOrCreate(materialAsset);
            return maskMaterial;
        }

        // Get the world transform for the specified entity.
        static AZ::Transform GetWorldTransformForEntity(EntityId entityId)
        {
            AZ::Transform worldTM;
            AZ::TransformBus::EventResult(worldTM, entityId, &AZ::TransformBus::Events::GetWorldTM);
            return worldTM;
        }

        EditorModeFeedbackSystemComponent::MeshHandleDrawPackets::~MeshHandleDrawPackets() = default;

        void EditorModeFeedbackSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorModeFeedbackSystemComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(0)
                    ;

                if (auto* editContext = serialize->GetEditContext())
                {
                    editContext->Class<EditorModeFeedbackSystemComponent>(
                        "Editor Mode Feedback System", "Manages discovery of Editor Mode Feedback effects")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }

            EditorModeFeatureProcessor::Reflect(context);
        }

        EditorModeFeedbackSystemComponent::~EditorModeFeedbackSystemComponent() = default;

        void EditorModeFeedbackSystemComponent::Activate()
        {
            AzFramework::ApplicationRequests::Bus::Broadcast(
                [this](AzFramework::ApplicationRequests::Bus::Events* ebus)
                {
                    m_registeryEnabled = ebus->IsEditorModeFeedbackEnabled();
                });

            if (!m_registeryEnabled)
            {
                return;
            }

            AzToolsFramework::Components::EditorComponentBase::Activate();
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
            AZ::TickBus::Handler::BusConnect();

            AZ::Interface<EditorModeFeedbackInterface>::Register(this);
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<EditorModeFeatureProcessor>();
            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Cannot get the pass system.");

            // Setup handler for load pass templates mappings
            m_loadTemplatesHandler = RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler(
                [this]()
                {
                    LoadPassTemplateMappings();
                });
           
            passSystem->AddPassCreator(Name("EditorModeFeedbackParentPass"), &EditorModeFeedbackParentPass::Create);
            passSystem->AddPassCreator(Name("EditorModeDesaturationPass"), &EditorModeDesaturationPass::Create);
            passSystem->AddPassCreator(Name("EditorModeTintPass"), &EditorModeTintPass::Create);
            passSystem->AddPassCreator(Name("EditorModeBlurPass"), &EditorModeBlurPass::Create);
            passSystem->AddPassCreator(Name("EditorModeOutlinePass"), &EditorModeOutlinePass::Create);
            passSystem->ConnectEvent(m_loadTemplatesHandler);
        }

        void EditorModeFeedbackSystemComponent::Deactivate()
        {
            if (!m_registeryEnabled)
            {
                return;
            }

            AZ::TickBus::Handler::BusDisconnect();
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
            AzToolsFramework::Components::EditorComponentBase::Deactivate();

            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<EditorModeFeatureProcessor>();
            m_loadTemplatesHandler.Disconnect();
            AZ::Interface<EditorModeFeedbackInterface>::Unregister(this);
        }

        void EditorModeFeedbackSystemComponent::LoadPassTemplateMappings()
        {
            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Cannot get the pass system.");

            const char* passTemplatesFile = "Passes/EditorModeFeedback_PassTemplates.azasset";
            passSystem->LoadPassTemplateMappings(passTemplatesFile);
        }

        bool EditorModeFeedbackSystemComponent::IsEnabled() const
        {
            return m_enabled && m_registeryEnabled;
        }

        void EditorModeFeedbackSystemComponent::RegisterOrUpdateDrawableComponent(
            EntityComponentIdPair entityComponentId, const MeshFeatureProcessorInterface::MeshHandle& meshHandle)
        {
            // Overwrite any existing mesh handle for this component-entity id
            auto& meshHandleDrawPackets =
                m_entityComponentMeshHandleDrawPackets[entityComponentId.GetEntityId()][entityComponentId.GetComponentId()];
            meshHandleDrawPackets.m_meshHandle = &meshHandle;

            // The same component can call RegisterDrawableEntity multiple times in order to update its model asset so always
            // clear any existing draw packets for the component upon registration
            meshHandleDrawPackets.m_meshDrawPackets.clear();
        }

        void EditorModeFeedbackSystemComponent::OnEditorModeActivated(
            [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode)
        {
            // Purge the draw packets for all registered 
            if (mode == AzToolsFramework::ViewportEditorMode::Focus)
            {
                for (auto& [entityId, componentMeshHandleDrawPackets] : m_entityComponentMeshHandleDrawPackets)
                {
                    for (auto& [componentid, meshHandleDrawPackets] : componentMeshHandleDrawPackets)
                    {
                        meshHandleDrawPackets.m_meshDrawPackets.clear();
                    }
                }

                if (auto* focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get())
                {
                    m_enabled = true;
                }
                else
                {
                    m_enabled = false;
                }
            }
        }

        void EditorModeFeedbackSystemComponent::OnEditorModeDeactivated(
            [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode)
        {
            if (mode == AzToolsFramework::ViewportEditorMode::Focus)
            {
                m_enabled = false;
            }
        }

        void EditorModeFeedbackSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            if (!IsEnabled())
            {
                return;
            }

            const auto focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get();
            if (!focusModeInterface)
            {
                return;
            }

            if (!m_maskMaterial)
            {
                m_maskMaterial = CreateMaskMaterial();
            }

            // Build the draw packets (where required) for each registered component and add them to the draw list
            const auto focusedEntityIds = focusModeInterface->GetFocusedEntities(AzToolsFramework::GetEntityContextId());
            for (const auto& focusedEntityId : focusedEntityIds)
            {
                const auto entityComponentMeshHandleDrawPacketsIterator = m_entityComponentMeshHandleDrawPackets.find(focusedEntityId);
                if (entityComponentMeshHandleDrawPacketsIterator == m_entityComponentMeshHandleDrawPackets.end())
                {
                    // No drawable data for this entity
                    continue;
                }

                auto& [entityId, componentMeshHandleDrawPackets] = *entityComponentMeshHandleDrawPacketsIterator;

                for (auto& [componentId, meshHandleDrawPackets] : componentMeshHandleDrawPackets)
                {
                    const auto scene = RPI::Scene::GetSceneForEntityId(entityId);
                    if (const auto* featureProcessor = scene->GetFeatureProcessor<MeshFeatureProcessorInterface>())
                    {
                        const auto view = GetViewFromScene(scene);
                        const auto worldTM = GetWorldTransformForEntity(focusedEntityId);
                        const auto modelAsset = featureProcessor->GetModelAsset(*meshHandleDrawPackets.m_meshHandle);
                        const auto model = RPI::Model::FindOrCreate(modelAsset);
                        const auto modelLodIndex = RPI::ModelLodUtils::SelectLod(view.get(), worldTM, *model);

                        // Build the mesh draw packets for this mesh if no draw packets currently exist or if the LoD for the
                        // draw packets no longer matches the loD of the model
                        if (meshHandleDrawPackets.m_meshHandle->IsValid() &&
                            (meshHandleDrawPackets.m_meshDrawPackets.empty() || meshHandleDrawPackets.m_modelLodIndex != modelLodIndex))
                        {
                            // The id value to write to the mask texture for this entity (unused in the current use case)
                            const auto maskMeshObjectSrg = CreateMaskShaderResourceGroup(
                                featureProcessor, *meshHandleDrawPackets.m_meshHandle, m_maskMaterial);
                            meshHandleDrawPackets.m_modelLodIndex = modelLodIndex;
                            
                            meshHandleDrawPackets.m_meshDrawPackets = BuildMeshDrawPackets(
                                meshHandleDrawPackets.m_modelLodIndex,
                                featureProcessor->GetModelAsset(*meshHandleDrawPackets.m_meshHandle),
                                m_maskMaterial,
                                maskMeshObjectSrg);
                        }
                    }
                    else
                    {
                        // This really shouldn't fail, but just in case...
                        AZ_Error(
                            "EditorModeFeedbackSystemComponent",
                            false,
                            AZStd::string::format("Could't get feature processor for entity '%s'", focusedEntityId.ToString().c_str()).c_str());
                    }

                    AZ::RPI::DynamicDrawInterface* dynamicDraw = AZ::RPI::GetDynamicDraw();
                    for (auto& drawPacket : meshHandleDrawPackets.m_meshDrawPackets)
                    {
                        drawPacket.Update(*scene);
                        dynamicDraw->AddDrawPacket(scene, drawPacket.GetRHIDrawPacket());
                    }
                }
            }
        }

        int EditorModeFeedbackSystemComponent::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }
    } // namespace Render
} // namespace AZ
