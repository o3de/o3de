/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        /// Get default material
        virtual AZStd::shared_ptr<Physics::Material> GetGenericDefaultMaterial() = 0;

        /// Returns weak pointers to physics materials.
        /// Connect to PhysicsMaterialNotifications::MaterialsReleased to be informed when material pointers are deleted by owner.
        virtual void GetMaterials(const MaterialSelection& materialSelection
            , AZStd::vector<AZStd::weak_ptr<Physics::Material>>& outMaterials) = 0;

        /// Returns a weak pointer to physics material with the given name.
        virtual AZStd::weak_ptr<Physics::Material> GetMaterialByName(const AZStd::string& name) = 0;

        /// Returns index of the first selected material in MaterialSelection's material library. 
        /// A MaterialSelection can contain multiple material selections.
        /// Returned index is 0-based where 0 is the Default material, and materials from the material library are 1 and onwards.
        virtual AZ::u32 GetFirstSelectedMaterialIndex(const MaterialSelection& materialSelection) = 0;
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
