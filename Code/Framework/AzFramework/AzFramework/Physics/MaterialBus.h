/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Material.h"

#include <AzCore/EBus/EBus.h>

namespace Physics
{
    /// Listens to requests for physics materials.
    class PhysicsMaterialRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // Implemented by sole owner of materials, e.g. class MaterialManager in PhysX gem.

        /// Get default material.
        virtual AZStd::shared_ptr<Physics::Material> GetGenericDefaultMaterial() = 0;

        /// Returns weak pointers to physics materials.
        /// Connect to PhysicsMaterialNotifications::MaterialsReleased to be informed when material pointers are deleted by owner.
        virtual void GetMaterials(const MaterialSelection& materialSelection
            , AZStd::vector<AZStd::shared_ptr<Physics::Material>>& outMaterials) = 0;

        /// Returns a weak pointer to physics material with the given id.
        virtual AZStd::shared_ptr<Physics::Material> GetMaterialById(Physics::MaterialId id) = 0;

        /// Returns a weak pointer to physics material with the given name.
        virtual AZStd::shared_ptr<Physics::Material> GetMaterialByName(const AZStd::string& name) = 0;

        /// Updates the material selection from the physics asset or sets it to default if there's no asset provided.
        /// @param shapeConfiguration The shape information that contains the physics asset.
        /// @param materialSelection The material selection to update.
        virtual void UpdateMaterialSelectionFromPhysicsAsset(
            const ShapeConfiguration& shapeConfiguration,
            MaterialSelection& materialSelection) = 0;
    };
    using PhysicsMaterialRequestBus = AZ::EBus<PhysicsMaterialRequests>;

    /// Dispatches changes in physics materials to others.
    class PhysicsMaterialNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; // SystemComponent of multiple gems may listen to changes in materials.
        typedef AZ::Uuid BusIdType;

        /// Notifies that material pointers have been deleted by owner.
        virtual void MaterialsReleased() = 0;
    };
    using PhysicsMaterialNotificationsBus = AZ::EBus<PhysicsMaterialNotifications>;
}
