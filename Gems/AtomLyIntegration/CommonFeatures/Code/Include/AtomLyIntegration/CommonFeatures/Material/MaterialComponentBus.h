/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignment.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    namespace Render
    {
        //! MaterialComponentRequestBus provides an interface to request operations on a MaterialComponent
        class MaterialComponentRequests : public ComponentBus
        {
        public:
            //! Get a map representing the default layout and values for all material assignment slots on the source model or object
            virtual MaterialAssignmentMap GetDefaultMaterialMap() const = 0;

            // O3DE_DEPRECATION_NOTICE(GHI-16783) Function is being replaced by GetDefaultMaterialMap
            virtual MaterialAssignmentMap GetDefautMaterialMap() const
            {
                return GetDefaultMaterialMap();
            }

            //! Search for a material assignment ID matching the lod and label parameters
            //! @param lod Index of the LOD to be searched for the material assignment ID. -1 is used to search the default material and
            //! model material slots.
            //! @param label Substring used to look up a material assignment ID with a matching label.
            //! @returns The corresponding material assignment ID is found, otherwise DefaultMaterialAssignmentId.
            virtual MaterialAssignmentId FindMaterialAssignmentId(
                const MaterialAssignmentLodIndex lod, const AZStd::string& label) const = 0;

            //! Get the material asset associated with the source model or object prior to overrides being applied.
            //! @param materialAssignmentId ID of material assignment slot for which the information is being requested.
            //! @returns Default asset ID associated with the material assignment ID, otherwise an invalid asset ID.
            virtual AZ::Data::AssetId GetDefaultMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const = 0;

            //! @param materialAssignmentId ID of material assignment slot for which the information is being requested.
            //! @returns Asset<MaterialAsset>::IsReady() state of the material asset associated with the source model
            //!          or object prior to overrides being applied.
            virtual bool IsDefaultMaterialAssetReady(const MaterialAssignmentId& materialAssignmentId) const = 0;

            //! Get the material asset associated with the source model or object prior to overrides being applied.
            //! @param materialAssignmentId ID of material assignment slot for which the information is being requested.
            //! @returns String corresponding to the display name of the material slot.
            virtual AZStd::string GetMaterialLabel(const MaterialAssignmentId& materialAssignmentId) const = 0;

            //! Replaces all material and property overrides with whatever is contained in the provided map.
            //! @param materials Map of material assignment data including materials, property overrides, and other parameters.
            virtual void SetMaterialMap(const MaterialAssignmentMap& materials) = 0;

            //! Returns all materials and properties used by the material component.
            //! @returns Map of material assigned data including materials, property overrides, and other parameters.
            virtual const MaterialAssignmentMap& GetMaterialMap() const = 0;

            //! Similar as above, but returns a deep copy of all materials.
            //! This "Copy" function is useful for Lua because GetMaterialMap()
            //! returns a reference and Lua treats it a as a reference too.
            //! Making further chages to the material component, for example by calling SetMaterialAssetId()
            //! would indirectly affect the MaterialAssignmentMap that was returned by reference.
            //! To avoid this scenario, a Lua script can call this function to get an actual copy that remains
            //! unaffected by calling functions like SetMaterialAssetId().
            virtual MaterialAssignmentMap GetMaterialMapCopy() const = 0;

            //! Clears all overridden materials and properties from the material component.
            virtual void ClearMaterialMap() = 0;

            //! Clears all overrides from the material component not associated with a specific LOD.
            virtual void ClearMaterialsOnModelSlots() = 0;

            //! Clears all overrides from the material component associated with a specific LOD.
            virtual void ClearMaterialsOnLodSlots() = 0;

            //! Clear all material overrides from the material component mapped to material assignment IDs that do not match the current
            //! material layout. This is usually used for clearing materials leftover between model changes or moving the material component
            //! from one entity to another.
            virtual void ClearMaterialsOnInvalidSlots() = 0;

            //! Clears all material overrides referencing material assets that cant' be located.
            virtual void ClearMaterialsWithMissingAssets() = 0;

            //! Updates all material overrides referencing material assets that can't be located to instead point to a default material
            //! asset.
            virtual void RepairMaterialsWithMissingAssets() = 0;

            //! Remaps material property overrides that have been renamed since they were assigned.
            //! @return the number of properties that were updated
            virtual uint32_t RepairMaterialsWithRenamedProperties() = 0;

            //! Convenience function to set the overridden material asset on the default material slot.
            //! @param materialAssetId Material asset that will be assigned to the default material slot.
            virtual void SetMaterialAssetIdOnDefaultSlot(const AZ::Data::AssetId& materialAssetId) = 0;

            //! Convenience function to get the current material asset on the default material slot.
            virtual const AZ::Data::AssetId GetMaterialAssetIdOnDefaultSlot() const = 0;

            //! Convenience function to clear be over written material has set on the default material slot.
            virtual void ClearMaterialAssetIdOnDefaultSlot() = 0;

            //! Assign a material asset to the slot corresponding to material assignment ID
            //! @param materialAssignmentId ID of material slot that the material will be assigned to.
            //! @param materialAssetId Material asset that will be assigned to the material slot.
            virtual void SetMaterialAssetId(const MaterialAssignmentId& materialAssignmentId, const AZ::Data::AssetId& materialAssetId) = 0;

            //! Retrieve the material asset associated with the material assignment ID
            //! @param materialAssignmentId ID of material slot.
            //! @returns The current material asset ID is found, otherwise invalid asset ID .
            virtual AZ::Data::AssetId GetMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) const = 0;

            //! @param materialAssignmentId ID of material assignment slot for which the information is being requested.
            //! @returns Asset<MaterialAsset>::IsReady() state of the material asset associated with the material assignment ID.
            virtual bool IsMaterialAssetReady(const MaterialAssignmentId& materialAssignmentId) const = 0;

            //! Removes the material asset associated with the material assignment ID
            //! @param materialAssignmentId ID of material slot.
            virtual void ClearMaterialAssetId(const MaterialAssignmentId& materialAssignmentId) = 0;

            //! Check if the material slot contains an explicit material asset override
            //! @param materialAssignmentId ID of material slot.
            //! @returns true if a valid material asset has been assigned.
            virtual bool IsMaterialAssetIdOverridden(const MaterialAssignmentId& materialAssignmentId) const = 0;

            //! Check if the material slot contains any overridden property values
            //! @param materialAssignmentId ID of material slot.
            //! @returns true if any property values have been overridden on this material slot.
            virtual bool HasPropertiesOverridden(const MaterialAssignmentId& materialAssignmentId) const = 0;

            //! Set a material property override value wrapped by an AZStd::any
            //! @param materialAssignmentId ID of material slot.
            //! @param propertyName Name of the property being assigned.
            //! @param value Value to be assigned to the specified property.
            virtual void SetPropertyValue(
                const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const AZStd::any& value) = 0;

            //! Get the current value of a material property wrapped by an AZStd::any
            //! @param materialAssignmentId ID of material slot.
            //! @param propertyName Name of the property being requested.
            //! @returns Value of the property is located, otherwise an empty AZStd::any.
            virtual AZStd::any GetPropertyValue(
                const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const = 0;

            //! Clear any property override associated with the material assignment ID and property name.
            //! @param materialAssignmentId ID of material slot.
            //! @param propertyName Name of the property being cleared.
            virtual void ClearPropertyValue(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) = 0;

            //! Clear all property overrides associated with the material assignment ID.
            //! @param materialAssignmentId ID of material slot.
            virtual void ClearPropertyValues(const MaterialAssignmentId& materialAssignmentId) = 0;

            //! Clear all property overrides for every material in the material component.
            virtual void ClearAllPropertyValues() = 0;

            //! Replaces all property overrides associated with the material assignment ID.
            //! @param materialAssignmentId ID of material slot.
            //! @param propertyOverrides Map of all property values being assigned.
            virtual void SetPropertyValues(
                const MaterialAssignmentId& materialAssignmentId, const MaterialPropertyOverrideMap& propertyOverrides) = 0;

            //! Retrieves a map of all property values associated with the material assignment ID.
            //! @param materialAssignmentId ID of material slot.
            //! @returns Map of all property values.
            virtual MaterialPropertyOverrideMap GetPropertyValues(const MaterialAssignmentId& materialAssignmentId) const = 0;

            //! Set Model UV overrides for a specific material assignment
            //! @param materialAssignmentId ID of material slot.
            //! @param modelUvOverrides Map of remapped UV channels.
            virtual void SetModelUvOverrides(
                const MaterialAssignmentId& materialAssignmentId, const AZ::RPI::MaterialModelUvOverrideMap& modelUvOverrides) = 0;

            //! Get Model UV overrides for a specific material assignment
            //! @param materialAssignmentId ID of material slot.
            //! @returns Map of remapped UV channels.
            virtual AZ::RPI::MaterialModelUvOverrideMap GetModelUvOverrides(const MaterialAssignmentId& materialAssignmentId) const = 0;

            //! Set material property override value with a specific type
            template<typename T>
            void SetPropertyValueT(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName, const T& value)
            {
                SetPropertyValue(materialAssignmentId, propertyName, AZStd::any(value));
            }

            //! Get material property override value with a specific type
            template<typename T>
            T GetPropertyValueT(const MaterialAssignmentId& materialAssignmentId, const AZStd::string& propertyName) const
            {
                const AZStd::any& value = GetPropertyValue(materialAssignmentId, propertyName);
                return !value.empty() && value.is<T>() ? AZStd::any_cast<T>(value) : T{};
            }
        };
        using MaterialComponentRequestBus = EBus<MaterialComponentRequests>;

        //! MaterialComponentNotificationBus notifications are sent whenever the state of the material component changes in a way that would
        //! affect tools or consumers.
        class MaterialComponentNotifications : public ComponentBus
        {
        public:
            //! This notification is sent whenever material changes are made that need to be reflected in the UI.
            virtual void OnMaterialsEdited(){};

            //! This notification is forwarded from the consumer whenever material slot layout or default values are changed.
            virtual void OnMaterialSlotLayoutChanged(){};

            //! This notification is sent once per tick whenever new material instances are created.
            virtual void OnMaterialsCreated([[maybe_unused]] const MaterialAssignmentMap& materials){};

            //! This notification is sent whenever the material component has completed adding or removing a batch of instances for the
            //! material consumer to apply. The notification is not sent for individual property changes because the material component
            //! applies property changes directly to the material instances it manages.
            //! The notification should only be sent once per batch of changes, after material assets have been loaded, reloaded, or if a
            //! material property change resulted in creating a new, unique instance. Other functions can be invoked through editing or
            //! script that might also result in this notification being sent.
            virtual void OnMaterialsUpdated([[maybe_unused]] const MaterialAssignmentMap& materials){};

            //! This notification is sent once per tick whenever the material component changes any material properties.
            virtual void OnMaterialPropertiesUpdated([[maybe_unused]] const MaterialAssignmentMap& materials){};
        };
        using MaterialComponentNotificationBus = EBus<MaterialComponentNotifications>;

        //! Any component that wishes to consume materials from the material component and interface with its tools must implement this bus.
        //! These functions provide the material component with the number, layout, default values, labels, and other information about
        //! available material slots.
        //! For example, the mesh and actor components implement the functions on this bus using data provided by their model assets. The
        //! number of available LODs and material slots can change from one model asset to the next.
        //! Components with a fixed LOD and material slot layout, like decals, might return simple constants for all of the functions or
        //! simply use the default material slot.
        class MaterialConsumerRequests : public ComponentBus
        {
        public:
            //! Search for a material assignment id matching lod and label substring
            //! @param lod The index of the LOD For the requested material slot. LOD values less than zero correspond to the default
            //! material slot or material slots without LODs.
            //! @param label A substring that will be used to search for the name of a material slot.
            virtual MaterialAssignmentId FindMaterialAssignmentId(
                const MaterialAssignmentLodIndex lod, const AZStd::string& label) const = 0;

            //! Returns a map of all material slot labels
            virtual MaterialAssignmentLabelMap GetMaterialLabels() const = 0;

            //! Returns the available material slots and default assigned materials
            virtual MaterialAssignmentMap GetDefaultMaterialMap() const = 0;

            // O3DE_DEPRECATION_NOTICE(GHI-16783) Function is being replaced by GetDefaultMaterialMap
            virtual MaterialAssignmentMap GetDefautMaterialMap() const
            {
                return GetDefaultMaterialMap();
            }

            //! Returns a map of UV Overridable UV channel names
            virtual AZStd::unordered_set<AZ::Name> GetModelUvNames() const = 0;
        };
        using MaterialConsumerRequestBus = EBus<MaterialConsumerRequests>;

        //! Notifications sent when the state of the material consumer changes in a way that affects the material component and tools
        class MaterialConsumerNotifications : public ComponentBus
        {
        public:
            //! This notification should be sent whenever the material consumer has updated its map of expected materials or their default
            //! values. For example, the mesh and actor components send this notification after their model assets have loaded. The material
            //! component will handle the notification and use MaterialConsumerRequestBus to enumerate all of the requested materials,
            //! update default values, and repopulate the UI.
            virtual void OnMaterialAssignmentSlotsChanged(){};
        };
        using MaterialConsumerNotificationBus = EBus<MaterialConsumerNotifications>;

    } // namespace Render
} // namespace AZ
