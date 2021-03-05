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
#include <AzFramework/Visibility/BoundsBus.h>

#include <IEntityRenderState.h>

#include <LmbrCentral/Rendering/RenderBoundsBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/MaterialOwnerBus.h>
#include <LmbrCentral/Rendering/RenderNodeBus.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MeshAsset.h>

namespace LmbrCentral
{
    /*!
     * RenderNode implementation responsible for integrating with the renderer.
     * The node owns render flags, the mesh instance, and the render transform.
     */
    class SkinnedMeshComponentRenderNode
        : public IRenderNode
        , public AZ::TransformNotificationBus::Handler
        , public AZ::Data::AssetBus::Handler
    {
        friend class EditorSkinnedMeshComponent;
    public:
        using MaterialPtr = _smart_ptr<IMaterial>;

        AZ_TYPE_INFO(SkinnedMeshComponentRenderNode, "{AE5CFE2B-6CFF-4B66-9B9C-C514BFDB8A88}");

        SkinnedMeshComponentRenderNode();
        ~SkinnedMeshComponentRenderNode() override;

        void CopyPropertiesTo(SkinnedMeshComponentRenderNode& rhs) const;

        //! Notifies render node which entity owns it, for subscribing to transform
        //! bus, etc.
        void AttachToEntity(AZ::EntityId id);

        //! Instantiate mesh instance.
        void CreateMesh();

        //! Destroy mesh instance.
        void DestroyMesh();

        //! Returns true if the node has geometry assigned.
        bool HasMesh() const;

        //! Assign a new mesh asset
        void SetMeshAsset(const AZ::Data::AssetId& id);

        AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() const { return m_characterDefinitionAsset; }

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
        AZ::EntityId GetEntityId() override { return m_attachedToEntityId; }
        //////////////////////////////////////////////////////////////////////////

        //! Invoked in the editor when a property requiring render state refresh
        //! has changed.
        void RefreshRenderState();

        //! Set/get auxiliary render flags.
        void SetAuxiliaryRenderFlags(uint32 flags);
        uint32 GetAuxiliaryRenderFlags() const { return m_auxiliaryRenderFlags; }
        void UpdateAuxiliaryRenderFlags(bool on, uint32 mask); ///< turn particular bits on/off

        //! Computes the entity - relative(local space) bounding box for
        //! the assigned mesh.
        virtual void UpdateLocalBoundingBox();
        void SetVisible(bool isVisible);
        bool GetVisible();

        static void Reflect(AZ::ReflectContext* context);

        static float GetDefaultMaxViewDist();

        static AZ::Uuid GetRenderOptionsUuid() { return AZ::AzTypeInfo<SkinnedRenderOptions>::Uuid(); }

        //! Retrieve skeleton joint count.
        AZ::u32 GetJointCount() const;
        //! Retrieve joint name by index.
        const char* GetJointNameByIndex(AZ::u32 jointIndex) const;
        //! Retrieve joint index by name.
        AZ::u32 GetJointIndexByName(const char* jointName) const;
        //! Retrieve joint character-local transform.
        AZ::Transform GetJointTransformCharacterRelative(AZ::u32 jointIndex) const;

        //! Registers or unregisters our render node with the render.
        void RegisterWithRenderer(bool registerWithRenderer);

    protected:

        //! Calculates base LOD distance based on mesh characteristics.
        //! We do this each time the mesh resource changes.
        void UpdateLodDistance(const SFrameLodInfo& frameLodInfo);

        //! Computes desired LOD level for the assigned mesh instance.
        CLodValue ComputeLOD(int wantedLod, const SRenderingPassInfo& passInfo);

        //! Updates the world-space bounding box and world space transform
        //! for the assigned mesh.
        void UpdateWorldBoundingBox();

        //! Applies configured render options to the render node.
        void ApplyRenderOptions();


        class SkinnedRenderOptions
        {
        public:

            AZ_TYPE_INFO(SkinnedRenderOptions, "{33E69F1C-518F-4DD2-88D1-DF6D12ECA54E}")

            SkinnedRenderOptions();

            float m_opacity; //!< Alpha/opacity value for rendering.
            float m_maxViewDist; //!< Maximum draw distance.
            float m_viewDistMultiplier; //!< Adjusts max view distance. If 1.0 then default max view distance is used.
            AZ::u32 m_lodRatio; //!< Controls LOD distance ratio.
            bool m_useVisAreas; //!< Allow VisAreas to control this component's visibility.
            bool m_castShadows; //!< Casts dynamic shadows.
            bool m_rainOccluder; //!< Occludes raindrops.
            bool m_acceptDecals; //!< Accepts decals.

            AZStd::function<void()> m_changeCallback;

            void OnChanged()
            {
                if (m_changeCallback)
                {
                    m_changeCallback();
                }
            }

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
        SkinnedRenderOptions m_renderOptions;

        //! Currently-assigned material. Null if no material is manually assigned.
        MaterialPtr m_materialOverride;

        //! The Id of the entity we're associated with, for bus subscription.
        AZ::EntityId m_attachedToEntityId;

        //! World and render transforms.
        //! These are equivalent, but for different math libraries.
        AZ::Transform m_worldTransform;
        Matrix34 m_renderTransform;

        //! Local and world bounding boxes.
        AABB m_localBoundingBox;
        AABB m_worldBoundingBox;

        //! Additional render flags -- for special editor behavior, etc.
        uint32 m_auxiliaryRenderFlags;

        //! Remember which flags have ever been toggled externally so that we can shut them off.
        uint32 m_auxiliaryRenderFlagsHistory;

        //! Reference to current asset
        AZ::Data::Asset<CharacterDefinitionAsset> m_characterDefinitionAsset;

        //! Computed LOD distance.
        float m_lodDistance;

        //! Identifies whether we've already registered our node with the renderer.
        bool m_isRegisteredWithRenderer;

        //! Tracks if the object was moved so we can notify the renderer.
        bool m_objectMoved;

        //! Editor-only flag to avoid duplicate asset loading during scene load.
        //! Duplicate asset loading can occur if we call the following in order, within the same frame:
        //! CreateMesh()... DestroyMesh()... CreateMesh()....
        //! The flag ensures mesh destruction/loading only occurs once the mesh asset loading job completes.
        bool m_isQueuedForDestroyMesh;
    };

    class SkinnedMeshComponent
        : public AZ::Component
        , private MeshComponentRequestBus::Handler
        , private AzFramework::BoundsRequestBus::Handler
        , private MaterialOwnerRequestBus::Handler
        , private RenderNodeRequestBus::Handler
        , private SkeletalHierarchyRequestBus::Handler
    {
    public:
        friend class EditorSkinnedMeshComponent;

        AZ_COMPONENT(SkinnedMeshComponent, "{C99EB110-CA74-4D95-83F0-2FCDD1FF418B}");

        ~SkinnedMeshComponent() override = default;

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // BoundsRequestBus and MeshComponentRequestBus overrides ...
        AZ::Aabb GetWorldBounds() override;
        AZ::Aabb GetLocalBounds() override;

        // MeshComponentRequestBus overrides ...
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() override { return m_skinnedMeshRenderNode.GetMeshAsset(); }
        void SetVisibility(bool newVisibility) override;
        bool GetVisibility() override;

        // SkeletalHierarchyRequestBus overrides ...
        AZ::u32 GetJointCount() override;
        const char* GetJointNameByIndex(AZ::u32 jointIndex) override;
        AZ::s32 GetJointIndexByName(const char* jointName) override;
        AZ::Transform GetJointTransformCharacterRelative(AZ::u32 jointIndex) override;

        // MaterialOwnerRequestBus overrides ...
        void SetMaterial(_smart_ptr<IMaterial>) override;
        _smart_ptr<IMaterial> GetMaterial() override;

        // RenderNodeRequestBus overrides ...
        IRenderNode* GetRenderNode() override;
        float GetRenderNodeRequestBusOrder() const override;
        static const float s_renderNodeRequestBusOrder;
    protected:

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MeshService", 0x71d8a455));
            provided.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("MeshService", 0x71d8a455));
            incompatible.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void Reflect(AZ::ReflectContext* context);

        // Reflected Data
        SkinnedMeshComponentRenderNode m_skinnedMeshRenderNode;
    };
} // namespace LmbrCentral
