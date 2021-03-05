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
#include "SkinnedMeshComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Asset/AssetManager.h>

#include <MathConversion.h>

#include <CryFile.h>
#include <I3DEngine.h>
#include "MeshComponent.h"

namespace LmbrCentral
{

    //////////////////////////////////////////////////////////////////////////
    /// Deprecated MeshComponent

    namespace ClassConverters
    {
        static bool DeprecateMeshComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    } // namespace ClassConverters

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////

    void SkinnedMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        SkinnedMeshComponentRenderNode::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            // Need to deprecate the old MeshComponent whenever we see one.
            serializeContext->ClassDeprecate("MeshComponent", "{9697D425-3D28-4414-93DD-1890E576AB4B}", &ClassConverters::DeprecateMeshComponent);

            serializeContext->Class<SkinnedMeshComponent, AZ::Component>()
                ->Version(1)
                ->Field("Skinned Mesh Render Node", &SkinnedMeshComponent::m_skinnedMeshRenderNode);
        }
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SkinnedMeshComponent>()->RequestBus("MeshComponentRequestBus");
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void SkinnedMeshComponentRenderNode::SkinnedRenderOptions::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<SkinnedMeshComponentRenderNode::SkinnedRenderOptions>()
                ->Version(4, &VersionConverter)
                ->Field("Opacity", &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_opacity)
                ->Field("MaxViewDistance", &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_maxViewDist)
                ->Field("ViewDistanceMultiplier", &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_viewDistMultiplier)
                ->Field("LODRatio", &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_lodRatio)
                ->Field("CastDynamicShadows", &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_castShadows)
                ->Field("UseVisAreas", &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_useVisAreas)
                ->Field("RainOccluder", &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_rainOccluder)
                ->Field("AcceptDecals", &SkinnedMeshComponentRenderNode::SkinnedRenderOptions::m_acceptDecals)
                ;
        }
    }

    bool SkinnedMeshComponentRenderNode::SkinnedRenderOptions::VersionConverter([[maybe_unused]] AZ::SerializeContext& context,
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

        // conversion from version 3:
        // - Remove CastLightmapShadows (m_castLightmap)
        // - Remove AffectDynamicWater (m_affectDynamicWater)
        // - Remove ReceiveWind (m_receiveWind)
        // - Remove AffectNavmesh (m_affectNavmesh)
        // - Remove VisibilityOccluder (m_visibilityOccluder)
        if (classElement.GetVersion() <= 3)
        {
            classElement.RemoveElementByName(AZ_CRC("CastLightmapShadows", 0x10ce0bf8));
            classElement.RemoveElementByName(AZ_CRC("AffectDynamicWater", 0xe6774a5b));
            classElement.RemoveElementByName(AZ_CRC("ReceiveWind", 0x952a1261));
            classElement.RemoveElementByName(AZ_CRC("AffectNavmesh", 0x77bd2697));
            classElement.RemoveElementByName(AZ_CRC("VisibilityOccluder", 0xe5819c29));
        }

        return true;
    }

    void SkinnedMeshComponentRenderNode::Reflect(AZ::ReflectContext* context)
    {
        SkinnedRenderOptions::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<SkinnedMeshComponentRenderNode>()
                ->Version(1)
                ->Field("Visible", &SkinnedMeshComponentRenderNode::m_visible)
                ->Field("Skinned Mesh", &SkinnedMeshComponentRenderNode::m_characterDefinitionAsset)
                ->Field("Material Override", &SkinnedMeshComponentRenderNode::m_material)
                ->Field("Render Options", &SkinnedMeshComponentRenderNode::m_renderOptions)
                ;
        }
    }

    float SkinnedMeshComponentRenderNode::GetDefaultMaxViewDist()
    {
        if (gEnv && gEnv->p3DEngine)
        {
            return gEnv->p3DEngine->GetMaxViewDistance(false);
        }

        // In the editor and the game, the dynamic lookup above should *always* hit.
        // This case essentially means no renderer (not even the null renderer) is present.
        return FLT_MAX;
    }

    AZ::u32 SkinnedMeshComponentRenderNode::GetJointCount() const
    {
        return 0;
    }

    const char* SkinnedMeshComponentRenderNode::GetJointNameByIndex([[maybe_unused]] AZ::u32 jointIndex) const
    {
        return nullptr;
    }

    AZ::u32 SkinnedMeshComponentRenderNode::GetJointIndexByName([[maybe_unused]] const char* jointName) const
    {
        return 0;
    }

    AZ::Transform SkinnedMeshComponentRenderNode::GetJointTransformCharacterRelative([[maybe_unused]] AZ::u32 jointIndex) const
    {
        return AZ::Transform::CreateIdentity();
    }

    SkinnedMeshComponentRenderNode::SkinnedRenderOptions::SkinnedRenderOptions()
        : m_opacity(1.f)
        , m_viewDistMultiplier(1.f)
        , m_lodRatio(100)
        , m_useVisAreas(true)
        , m_castShadows(true)
        , m_rainOccluder(true)
        , m_acceptDecals(true)
    {
        m_maxViewDist = GetDefaultMaxViewDist();
    }

    SkinnedMeshComponentRenderNode::SkinnedMeshComponentRenderNode()
        : m_materialOverride(nullptr)
        , m_auxiliaryRenderFlags(0)
        , m_auxiliaryRenderFlagsHistory(0)
        , m_lodDistance(0.f)
        , m_isRegisteredWithRenderer(false)
        , m_objectMoved(false)
        , m_characterDefinitionAsset(AZ::Data::AssetLoadBehavior::QueueLoad)
        , m_visible(true)
        , m_isQueuedForDestroyMesh(false)
    {
        m_localBoundingBox.Reset();
        m_worldBoundingBox.Reset();
        m_worldTransform = AZ::Transform::CreateIdentity();
        m_renderTransform = Matrix34::CreateIdentity();
    }

    SkinnedMeshComponentRenderNode::~SkinnedMeshComponentRenderNode()
    {
        DestroyMesh();
    }

    void SkinnedMeshComponentRenderNode::CopyPropertiesTo(SkinnedMeshComponentRenderNode& rhs) const
    {
        rhs.m_visible = m_visible;
        rhs.m_materialOverride = m_materialOverride;
        rhs.m_characterDefinitionAsset = m_characterDefinitionAsset;
        rhs.m_material = m_material;
        rhs.m_renderOptions = m_renderOptions;
    }

    void SkinnedMeshComponentRenderNode::AttachToEntity(AZ::EntityId id)
    {
        if (AZ::TransformNotificationBus::Handler::BusIsConnectedId(m_attachedToEntityId))
        {
            AZ::TransformNotificationBus::Handler::BusDisconnect(m_attachedToEntityId);
        }

        if (id.IsValid())
        {
            if (!AZ::TransformNotificationBus::Handler::BusIsConnectedId(id))
            {
                AZ::TransformNotificationBus::Handler::BusConnect(id);
            }

            AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(entityTransform, id, AZ::TransformBus, GetWorldTM);
            UpdateWorldTransform(entityTransform);
        }

        m_attachedToEntityId = id;
    }

    void SkinnedMeshComponentRenderNode::OnAssetPropertyChanged()
    {
        if (HasMesh())
        {
            DestroyMesh();
        }

        if (AZ::Data::AssetBus::Handler::BusIsConnected())
        {
            AZ::Data::AssetBus::Handler::BusDisconnect();
        }

        CreateMesh();
    }

    void SkinnedMeshComponentRenderNode::RefreshRenderState()
    {
        if (gEnv->IsEditor())
        {
            UpdateLocalBoundingBox();

            AZ::Transform parentTransform = AZ::Transform::CreateIdentity();
            EBUS_EVENT_ID_RESULT(parentTransform, m_attachedToEntityId, AZ::TransformBus, GetWorldTM);
            OnTransformChanged(AZ::Transform::CreateIdentity(), parentTransform);

            m_renderOptions.OnChanged();

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

    void SkinnedMeshComponentRenderNode::SetAuxiliaryRenderFlags(uint32 flags)
    {
        m_auxiliaryRenderFlags = flags;
        m_auxiliaryRenderFlagsHistory |= flags;
    }

    void SkinnedMeshComponentRenderNode::UpdateAuxiliaryRenderFlags(bool on, uint32 mask)
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

    void SkinnedMeshComponentRenderNode::CreateMesh()
    {
        //keep from hooking up component data when the component could be
        //not activated on initialization due to component incompatibility
        if (!m_attachedToEntityId.IsValid())
        {
            return;
        }

        if(m_characterDefinitionAsset.GetId().IsValid())
        {
            if (!AZ::Data::AssetBus::Handler::BusIsConnected())
            {
                AZ::Data::AssetBus::Handler::BusConnect(m_characterDefinitionAsset.GetId());
            }

            if (m_characterDefinitionAsset.IsReady())
            {
                OnAssetReady(m_characterDefinitionAsset);
            }
            else
            {
                m_characterDefinitionAsset.QueueLoad();
            }
        }
    }

    void SkinnedMeshComponentRenderNode::DestroyMesh()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();

        RegisterWithRenderer(false);

        m_characterDefinitionAsset.Release();
        m_isQueuedForDestroyMesh = false;
    }

    bool SkinnedMeshComponentRenderNode::HasMesh() const
    {
        return false;
    }

    void SkinnedMeshComponentRenderNode::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        m_characterDefinitionAsset.Create(id);
        OnAssetPropertyChanged();
    }


    void SkinnedMeshComponentRenderNode::GetMemoryUsage(class ICrySizer* pSizer) const
    {
        pSizer->AddObjectSize(this);
    }

    void SkinnedMeshComponentRenderNode::OnTransformChanged(const AZ::Transform&, const AZ::Transform& parentWorld)
    {
        // The entity to which we're attached has moved.
        UpdateWorldTransform(parentWorld);
    }

    void SkinnedMeshComponentRenderNode::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // For skinned meshes, check the actual pointer since we create a new instance of the asset for every request
        // to work around limitations on the Cry asset side. 
        // That behavior is driven by SkinnedMeshAsset::IsRegisterReadonlyAndShareable()==false.
        if (asset.Get() == m_characterDefinitionAsset.Get())
        {
            // Here is the soonest we can intercept an asset-load for cancelling (AssetLoadJob does not currently support cancelling).
            // We release the asset and return before altering any ebus connections or calls related to asset-loading completion because,
            // in reality, we've already cancelled the loading operation. Functions further below invoke loading logic to finalize
            // asset loading state - we want to avoid this; bail early instead.
            if (m_isQueuedForDestroyMesh)
            {
                DestroyMesh();
                return;
            }
        }
    }

    void SkinnedMeshComponentRenderNode::UpdateWorldTransform(const AZ::Transform& entityTransform)
    {
        m_worldTransform = entityTransform;

        m_renderTransform = AZTransformToLYTransform(m_worldTransform);

        UpdateWorldBoundingBox();

        m_objectMoved = true;
    }

    void SkinnedMeshComponentRenderNode::UpdateLocalBoundingBox()
    {
        m_localBoundingBox.Reset();
        UpdateWorldBoundingBox();
    }

    void SkinnedMeshComponentRenderNode::SetVisible(bool isVisible)
    {
        if (m_visible != isVisible)
        {
            m_visible = isVisible;
            RegisterWithRenderer(false);
            RegisterWithRenderer(true);
        }
    }

    bool SkinnedMeshComponentRenderNode::GetVisible()
    {
        return m_visible;
    }

    void SkinnedMeshComponentRenderNode::UpdateWorldBoundingBox()
    {
        m_worldBoundingBox.SetTransformedAABB(m_renderTransform, m_localBoundingBox);

        if (m_isRegisteredWithRenderer)
        {
            // Re-register with the renderer to update culling info
            gEnv->p3DEngine->RegisterEntity(this);
        }
    }

    void SkinnedMeshComponentRenderNode::RegisterWithRenderer(bool registerWithRenderer)
    {
        if (gEnv && gEnv->p3DEngine)
        {
            if (registerWithRenderer)
            {
                if (!m_isRegisteredWithRenderer)
                {
                    ApplyRenderOptions();

                    gEnv->p3DEngine->RegisterEntity(this);

                    m_isRegisteredWithRenderer = true;
                }
            }
            else
            {
                if (m_isRegisteredWithRenderer)
                {
                    gEnv->p3DEngine->FreeRenderNodeState(this);
                    m_isRegisteredWithRenderer = false;
                }
            }
        }
    }

    namespace SkinnedMeshInternal
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

    void SkinnedMeshComponentRenderNode::ApplyRenderOptions()
    {
        using SkinnedMeshInternal::UpdateRenderFlag;
        unsigned int flags = GetRndFlags();
        flags |= ERF_COMPONENT_ENTITY;

        // Turn off any flag which has ever been set via auxiliary render flags.
        UpdateRenderFlag(false, m_auxiliaryRenderFlagsHistory, flags);

        // Update flags according to current render settings
        UpdateRenderFlag(m_renderOptions.m_useVisAreas == false, ERF_OUTDOORONLY, flags);
        UpdateRenderFlag(m_renderOptions.m_castShadows, ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS, flags);
        UpdateRenderFlag(m_renderOptions.m_rainOccluder, ERF_RAIN_OCCLUDER, flags);
        UpdateRenderFlag(true, ERF_EXCLUDE_FROM_TRIANGULATION, flags);
        UpdateRenderFlag(m_visible == false, ERF_HIDDEN, flags);
        UpdateRenderFlag(false == m_renderOptions.m_acceptDecals, ERF_NO_DECALNODE_DECALS, flags);

        // Apply current auxiliary render flags
        UpdateRenderFlag(true, m_auxiliaryRenderFlags, flags);

        m_fWSMaxViewDist = m_renderOptions.m_maxViewDist;

        SetViewDistanceMultiplier(m_renderOptions.m_viewDistMultiplier);

        SetLodRatio(static_cast<int>(m_renderOptions.m_lodRatio));

        SetRndFlags(flags);
    }

    CLodValue SkinnedMeshComponentRenderNode::ComputeLOD(int wantedLod, [[maybe_unused]] const SRenderingPassInfo& passInfo)
    {
        return CLodValue(wantedLod);
    }

    AZ::Aabb SkinnedMeshComponentRenderNode::CalculateWorldAABB() const
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        if (!m_worldBoundingBox.IsReset())
        {
            aabb.AddPoint(LYVec3ToAZVec3(m_worldBoundingBox.min));
            aabb.AddPoint(LYVec3ToAZVec3(m_worldBoundingBox.max));
        }
        return aabb;
    }

    AZ::Aabb SkinnedMeshComponentRenderNode::CalculateLocalAABB() const
    {
        AZ::Aabb aabb = AZ::Aabb::CreateNull();
        if (!m_localBoundingBox.IsReset())
        {
            aabb.AddPoint(LYVec3ToAZVec3(m_localBoundingBox.min));
            aabb.AddPoint(LYVec3ToAZVec3(m_localBoundingBox.max));
        }
        return aabb;
    }

    /*IRenderNode*/ void SkinnedMeshComponentRenderNode::Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo)
    {
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
        if (rParams.pMatrix->IsValid())
        {
            rParams.lodValue = ComputeLOD(inRenderParams.lodValue.LodA(), passInfo);
        }

        rParams.pMaterial = previousMaterial;
        rParams.dwFObjFlags = previousObjectFlags;
    }

    /*IRenderNode*/ bool SkinnedMeshComponentRenderNode::GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const
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

    void SkinnedMeshComponentRenderNode::UpdateLodDistance([[maybe_unused]] const SFrameLodInfo& frameLodInfo)
    {
        SMeshLodInfo lodInfo;
        m_lodDistance = sqrt(lodInfo.fGeometricMean);
    }

    /*IRenderNode*/ EERType SkinnedMeshComponentRenderNode::GetRenderNodeType()
    {
        return eERType_SkinnedMeshRenderComponent;
    }


    /*IRenderNode*/ const char* SkinnedMeshComponentRenderNode::GetName() const
    {
        return "SkinnedMeshComponentRenderNode";
    }

    /*IRenderNode*/ const char* SkinnedMeshComponentRenderNode::GetEntityClassName() const
    {
        return "SkinnedMeshComponentRenderNode";
    }


    /*IRenderNode*/ Vec3 SkinnedMeshComponentRenderNode::GetPos([[maybe_unused]] bool bWorldOnly /*= true*/) const
    {
        return m_renderTransform.GetTranslation();
    }

    /*IRenderNode*/ const AABB SkinnedMeshComponentRenderNode::GetBBox() const
    {
        return m_worldBoundingBox;
    }

    /*IRenderNode*/ void SkinnedMeshComponentRenderNode::SetBBox(const AABB& WSBBox)
    {
        m_worldBoundingBox = WSBBox;
    }

    /*IRenderNode*/ void SkinnedMeshComponentRenderNode::OffsetPosition(const Vec3& delta)
    {
        // Recalculate local transform
        AZ::Transform localTransform = AZ::Transform::CreateIdentity();
        EBUS_EVENT_ID_RESULT(localTransform, m_attachedToEntityId, AZ::TransformBus, GetLocalTM);

        localTransform.SetTranslation(localTransform.GetTranslation() + LYVec3ToAZVec3(delta));
        EBUS_EVENT_ID(m_attachedToEntityId, AZ::TransformBus, SetLocalTM, localTransform);

        m_objectMoved = true;
    }

    /*IRenderNode*/ void SkinnedMeshComponentRenderNode::SetMaterial(_smart_ptr<IMaterial> pMat)
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

    /*IRenderNode*/ _smart_ptr<IMaterial> SkinnedMeshComponentRenderNode::GetMaterial([[maybe_unused]] Vec3* pHitPos /*= nullptr*/)
    {
        if (m_materialOverride)
        {
            return m_materialOverride;
        }

        return nullptr;
    }

    /*IRenderNode*/ _smart_ptr<IMaterial> SkinnedMeshComponentRenderNode::GetMaterialOverride()
    {
        return m_materialOverride;
    }

    /*IRenderNode*/ float SkinnedMeshComponentRenderNode::GetMaxViewDist()
    {
        return(m_renderOptions.m_maxViewDist * 0.75f * GetViewDistanceMultiplier());
    }

    /*IRenderNode*/ IStatObj* SkinnedMeshComponentRenderNode::GetEntityStatObj([[maybe_unused]] unsigned int nPartId, [[maybe_unused]] unsigned int nSubPartId, [[maybe_unused]] Matrix34A* pMatrix, [[maybe_unused]] bool bReturnOnlyVisible)
    {
        return nullptr;
    }

    /*IRenderNode*/ _smart_ptr<IMaterial> SkinnedMeshComponentRenderNode::GetEntitySlotMaterial(unsigned int nPartId, [[maybe_unused]] bool bReturnOnlyVisible, [[maybe_unused]] bool* pbDrawNear)
    {
        if (0 == nPartId)
        {
            return m_materialOverride;
        }

        return nullptr;
    }

    //////////////////////////////////////////////////////////////////////////
    // SkinnedMeshComponent
    const float SkinnedMeshComponent::s_renderNodeRequestBusOrder = 100.f;

    void SkinnedMeshComponent::Activate()
    {
        m_skinnedMeshRenderNode.AttachToEntity(m_entity->GetId());

        // Note we are purposely connecting to buses before calling m_mesh.CreateMesh().
        // m_mesh.CreateMesh() can result in events (eg: OnMeshCreated) that we want receive.
        MaterialOwnerRequestBus::Handler::BusConnect(m_entity->GetId());
        MeshComponentRequestBus::Handler::BusConnect(m_entity->GetId());
        RenderNodeRequestBus::Handler::BusConnect(m_entity->GetId());
        SkeletalHierarchyRequestBus::Handler::BusConnect(GetEntityId());
        AzFramework::BoundsRequestBus::Handler::BusConnect(GetEntityId());

        m_skinnedMeshRenderNode.CreateMesh();
    }

    void SkinnedMeshComponent::Deactivate()
    {
        AzFramework::BoundsRequestBus::Handler::BusDisconnect();
        SkeletalHierarchyRequestBus::Handler::BusDisconnect();
        MaterialOwnerRequestBus::Handler::BusDisconnect();
        MeshComponentRequestBus::Handler::BusDisconnect();
        RenderNodeRequestBus::Handler::BusDisconnect();

        m_skinnedMeshRenderNode.DestroyMesh();
        m_skinnedMeshRenderNode.AttachToEntity(AZ::EntityId());
    }

    AZ::Aabb SkinnedMeshComponent::GetWorldBounds()
    {
        return m_skinnedMeshRenderNode.CalculateWorldAABB();
    }

    AZ::Aabb SkinnedMeshComponent::GetLocalBounds()
    {
        return m_skinnedMeshRenderNode.CalculateLocalAABB();
    }

    void SkinnedMeshComponent::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        m_skinnedMeshRenderNode.SetMeshAsset(id);
    }

    void SkinnedMeshComponent::SetMaterial(_smart_ptr<IMaterial> material)
    {
        m_skinnedMeshRenderNode.SetMaterial(material);
    }

    _smart_ptr<IMaterial> SkinnedMeshComponent::GetMaterial()
    {
        return m_skinnedMeshRenderNode.GetMaterial();
    }

    IRenderNode* SkinnedMeshComponent::GetRenderNode()
    {
        return &m_skinnedMeshRenderNode;
    }

    float SkinnedMeshComponent::GetRenderNodeRequestBusOrder() const
    {
        return s_renderNodeRequestBusOrder;
    }

    bool SkinnedMeshComponent::GetVisibility()
    {
        return m_skinnedMeshRenderNode.GetVisible();
    }

    void SkinnedMeshComponent::SetVisibility(bool isVisible)
    {
        m_skinnedMeshRenderNode.SetVisible(isVisible);
    }

    AZ::u32 SkinnedMeshComponent::GetJointCount()
    {
        return m_skinnedMeshRenderNode.GetJointCount();
    }

    const char* SkinnedMeshComponent::GetJointNameByIndex(AZ::u32 jointIndex)
    {
        return m_skinnedMeshRenderNode.GetJointNameByIndex(jointIndex);
    }

    AZ::s32 SkinnedMeshComponent::GetJointIndexByName(const char* jointName)
    {
        return m_skinnedMeshRenderNode.GetJointIndexByName(jointName);
    }

    AZ::Transform SkinnedMeshComponent::GetJointTransformCharacterRelative(AZ::u32 jointIndex)
    {
        return m_skinnedMeshRenderNode.GetJointTransformCharacterRelative(jointIndex);
    }

    //////////////////////////////////////////////////////////////////////////
    /// Deprecated MeshComponent

    namespace ClassConverters
    {
        /// Convert MeshComponentRenderNode::RenderOptions to latest version.
        void MeshComponentRenderNodeRenderOptionsVersion1To2Converter(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement)
        {
            // conversion from version 1:
            // - Need to convert Hidden to Visible
            // - Need to convert OutdoorOnly to IndoorOnly
            if (classElement.GetVersion() <= 1)
            {
                int hiddenIndex = classElement.FindElement(AZ_CRC("Hidden", 0x885de9bd));
                int visibleIndex = classElement.FindElement(AZ_CRC("Visible", 0x7ab0e859));

                // There was a brief time where hidden became visible but the version was not patched and this will handle that case
                // This should also be a reminder to always up the version at the time of renaming or removing parameters
                if (visibleIndex <= -1)
                {
                    if (hiddenIndex > -1)
                    {
                        //Invert hidden and rename to visible
                        AZ::SerializeContext::DataElementNode& hidden = classElement.GetSubElement(hiddenIndex);
                        bool hiddenData;

                        hidden.GetData<bool>(hiddenData);
                        hidden.SetData<bool>(context, !hiddenData);

                        hidden.SetName("Visible");
                    }
                }

                int outdoorOnlyIndex = classElement.FindElement(AZ_CRC("OutdoorOnly", 0x87f67f36));
                if (outdoorOnlyIndex > -1)
                {
                    //Invert outdoor only and rename to indoor only
                    AZ::SerializeContext::DataElementNode& outdoorOnly = classElement.GetSubElement(outdoorOnlyIndex);
                    bool outdoorOnlyData;

                    outdoorOnly.GetData<bool>(outdoorOnlyData);
                    outdoorOnly.SetData<bool>(context, !outdoorOnlyData);

                    outdoorOnly.SetName("IndoorOnly");
                }
            }
        }
        /// Convert MeshComponentRenderNode::RenderOptions to latest version.
        void MeshComponentRenderNodeRenderOptionsVersion2To3Converter([[maybe_unused]] AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement)
        {
            // conversion from version 2:
            // - Need to remove Visible
            if (classElement.GetVersion() <= 2)
            {
                classElement.RemoveElementByName(AZ_CRC("Visible", 0x7ab0e859));
            }
        }

        static bool DeprecateMeshComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            //////////////////////////////////////////////////////////////////////////
            // Pull data out of the old version
            auto renderNode = classElement.GetSubElement(classElement.FindElement(AZ_CRC("Mesh", 0xe16f3a56)));

            auto materialOverride = renderNode.GetSubElement(renderNode.FindElement(AZ_CRC("Material Override", 0xebc12e43)));

            auto renderOptions = renderNode.GetSubElement(renderNode.FindElement(AZ_CRC("Render Options", 0xb5bc5e06)));
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
                newComponentStringGuid = "{2F4BAD46-C857-4DCB-A454-C412DE67852A}";
                renderNodeName = "Static Mesh Render Node";
                renderNodeUuid = AZ::AzTypeInfo<MeshComponentRenderNode>::Uuid();
                meshAssetUuId = AZ::AzTypeInfo<MeshAsset>::Uuid();
                renderOptionUuid = MeshComponentRenderNode::GetRenderOptionsUuid();
                meshTypeString = "Static Mesh";
            }
            else
            {
                newComponentStringGuid = "{C99EB110-CA74-4D95-83F0-2FCDD1FF418B}";
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
            //create a static mesh render node
            int renderNodeIndex = classElement.AddElement(context, renderNodeName.c_str(), renderNodeUuid);
            auto& newrenderNode = classElement.GetSubElement(renderNodeIndex);
            AZ::Data::Asset<AZ::Data::AssetData> assetData;
            if (!path.empty())
            {
                assetData = AZ::Data::AssetManager::Instance().GetAsset(meshAssetId, meshAssetUuId, AZ::Data::AssetLoadBehavior::Default);
            }
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
