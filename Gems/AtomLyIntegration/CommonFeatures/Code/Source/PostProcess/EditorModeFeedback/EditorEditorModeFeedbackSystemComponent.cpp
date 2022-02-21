/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcess/EditorModeFeedback/EditorEditorModeFeedbackSystemComponent.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
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
       static AZStd::vector<RPI::MeshDrawPacket> BuildMeshDrawPackets(
           const RPI::View* view,
           const AZ::Transform& worldTM,
           const Data::Asset<RPI::ModelAsset>& modelAsset,
           const Data::Instance<RPI::Material>& material,
           const Data::Instance<RPI::ShaderResourceGroup>& meshObjectSrg)
       {
           AZStd::vector<RPI::MeshDrawPacket> meshDrawPackets;
           RPI::Model& model = *RPI::Model::FindOrCreate(modelAsset);
           auto modelLodIndex = RPI::ModelLodUtils::SelectLod(view, worldTM, model);
           const Data::Asset<RPI::ModelLodAsset>& modelLodAsset = modelAsset->GetLodAssets()[0];
           RPI::ModelLod& modelLod = *RPI::ModelLod::FindOrCreate(modelLodAsset, modelAsset).get();
       
           for (auto i = 0; i < modelLod.GetMeshes().size(); i++)
           {
               RPI::MeshDrawPacket drawPacket(modelLod, i, material, meshObjectSrg);
               meshDrawPackets.push_back(AZStd::move(drawPacket));
           }
       
           return meshDrawPackets;
       }

        static Data::Instance<RPI::Material> CreateMaskMaterial()
        {
            AZStd::string path = "shaders/postprocessing/editormodemask.azmaterial";
            auto materialAsset = GetAssetFromPath<RPI::MaterialAsset>(path, Data::AssetLoadBehavior::PreLoad, true);
            auto maskMaterial = RPI::Material::FindOrCreate(materialAsset);
            return maskMaterial;
        }

        static Data::Instance<RPI::ShaderResourceGroup> CreateMaskShaderResourceGroup(Data::Instance<RPI::Material> maskMaterial)
        {
            auto& shaderAsset = maskMaterial->GetAsset()->GetMaterialTypeAsset()->GetShaderAssetForObjectSrg();
            auto& objectSrgLayout = maskMaterial->GetAsset()->GetObjectSrgLayout();
            auto maskMeshObjectSrg = RPI::ShaderResourceGroup::Create(shaderAsset, objectSrgLayout->GetName());

            return maskMeshObjectSrg;
        }

        static void SetMeshObjectId(
            uint32_t objectId,
            Data::Instance<RPI::ShaderResourceGroup>& shaderResourceGroup)
        {
            RHI::ShaderInputNameIndex objectIdIndex = "m_objectId";
            shaderResourceGroup->SetConstant(objectIdIndex, objectId);
            RHI::ShaderInputNameIndex maskIdIndex = "m_maskId";
            shaderResourceGroup->SetConstant(maskIdIndex, 1);
            shaderResourceGroup->Compile();
        }

        static const RPI::ViewPtr GetViewFromScene(const RPI::Scene* scene)
        {
            auto viewportContextRequests = RPI::ViewportContextRequests::Get();
            auto viewportContext = viewportContextRequests->GetViewportContextByScene(scene);
            const RPI::ViewPtr viewPtr = viewportContext->GetDefaultView();
            return viewPtr;
        }

        void EditorEditorModeFeedbackSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<EditorEditorModeFeedbackSystemComponent, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<EditorEditorModeFeedbackSystemComponent>(
                          "Editor Mode Feedback System", "Manages discovery of Editr Mode Feedback effects")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        EditorEditorModeFeedbackSystemComponent::~EditorEditorModeFeedbackSystemComponent() = default;

        void EditorEditorModeFeedbackSystemComponent::Init()
        {
            AzToolsFramework::Components::EditorComponentBase::Init();
        }

        void EditorEditorModeFeedbackSystemComponent::Activate()
        {
            AzToolsFramework::Components::EditorComponentBase::Activate();
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
            AZ::Interface<EditorModeFeedbackInterface>::Register(this);
            AZ::TickBus::Handler::BusConnect();
        }

        void EditorEditorModeFeedbackSystemComponent::Deactivate()
        {
            AZ::TickBus::Handler::BusDisconnect();
            AZ::Interface<EditorModeFeedbackInterface>::Unregister(this);
            AzToolsFramework::ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
            AzToolsFramework::Components::EditorComponentBase::Deactivate();
        }

        bool EditorEditorModeFeedbackSystemComponent::IsEnabled() const
        {
            return m_enabled;
        }

        void EditorEditorModeFeedbackSystemComponent::RegisterDrawableComponent(
            EntityComponentIdPair entityComponentId, const MeshFeatureProcessorInterface::MeshHandle& meshHandle)
        {
            auto& [componentMeshHandle, componentMeshDrawPackets] =
                m_entityComponentMeshDrawPackets[entityComponentId.GetEntityId()][entityComponentId.GetComponentId()];
            componentMeshHandle = &meshHandle;
 
            // The same component can call RegisterDrawableEntity multiple times in order to update its model asset so always
            // clear any existing draw packets for the component upon registration
            componentMeshDrawPackets.clear();
        }

        void EditorEditorModeFeedbackSystemComponent::OnEditorModeActivated(
            [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode)
        {
            if (mode == AzToolsFramework::ViewportEditorMode::Focus)
            {
                for (auto& it : m_entityComponentMeshDrawPackets)
                {
                    for (auto& it2 : it.second)
                    {
                        auto& [componentMeshHandle, componentMeshDrawPackets] = it2.second;
                        componentMeshDrawPackets.clear();
                    }
                }

                m_enabled = true;
            }
        }

        void EditorEditorModeFeedbackSystemComponent::OnEditorModeDeactivated(
            [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState,
            AzToolsFramework::ViewportEditorMode mode)
        {
            if (mode == AzToolsFramework::ViewportEditorMode::Focus)
            {
                m_enabled = false;
            }
        }

        void EditorEditorModeFeedbackSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            if (!m_enabled)
            {
                return;
            }

            const auto focusModeInterface = AZ::Interface<AzToolsFramework::FocusModeInterface>::Get();
            if (!focusModeInterface)
            {
                return;
            }

            // TODO: see if there is a more reliable method of creating the required resources once the depender systems are initialized
            if (!m_maskMaterial)
            {
                m_maskMaterial = CreateMaskMaterial();
            }

            // Build the draw packets (where required) for each registered component and add them to the draw list
            const auto focusedEntityIds = focusModeInterface->GetFocusedEntities(AzToolsFramework::GetEntityContextId());
            for (const auto& focusedEntityId : focusedEntityIds)
            {
                const auto it = m_entityComponentMeshDrawPackets.find(focusedEntityId);
                if (it == m_entityComponentMeshDrawPackets.end())
                {
                    // No drawable data for this entity
                    continue;
                }

                auto& [entityId, componentModelAssetDrawPackets] = *it;

                for (auto& it2 : componentModelAssetDrawPackets)
                {
                    auto& [componentId, meshDrawPackets] = it2;

                    const auto scene = RPI::Scene::GetSceneForEntityId(entityId);
                    if (meshDrawPackets.m_drawPackets.empty())
                    {                        
                        if (auto featureProcessor = RPI::Scene::GetFeatureProcessorForEntity<MeshFeatureProcessorInterface>(focusedEntityId))
                        {
                            auto objectId = featureProcessor->GetObjectId(*meshDrawPackets.m_meshHandle).GetIndex();
                            auto maskMeshObjectSrg = CreateMaskShaderResourceGroup(m_maskMaterial);
                            SetMeshObjectId(objectId, maskMeshObjectSrg);
                            const auto view = GetViewFromScene(scene);

                            AZ::Transform worldTM;
                            AZ::TransformBus::EventResult(worldTM, focusedEntityId, &AZ::TransformBus::Events::GetWorldTM);
                            auto modelAsset = featureProcessor->GetModelAsset(*meshDrawPackets.m_meshHandle);
                            meshDrawPackets.m_drawPackets = BuildMeshDrawPackets(view.get(), worldTM, modelAsset, m_maskMaterial, maskMeshObjectSrg);
                        }
                        else
                        {
                            // This really shouldn't fail, but just in case...
                            AZ_Error("EditorEditorModeFeedbackSystemComponent", false, AZStd::string::format("Could't get feature processor for entity '%s'", focusedEntityId.ToString().c_str()).c_str());
                        }
                    }

                    AZ::RPI::DynamicDrawInterface* dynamicDraw = AZ::RPI::GetDynamicDraw();
                    for (auto& drawPacket : meshDrawPackets.m_drawPackets)
                    {
                        drawPacket.Update(*scene);
                        dynamicDraw->AddDrawPacket(scene, drawPacket.GetRHIDrawPacket());
                    }
                }
            }
        }

        int EditorEditorModeFeedbackSystemComponent::GetTickOrder()
        {
            return AZ::TICK_PRE_RENDER;
        }
    }
}
