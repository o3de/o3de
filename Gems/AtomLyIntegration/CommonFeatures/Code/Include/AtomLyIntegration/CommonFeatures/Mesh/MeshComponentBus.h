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
            //! Sets the model asset used by the component.
            virtual void SetModelAsset(Data::Asset<RPI::ModelAsset> modelAsset) = 0;
            //! Returns the model asset used by the component.
            virtual Data::Asset<const RPI::ModelAsset> GetModelAsset() const = 0;

            //! Sets the model used by the component via its AssetId.
            virtual void SetModelAssetId(Data::AssetId modelAssetId) = 0;
            //! Returns the AssetId for the model used by the component.
            virtual Data::AssetId GetModelAssetId() const = 0;

            //! Sets the model used by the component via its path.
            virtual void SetModelAssetPath(const AZStd::string& path) = 0;
            //! Returns the path for the model used by the component.
            virtual AZStd::string GetModelAssetPath() const = 0;

            //! Returns the model instance used by the component.
            virtual Data::Instance<RPI::Model> GetModel() const = 0;

            //! Sets the sort key for the component.
            //! Transparent models are drawn in order first by sort key, then depth.
            //! Use this to force certain transparent models to draw before or after others.
            virtual void SetSortKey(RHI::DrawItemSortKey sortKey) = 0;
            //! Returns the sort key for the component.
            virtual RHI::DrawItemSortKey GetSortKey() const = 0;

            //! Sets if this model should be considered to be always moving, even when the transform doesn't change. Useful for things like vertex shader animation.
            virtual void SetIsAlwaysDynamic(bool isAlwaysDynamic) = 0;
            //! Returns if this model is considered to always be moving.
            virtual bool GetIsAlwaysDynamic() const = 0;

            //! Sets the LodType, which determines how Lods will be selected during rendering.
            virtual void SetLodType(RPI::Cullable::LodType lodType) = 0;
            //! Returns the LodType.
            virtual RPI::Cullable::LodType GetLodType() const = 0;

            //! Sets the Lod that is rendered for all views when used with LodType::SpecificLod.
            virtual void SetLodOverride(RPI::Cullable::LodOverride lodOverride) = 0;
            //! Returns the LodOverride.
            virtual RPI::Cullable::LodOverride GetLodOverride() const = 0;

            //! Sets the minimum screen coverage. the minimum proportion of screen area an entity takes up when used with LodType::ScreenCoverage.
            //! If the entity is smaller than the minimum coverage, it is culled.
            virtual void SetMinimumScreenCoverage(float minimumScreenCoverage) = 0;
            //! Returns the minimum screen coverage.
            virtual float GetMinimumScreenCoverage() const = 0;

            //! Sets the quality rate at which mesh quality decays.
            //! (0 -> always stay highest quality lod, 1 -> quality falls off to lowest quality lod immediately).
            virtual void SetQualityDecayRate(float qualityDecayRate) = 0;
            //! Returns the quality decay rate.
            virtual float GetQualityDecayRate() const = 0;

            //! Sets if the model should be visible (true) or hidden (false).
            virtual void SetVisibility(bool visible) = 0;
            //! Returns the visibility. If the model is visible (true), that only means that it has not been explicitly hidden.
            //! The model may still not be visible by any views being rendered. If it is not visible (false), it will not be rendered by any views,
            //! regardless of whether or not the model is in the view frustum.
            virtual bool GetVisibility() const = 0;

            //! Enables (true) or disables (false) ray tracing for this model.
            virtual void SetRayTracingEnabled(bool enabled) = 0;
            //! Returns whether ray tracing is enabled (true) or disabled (false) for this model.
            virtual bool GetRayTracingEnabled() const = 0;

            //! Sets the option to exclude this mesh from baked reflection probe cubemaps
            virtual void SetExcludeFromReflectionCubeMaps(bool excludeFromReflectionCubeMaps) = 0;
            //! Returns whether this mesh is excluded from baked reflection probe cubemaps
            virtual bool GetExcludeFromReflectionCubeMaps() const = 0;

            //! Returns the axis-aligned bounding box for the model at its world position.
            virtual AZ::Aabb GetWorldBounds() const = 0;

            //! Returns the axis-aligned bounding box in model space.
            virtual AZ::Aabb GetLocalBounds() const = 0;
        };
        using MeshComponentRequestBus = EBus<MeshComponentRequests>;

        /**
         * MeshComponent can send out notifications on the MeshComponentNotificationBus
         */
        class MeshComponentNotifications
            : public ComponentBus
        {
        public:
            // Notifications can be triggered from job threads, so this uses a mutex to guard against
            // listeners joining or leaving the ebus on other threads mid-notification.
            using MutexType = AZStd::recursive_mutex;

            //! Notifies listeners when a model has been loaded.
            //! If the model is already loaded when first connecting to the MeshComponentNotificationBus,
            //! the OnModelReady event will occur when connecting.
            virtual void OnModelReady(const Data::Asset<RPI::ModelAsset>& modelAsset, const Data::Instance<RPI::Model>& model) = 0;
            //! Notifies listeners when the instance of the model for this component is about to be released.
            virtual void OnModelPreDestroy() {}

            //! Notifies listeners when a new ObjectSrg was created (this is where you'd like to update your custom ObjectSrg)
            virtual void OnObjectSrgCreated(const Data::Instance<RPI::ShaderResourceGroup>& /*objectSrg*/) {}

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
