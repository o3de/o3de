/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    namespace Render
    {
        /**
         * MeshComponentRequestBus provides an interface to request operations on a MeshComponent
         */
        class MeshComponentRequests
            : public ComponentBus
        {
        public:
            virtual void SetModelAsset(Data::Asset<RPI::ModelAsset> modelAsset) = 0;
            virtual Data::Asset<const RPI::ModelAsset> GetModelAsset() const = 0;

            virtual void SetModelAssetId(Data::AssetId modelAssetId) = 0;
            virtual Data::AssetId GetModelAssetId() const = 0;

            virtual void SetModelAssetPath(const AZStd::string& path) = 0;
            virtual AZStd::string GetModelAssetPath() const = 0;

            virtual Data::Instance<RPI::Model> GetModel() const = 0;

            virtual void SetSortKey(RHI::DrawItemSortKey sortKey) = 0;
            virtual RHI::DrawItemSortKey GetSortKey() const = 0;

            virtual void SetLodType(RPI::Cullable::LodType lodType) = 0;
            virtual RPI::Cullable::LodType GetLodType() const = 0;

            virtual void SetLodOverride(RPI::Cullable::LodOverride lodOverride) = 0;
            virtual RPI::Cullable::LodOverride GetLodOverride() const = 0;

            virtual void SetMinimumScreenCoverage(float minimumScreenCoverage) = 0;
            virtual float GetMinimumScreenCoverage() const = 0;

            virtual void SetQualityDecayRate(float qualityDecayRate) = 0;
            virtual float GetQualityDecayRate() const = 0;

            virtual void SetVisibility(bool visible) = 0;
            virtual bool GetVisibility() const = 0;

            virtual AZ::Aabb GetWorldBounds() = 0;

            virtual AZ::Aabb GetLocalBounds() = 0;
        };
        using MeshComponentRequestBus = EBus<MeshComponentRequests>;

        /**
         * MeshComponent can send out notifications on the MeshComponentNotificationBus
         */
        class MeshComponentNotifications
            : public ComponentBus
        {
        public:
            virtual void OnModelReady(const Data::Asset<RPI::ModelAsset>& modelAsset, const Data::Instance<RPI::Model>& model) = 0;
            virtual void OnModelPreDestroy() {}

            /**
             * When connecting to this bus if the asset is ready you will immediately get an OnModelReady event
             */
            template<class Bus>
            struct ConnectionPolicy
                : public AZ::EBusConnectionPolicy<Bus>
            {
                static void Connect(
                    typename Bus::BusPtr& busPtr,
                    typename Bus::Context& context,
                    typename Bus::HandlerNode& handler,
                    typename Bus::Context::ConnectLockGuard& connectLock,
                    const typename Bus::BusIdType& id = 0)
                {
                    AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                    Data::Asset<RPI::ModelAsset> modelAsset;
                    MeshComponentRequestBus::EventResult(modelAsset, id, &MeshComponentRequestBus::Events::GetModelAsset);
                    Data::Instance<RPI::Model> model;
                    MeshComponentRequestBus::EventResult(model, id, &MeshComponentRequestBus::Events::GetModel);

                    if (model &&
                        modelAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
                    {
                        handler->OnModelReady(modelAsset, model);
                    }
                }
            };
        };
        using MeshComponentNotificationBus = EBus<MeshComponentNotifications>;
    } // namespace Render
} // namespace AZ
