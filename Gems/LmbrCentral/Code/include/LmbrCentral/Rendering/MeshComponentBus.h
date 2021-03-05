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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Asset/AssetCommon.h>

struct IStatObj;
struct SRenderingPassInfo;
struct SRendParams;

namespace LmbrCentral
{
    /*!
     * MeshComponentRequestBus
     * Messages serviced by the mesh component.
     */
    class MeshComponentRequests
        : public AZ::ComponentBus
    {
    public:

        /**
        * Returns the axis aligned bounding box in world coordinates
        */
        virtual AZ::Aabb GetWorldBounds() = 0;

        /**
        * Returns the axis aligned bounding box in model coordinates
        */
        virtual AZ::Aabb GetLocalBounds() = 0;

        /**
        * Sets the mesh asset for this component
        */
        virtual void SetMeshAsset(const AZ::Data::AssetId& id) = 0;

        /**
        * Returns the asset used by the mesh
        */
        virtual AZ::Data::Asset<AZ::Data::AssetData> GetMeshAsset() = 0;

        /**
        * Returns true if the mesh is currently visible
        */
        virtual bool GetVisibility() { return true; }

        /**
        * Sets the current visibility of the mesh
        */
        virtual void SetVisibility([[maybe_unused]] bool isVisible) {}
    };

    using MeshComponentRequestBus = AZ::EBus<MeshComponentRequests>;

    /*!
     * SkeletonHierarchyRequestBus
     * Messages serviced by components to provide information about skeletal hierarchies.
     */
    class SkeletalHierarchyRequests
        : public AZ::ComponentBus
    {
    public:

        /**
         * \return Number of joints in the skeleton joint hierarchy.
         */
        virtual AZ::u32 GetJointCount() { return 0; }

        /**
        * \param jointIndex Index of joint whose name should be returned.
        * \return Name of the joint at the specified index. Null if joint index is not valid.
        */
        virtual const char* GetJointNameByIndex(AZ::u32 /*jointIndex*/) { return nullptr; }

        /**
        * \param jointName Name of joint whose index should be returned.
        * \return Index of the joint with the specified name. -1 if the joint was not found.
        */
        virtual AZ::s32 GetJointIndexByName(const char* /*jointName*/) { return 0; }

        /**
        * \param jointIndex Index of joint whose local-space transform should be returned.
        * \return Joint's character-space transform. Identify if joint index was not valid.
        */
        virtual AZ::Transform GetJointTransformCharacterRelative(AZ::u32 /*jointIndex*/) { return AZ::Transform::CreateIdentity(); }
    };

    using SkeletalHierarchyRequestBus = AZ::EBus<SkeletalHierarchyRequests>;

    /*!
    * LegacyMeshComponentRequestBus
    * Messages serviced by the mesh component.
    */
    class LegacyMeshComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual IStatObj* GetStatObj() = 0;
    };

    using LegacyMeshComponentRequestBus = AZ::EBus<LegacyMeshComponentRequests>;

    /*!
     * MeshComponentNotificationBus
     * Events dispatched by the mesh component.
     */
    class MeshComponentNotifications
        : public AZ::ComponentBus
    {
    public:

        /**
         * Notifies listeners the mesh instance has been created.
         * \param asset - The asset the mesh instance is based on.
         */
        virtual void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) { (void)asset; }

        /**
         * Notifies listeners that the mesh instance has been destroyed.
        */
        virtual void OnMeshDestroyed() {}

        virtual void OnBoundsReset() {};

        /*
         * Notifies listeners prior to making the render call 
         */
        virtual void OnMeshPreRender([[maybe_unused]] const struct SRendParams& inOutRenderParams, [[maybe_unused]] const SRenderingPassInfo& passInfo) {};

        /**
         * When connecting to this bus if the asset is ready you will immediately get an OnMeshCreated event
        **/
        template<class Bus>
        struct ConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                AZ::Data::Asset<AZ::Data::AssetData> asset;
                EBUS_EVENT_ID_RESULT(asset, id, MeshComponentRequestBus, GetMeshAsset);
                if (asset.GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
                {
                    handler->OnMeshCreated(asset);
                }
            }
        };
    };

    using MeshComponentNotificationBus = AZ::EBus<MeshComponentNotifications>;
} // namespace LmbrCentral
