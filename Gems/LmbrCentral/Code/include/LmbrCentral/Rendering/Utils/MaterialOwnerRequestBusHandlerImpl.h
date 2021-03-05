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

#include <LmbrCentral/Rendering/MaterialOwnerBus.h>
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Color.h>

#include <IEntityRenderState.h>
#include <I3DEngine.h>

struct IRenderNode;

namespace LmbrCentral
{
    //! This is helper class to provide common implementation for the MaterialOwnerRequests interface 
    //! that will be needed by most components that have materials.
    //! This does not actually inherit the MaterialOwnerRequestBus::Handler interface because it is
    //! not intended to subscribe to that bus, but it does provide implementations for all the same functions.
    class MaterialOwnerRequestBusHandlerImpl
        : public MeshComponentNotificationBus::Handler
        , public AZ::TickBus::Handler
        , public MaterialOwnerRequestBus::Handler        
    {
        using MaterialPtr = _smart_ptr < IMaterial >;

    public:
        AZ_CLASS_ALLOCATOR(MaterialOwnerRequestBusHandlerImpl, AZ::SystemAllocator, 0);        

        //! Initializes the MaterialOwnerRequestBusHandlerImpl, to be called when the Material owner is activated.
        //! \param  renderNode  holds the active material that will be manipulated
        //! \param  entityId    ID of the entity that has the Material owner
        //! \param registerBus  Signals to this impl that it should connect to the MaterialOwnerRequestBus on the specified id.
        //
        // Ideally this would be apart of the normal activate, but to keep old behavior consistent
        // going to add an extra flow into this to maintain the current workflows of:
        // DecalComponent and MeshComponent
        //
        // While still allowing a nicer interface for ActorComponent to utilize.      
        void Activate(IRenderNode* renderNode, const AZ::EntityId& entityId, bool registerBus = false)
        {
            m_clonedMaterial = nullptr;
            m_renderNode = renderNode;
            m_readyEventSent = false;

            MaterialOwnerNotificationBus::Bind(m_notificationBus, entityId);

            if (m_renderNode)
            {
                if (!m_renderNode->IsReady())
                {
                    // Some material owners, in particular MeshComponents, may not be ready upon activation because the
                    // actual mesh data and default material haven't been loaded yet. Until the RenderNode is ready,
                    // it's material probably isn't valid.
                    MeshComponentNotificationBus::Handler::BusConnect(entityId);
                }
                else
                {
                    // For some material owner types (like DecalComponent), the material is ready immediately. But we can't
                    // send the event yet because components are still being Activated, so we delay until the first tick.
                    AZ::TickBus::Handler::BusConnect();
                }

                if (registerBus)
                {
                    MaterialOwnerRequestBus::Handler::BusConnect(entityId);
                }
            }
        }

        void Deactivate()
        {
            m_notificationBus = nullptr;
            MeshComponentNotificationBus::Handler::BusDisconnect();
            MaterialOwnerRequestBus::Handler::BusDisconnect();
            AZ::TickBus::Handler::BusDisconnect();
        }

        //! Returns whether the Material has been cloned. MaterialOwnerRequestBusHandlerImpl clones the IRenderNode's Material
        //! rather than modify the original to avoid affecting other entities in the scene.
        bool IsMaterialCloned() const
        {
            return nullptr != m_clonedMaterial;
        }

        void SetMaterialHandle(const MaterialHandle& materialHandle)
        {
            SetMaterial(materialHandle.m_material);
        }

        MaterialHandle GetMaterialHandle()
        {
            MaterialHandle m;
            m.m_material = GetMaterial();
            return m;
        }

        //////////////////////////////////////////////////////////////////////////
        // MaterialOwnerRequestBus interface implementation
        bool IsMaterialOwnerReady() override
        {
            return m_renderNode && m_renderNode->IsReady();
        }

        void SetMaterial(MaterialPtr material) override
        {
            if (m_renderNode)
            {
                if (material && material->IsSubMaterial())
                {
                    AZ_Error("MaterialOwnerRequestBus", false, "Material Owner cannot be given a Sub-Material.");
                }
                else
                {
                    m_clonedMaterial = nullptr;
                    m_renderNode->SetMaterial(material);
                }
            }
        }

        MaterialPtr GetMaterial() override
        {
            MaterialPtr material = nullptr;

            if (m_renderNode)
            {
                material = m_renderNode->GetMaterial();

                if (!m_renderNode->IsReady())
                {
                    if (material)
                    {
                        AZ_Warning("MaterialOwnerRequestBus", false, "A Material was found, but Material Owner is not ready. May have unexpected results. (Try using MaterialOwnerNotificationBus.OnMaterialOwnerReady or MaterialOwnerRequestBus.IsMaterialOwnerReady)");
                    }
                    else
                    {
                        AZ_Error("MaterialOwnerRequestBus", false, "Material Owner is not ready and no Material was found. Assets probably have not finished loading yet. (Try using MaterialOwnerNotificationBus.OnMaterialOwnerReady or MaterialOwnerRequestBus.IsMaterialOwnerReady)");
                    }
                }

                AZ_Assert(nullptr == m_clonedMaterial || material == m_clonedMaterial, "MaterialOwnerRequestBusHandlerImpl and RenderNode are out of sync");
            }

            return material;
        }        

        void SetMaterialParamVector4(const AZStd::string& name, const AZ::Vector4& value, int materialId = 1) override
        {
            if (GetMaterial())
            {
                CloneMaterial();

                const int materialIndex = materialId - 1;
                Vec4 vec4(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
                const bool success = GetMaterial()->SetGetMaterialParamVec4(name.c_str(), vec4, false, true, materialIndex);
                AZ_Error("Material Owner", success, "Failed to set Material ID %d, param '%s'.", materialId, name.c_str());
            }
        }

        void SetMaterialParamVector3(const AZStd::string& name, const AZ::Vector3& value, int materialId = 1) override
        {
            if (GetMaterial())
            {
                CloneMaterial();

                const int materialIndex = materialId - 1;
                Vec3 vec3(value.GetX(), value.GetY(), value.GetZ());
                const bool success = GetMaterial()->SetGetMaterialParamVec3(name.c_str(), vec3, false, true, materialIndex);
                AZ_Error("Material Owner", success, "Failed to set Material ID %d, param '%s'.", materialId, name.c_str());
            }
        }

        void SetMaterialParamColor(const AZStd::string& name, const AZ::Color& value, int materialId = 1) override
        {
            if (GetMaterial())
            {
                // When value had garbage data is was not only making the material render black, it also corrupted something
                // on the GPU, making black boxes flicker over the sky.
                // It was garbage due to a bug in the Color object node where all fields have to be set to some value manually; the default is not 0.
                if ((value.GetR() < 0 || value.GetR() > 1) ||
                    (value.GetG() < 0 || value.GetG() > 1) ||
                    (value.GetB() < 0 || value.GetB() > 1) ||
                    (value.GetA() < 0 || value.GetA() > 1))
                {
                    return;
                }
                
                CloneMaterial();

                const int materialIndex = materialId - 1;
                Vec4 vec4(value.GetR(), value.GetG(), value.GetB(), value.GetA());
                const bool success = GetMaterial()->SetGetMaterialParamVec4(name.c_str(), vec4, false, true, materialIndex);
                AZ_Error("Material Owner", success, "Failed to set Material ID %d, param '%s'.", materialId, name.c_str());
            }
        }
        void SetMaterialParamFloat(const AZStd::string& name, float value, int materialId = 1) override
        {
            if (GetMaterial())
            {
                if (!IsMaterialCloned())
                {
                    CloneMaterial();
                }

                const int materialIndex = materialId - 1;
                const bool success = GetMaterial()->SetGetMaterialParamFloat(name.c_str(), value, false, true, materialIndex);
                AZ_Error("Material Owner", success, "Failed to set Material ID %d, param '%s'.", materialId, name.c_str());
            }
        }

        AZ::Vector4 GetMaterialParamVector4(const AZStd::string& name, int materialId = 1) override
        {
            AZ::Vector4 value = AZ::Vector4::CreateZero();

            MaterialPtr material = GetMaterial();
            if (material)
            {
                const int materialIndex = materialId - 1;
                Vec4 vec4;
                if (material->SetGetMaterialParamVec4(name.c_str(), vec4, true, true, materialIndex))
                {
                    value.Set(vec4.x, vec4.y, vec4.z, vec4.w);
                }
                else
                {
                    AZ_Error("Material Owner", false, "Failed to read Material ID %d, param '%s'.", materialId, name.c_str());
                }
            }

            return value;
        }

        AZ::Vector3 GetMaterialParamVector3(const AZStd::string& name, int materialId = 1) override
        {
            AZ::Vector3 value = AZ::Vector3::CreateZero();

            MaterialPtr material = GetMaterial();
            if (material)
            {
                const int materialIndex = materialId - 1;
                Vec3 vec3;
                if (material->SetGetMaterialParamVec3(name.c_str(), vec3, true, true, materialIndex))
                {
                    value.Set(vec3.x, vec3.y, vec3.z);
                }
                else
                {
                    AZ_Error("Material Owner", false, "Failed to read Material ID %d, param '%s'.", materialId, name.c_str());
                }
            }

            return value;
        }

        AZ::Color GetMaterialParamColor(const AZStd::string& name, int materialId = 1) override
        {
            AZ::Color value = AZ::Color::CreateZero();

            MaterialPtr material = GetMaterial();
            if (material)
            {
                const int materialIndex = materialId - 1;
                Vec4 vec4;
                if (material->SetGetMaterialParamVec4(name.c_str(), vec4, true, true, materialIndex))
                {
                    value.Set(vec4.x, vec4.y, vec4.z, vec4.w);
                }
                else
                {
                    AZ_Error("Material Owner", false, "Failed to read Material ID %d, param '%s'.", materialId, name.c_str());
                }
            }

            return value;
        }

        float GetMaterialParamFloat(const AZStd::string& name, int materialId = 1) override
        {
            float value = 0.0f;

            MaterialPtr material = GetMaterial();
            if (material)
            {
                const int materialIndex = materialId - 1;
                const bool success = material->SetGetMaterialParamFloat(name.c_str(), value, true, true, materialIndex);
                AZ_Error("Material Owner", success, "Failed to read Material ID %d, param '%s'.", materialId, name.c_str());
            }

            return value;
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // MeshComponentNotificationBus interface implementation
        void OnMeshCreated([[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset) override
        {
            AZ_Assert(IsMaterialOwnerReady(), "Got OnMeshCreated but the RenderNode still isn't ready");
            SendReadyEvent();
            MeshComponentNotificationBus::Handler::BusDisconnect();
        }
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TickBus interface implementation
        void OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time) override
        {
            if (!m_readyEventSent && IsMaterialOwnerReady())
            {
                SendReadyEvent();
                AZ::TickBus::Handler::BusDisconnect();
            }
        }
        //////////////////////////////////////////////////////////////////////////

    private:

        //! Clones the active material and applies it to the IRenderNode
        void CloneMaterial()
        {
            if (!IsMaterialCloned())
            {
                CloneMaterial(GetMaterial());
            }
        }

        //! Clones the specified material and applies it to the IRenderNode
        void CloneMaterial(MaterialPtr material)
        {
            if (material && m_renderNode)
            {
                AZ_Assert(nullptr == m_clonedMaterial, "Material has already been cloned. This operation is wasteful.");

                m_clonedMaterial = gEnv->p3DEngine->GetMaterialManager()->CloneMultiMaterial(material);
                AZ_Assert(m_clonedMaterial, "Failed to clone material. The original will be used.");
                if (m_clonedMaterial)
                {
                    m_renderNode->SetMaterial(m_clonedMaterial);
                }
            }
        }

        //! Send the OnMaterialOwnerReady event
        void SendReadyEvent()
        {
            AZ_Assert(!m_readyEventSent, "OnMaterialOwnerReady already sent");
            if (!m_readyEventSent)
            {
                m_readyEventSent = true;
                MaterialOwnerNotificationBus::Event(m_notificationBus, &MaterialOwnerNotifications::OnMaterialOwnerReady);
            }
        }

        MaterialOwnerNotificationBus::BusPtr m_notificationBus = nullptr; //!< Cached bus pointer to the notification bus.
        IRenderNode* m_renderNode = nullptr;    //!< IRenderNode which holds the active material that will be manipulated.
        MaterialPtr m_clonedMaterial = nullptr; //!< The component's material can be cloned here to make a copy that is unique to this component.
        bool m_readyEventSent = false;          //! Tracks whether OnMaterialOwnerReady has been sent yet.
    };

} // namespace LmbrCentral
