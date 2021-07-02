/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Public/Material/Material.h>

namespace AZ
{
    namespace Render
    {
        enum LuxCoreTextureType
        {
            Default = 0,
            IBL,
            Albedo,
            Normal
        };

        class LuxCoreRequests
            : public EBusTraits
        {

        public:
            /// Overrides the default AZ::EBusTraits handler policy to allow one listener only.
            static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
            virtual ~LuxCoreRequests() {}
            virtual void SetCameraEntityID(AZ::EntityId id) = 0;
            virtual void AddMesh(Data::Asset<RPI::ModelAsset> modelAsset) = 0;
            virtual void AddMaterial(Data::Instance<RPI::Material> material) = 0;
            virtual void AddTexture(Data::Instance<RPI::Image> texture, LuxCoreTextureType type) = 0;
            virtual void AddObject(Data::Asset<RPI::ModelAsset> modelAsset, Data::InstanceId materialInstanceId) = 0;
            virtual bool CheckTextureStatus() = 0;
            virtual void RenderInLuxCore() = 0;
            virtual void ClearLuxCore() = 0;
            virtual void ClearObject() = 0;
        };

        typedef AZ::EBus<LuxCoreRequests> LuxCoreRequestsBus;

        class LuxCoreNotification
            : public EBusTraits
        {
        public:
            static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
            /**
             * Overrides the default AZ::EBusTraits ID type so that AssetId are
             * used to access the addresses of the bus.
             */
            typedef Data::AssetId BusIdType;
            virtual ~LuxCoreNotification() {}

            virtual void OnRenderPrepare() {}
        };
        typedef AZ::EBus<LuxCoreNotification> LuxCoreNotificationBus;
    }
}
