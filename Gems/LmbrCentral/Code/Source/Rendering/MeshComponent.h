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
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TransformBus.h>

#include <AzFramework/Asset/AssetCatalogBus.h>

#include <IEntityRenderState.h>

#include <LmbrCentral/Rendering/MaterialOwnerBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/MeshModificationBus.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/RenderBoundsBus.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Render/GeometryIntersectionBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Visibility/BoundsBus.h>

namespace LmbrCentral
{
    class MaterialOwnerRequestBusHandlerImpl;

    /*!
    * RenderNode implementation responsible for integrating with the renderer.
    * The node owns render flags, the mesh instance, and the render transform.
    */
    class MeshComponentRenderNode
        : public IRenderNode
        , public AZ::TransformNotificationBus::Handler
        , public AZ::Data::AssetBus::Handler
    {
        friend class EditorMeshComponent;
    public:
        using MaterialPtr = _smart_ptr < IMaterial > ;
        using MeshPtr = _smart_ptr < IStatObj > ;

        AZ_TYPE_INFO(MeshComponentRenderNode, "{46FF2BC4-BEF9-4CC4-9456-36C127C310D7}");

        MeshComponentRenderNode();
        ~MeshComponentRenderNode() override;

        void CopyPropertiesTo(MeshComponentRenderNode& rhs) const;

        //! Notifies render node which entity owns it, for subscribing to transform
        //! bus, etc.
        void AttachToEntity(AZ::EntityId id);

        //! Returns true after all required assets are loaded
        bool IsReady() const override;

        //! Instantiate mesh instance.
        void CreateMesh();

        //! Destroy mesh instance.
        void DestroyMesh();

        //! Returns true if the node has geometry assigned.
        bool HasMesh() const;

        //! Assign a new mesh asset
        void SetMeshAsset(const AZ::Data::AssetId& id);

        //! Get the mesh asset
        AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() { return m_meshAsset; }

        //! Invoked in the editor when the user assigns a new asset.
        void OnAssetPropertyChanged();

        //! Render the mesh
        void RenderMesh(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo);

        //! Updates the render node's world transform based on the entity's.
        void UpdateWorldTransform(const AZ::Transform& entityTransform);

        //! Computes world-space AABB.
        AZ::Aabb CalculateWorldAABB() const;

        //! Computes local-space AABB.
        AZ::Aabb CalculateLocalAABB() const;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus::Handler interface implementation
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // IRenderNode interface implementation
        void Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo) override;
        bool GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const override;
        float GetFirstLodDistance() const override { return m_lodDistance; }
        EERType GetRenderNodeType() override;
        bool CanExecuteRenderAsJob() override;
        const char* GetName() const override;
        const char* GetEntityClassName() const override;
        Vec3 GetPos(bool bWorldOnly = true) const override;
        const AABB GetBBox() const override;
        void SetBBox(const AABB& WSBBox) override;
        void OffsetPosition(const Vec3& delta) override;
        void SetMaterial(_smart_ptr<IMaterial> pMat) override;
        _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = nullptr) override;
        _smart_ptr<IMaterial> GetMaterialOverride() override;
        IStatObj* GetEntityStatObj(unsigned int nPartId = 0, unsigned int nSubPartId = 0, Matrix34A* pMatrix = nullptr, bool bReturnOnlyVisible = false) override;
        _smart_ptr<IMaterial> GetEntitySlotMaterial(unsigned int nPartId, bool bReturnOnlyVisible = false, bool* pbDrawNear = nullptr) override;
        float GetMaxViewDist() override;
        void GetMemoryUsage(class ICrySizer* pSizer) const override;
        AZ::EntityId GetEntityId() override { return m_renderOptions.m_attachedToEntityId; }
        float GetUniformScale() override;
        float GetColumnScale(int column) override;
        //////////////////////////////////////////////////////////////////////////

        //! Invoked in the editor when a property requiring render state refresh
        //! has changed.
        void RefreshRenderState();

        //! Set/get auxiliary render flags.
        void SetAuxiliaryRenderFlags(uint32 flags);
        uint32 GetAuxiliaryRenderFlags() const { return m_auxiliaryRenderFlags; }
        void UpdateAuxiliaryRenderFlags(bool on, uint32 mask);

        void SetVisible(bool isVisible);
        bool GetVisible();

        static void Reflect(AZ::ReflectContext* context);

        static float GetDefaultMaxViewDist();
        static AZ::Uuid GetRenderOptionsUuid() { return AZ::AzTypeInfo<MeshRenderOptions>::Uuid(); }

        //! Registers or unregisters our render node with the render.
        void RegisterWithRenderer(bool registerWithRenderer);
        bool IsRegisteredWithRenderer() const { return m_isRegisteredWithRenderer; }

        //! This function caches off the static flag stat of the transform;
        void SetTransformStaticState(bool isStatic);
        const AZ::Transform& GetTransform() const;

        void SetContextId(AzFramework::EntityContextId contextId) { m_contextId = contextId; }

    protected:

        //! Calculates base LOD distance based on mesh characteristics.
        //! We do this each time the mesh resource changes.
        void UpdateLodDistance(const SFrameLodInfo& frameLodInfo);

        //! Computes desired LOD level for the assigned mesh instance.
        CLodValue ComputeLOD(int wantedLod, const SRenderingPassInfo& passInfo);

        //! Computes the entity-relative (local space) bounding box for
        //! the assigned mesh.
        virtual void UpdateLocalBoundingBox();

        //! Updates the world-space bounding box and world space transform
        //! for the assigned mesh.
        void UpdateWorldBoundingBox();

        //! Applies configured render options to the render node.
        void ApplyRenderOptions();

        //! Populates the render mesh from the mesh asset
        void BuildRenderMesh();

        class MeshRenderOptions
        {
        public:

            AZ_TYPE_INFO(MeshRenderOptions, "{EFF77BEB-CB99-44A3-8F15-111B0200F50D}")

            MeshRenderOptions();

            float m_opacity; //!< Alpha/opacity value for rendering.
            float m_maxViewDist; //!< Maximum draw distance.
            float m_viewDistMultiplier; //!< Adjusts max view distance. If 1.0 then default max view distance is used.
            AZ::u32 m_lodRatio; //!< Controls LOD distance ratio.
            bool m_useVisAreas; //!< Allow VisAreas to control this component's visibility.
            bool m_castShadows; //!< Casts shadows.
            bool m_lodBoundingBoxBased; //!< LOD based on Bounding Boxes.
            bool m_rainOccluder; //!< Occludes raindrops.
            bool m_affectNavmesh; //!< Cuts out of the navmesh.
            bool m_affectDynamicWater; //!< Affects dynamic water (ripples).
            bool m_acceptDecals; //!< Accepts decals.
            bool m_receiveWind; //!< Receives wind.
            bool m_visibilityOccluder; //!< Appropriate for visibility occluding.
            bool m_dynamicMesh; // Mesh can change or deform independent of transform
            bool m_hasStaticTransform;
            bool m_affectGI; //!< Mesh affects Global Illumination.

            //! The Id of the entity we're associated with, for bus subscription.
            //Moved from render mesh to this struct for serialization/reflection utility
            AZ::EntityId m_attachedToEntityId;

            AZStd::function<void()> m_changeCallback;

            // Minor property changes don't require refreshing/rebuilding the property tree since no other properties
            // are shown/hidden as a result of a change.
            AZ::u32 OnMinorChanged()
            {
                if (m_changeCallback)
                {
                    m_changeCallback();
                }
                return AZ::Edit::PropertyRefreshLevels::None;
            }

            AZ::u32 OnMajorChanged()
            {
                if (m_changeCallback)
                {
                    m_changeCallback();
                }
                return AZ::Edit::PropertyRefreshLevels::EntireTree;
            }

            //Returns true if the transform is static and the mesh is not deformable.
            bool IsStatic() const;
            bool AffectsGi() const;
            AZ::Crc32 StaticPropertyVisibility() const;
            static void Reflect(AZ::ReflectContext* context);

        private:
            static bool VersionConverter(AZ::SerializeContext& context,
                AZ::SerializeContext::DataElementNode& classElement);
        };

        //! Should be visible.
        bool m_visible;

        //! User-specified material override.
        AzFramework::SimpleAssetReference<MaterialAsset> m_material;

        //! Render flags/options.
        MeshRenderOptions m_renderOptions;

        //! Currently-assigned material. Null if no material is manually assigned.
        MaterialPtr m_materialOverride;

        //! World and render transforms.
        //! These are equivalent, but for different math libraries.
        AZ::Transform m_worldTransform;
        Matrix34 m_renderTransform;

        //! Local and world bounding boxes.
        AABB m_localBoundingBox;
        AABB m_worldBoundingBox;

        //! Additional render flags -- for special editor behavior, etc.
        uint32 m_auxiliaryRenderFlags;

        //! Remember which flags have ever been toggled externally so that we can shut them off
        uint32 m_auxiliaryRenderFlagsHistory;

        //! Reference to current asset
        AZ::Data::Asset<MeshAsset> m_meshAsset;
        MeshPtr m_statObj;

        //! Computed LOD distance.
        float m_lodDistance;

        //! Computed first LOD distance (the following are multiplies of the index)
        float m_lodDistanceScaled;

        //! Scale we need to multiply the distance by.
        float m_lodDistanceScaleValue;

        //! Identifies whether we've already registered our node with the renderer.
        bool m_isRegisteredWithRenderer;

        //! Tracks if the object was moved so we can notify the renderer.
        bool m_objectMoved;

        // Helper to store indices for meshes to be modified by other components.
        MeshModificationRequestHelper m_modificationHelper;

        // EntityContext of the component
        AzFramework::EntityContextId m_contextId;
    };



    class MeshComponent
        : public AZ::Component
        , public MeshComponentRequestBus::Handler
        , public MaterialOwnerRequestBus::Handler
        , public RenderNodeRequestBus::Handler
        , public LegacyMeshComponentRequestBus::Handler
        , public AzFramework::BoundsRequestBus::Handler
        , public AzFramework::RenderGeometry::IntersectionRequestBus::Handler
    {
    public:
        friend class EditorMeshComponent;

        AZ_COMPONENT(MeshComponent, "{2F4BAD46-C857-4DCB-A454-C412DE67852A}");

        MeshComponent();
        ~MeshComponent() override;

        AZ_DISABLE_COPY_MOVE(MeshComponent);

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // BoundsRequestBus and MeshComponentRequestBus overrides ...
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

        // MeshComponentRequestBus overrides ...
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override { return m_meshRenderNode.GetMeshAsset(); }
        void SetVisibility(bool newVisibility) override;
        bool GetVisibility() override;

        // MaterialOwnerRequestBus overrides ...
        bool IsMaterialOwnerReady() override;
        void SetMaterial(_smart_ptr<IMaterial>) override;
        _smart_ptr<IMaterial> GetMaterial() override;
        void SetMaterialHandle(const MaterialHandle& materialHandle) override;
        MaterialHandle GetMaterialHandle() override;
        void SetMaterialParamVector4(const AZStd::string& /*name*/, const AZ::Vector4& /*value*/, int /*materialId = 1*/) override;
        void SetMaterialParamVector3(const AZStd::string& /*name*/, const AZ::Vector3& /*value*/, int /*materialId = 1*/) override;
        void SetMaterialParamColor(const AZStd::string& /*name*/, const AZ::Color& /*value*/, int /*materialId = 1*/) override;
        void SetMaterialParamFloat(const AZStd::string& /*name*/, float /*value*/, int /*materialId = 1*/) override;
        AZ::Vector4 GetMaterialParamVector4(const AZStd::string& /*name*/, int /*materialId = 1*/) override;
        AZ::Vector3 GetMaterialParamVector3(const AZStd::string& /*name*/, int /*materialId = 1*/) override;
        AZ::Color   GetMaterialParamColor(const AZStd::string& /*name*/, int /*materialId = 1*/) override;
        float       GetMaterialParamFloat(const AZStd::string& /*name*/, int /*materialId = 1*/) override;

        // RenderNodeRequestBus overrides ...
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;
        static const float s_renderNodeRequestBusOrder;

        // MeshComponentRequestBus overrides ...
        IStatObj* GetStatObj() override;

        // IntersectionRequestBus overrides ...
        AzFramework::RenderGeometry::RayResult RenderGeometryIntersect(const AzFramework::RenderGeometry::RayRequest& ray) override;

    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
            provided.push_back(AZ_CRC("LegacyMeshService", 0xb462a299));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("MeshService", 0x71d8a455));
            incompatible.push_back(AZ_CRC("LegacyMeshService", 0xb462a299));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void Reflect(AZ::ReflectContext* context);

        void RequireSendingRenderMeshForEditing(size_t lodIndex, size_t primitiveIndex);
        void NoRenderMeshesForEditing();

        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        MeshComponentRenderNode m_meshRenderNode;
        MaterialOwnerRequestBusHandlerImpl* m_materialBusHandler;
    };

} // namespace LmbrCentral
