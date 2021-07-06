/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <Atom/Feature/Material/MaterialAssignment.h>

namespace AZ
{
    namespace Render
    {
        //! MaterialComponentRequestBus provides an interface to request operations on a MaterialComponent
        class MaterialComponentRequests
            : public ComponentBus
        {
        public:
            //! Get all material assignments that can be overridden
            virtual MaterialAssignmentMap GetOriginalMaterialAssignments() const = 0;
            //! Set material overrides
            virtual void SetMaterialOverrides(const MaterialAssignmentMap& materials) = 0;
            //! Get material overrides
            virtual const MaterialAssignmentMap& GetMaterialOverrides() const = 0;
            //! Clear all material overrides
            virtual void ClearAllMaterialOverrides() = 0;
            //! Set default material override
            virtual void SetDefaultMaterialOverride(const AZ::Data::AssetId& materialAssetId) = 0;
            //! Get default material override
            virtual const AZ::Data::AssetId GetDefaultMaterialOverride() const = 0;
            //! Clear default material override
            virtual void ClearDefaultMaterialOverride() = 0;
            //! Set material override
            virtual void SetMaterialOverride(const MaterialAssignmentId& materialAssignmentId, const AZ::Data::AssetId& materialAssetId) = 0;
            //! Get material override
            virtual const AZ::Data::AssetId GetMaterialOverride(const MaterialAssignmentId& materialAssignmentId) const = 0;
            //! Clear material override
            virtual void ClearMaterialOverride(const MaterialAssignmentId& materialAssignmentId) = 0;
            //! Set a material property value override
            virtual void SetPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const Name& propertyName, const AZStd::any& propertyValue) = 0;
            //! Get a material property value override
            virtual AZStd::any GetPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const Name& propertyName) const = 0;
            //! Clear property override for a specific material assignment
            virtual void ClearPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const Name& propertyName) = 0;
            //! Clear property overrides for a specific material assignment
            virtual void ClearPropertyOverrides(const MaterialAssignmentId& materialAssignmentId) = 0;
            //! Clear all property overrides
            virtual void ClearAllPropertyOverrides() = 0;
            //! Get Property overrides for a specific material assignment
            virtual MaterialPropertyOverrideMap GetPropertyOverrides(const MaterialAssignmentId& materialAssignmentId) const = 0;
        };
        using MaterialComponentRequestBus = EBus<MaterialComponentRequests>;

        //! MaterialComponent can send out notifications on the MaterialComponentNotificationBus 
        class MaterialComponentNotifications
            : public ComponentBus
        {
        public:
            virtual void OnMaterialsUpdated([[maybe_unused]] const MaterialAssignmentMap& materials) {}
            virtual void OnMaterialsEdited([[maybe_unused]] const MaterialAssignmentMap& materials) {}
        };
        using MaterialComponentNotificationBus = EBus<MaterialComponentNotifications>;

        //! Bus for retrieving information about materials embedded in or used by a source, like a model
        class MaterialReceiverRequests
            : public ComponentBus
        {
        public:
            virtual MaterialAssignmentMap GetMaterialAssignments() const = 0;
            virtual AZStd::unordered_set<AZ::Name> GetModelUvNames() const = 0;
        };
        using MaterialReceiverRequestBus = EBus<MaterialReceiverRequests>;

        //! Bus for notifying when materials embedded in or used by a source, like a model, change
        class MaterialReceiverNotifications
            : public ComponentBus
        {
        public:
            virtual void OnMaterialAssignmentsChanged() = 0;
        };
        using MaterialReceiverNotificationBus = EBus<MaterialReceiverNotifications>;

    } // namespace Render
} // namespace AZ
