/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            //! Get material assignment id matching lod and label substring
            virtual MaterialAssignmentId FindMaterialAssignmentId(const MaterialAssignmentLodIndex lod, const AZStd::string& label) const = 0;
            //! Get default material asset
            virtual AZ::Data::AssetId GetDefaultMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const = 0;
            //! Get material slot label
            virtual AZStd::string GetMaterialSlotLabel(const MaterialAssignmentId& materialAssignmentId) const = 0;
            //! Set material overrides
            virtual void SetMaterialOverrides(const MaterialAssignmentMap& materials) = 0;
            //! Get material overrides
            virtual const MaterialAssignmentMap& GetMaterialOverrides() const = 0;
            //! Clear all material overrides
            virtual void ClearAllMaterialOverrides() = 0;
            //! Clear non-lod material overrides
            virtual void ClearModelMaterialOverrides() = 0;
            //! Clear lod material overrides
            virtual void ClearLodMaterialOverrides() = 0;
            //! Clear residual materials that don't correspond to the associated model
            virtual void ClearIncompatibleMaterialOverrides() = 0;
            //! Clear materials that reference missing assets
            virtual void ClearInvalidMaterialOverrides() = 0;
            //! Repair materials that reference missing assets by assigning the default asset
            virtual void RepairInvalidMaterialOverrides() = 0;
            //! Set default material override
            virtual void SetDefaultMaterialOverride(const AZ::Data::AssetId& materialAssetId) = 0;
            //! Get default material override
            virtual const AZ::Data::AssetId GetDefaultMaterialOverride() const = 0;
            //! Clear default material override
            virtual void ClearDefaultMaterialOverride() = 0;
            //! Set material override
            virtual void SetMaterialOverride(const MaterialAssignmentId& materialAssignmentId, const AZ::Data::AssetId& materialAssetId) = 0;
            //! Get material override
            virtual AZ::Data::AssetId GetMaterialOverride(const MaterialAssignmentId& materialAssignmentId) const = 0;
            //! Clear material override
            virtual void ClearMaterialOverride(const MaterialAssignmentId& materialAssignmentId) = 0;
            //! Set a material property override value wrapped by an AZStd::any
            virtual void SetPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZStd::any& value) = 0;
            //! Set a material property override value to a bool
            virtual void SetPropertyOverrideBool(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const bool& value) = 0;
            //! Set a material property override value to a integer
            virtual void SetPropertyOverrideInt32(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const int32_t& value) = 0;
            //! Set a material property override value to a unsigned integer
            virtual void SetPropertyOverrideUInt32(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const uint32_t& value) = 0;
            //! Set a material property override value to a float
            virtual void SetPropertyOverrideFloat(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const float& value) = 0;
            //! Set a material property override value to a Vector2
            virtual void SetPropertyOverrideVector2(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZ::Vector2& value) = 0;
            //! Set a material property override value to a Vector3
            virtual void SetPropertyOverrideVector3(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZ::Vector3& value) = 0;
            //! Set a material property override value to a Vector4
            virtual void SetPropertyOverrideVector4(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZ::Vector4& value) = 0;
            //! Set a material property override value to a color
            virtual void SetPropertyOverrideColor(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZ::Color& value) = 0;
            //! Set a material property override value to an image asset
            virtual void SetPropertyOverrideImageAsset(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZ::Data::Asset<AZ::RPI::ImageAsset>& value) = 0;
            //! Set a material property override value to an image instance
            virtual void SetPropertyOverrideImageInstance(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZ::Data::Instance<AZ::RPI::Image>& value) = 0;
            //! Set a material property override value to a string
            virtual void SetPropertyOverrideString(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZStd::string& value) = 0;
            //! Get a material property override value wrapped by an AZStd::any
            virtual AZStd::any GetPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as a bool
            virtual bool GetPropertyOverrideBool(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as an integer
            virtual int32_t GetPropertyOverrideInt32(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as an unsigned integer
            virtual uint32_t GetPropertyOverrideUInt32(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as a float
            virtual float GetPropertyOverrideFloat(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as a Vector2
            virtual AZ::Vector2 GetPropertyOverrideVector2(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as a Vector3
            virtual AZ::Vector3 GetPropertyOverrideVector3(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as a Vector4
            virtual AZ::Vector4 GetPropertyOverrideVector4(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as a Color
            virtual AZ::Color GetPropertyOverrideColor(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as an image asset
            virtual AZ::Data::Asset<AZ::RPI::ImageAsset> GetPropertyOverrideImageAsset(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as an image instance
            virtual AZ::Data::Instance<AZ::RPI::Image> GetPropertyOverrideImageInstance(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Get a material property override value as a string
            virtual AZStd::string GetPropertyOverrideString(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;
            //! Clear property override for a specific material assignment
            virtual void ClearPropertyOverride(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) = 0;
            //! Clear property overrides for a specific material assignment
            virtual void ClearPropertyOverrides(const MaterialAssignmentId& materialAssignmentId) = 0;
            //! Clear all property overrides
            virtual void ClearAllPropertyOverrides() = 0;
            //! Set Property overrides for a specific material assignment
            virtual void SetPropertyOverrides(
                const MaterialAssignmentId& materialAssignmentId, const MaterialPropertyOverrideMap& propertyOverrides) = 0;
            //! Get Property overrides for a specific material assignment
            virtual MaterialPropertyOverrideMap GetPropertyOverrides(const MaterialAssignmentId& materialAssignmentId) const = 0;
            //! Set Model UV overrides for a specific material assignment
            virtual void SetModelUvOverrides(
                const MaterialAssignmentId& materialAssignmentId, const AZ::RPI::MaterialModelUvOverrideMap& modelUvOverrides) = 0;
            //! Get Model UV overrides for a specific material assignment
            virtual AZ::RPI::MaterialModelUvOverrideMap GetModelUvOverrides(const MaterialAssignmentId& materialAssignmentId) const = 0;
        };
        using MaterialComponentRequestBus = EBus<MaterialComponentRequests>;

        //! MaterialComponent can send out notifications on the MaterialComponentNotificationBus 
        class MaterialComponentNotifications
            : public ComponentBus
        {
        public:

            //! This message is sent every time a material or property update affects UI.
            virtual void OnMaterialsEdited() {}

            //! This message is sent when one or more material property changes have been applied, at most once per frame.
            virtual void OnMaterialsUpdated([[maybe_unused]] const MaterialAssignmentMap& materials) {}

            //! This message is sent when the component has created the material instance to be used for rendering.
            virtual void OnMaterialInstanceCreated([[maybe_unused]] const MaterialAssignment& materialAssignment) {}
        };
        using MaterialComponentNotificationBus = EBus<MaterialComponentNotifications>;

        //! Bus for retrieving information about materials embedded in or used by a source, like a model
        class MaterialReceiverRequests
            : public ComponentBus
        {
        public:
            //! Get material assignment id matching lod and label substring
            virtual MaterialAssignmentId FindMaterialAssignmentId(
                const MaterialAssignmentLodIndex lod, const AZStd::string& label) const = 0;
                
            //! Returns the list of all ModelMaterialSlot's for the model, across all LODs.
            virtual RPI::ModelMaterialSlotMap GetModelMaterialSlots() const = 0;

            //! Returns the available, overridable material slots and the default assigned materials
            virtual MaterialAssignmentMap GetMaterialAssignments() const = 0;

            virtual AZStd::unordered_set<AZ::Name> GetModelUvNames() const = 0;
        };
        using MaterialReceiverRequestBus = EBus<MaterialReceiverRequests>;

        //! Bus for notifying when materials embedded in or used by a source, like a model, change
        class MaterialReceiverNotifications
            : public ComponentBus
        {
        public:
            //! Notification that overridable material slots are available or have changed
            virtual void OnMaterialAssignmentsChanged() = 0;
        };
        using MaterialReceiverNotificationBus = EBus<MaterialReceiverNotifications>;

    } // namespace Render
} // namespace AZ
