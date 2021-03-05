/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "LmbrCentral_precompiled.h"

#include "EditorSkinnedMeshComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <MathConversion.h>

#include <CryFile.h>
#include <IPhysics.h> // For basic physicalization at edit-time for object snapping.
#include "MeshComponent.h"
#include <AzCore/Asset/AssetManager.h>

#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>

#include "CryPhysicsDeprecation.h"

namespace LmbrCentral
{

    //////////////////////////////////////////////////////////////////////////
    /// Deprecated EditorMeshComponent
    //////////////////////////////////////////////////////////////////////////
    namespace ClassConverters
    {
        static bool DeprecateEditorMeshComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    } // namespace ClassConverters
    //////////////////////////////////////////////////////////////////////////


    void EditorSkinnedMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            // Need to deprecate the old EditorMeshComponent whenever we see one.
            serializeContext->ClassDeprecate("EditorMeshComponent", "{C4C69E93-4C1F-446D-AFAB-F8835AD8EFB0}", &ClassConverters::DeprecateEditorMeshComponent);
            serializeContext->Class<EditorSkinnedMeshComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("Skinned Mesh Render Node", &EditorSkinnedMeshComponent::m_mesh)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();

            if (editContext)
            {
                editContext->Class<EditorSkinnedMeshComponent>("Skinned Mesh", "The Skinned Mesh component is the primary way to add animated visual geometry to entities")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Animation (Legacy)")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SkinnedMesh.svg")
                        ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, AZ::AzTypeInfo<LmbrCentral::CharacterDefinitionAsset>::Uuid())
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SkinnedMesh.png")
                        ->Attribute(AZ::Edit::Attributes::PreferNoViewportIcon, true)
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-skinned-mesh.html")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorSkinnedMeshComponent::m_mesh);

                editContext->Class<SkinnedMeshComponentRenderNode::SkinnedRenderOptions>(
                    "Render Options", "Rendering options for the mesh.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Options")

                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_opacity, "Opacity", "Opacity value")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::OnChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_maxViewDist, "Max view distance", "Maximum view distance in meters.")
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, &SkinnedMeshComponentRenderNode::GetDefaultMaxViewDist)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::OnChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_viewDistMultiplier, "View distance multiplier", "Adjusts max view distance. If 1.0 then default is used. 1.1 would be 10% further than default.")
                        ->Attribute(AZ::Edit::Attributes::Suffix, "x")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::OnChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_lodRatio, "LOD distance ratio", "Controls LOD ratio over distance.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::OnChanged)
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 255)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_castShadows, "Cast shadows", "Object will cast shadows.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::OnChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_useVisAreas, "Use VisAreas", "Allow VisAreas to control this component's visibility.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::OnChanged)

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced")

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_rainOccluder, "Rain occluder", "Occludes dynamic raindrops.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::OnChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_acceptDecals, "Accept decals", "Can receive decals.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::OnChanged)
                    ;

                editContext->Class<SkinnedMeshComponentRenderNode>(
                    "Mesh Rendering", "Attach geometry to the entity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkinnedMeshComponentRenderNode::m_visible, "Visible", "Is mesh initially visible?")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::RefreshRenderState)
                    // historical note:  This used to be a "SkinnedMeshAsset" but that became a new type.  For compatibility we are preserving the serialization name "SkinnedMeshAsset"
                    ->DataElement("SkinnedMeshAsset", &SkinnedMeshComponentRenderNode::m_characterDefinitionAsset, "Character definition", "Character Definition reference")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::OnAssetPropertyChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SkinnedMeshComponentRenderNode::m_renderOptions, "Render options", "Render/draw options.")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SkinnedMeshComponentRenderNode::RefreshRenderState)
                    ;

            }
        }
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<EditorSkinnedMeshComponent>()->RequestBus("MeshComponentRequestBus");
        }
    }

    void EditorSkinnedMeshComponent::Activate()
    {
        EditorComponentBase::Activate();

        m_mesh.AttachToEntity(m_entity->GetId());

        // Check current visibility and updates render flags appropriately
        bool visible = false;
        AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
            visible, GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);

        m_mesh.UpdateAuxiliaryRenderFlags(!visible, ERF_HIDDEN);

        // Note we are purposely connecting to buses before calling m_mesh.CreateMesh().
        // m_mesh.CreateMesh() can result in events (eg: OnMeshCreated) that we want receive.
        MaterialOwnerRequestBus::Handler::BusConnect(GetEntityId());
        MeshComponentRequestBus::Handler::BusConnect(GetEntityId());
        MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
        RenderNodeRequestBus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusConnect(GetEntityId());
        SkeletalHierarchyRequestBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusConnect(GetEntityId());
        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());

        auto renderOptionsChangeCallback =
            [this]()
        {
            this->m_mesh.ApplyRenderOptions();
        };
        m_mesh.m_renderOptions.m_changeCallback = renderOptionsChangeCallback;

        if (m_mesh.m_isQueuedForDestroyMesh)
        {
            m_mesh.m_isQueuedForDestroyMesh = false;
        }
        else
        {
            m_mesh.CreateMesh();
        }
    }

    void EditorSkinnedMeshComponent::Deactivate()
    {
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        SkeletalHierarchyRequestBus::Handler::BusDisconnect();
        MaterialOwnerRequestBus::Handler::BusDisconnect();
        MeshComponentRequestBus::Handler::BusDisconnect();
        MeshComponentNotificationBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorVisibilityNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorComponentSelectionRequestsBus::Handler::BusDisconnect();

        DestroyEditorPhysics();

        m_mesh.m_renderOptions.m_changeCallback = 0;

        if (!m_mesh.GetMeshAsset().IsReady())
        {
            m_mesh.m_isQueuedForDestroyMesh = true;
        }
        else
        {
            m_mesh.DestroyMesh();
        }

        m_mesh.AttachToEntity(AZ::EntityId());

        EditorComponentBase::Deactivate();
    }

    void EditorSkinnedMeshComponent::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        AZ_UNUSED(asset)
        CRY_PHYSICS_REPLACEMENT_ASSERT();
    }

    void EditorSkinnedMeshComponent::OnMeshDestroyed()
    {
        DestroyEditorPhysics();
    }

    IRenderNode* EditorSkinnedMeshComponent::GetRenderNode()
    {
        return &m_mesh;
    }

    float EditorSkinnedMeshComponent::GetRenderNodeRequestBusOrder() const
    {
        return s_renderNodeRequestBusOrder;
    }

    void EditorSkinnedMeshComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        AZ_UNUSED(world)
        CRY_PHYSICS_REPLACEMENT_ASSERT();
    }

    AZ::Aabb EditorSkinnedMeshComponent::GetWorldBounds()
    {
        return m_mesh.CalculateWorldAABB();
    }

    AZ::Aabb EditorSkinnedMeshComponent::GetLocalBounds()
    {
        return m_mesh.CalculateLocalAABB();
    }

    void EditorSkinnedMeshComponent::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        m_mesh.SetMeshAsset(id);
        EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, AddDirtyEntity, GetEntityId());
    }

    void EditorSkinnedMeshComponent::SetMaterial(_smart_ptr<IMaterial> material)
    {
        m_mesh.SetMaterial(material);

        EBUS_EVENT(AzToolsFramework::ToolsApplicationEvents::Bus, InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    _smart_ptr<IMaterial> EditorSkinnedMeshComponent::GetMaterial()
    {
        return m_mesh.GetMaterial();
    }

    void EditorSkinnedMeshComponent::SetPrimaryAsset(const AZ::Data::AssetId& id)
    {
        SetMeshAsset(id);
    }

    void EditorSkinnedMeshComponent::OnEntityVisibilityChanged(bool visibility)
    {
        m_mesh.UpdateAuxiliaryRenderFlags(!visibility, ERF_HIDDEN);
        m_mesh.RefreshRenderState();
    }

    void EditorSkinnedMeshComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (SkinnedMeshComponent* meshComponent = gameEntity->CreateComponent<SkinnedMeshComponent>())
        {
            m_mesh.CopyPropertiesTo(meshComponent->m_skinnedMeshRenderNode);
        }
    }

    AZ::Aabb EditorSkinnedMeshComponent::GetEditorSelectionBoundsViewport(
        const AzFramework::ViewportInfo& /*viewportInfo*/)
    {
        return GetWorldBounds();
    }

    void EditorSkinnedMeshComponent::CreateEditorPhysics()
    {
        DestroyEditorPhysics();

        if (!GetTransform())
        {
            return;
        }

        IStatObj* geometry = m_mesh.GetEntityStatObj();
        if (!geometry)
        {
            return;
        }

        CRY_PHYSICS_REPLACEMENT_ASSERT();
    }

    void EditorSkinnedMeshComponent::DestroyEditorPhysics()
    {
        // If physics is completely torn down, all physical entities are by extension completely invalid (dangling pointers).
        // It doesn't matter that we held a reference.
        CRY_PHYSICS_REPLACEMENT_ASSERT();
        m_physTransform = AZ::Transform::CreateIdentity();
    }

    bool EditorSkinnedMeshComponent::GetVisibility()
    {
        return m_mesh.GetVisible();
    }

    void EditorSkinnedMeshComponent::SetVisibility(bool isVisible)
    {
        m_mesh.SetVisible(isVisible);
    }

    AZ::u32 EditorSkinnedMeshComponent::GetJointCount()
    {
        return m_mesh.GetJointCount();
    }

    const char* EditorSkinnedMeshComponent::GetJointNameByIndex(AZ::u32 jointIndex)
    {
        return m_mesh.GetJointNameByIndex(jointIndex);
    }

    AZ::s32 EditorSkinnedMeshComponent::GetJointIndexByName(const char* jointName)
    {
        return m_mesh.GetJointIndexByName(jointName);
    }

    AZ::Transform EditorSkinnedMeshComponent::GetJointTransformCharacterRelative(AZ::u32 jointIndex)
    {
        return m_mesh.GetJointTransformCharacterRelative(jointIndex);
    }

    //////////////////////////////////////////////////////////////////////////
    /// Deprecated EditorMeshComponent
    //////////////////////////////////////////////////////////////////////////
    namespace ClassConverters
    {
        extern void MeshComponentRenderNodeRenderOptionsVersion1To2Converter(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode&);
        extern void MeshComponentRenderNodeRenderOptionsVersion2To3Converter(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode&);

        static bool DeprecateEditorMeshComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {

            //////////////////////////////////////////////////////////////////////////
            // Pull data out of the old version
            auto index = classElement.FindElement(AZ_CRC("Mesh", 0xe16f3a56));
            if (index < 0)
            {
                return false;
            }

            auto renderNode = classElement.GetSubElement(index);

            index = classElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));
            if (index < 0)
            {
                return false;
            }

            auto editorBaseClass = classElement.GetSubElement(index);

            index = renderNode.FindElement(AZ_CRC("Material Override", 0xebc12e43));
            if (index < 0)
            {
                return false;
            }

            auto materialOverride = renderNode.GetSubElement(index);

            index = renderNode.FindElement(AZ_CRC("Render Options", 0xb5bc5e06));
            if (index < 0)
            {
                return false;
            }

            auto renderOptions = renderNode.GetSubElement(index);
            MeshComponentRenderNodeRenderOptionsVersion1To2Converter(context, renderOptions);

            // There is a visible field in the render options that we might want to keep so we should grab it now
            // this will default to true if the mesh component is at version 3 and does not have a Visible render node option
            bool visible = true;
            renderOptions.GetChildData(AZ_CRC("Visible", 0x7ab0e859), visible);
            MeshComponentRenderNodeRenderOptionsVersion2To3Converter(context, renderOptions);

            // Use the visible field found in the render node options as a default if we found it earlier, default to true otherwise
            renderNode.GetChildData(AZ_CRC("Visible", 0x7ab0e859), visible);

            float opacity = 1.f;
            renderOptions.GetChildData(AZ_CRC("Opacity", 0x43fd6d66), opacity);
            float maxViewDistance;
            renderOptions.GetChildData(AZ_CRC("MaxViewDistance", 0xa2945dd7), maxViewDistance);
            float viewDIstanceMultuplier = 1.f;
            renderOptions.GetChildData(AZ_CRC("ViewDistanceMultiplier", 0x86a77124), viewDIstanceMultuplier);
            unsigned lodRatio = 100;
            renderOptions.GetChildData(AZ_CRC("LODRatio", 0x36bf54bf), lodRatio);
            bool castDynamicShadows = true;
            renderOptions.GetChildData(AZ_CRC("CastDynamicShadows", 0x55c75b43), castDynamicShadows);
            bool castLightmapShadows = true;
            renderOptions.GetChildData(AZ_CRC("CastLightmapShadows", 0x10ce0bf8), castLightmapShadows);
            bool indoorOnly = false;
            renderOptions.GetChildData(AZ_CRC("IndoorOnly", 0xc8ab6ddb), indoorOnly);
            bool bloom = true;
            renderOptions.GetChildData(AZ_CRC("Bloom", 0xc6cd7d1b), bloom);
            bool motionBlur = true;
            renderOptions.GetChildData(AZ_CRC("MotionBlur", 0x917cdb53), motionBlur);
            bool rainOccluder = false;
            renderOptions.GetChildData(AZ_CRC("RainOccluder", 0x4f245a07), rainOccluder);
            bool affectDynamicWater = false;
            renderOptions.GetChildData(AZ_CRC("AffectDynamicWater", 0xe6774a5b), affectDynamicWater);
            bool receiveWind = false;
            renderOptions.GetChildData(AZ_CRC("ReceiveWind", 0x952a1261), receiveWind);
            bool acceptDecals = true;
            renderOptions.GetChildData(AZ_CRC("AcceptDecals", 0x3b3240a7), acceptDecals);
            bool visibilityOccluder = false;
            renderOptions.GetChildData(AZ_CRC("VisibilityOccluder", 0xe5819c29), visibilityOccluder);
            bool depthTest = true;
            renderOptions.GetChildData(AZ_CRC("DepthTest", 0x532f68b9), depthTest);
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Parse the asset reference so we know if it's a static or skinned mesh
            int meshAssetElementIndex = renderNode.FindElement(AZ_CRC("Mesh", 0xe16f3a56));
            AZStd::string path;
            AZ::Data::AssetId meshAssetId;
            if (meshAssetElementIndex != -1)
            {
                auto meshAssetNode = renderNode.GetSubElement(meshAssetElementIndex);
                // pull the raw data from the old asset node to get the asset id so we can create an asset of the new type
                AZStd::string rawElementString = AZStd::string(&meshAssetNode.GetRawDataElement().m_buffer[0]);
                // raw data will look something like this
                //"id={41FDB841-F602-5603-BFFA-8BAA6930347B}:0,type={202B64E8-FD3C-4812-A842-96BC96E38806}"
                //    ^           38 chars                 ^
                unsigned int beginningOfSequence = rawElementString.find("id={") + 3;
                const int guidLength = 38;
                AZStd::string assetGuidString = rawElementString.substr(beginningOfSequence, guidLength);

                meshAssetId = AZ::Data::AssetId(AZ::Uuid::CreateString(assetGuidString.c_str()));
                EBUS_EVENT_RESULT(path, AZ::Data::AssetCatalogRequestBus, GetAssetPathById, meshAssetId);
                int renderOptionsIndex = -1;
            }


            AZStd::string newComponentStringGuid;
            AZStd::string renderNodeName;
            AZStd::string meshTypeString;
            AZ::Uuid meshAssetUuId;
            AZ::Uuid renderNodeUuid;
            AZ::Uuid renderOptionUuid;
            AZ::Data::Asset<AZ::Data::AssetData> meshAssetData;

            // Switch to the new component type based on the asset type of the original
            // .cdf,.chr files become skinned mesh assets inside of skinned mesh components
            // otherwise it becomes a static mesh asset in a static mesh component
            if (path.find(CRY_CHARACTER_DEFINITION_FILE_EXT) == AZStd::string::npos)
            {
                newComponentStringGuid = "{FC315B86-3280-4D03-B4F0-5553D7D08432}";
                renderNodeName = "Static Mesh Render Node";
                renderNodeUuid = AZ::AzTypeInfo<MeshComponentRenderNode>::Uuid();
                meshAssetUuId = AZ::AzTypeInfo<MeshAsset>::Uuid();
                renderOptionUuid = MeshComponentRenderNode::GetRenderOptionsUuid();
                meshTypeString = "Static Mesh";
            }
            else
            {
                newComponentStringGuid = "{D3E1A9FC-56C9-4997-B56B-DA186EE2D62A}";
                renderNodeName = "Skinned Mesh Render Node";
                renderNodeUuid = AZ::AzTypeInfo<SkinnedMeshComponentRenderNode>::Uuid();
                meshAssetUuId = AZ::AzTypeInfo<CharacterDefinitionAsset>::Uuid();
                renderOptionUuid = SkinnedMeshComponentRenderNode::GetRenderOptionsUuid();
                meshTypeString = "Skinned Mesh";
            }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Convert.  this will destroy the old mesh component and change the uuid to the new type
            classElement.Convert(context, newComponentStringGuid.c_str());
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // add data back in as appropriate
            classElement.AddElement(editorBaseClass);

            //create a static mesh render node
            int renderNodeIndex = classElement.AddElement(context, renderNodeName.c_str(), renderNodeUuid);
            auto& newrenderNode = classElement.GetSubElement(renderNodeIndex);
            AZ::Data::Asset<AZ::Data::AssetData> assetData(meshAssetId, meshAssetUuId);
            newrenderNode.AddElementWithData(context, meshTypeString.c_str(), assetData);
            newrenderNode.AddElement(materialOverride);
            newrenderNode.AddElementWithData(context, "Visible", visible);

            // render options
            int renderOptionsIndex = newrenderNode.AddElement(context, "Render Options", renderOptionUuid);
            auto& newRenderOptions = newrenderNode.GetSubElement(renderOptionsIndex);
            newRenderOptions.AddElementWithData(context, "Opacity", opacity);
            newRenderOptions.AddElementWithData(context, "MaxViewDistance", maxViewDistance);
            newRenderOptions.AddElementWithData(context, "ViewDistanceMultiplier", viewDIstanceMultuplier);
            newRenderOptions.AddElementWithData(context, "LODRatio", lodRatio);
            newRenderOptions.AddElementWithData(context, "CastDynamicShadows", castDynamicShadows);
            newRenderOptions.AddElementWithData(context, "CastLightmapShadows", castLightmapShadows);
            newRenderOptions.AddElementWithData(context, "IndoorOnly", indoorOnly);
            newRenderOptions.AddElementWithData(context, "Bloom", bloom);
            newRenderOptions.AddElementWithData(context, "MotionBlur", motionBlur);
            newRenderOptions.AddElementWithData(context, "RainOccluder", rainOccluder);
            newRenderOptions.AddElementWithData(context, "AffectDynamicWater", affectDynamicWater);
            newRenderOptions.AddElementWithData(context, "ReceiveWind", receiveWind);
            newRenderOptions.AddElementWithData(context, "AcceptDecals", acceptDecals);
            newRenderOptions.AddElementWithData(context, "VisibilityOccluder", visibilityOccluder);
            newRenderOptions.AddElementWithData(context, "DepthTest", depthTest);
            //////////////////////////////////////////////////////////////////////////
            return true;
        }
    } // namespace ClassConverters
} // namespace LmbrCentral
