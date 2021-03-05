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
#include "MeshComponent.h"
#include <LmbrCentral/Rendering/Utils/MaterialOwnerRequestBusHandlerImpl.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LmbrCentral/Rendering/GiRegistrationBus.h>
#include <MathConversion.h>

#include <I3DEngine.h>

#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Render/GeometryIntersectionBus.h>
#include <AzFramework/Visibility/EntityBoundsUnionBus.h>

namespace LmbrCentral
{
    //////////////////////////////////////////////////////////////////////////

    //! Handler/binding code that is required for Behavior Context reflection of EBus Notifications.
    class MaterialOwnerNotificationBusBehaviorHandler : public MaterialOwnerNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(MaterialOwnerNotificationBusBehaviorHandler, "{77705C0E-5ADE-496C-85FF-9278565E278E}", AZ::SystemAllocator
            , OnMaterialOwnerReady);

        void OnMaterialOwnerReady() override
        {
            Call(FN_OnMaterialOwnerReady);
        }
    };

    //////////////////////////////////////////////////////////////////////////

    AZ::BehaviorParameterOverrides CreateMaterialIdDetails(AZ::BehaviorContext* behaviorContext)
    {
        return{ "MaterialID", "The ID of a Material slot to access, if the Owner has multiple Materials. IDs start at 1.", behaviorContext->MakeDefaultValue(1) };
    }

    AZStd::array<AZ::BehaviorParameterOverrides, 2> GetMaterialParamArgs(AZ::BehaviorContext* behaviorContext)
    {
        AZ::BehaviorParameterOverrides getParamNameDetails = { "ParamName", "The name of the Material param to return" };
        return{ { getParamNameDetails, CreateMaterialIdDetails(behaviorContext) } };
    }

    void MeshComponent::Reflect(AZ::ReflectContext* context)
    {
        MeshComponentRenderNode::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<MeshComponent, AZ::Component>()
                ->Version(1)
                ->Field("Static Mesh Render Node", &MeshComponent::m_meshRenderNode);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MeshComponentRequestBus>("MeshComponentRequestBus")
                ->Event("SetVisibility", &MeshComponentRequestBus::Events::SetVisibility)
                ->Event("GetVisibility", &MeshComponentRequestBus::Events::GetVisibility)
                ->VirtualProperty("Visibility", "GetVisibility", "SetVisibility");

            const char* setMaterialParamTooltip = "Sets a Material param value for the given Entity. The Material will be cloned once before any changes are applied, so other instances are not affected.";
            const char* getMaterialParamTooltip = "Returns a Material param value for the given Entity";
            AZ::BehaviorParameterOverrides setParamNameDetails = { "ParamName", "The name of the Material param to set" };
            const char* newValueTooltip = "The new value to apply";

            behaviorContext->EBus<MaterialOwnerRequestBus>("MaterialOwnerRequestBus", nullptr, "Includes functions for Components that have a Material such as Mesh Component, Decal Component, etc.")
                    ->Attribute(AZ::Script::Attributes::Category, "Rendering")
                ->Event("IsMaterialOwnerReady", &MaterialOwnerRequestBus::Events::IsMaterialOwnerReady)
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Indicates whether the Material Owner is fully initialized, and is ready for Material requests")
                ->Event("SetMaterial", &MaterialOwnerRequestBus::Events::SetMaterialHandle)
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Sets an Entity's Material")
                ->Event("GetMaterial", &MaterialOwnerRequestBus::Events::GetMaterialHandle)
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Returns an Entity's current Material")
                ->Event("SetParamVector4", &MaterialOwnerRequestBus::Events::SetMaterialParamVector4, { { setParamNameDetails, { "Vector4", newValueTooltip }, CreateMaterialIdDetails(behaviorContext) } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialParamTooltip)
                ->Event("SetParamVector3", &MaterialOwnerRequestBus::Events::SetMaterialParamVector3, { { setParamNameDetails, { "Vector3", newValueTooltip }, CreateMaterialIdDetails(behaviorContext) } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialParamTooltip)
                ->Event("SetParamColor", &MaterialOwnerRequestBus::Events::SetMaterialParamColor,     { { setParamNameDetails, { "Color"  , newValueTooltip }, CreateMaterialIdDetails(behaviorContext) } })
                    ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialParamTooltip)
                ->Event("SetParamNumber", &MaterialOwnerRequestBus::Events::SetMaterialParamFloat,    { { setParamNameDetails, { "Number" , newValueTooltip }, CreateMaterialIdDetails(behaviorContext) } }) // Using ParamNumber instead of ParamFloat because in Script Canvas all primitives are just "numbers"
                    ->Attribute(AZ::Script::Attributes::ToolTip, setMaterialParamTooltip)
                ->Event("GetParamVector4", &MaterialOwnerRequestBus::Events::GetMaterialParamVector4, GetMaterialParamArgs(behaviorContext))
                    ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialParamTooltip)
                ->Event("GetParamVector3", &MaterialOwnerRequestBus::Events::GetMaterialParamVector3, GetMaterialParamArgs(behaviorContext))
                    ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialParamTooltip)
                ->Event("GetParamColor", &MaterialOwnerRequestBus::Events::GetMaterialParamColor, GetMaterialParamArgs(behaviorContext))
                    ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialParamTooltip)
                ->Event("GetParamNumber", &MaterialOwnerRequestBus::Events::GetMaterialParamFloat, GetMaterialParamArgs(behaviorContext)) // Using ParamNumber instead of ParamFloat because in Script Canvas all primitives are just "numbers"
                    ->Attribute(AZ::Script::Attributes::ToolTip, getMaterialParamTooltip);

            behaviorContext->EBus<MaterialOwnerNotificationBus>("MaterialOwnerNotificationBus", nullptr, "Provides notifications from Components that have a Material such as Mesh Component, Decal Component, etc.")
                ->Attribute(AZ::Script::Attributes::Category, "Rendering")
                ->Handler<MaterialOwnerNotificationBusBehaviorHandler>()
                ;

            behaviorContext->Class<MeshComponent>()->RequestBus("MeshComponentRequestBus");
        }
    }


    //////////////////////////////////////////////////////////////////////////

    void MeshComponentRenderNode::MeshRenderOptions::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<MeshComponentRenderNode::MeshRenderOptions>()
                ->Version(5, &VersionConverter)
                ->Field("Opacity", &MeshComponentRenderNode::MeshRenderOptions::m_opacity)
                ->Field("MaxViewDistance", &MeshComponentRenderNode::MeshRenderOptions::m_maxViewDist)
                ->Field("ViewDistanceMultiplier", &MeshComponentRenderNode::MeshRenderOptions::m_viewDistMultiplier)
                ->Field("LODRatio", &MeshComponentRenderNode::MeshRenderOptions::m_lodRatio)
                ->Field("CastShadows", &MeshComponentRenderNode::MeshRenderOptions::m_castShadows)
                ->Field("LODBBoxBased", &MeshComponentRenderNode::MeshRenderOptions::m_lodBoundingBoxBased)
                ->Field("UseVisAreas", &MeshComponentRenderNode::MeshRenderOptions::m_useVisAreas)
                ->Field("RainOccluder", &MeshComponentRenderNode::MeshRenderOptions::m_rainOccluder)
                ->Field("AffectDynamicWater", &MeshComponentRenderNode::MeshRenderOptions::m_affectDynamicWater)
                ->Field("ReceiveWind", &MeshComponentRenderNode::MeshRenderOptions::m_receiveWind)
                ->Field("AcceptDecals", &MeshComponentRenderNode::MeshRenderOptions::m_acceptDecals)
                ->Field("AffectNavmesh", &MeshComponentRenderNode::MeshRenderOptions::m_affectNavmesh)
                ->Field("VisibilityOccluder", &MeshComponentRenderNode::MeshRenderOptions::m_visibilityOccluder)
                ->Field("DynamicMesh", &MeshComponentRenderNode::MeshRenderOptions::m_dynamicMesh)
                ->Field("AffectsGI", &MeshComponentRenderNode::MeshRenderOptions::m_affectGI)
                ;
        }
    }

    bool MeshComponentRenderNode::MeshRenderOptions::VersionConverter(AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement)
    {
        // conversion from version 1:
        // - Remove Bloom (m_allowBloom)
        // - Remove MotionBlur (m_allowMotionBlur)
        // - Remove DepthTest (m_depthTest)
        if (classElement.GetVersion() <= 1)
        {
            classElement.RemoveElementByName(AZ_CRC("Bloom", 0xc6cd7d1b));
            classElement.RemoveElementByName(AZ_CRC("MotionBlur", 0x917cdb53));
            classElement.RemoveElementByName(AZ_CRC("DepthTest", 0x532f68b9));
        }

        // conversion from version 2:
        // - Remove IndoorOnly (m_indoorOnly)
        if (classElement.GetVersion() <= 2)
        {
            classElement.RemoveElementByName(AZ_CRC("IndoorOnly", 0xc8ab6ddb));
        }

        if (classElement.GetVersion() <= 3)
        {
            classElement.RemoveElementByName(AZ_CRC("CastLightmapShadows", 0x10ce0bf8));
            int index = classElement.FindElement(AZ_CRC("CastDynamicShadows", 0x55c75b43));
            AZ::SerializeContext::DataElementNode& shadowNode = classElement.GetSubElement(index);
            shadowNode.SetName("CastShadows");
        }

        // conversion from version 4:
        // - Set "CastShadows" to false if "Opacity" is less than 1.0f, in order to not break old assets.
        //   The new system ignores opacity for shadow casting and relies only on the "CastShadows" flag.
        if (classElement.GetVersion() <= 4)
        {
            float opacity;
            int opacityElementIndex = classElement.FindElement(AZ_CRC("Opacity", 0x43fd6d66));
            AZ::SerializeContext::DataElementNode& opacityNode = classElement.GetSubElement(opacityElementIndex);
            opacityNode.GetData(opacity);

            if (opacity < 1.0f)
            {
                int castShadowsElementIndex = classElement.FindElement(AZ_CRC("CastShadows", 0xbe687463));
                AZ::SerializeContext::DataElementNode& castShadowsNode = classElement.GetSubElement(castShadowsElementIndex);
                castShadowsNode.SetData(context, false);
            }
        }
       
        return true;
    }

    bool MeshComponentRenderNode::MeshRenderOptions::IsStatic() const
    {
        return (m_hasStaticTransform && !m_dynamicMesh && !m_receiveWind);
    }

    bool MeshComponentRenderNode::MeshRenderOptions::AffectsGi() const
    {
        return m_affectGI && IsStatic();
    }

    AZ::Crc32 MeshComponentRenderNode::MeshRenderOptions::StaticPropertyVisibility() const
    {
        return IsStatic() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void MeshComponentRenderNode::Reflect(AZ::ReflectContext* context)
    {
        MeshRenderOptions::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<MeshComponentRenderNode>()
                ->Version(1)
                ->Field("Visible", &MeshComponentRenderNode::m_visible)
                ->Field("Static Mesh", &MeshComponentRenderNode::m_meshAsset)
                ->Field("Material Override", &MeshComponentRenderNode::m_material)
                ->Field("Render Options", &MeshComponentRenderNode::m_renderOptions)
                ;
        }
    }

    float MeshComponentRenderNode::GetDefaultMaxViewDist()
    {
        if (gEnv && gEnv->p3DEngine)
        {
            return gEnv->p3DEngine->GetMaxViewDistance(false);
        }

        // In the editor and the game, the dynamic lookup above should *always* hit.
        // This case essentially means no renderer (not even the null renderer) is present.
        return FLT_MAX;
    }

    MeshComponentRenderNode::MeshRenderOptions::MeshRenderOptions()
        : m_opacity(1.f)
        , m_viewDistMultiplier(1.f)
        , m_lodRatio(100)
        , m_useVisAreas(true)
        , m_castShadows(true)
        , m_lodBoundingBoxBased(false)
        , m_rainOccluder(true)
        , m_affectNavmesh(true)
        , m_affectDynamicWater(false)
        , m_acceptDecals(true)
        , m_receiveWind(false)
        , m_visibilityOccluder(false)
        , m_dynamicMesh(false)
        , m_hasStaticTransform(false)
        , m_affectGI(true)
    {
        m_maxViewDist = GetDefaultMaxViewDist();
    }

    MeshComponentRenderNode::MeshComponentRenderNode()
        : m_statObj(nullptr)
        , m_materialOverride(nullptr)
        , m_auxiliaryRenderFlags(0)
        , m_auxiliaryRenderFlagsHistory(0)
        , m_lodDistance(0.f)
        , m_lodDistanceScaled(FLT_MAX / (SMeshLodInfo::s_nMaxLodCount + 1))  // defualt overflow prevention - it is scaled by (SMeshLodInfo::s_nMaxLodCount + 1)
        , m_lodDistanceScaleValue(1.0f)  
        , m_isRegisteredWithRenderer(false)
        , m_objectMoved(false)
        , m_meshAsset(AZ::Data::AssetLoadBehavior::QueueLoad)
        , m_visible(true)
    {
        m_localBoundingBox.Reset();
        m_worldBoundingBox.Reset();
        m_worldTransform = AZ::Transform::CreateIdentity();
        m_renderTransform = Matrix34::CreateIdentity();
    }

    MeshComponentRenderNode::~MeshComponentRenderNode()
    {
        DestroyMesh();
    }

    void MeshComponentRenderNode::CopyPropertiesTo(MeshComponentRenderNode& rhs) const
    {
        rhs.m_visible = m_visible;
        rhs.m_materialOverride = m_materialOverride;
        rhs.m_meshAsset = m_meshAsset;
        rhs.m_material = m_material;
        rhs.m_renderOptions = m_renderOptions;
    }

    void MeshComponentRenderNode::AttachToEntity(AZ::EntityId id)
    {
        if (AZ::TransformNotificationBus::Handler::BusIsConnectedId(m_renderOptions.m_attachedToEntityId))
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect(m_renderOptions.m_attachedToEntityId);
        }

        if (m_modificationHelper.IsConnected())
        {
            m_modificationHelper.Disconnect();
        }

        if (id.IsValid())
        {
            if (!AZ::TransformNotificationBus::Handler::BusIsConnectedId(id))
            {
                AZ::TransformNotificationBus::Handler::BusConnect(id);
            }

            auto transformHandler = AZ::TransformBus::FindFirstHandler(id);

            UpdateWorldTransform(transformHandler->GetWorldTM());

            AzFramework::EntityBoundsUnionRequestBus::Broadcast(
                &AzFramework::EntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, GetEntityId());

            m_modificationHelper.Connect(id);
        }

        m_renderOptions.m_attachedToEntityId = id;
    }

    void MeshComponentRenderNode::OnAssetPropertyChanged()
    {
        if (HasMesh())
        {
            DestroyMesh();
        }

        AZ::Data::AssetBus::Handler::BusDisconnect();

        CreateMesh();
        AzFramework::RenderGeometry::IntersectionNotificationBus::Event(m_contextId,
            &AzFramework::RenderGeometry::IntersectionNotifications::OnGeometryChanged, GetEntityId());
    }

    void MeshComponentRenderNode::RefreshRenderState()
    {
        if (gEnv->IsEditor())
        {
            UpdateLocalBoundingBox();

            AZ::Transform parentTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(parentTransform, m_renderOptions.m_attachedToEntityId, AZ::TransformBus, GetWorldTM);
            OnTransformChanged(AZ::Transform::CreateIdentity(), parentTransform);

            if (HasMesh())
            {
                // Re-register with the renderer, as some render settings/flags require it.
                // Note that this is editor-only behavior (hence the guard above).
                if (m_isRegisteredWithRenderer)
                {
                    RegisterWithRenderer(false);
                    RegisterWithRenderer(true);
                }
            }
        }
    }

    void MeshComponentRenderNode::SetTransformStaticState(bool isStatic)
    {
        m_renderOptions.m_hasStaticTransform = isStatic;
    }

    const AZ::Transform& MeshComponentRenderNode::GetTransform() const
    {
        return m_worldTransform;
    }

    void MeshComponentRenderNode::SetAuxiliaryRenderFlags(uint32 flags)
    {
        m_auxiliaryRenderFlags = flags;
        m_auxiliaryRenderFlagsHistory |= flags;
    }

    void MeshComponentRenderNode::UpdateAuxiliaryRenderFlags(bool on, uint32 mask)
    {
        if (on)
        {
            m_auxiliaryRenderFlags |= mask;
        }
        else
        {
            m_auxiliaryRenderFlags &= ~mask;
        }

        m_auxiliaryRenderFlagsHistory |= mask;
    }

    bool MeshComponentRenderNode::IsReady() const
    {
        return HasMesh();
    }

    void MeshComponentRenderNode::CreateMesh()
    {
        if (m_meshAsset.GetId().IsValid())
        {
            if (!AZ::Data::AssetBus::Handler::BusIsConnected())
            {
                AZ::Data::AssetBus::Handler::BusConnect(m_meshAsset.GetId());
            }

            m_meshAsset.QueueLoad();
        }
    }

    void MeshComponentRenderNode::DestroyMesh()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();

        RegisterWithRenderer(false);
        m_statObj = nullptr;

        EBUS_EVENT_ID(m_renderOptions.m_attachedToEntityId, MeshComponentNotificationBus, OnMeshDestroyed);

        m_meshAsset.Release();
    }

    bool MeshComponentRenderNode::HasMesh() const
    {
        return m_statObj != nullptr;
    }

    void MeshComponentRenderNode::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        AZ::Data::Asset<MeshAsset> asset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<MeshAsset>(id, m_meshAsset.GetAutoLoadBehavior());

        if (asset)
        {
            m_meshAsset = asset;
            OnAssetPropertyChanged();
        }
    }

    void MeshComponentRenderNode::GetMemoryUsage(class ICrySizer* pSizer) const
    {
        pSizer->AddObjectSize(this);
    }

    float MeshComponentRenderNode::GetUniformScale()
    {
        AZ::Vector3 scales = m_worldTransform.GetScale();
        AZ_Assert((scales.GetX() == scales.GetY()) && (scales.GetY() == scales.GetZ()), "Scales are not uniform");
        return scales.GetX();
    }

    float MeshComponentRenderNode::GetColumnScale(int column)
    {
        return m_worldTransform.GetScale().GetElement(column);
    }

    void MeshComponentRenderNode::OnTransformChanged(const AZ::Transform&, const AZ::Transform& parentWorld)
    {
        // The entity to which we're attached has moved.
        UpdateWorldTransform(parentWorld);
        AzFramework::RenderGeometry::IntersectionNotificationBus::Event(m_contextId,
            &AzFramework::RenderGeometry::IntersectionNotifications::OnGeometryChanged, GetEntityId());
    }

    void MeshComponentRenderNode::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_meshAsset)
        {
            m_meshAsset = asset;
            BuildRenderMesh();

            if (HasMesh())
            {
                const AZStd::string& materialOverridePath = m_material.GetAssetPath();
                if (!materialOverridePath.empty())
                {
                    m_materialOverride = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial(materialOverridePath.c_str());

                    AZ_Warning("MeshComponent", m_materialOverride != gEnv->p3DEngine->GetMaterialManager()->GetDefaultMaterial(),
                        "Failed to load override Material \"%s\".",
                        materialOverridePath.c_str());
                }
                else
                {
                    m_materialOverride = nullptr;
                }

                UpdateLocalBoundingBox();
                UpdateLodDistance(gEnv->p3DEngine->GetFrameLodInfo());
                RegisterWithRenderer(true);

                // Inform listeners that the mesh has been changed
                LmbrCentral::MeshComponentNotificationBus::Event(m_renderOptions.m_attachedToEntityId, &LmbrCentral::MeshComponentNotifications::OnMeshCreated, asset);
                AzFramework::RenderGeometry::IntersectionNotificationBus::Event(m_contextId, &AzFramework::RenderGeometry::IntersectionNotifications::OnGeometryChanged, GetEntityId());
            }
        }
    }

    void MeshComponentRenderNode::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // note that this also corrects the assetId if it is incorrect - do not remove the following line
        // even if you call OnAssetReady
        OnAssetReady(asset);
    }

    void MeshComponentRenderNode::UpdateWorldTransform(const AZ::Transform& entityTransform)
    {
        m_worldTransform = entityTransform;

        m_renderTransform = AZTransformToLYTransform(m_worldTransform);

        UpdateWorldBoundingBox();
        if (m_isRegisteredWithRenderer && m_renderOptions.AffectsGi())
        {
            GiRegistrationBus::Broadcast(&GiRegistration::UpsertToGi,
                m_renderOptions.m_attachedToEntityId,
                m_worldTransform,
                CalculateWorldAABB(),
                m_meshAsset,
                GetMaterial());
        }

        m_objectMoved = true;
    }

    void MeshComponentRenderNode::UpdateLocalBoundingBox()
    {
        m_localBoundingBox.Reset();

        if (HasMesh())
        {
            m_localBoundingBox.Add(m_statObj->GetAABB());
        }

        AzFramework::EntityBoundsUnionRequestBus::Broadcast(
            &AzFramework::EntityBoundsUnionRequestBus::Events::RefreshEntityLocalBoundsUnion, GetEntityId());

        UpdateWorldBoundingBox();
    }

    void MeshComponentRenderNode::UpdateWorldBoundingBox()
    {
        m_worldBoundingBox.SetTransformedAABB(m_renderTransform, m_localBoundingBox);

        if (m_isRegisteredWithRenderer)
        {
            // Re-register with the renderer to update culling info
            gEnv->p3DEngine->RegisterEntity(this);
        }
    }

    void MeshComponentRenderNode::SetVisible(bool isVisible)
    {
        if (m_visible != isVisible)
        {
            m_visible = isVisible;
            RegisterWithRenderer(false);
            RegisterWithRenderer(true);
        }
    }

    bool MeshComponentRenderNode::GetVisible()
    {
        return m_visible;
    }

    void MeshComponentRenderNode::RegisterWithRenderer(bool registerWithRenderer)
    {
        if (gEnv && gEnv->p3DEngine)
        {
            if (registerWithRenderer)
            {
                if (!m_isRegisteredWithRenderer)
                {
                    ApplyRenderOptions();

                    gEnv->p3DEngine->RegisterEntity(this);

                    if (m_renderOptions.AffectsGi())
                    {
                        GiRegistrationBus::Broadcast(&GiRegistration::UpsertToGi,
                            m_renderOptions.m_attachedToEntityId,
                            m_worldTransform,
                            CalculateWorldAABB(),
                            m_meshAsset,
                            GetMaterial());
                    }

                    m_isRegisteredWithRenderer = true;
                }
            }
            else
            {
                if (m_isRegisteredWithRenderer)
                {
                    gEnv->p3DEngine->FreeRenderNodeState(this);

                    GiRegistrationBus::Broadcast(&GiRegistration::RemoveFromGi,
                        m_renderOptions.m_attachedToEntityId);

                    m_isRegisteredWithRenderer = false;
                }
            }
        }
    }

    namespace MeshInternal
    {
        void UpdateRenderFlag(bool enable, int mask, unsigned int& flags)
        {
            if (enable)
            {
                flags |= mask;
            }
            else
            {
                flags &= ~mask;
            }
        }
    }

    void MeshComponentRenderNode::ApplyRenderOptions()
    {
        using MeshInternal::UpdateRenderFlag;
        unsigned int flags = GetRndFlags();
        flags |= ERF_COMPONENT_ENTITY;

        // Turn off any flag which has ever been set via auxiliary render flags
        UpdateRenderFlag(false, m_auxiliaryRenderFlagsHistory, flags);

        // Update flags according to current render settings
        UpdateRenderFlag(m_renderOptions.m_useVisAreas == false, ERF_OUTDOORONLY, flags);
        UpdateRenderFlag(m_renderOptions.m_castShadows, ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS, flags);
        UpdateRenderFlag(m_renderOptions.m_rainOccluder && m_renderOptions.IsStatic(), ERF_RAIN_OCCLUDER, flags);
        UpdateRenderFlag(m_visible == false, ERF_HIDDEN, flags);
        UpdateRenderFlag(m_renderOptions.m_receiveWind, ERF_RECVWIND, flags);
        UpdateRenderFlag(m_renderOptions.m_visibilityOccluder  && m_renderOptions.IsStatic(), ERF_GOOD_OCCLUDER, flags);
        //Dynamic meshes shouldn't affect the navmeshes. If that decision is changed we should change this line to no longer require
        //static and note that the flag is tied to the negation of the navemesh boolean. 
        //Also see the editormeshcomponent.cpp AffectNavemesh function. 
        UpdateRenderFlag(!(m_renderOptions.m_affectNavmesh && m_renderOptions.IsStatic()), ERF_EXCLUDE_FROM_TRIANGULATION, flags);
        UpdateRenderFlag(false == m_renderOptions.m_affectDynamicWater && m_renderOptions.IsStatic(), ERF_NODYNWATER, flags);
        UpdateRenderFlag(false == m_renderOptions.m_acceptDecals, ERF_NO_DECALNODE_DECALS, flags);

        UpdateRenderFlag(m_renderOptions.m_lodBoundingBoxBased, ERF_LOD_BBOX_BASED, flags);

        // Apply current auxiliary render flags
        UpdateRenderFlag(true, m_auxiliaryRenderFlags, flags);

        m_fWSMaxViewDist = m_renderOptions.m_maxViewDist;

        SetViewDistanceMultiplier(m_renderOptions.m_viewDistMultiplier);

        SetLodRatio(static_cast<int>(m_renderOptions.m_lodRatio));

        SetRndFlags(flags);
    }

    CLodValue MeshComponentRenderNode::ComputeLOD( int wantedLod, const SRenderingPassInfo& passInfo)
    {
        // Default values as per the CVar - default fade going between 2 and 8 meters with dissolve enabled
        float   dissolveDistMin = 2.0f;
        float   dissolveDistMax = 8.0f;
        int     dissolveEnabled = 1;

        if (gEnv && gEnv->pConsole)
        {
            static ICVar* dissolveDistMinCvar = gEnv->pConsole->GetCVar("e_DissolveDistMin");
            static ICVar* dissolveDistMaxCvar = gEnv->pConsole->GetCVar("e_DissolveDistMax");
            static ICVar* dissolveEnabledCvar = gEnv->pConsole->GetCVar("e_Dissolve");

            dissolveDistMin = dissolveDistMinCvar->GetFVal();
            dissolveDistMax = dissolveDistMaxCvar->GetFVal();
            dissolveEnabled = dissolveEnabledCvar->GetIVal() ;
        }

        const Vec3  cameraPos = passInfo.GetCamera().GetPosition();
        const float entityDistance = sqrt_tpl(Distance::Point_AABBSq(cameraPos, GetBBox())) * passInfo.GetZoomFactor();

        wantedLod = CLAMP(wantedLod, m_statObj->GetMinUsableLod(), SMeshLodInfo::s_nMaxLodCount);
        int         currentLod = m_statObj->FindNearesLoadedLOD(wantedLod, true);

        if (dissolveEnabled && passInfo.IsGeneralPass())
        {
            float   invDissolveDist = 1.0f / CLAMP(0.1f * m_fWSMaxViewDist, dissolveDistMin, dissolveDistMax );
            int     nextLod = m_statObj->FindNearesLoadedLOD(currentLod + 1, true);
            
            // If the user chose to base LOD switch on bounding boxes, then we do not use the geometric mean computed at init.
            if (GetRndFlags() & ERF_LOD_BBOX_BASED)
            {
                const float lodRatio = GetLodRatioNormalized();
                if (lodRatio > 0.0f)
                {
                    // We do not use a geometric mean  per object but a global value for all objects.
                    static ICVar* lodBoundingBoxDistanceMultiplier = gEnv->pConsole->GetCVar("e_LodBoundingBoxDistanceMultiplier");

                    m_lodDistanceScaled = lodBoundingBoxDistanceMultiplier->GetFVal() * m_lodDistanceScaleValue;
                }
            }
            else
            {
                m_lodDistanceScaled = m_lodDistance * m_lodDistanceScaleValue;
            }

            float   lodDistance = m_lodDistanceScaled * (currentLod + 1);
            uint8   dissolveRatio255 = (uint8)SATURATEB((1.0f + (entityDistance - lodDistance) * invDissolveDist) * 255.f);

            if (dissolveRatio255 == 255)
            {
                return CLodValue(nextLod, 0, -1);
            }
            return CLodValue(currentLod, dissolveRatio255, nextLod);
        }

        return CLodValue(currentLod);
    }

    AZ::Aabb MeshComponentRenderNode::CalculateWorldAABB() const
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        if (!m_worldBoundingBox.IsReset())
        {
            aabb.AddPoint(LYVec3ToAZVec3(m_worldBoundingBox.min));
            aabb.AddPoint(LYVec3ToAZVec3(m_worldBoundingBox.max));
        }
        return aabb;
    }

    AZ::Aabb MeshComponentRenderNode::CalculateLocalAABB() const
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        if (!m_localBoundingBox.IsReset())
        {
            aabb.AddPoint(LYVec3ToAZVec3(m_localBoundingBox.min));
            aabb.AddPoint(LYVec3ToAZVec3(m_localBoundingBox.max));
        }
        return aabb;
    }

    /*IRenderNode*/ void MeshComponentRenderNode::Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo)
    {
        if (!HasMesh())
        {
            return;
        }

        if (!m_modificationHelper.GetMeshModified())
        {
            IStatObj* obj = GetEntityStatObj();
            int subObjectCount = obj->GetSubObjectCount();

            AZStd::function<IStatObj*(size_t)> getSubObject;
            if (subObjectCount == 0)
            {
                getSubObject = [obj](size_t index)
                {
                    if (index > 0)
                    {
                        AZ_Warning("MeshComponentRenderNode", false, "Mesh indices out of range");
                        return static_cast<IStatObj*>(nullptr);
                    }
                    return obj;
                };
            }
            else
            {
                getSubObject = [obj, subObjectCount](size_t index)
                {
                    if (index >= subObjectCount)
                    {
                        AZ_Warning("MeshComponentRenderNode", false, "Mesh indices out of range");
                        return static_cast<IStatObj*>(nullptr);
                    }
                    return obj->GetSubObject(index)->pStatObj;
                };
            }

            for (const LmbrCentral::MeshModificationRequestHelper::MeshLODPrimIndex& meshIndices : m_modificationHelper.MeshesToEdit())
            {
                if (meshIndices.lodIndex != 0)
                {
                    continue;
                }

                IStatObj* subObject = getSubObject(meshIndices.primitiveIndex);
                if (!subObject)
                {
                    continue;
                }

                MeshModificationNotificationBus::Event(
                    GetEntityId(),
                    &MeshModificationNotificationBus::Events::ModifyMesh,
                    meshIndices.lodIndex,
                    meshIndices.primitiveIndex,
                    subObject->GetRenderMesh());
            }

            m_modificationHelper.SetMeshModified(true);
        }

        SRendParams rParams(inRenderParams);

        // Assign a unique pInstance pointer, otherwise effects involving SRenderObjData will not work for this object.  CEntityObject::Render does this for legacy entities.
        rParams.pInstance = this;

        rParams.fAlpha = m_renderOptions.m_opacity;

        _smart_ptr<IMaterial> previousMaterial = rParams.pMaterial;
        const int previousObjectFlags = rParams.dwFObjFlags;

        if (m_materialOverride)
        {
            rParams.pMaterial = m_materialOverride;
        }

        if (m_objectMoved)
        {
            rParams.dwFObjFlags |= FOB_DYNAMIC_OBJECT;
            m_objectMoved = false;
        }

        rParams.pMatrix = &m_renderTransform;
        rParams.bForceDrawStatic = !m_renderOptions.m_dynamicMesh;
        if (rParams.pMatrix->IsValid())
        {
            rParams.lodValue = ComputeLOD(inRenderParams.lodValue.LodA(), passInfo);
            m_statObj->Render(rParams, passInfo);
        }

        rParams.pMaterial = previousMaterial;
        rParams.dwFObjFlags = previousObjectFlags;
    }

    /*IRenderNode*/ bool MeshComponentRenderNode::GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const
    {
        const float lodRatio = GetLodRatioNormalized();
        if (lodRatio > 0.0f)
        {
            const float distMultiplier = 1.f / (lodRatio * frameLodInfo.fTargetSize);

            for (int lodIndex = 0; lodIndex < SMeshLodInfo::s_nMaxLodCount; ++lodIndex)
            {
                distances[lodIndex] = m_lodDistance * (lodIndex + 1) * distMultiplier;
            }
        }
        else
        {
            for (int lodIndex = 0; lodIndex < SMeshLodInfo::s_nMaxLodCount; ++lodIndex)
            {
                distances[lodIndex] = FLT_MAX;
            }
        }
        return true;
    }

    void MeshComponentRenderNode::UpdateLodDistance(const SFrameLodInfo& frameLodInfo)
    {
        SMeshLodInfo lodInfo;

        if (HasMesh())
        {
            m_statObj->ComputeGeometricMean(lodInfo);
        }

        m_lodDistance = sqrt(lodInfo.fGeometricMean);

        // The following computation need to stay in accordance with the 'GetLodDistances' formula.
        const float lodRatio = GetLodRatioNormalized();
        if (lodRatio > 0.0f)
        {
            m_lodDistanceScaled = m_lodDistance / (lodRatio * frameLodInfo.fTargetSize);
            m_lodDistanceScaleValue = 1.0f / (lodRatio * frameLodInfo.fTargetSize);
        }
    }

    /*IRenderNode*/ EERType MeshComponentRenderNode::GetRenderNodeType()
    {
        return m_renderOptions.IsStatic() ? eERType_StaticMeshRenderComponent : eERType_DynamicMeshRenderComponent;
    }

    /*IRenderNode*/ bool MeshComponentRenderNode::CanExecuteRenderAsJob()
    {
        return !m_renderOptions.m_dynamicMesh 
            && !m_renderOptions.m_receiveWind 
            && m_modificationHelper.MeshesToEdit().empty();
    }

    /*IRenderNode*/ const char* MeshComponentRenderNode::GetName() const
    {
        return "MeshComponentRenderNode";
    }

    /*IRenderNode*/ const char* MeshComponentRenderNode::GetEntityClassName() const
    {
        return "MeshComponentRenderNode";
    }

    /*IRenderNode*/ Vec3 MeshComponentRenderNode::GetPos([[maybe_unused]] bool bWorldOnly /*= true*/) const
    {
        return m_renderTransform.GetTranslation();
    }

    /*IRenderNode*/ const AABB MeshComponentRenderNode::GetBBox() const
    {
        return m_worldBoundingBox;
    }

    /*IRenderNode*/ void MeshComponentRenderNode::SetBBox(const AABB& WSBBox)
    {
        m_worldBoundingBox = WSBBox;
    }

    /*IRenderNode*/ void MeshComponentRenderNode::OffsetPosition(const Vec3& delta)
    {
        // Recalculate local transform
        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(localTransform, m_renderOptions.m_attachedToEntityId, AZ::TransformBus, GetLocalTM);

        localTransform.SetTranslation(localTransform.GetTranslation() + LYVec3ToAZVec3(delta));
        EBUS_EVENT_ID(m_renderOptions.m_attachedToEntityId, AZ::TransformBus, SetLocalTM, localTransform);

        m_objectMoved = true;
    }

    /*IRenderNode*/ void MeshComponentRenderNode::SetMaterial(_smart_ptr<IMaterial> pMat)
    {
        m_materialOverride = pMat;

        if (pMat)
        {
            m_material.SetAssetPath(pMat->GetName());
        }
        else
        {
            // If no material is provided, we intend to reset to the original material so we treat
            // it as an asset reset to recreate the mesh.
            m_material.SetAssetPath("");
            OnAssetPropertyChanged();
        }
    }

    /*IRenderNode*/ _smart_ptr<IMaterial> MeshComponentRenderNode::GetMaterial([[maybe_unused]] Vec3* pHitPos /*= nullptr*/)
    {
        if (m_materialOverride)
        {
            return m_materialOverride;
        }

        if (HasMesh())
        {
            return m_statObj->GetMaterial();
        }

        return nullptr;
    }

    /*IRenderNode*/ _smart_ptr<IMaterial> MeshComponentRenderNode::GetMaterialOverride()
    {
        return m_materialOverride;
    }

    /*IRenderNode*/ float MeshComponentRenderNode::GetMaxViewDist()
    {
        return(m_renderOptions.m_maxViewDist * 0.75f * GetViewDistanceMultiplier());
    }

    /*IRenderNode*/ IStatObj* MeshComponentRenderNode::GetEntityStatObj(unsigned int nPartId, [[maybe_unused]] unsigned int nSubPartId, Matrix34A* pMatrix, [[maybe_unused]] bool bReturnOnlyVisible)
    {
        if (0 == nPartId)
        {
            if (pMatrix)
            {
                *pMatrix = m_renderTransform;
            }

            return m_statObj;
        }

        return nullptr;
    }

    /*IRenderNode*/ _smart_ptr<IMaterial> MeshComponentRenderNode::GetEntitySlotMaterial(unsigned int nPartId, [[maybe_unused]] bool bReturnOnlyVisible, [[maybe_unused]] bool* pbDrawNear)
    {
        if (0 == nPartId)
        {
            return m_materialOverride;
        }

        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    // MeshComponent
    const float MeshComponent::s_renderNodeRequestBusOrder = 100.f;

    MeshComponent::MeshComponent()
    {
        m_materialBusHandler = aznew MaterialOwnerRequestBusHandlerImpl();
    }

    MeshComponent::~MeshComponent()
    {
        delete m_materialBusHandler;
    }

    void MeshComponent::Activate()
    {
        m_meshRenderNode.AttachToEntity(m_entity->GetId());
        m_materialBusHandler->Activate(&m_meshRenderNode, m_entity->GetId());
        bool isStatic = false;
        AZ::TransformBus::EventResult(isStatic, m_entity->GetId(), &AZ::TransformBus::Events::IsStaticTransform);
        m_meshRenderNode.SetTransformStaticState(isStatic);
        // Note we are purposely connecting to buses before calling m_mesh.CreateMesh().
        // m_mesh.CreateMesh() can result in events (eg: OnMeshCreated) that we want receive.
        MaterialOwnerRequestBus::Handler::BusConnect(m_entity->GetId());
        MeshComponentRequestBus::Handler::BusConnect(m_entity->GetId());
        AzFramework::BoundsRequestBus::Handler::BusConnect(m_entity->GetId());
        RenderNodeRequestBus::Handler::BusConnect(m_entity->GetId());
        AzFramework::EntityContextId contextId;
        AzFramework::EntityIdContextQueryBus::EventResult(contextId, GetEntityId(), &AzFramework::EntityIdContextQueries::GetOwningContextId);
        AzFramework::RenderGeometry::IntersectionRequestBus::Handler::BusConnect({ GetEntityId(), contextId });
        m_meshRenderNode.SetContextId(contextId);
        m_meshRenderNode.CreateMesh();
        LegacyMeshComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void MeshComponent::Deactivate()
    {
        AzFramework::RenderGeometry::IntersectionRequestBus::Handler::BusDisconnect();

        MeshComponentRequestBus::Handler::BusDisconnect();
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        MaterialOwnerRequestBus::Handler::BusDisconnect();
        LegacyMeshComponentRequestBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();

        m_meshRenderNode.DestroyMesh();
        m_meshRenderNode.AttachToEntity(AZ::EntityId());
        m_materialBusHandler->Deactivate();
    }

    AZ::Aabb MeshComponent::GetWorldBounds()
    {
        return m_meshRenderNode.CalculateWorldAABB();
    }

    AZ::Aabb MeshComponent::GetLocalBounds()
    {
        return m_meshRenderNode.CalculateLocalAABB();
    }

    void MeshComponent::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        m_meshRenderNode.SetMeshAsset(id);
    }

    bool MeshComponent::IsMaterialOwnerReady()
    {
        return m_materialBusHandler->IsMaterialOwnerReady();
    }

    void MeshComponent::SetMaterial(_smart_ptr<IMaterial> material)
    {
        m_materialBusHandler->SetMaterial(material);
    }

    _smart_ptr<IMaterial> MeshComponent::GetMaterial()
    {
        return m_materialBusHandler->GetMaterial();
    }

    void MeshComponent::SetMaterialHandle(const MaterialHandle& materialHandle)
    {
        m_materialBusHandler->SetMaterialHandle(materialHandle);
    }

    MaterialHandle MeshComponent::GetMaterialHandle()
    {
        return m_materialBusHandler->GetMaterialHandle();
    }

    void MeshComponent::SetMaterialParamVector4(const AZStd::string& name, const AZ::Vector4& value, int materialId)
    {
        m_materialBusHandler->SetMaterialParamVector4(name, value, materialId);
    }

    void MeshComponent::SetMaterialParamVector3(const AZStd::string& name, const AZ::Vector3& value, int materialId)
    {
        m_materialBusHandler->SetMaterialParamVector3(name, value, materialId);
    }

    void MeshComponent::SetMaterialParamColor(const AZStd::string& name, const AZ::Color& value, int materialId)
    {
        m_materialBusHandler->SetMaterialParamColor(name, value, materialId);
    }

    void MeshComponent::SetMaterialParamFloat(const AZStd::string& name, float value, int materialId)
    {
        m_materialBusHandler->SetMaterialParamFloat(name, value, materialId);
    }

    AZ::Vector4 MeshComponent::GetMaterialParamVector4(const AZStd::string& name, int materialId)
    {
        return m_materialBusHandler->GetMaterialParamVector4(name, materialId);
    }

    AZ::Vector3 MeshComponent::GetMaterialParamVector3(const AZStd::string& name, int materialId)
    {
        return m_materialBusHandler->GetMaterialParamVector3(name, materialId);
    }

    AZ::Color MeshComponent::GetMaterialParamColor(const AZStd::string& name, int materialId)
    {
        return m_materialBusHandler->GetMaterialParamColor(name, materialId);
    }

    float MeshComponent::GetMaterialParamFloat(const AZStd::string& name, int materialId)
    {
        return m_materialBusHandler->GetMaterialParamFloat(name, materialId);
    }

    IRenderNode* MeshComponent::GetRenderNode()
    {
        return &m_meshRenderNode;
    }

    float MeshComponent::GetRenderNodeRequestBusOrder() const
    {
        return s_renderNodeRequestBusOrder;
    }

    IStatObj* MeshComponent::GetStatObj()
    {
        return m_meshRenderNode.GetEntityStatObj();
    }

    AzFramework::RenderGeometry::RayResult MeshComponent::RenderGeometryIntersect(const AzFramework::RenderGeometry::RayRequest& ray)
    {
        AzFramework::RenderGeometry::RayResult result;
        if (!GetVisibility() && ray.m_onlyVisible)
        {
            return result;
        }

        if (IStatObj* geometry = GetStatObj())
        {
            const AZ::Vector3 rayDirection = (ray.m_endWorldPosition - ray.m_startWorldPosition);
            const AZ::Transform& transform = m_meshRenderNode.GetTransform();
            const AZ::Transform inverseTransform = transform.GetInverse();

            const AZ::Vector3 rayStartLocal = inverseTransform.TransformPoint(ray.m_startWorldPosition);
            const AZ::Vector3 rayDistNormLocal = inverseTransform.TransformVector(rayDirection).GetNormalized();

            SRayHitInfo hi;
            hi.inReferencePoint = AZVec3ToLYVec3(rayStartLocal);
            hi.inRay = Ray(hi.inReferencePoint, AZVec3ToLYVec3(rayDistNormLocal));
            hi.bInFirstHit = true;
            hi.bGetVertColorAndTC = true;
            if (geometry->RayIntersection(hi))
            {
                AZ::Matrix3x4 invTransformMatrix = AZ::Matrix3x4::CreateFromTransform(inverseTransform);
                invTransformMatrix.Transpose();

                result.m_uv = LYVec2ToAZVec2(hi.vHitTC);
                result.m_worldPosition = transform.TransformPoint(LYVec3ToAZVec3(hi.vHitPos));
                result.m_worldNormal = invTransformMatrix.Multiply3x3(LYVec3ToAZVec3(hi.vHitNormal)).GetNormalized();
                result.m_distance = (result.m_worldPosition - ray.m_startWorldPosition).GetLength();
                result.m_entityAndComponent = { GetEntityId(), GetId() };
            }
        }
        return result;
    }

    bool MeshComponent::GetVisibility()
    {
        return m_meshRenderNode.GetVisible();
    }

    void MeshComponent::SetVisibility(bool isVisible)
    {
        m_meshRenderNode.SetVisible(isVisible);
    }

    void MeshComponentRenderNode::BuildRenderMesh()
    {
        m_statObj = nullptr; // Release smart pointer

        MeshAsset* data = m_meshAsset.Get();
        if (!data || !data->m_statObj)
        {
            return;
        }

        // Populate m_statObj. If the mesh doesn't require to be unique, we reuse the render mesh from the asset. If the
        // mesh requires to be unique, we create a copy of the asset's render mesh since it will be modified.

        bool hasClothData = !data->m_statObj->GetClothData().empty();
        const int subObjectCount = data->m_statObj->GetSubObjectCount();
        for (int i = 0; i < subObjectCount && !hasClothData; ++i)
        {
            IStatObj::SSubObject* subObject = data->m_statObj->GetSubObject(i);
            if (subObject &&
                subObject->pStatObj &&
                !subObject->pStatObj->GetClothData().empty())
            {
                hasClothData = true;
            }
        }

        bool useUniqueMesh = hasClothData;

        if (useUniqueMesh)
        {
            // Create a copy since each mesh can be deforming differently and we need to send different meshes to render
            m_statObj = data->m_statObj->Clone( /*bCloneGeometry*/ true, /*bCloneChildren*/ true, /*bMeshesOnly*/ false);
        }
        else
        {
            // Reuse the same render mesh
            m_statObj = data->m_statObj;
        }
    }
} // namespace LmbrCentral
