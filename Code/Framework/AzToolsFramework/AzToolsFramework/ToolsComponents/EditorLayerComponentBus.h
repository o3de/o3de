/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QColor>
#include <QString>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Math/Color.h>
#include "LayerResult.h"

namespace AzToolsFramework
{
    namespace Layers
    {
        /**
         * This bus can be used to check if an entity is a layer, and to retrieve information about that layer.
         */
        class EditorLayerComponentRequests
            : public AZ::ComponentBus
        {
        public:
            /*
             * It's expensive to check if an entity has a component, so this ebus call
             * is used to check if an entity has a layer.
             */
            virtual bool HasLayer() = 0;

            /**
             * Requests this layer to restore its cached editor data. This information is cached and restored as part of saving,
             * so that the data can show up in the inspector to edit the layer, but does not save to the layer entity in the level.
             * The data instead saves to the layer file.
             */
            virtual void RestoreEditorData() = 0;

            /**
             * Writes this layer to disk, and gathers entities and instances tracked by this layer, so they can be removed from
             * the level slice.
             * \param levelAbsoluteFolder The path to the level this layer is saved in.
             * \param entityList An output parameter that is populated with entities tracked by this layer.
             * \param layerInstances An output parameter that is populated with instances tracked by this layer.
             */
            virtual LayerResult WriteLayerAndGetEntities(
                QString levelAbsoluteFolder,
                AZStd::vector<AZ::Entity*>& entityList,
                AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances) = 0;

            /**
             * Populated the passed in map with the number of layers (map value) that have the same name (map key).
             * \param nameConflictMapping An output parameter populated with each layer's file name as the key and the number of
             *                            layers with that name as the value.
             */
            virtual void UpdateLayerNameConflictMapping(AZStd::unordered_map<AZStd::string, int>& nameConflictMapping) = 0;

            /**
            * Sets the color of the layer.
            */
            virtual void SetLayerColor(AZ::Color newColor) = 0;

            /**
             * Retrieves the color of the layer.
             */
            virtual QColor GetLayerColor() = 0;

            /**
            * Retrieve the layer's color property value in it's native format
            */
            virtual AZ::Color GetColorPropertyValue() = 0;

            /**
             * Retrieves the save format.
             */
            virtual bool IsSaveFormatBinary() = 0;

            /**
             * Returns true if the layer name is valid for saving to disk.
             */
            virtual bool IsLayerNameValid() = 0;

            /*
             * If successful, returns the layer's file name without an extension. If not successful, returns an error message.
             */
            virtual AZ::Outcome<AZStd::string, AZStd::string> GetLayerBaseFileName() = 0;

            /**
             *If successful, returns the layer's file name with an extension. If not successful, returns an error message.
             */
            virtual AZ::Outcome<AZStd::string, AZStd::string> GetLayerFullFileName() = 0;

            /**
             *If successful, returns the layer's full file path. If not successful, returns an error message.
             */
            virtual AZ::Outcome<AZStd::string, AZStd::string> GetLayerFullFilePath(const QString& levelAbsoluteFolder) = 0;

            /**
             * Returns true if this layer has unsaved changes, false if not.
             */
            virtual bool HasUnsavedChanges() = 0;

            /**
             * Tells the layer to mark itself as having unsaved changes.
             */
            virtual void MarkLayerWithUnsavedChanges() = 0;

            /**
            * Tells the layer to mark itself as needing overwrite check.
            * When a new layer is created, mark it as requiring an overwrite check, this will also be set when
            * rename is called and reset when the layer is succesfully saved or just loaded
            */
            virtual void SetOverwriteFlag(bool set) = 0;
            /**
            * Returns the value of the overwrite check.
            */
            virtual bool GetOverwriteFlag() = 0;

            /**
             * Returns true if the layer is saved on disk.
             */
            virtual bool DoesLayerFileExistOnDisk(const QString& levelAbsoluteFolder) = 0;

            /**
             * Layers themselves are never visible, and need to be set not visible in other systems to function as designed.
             * However, layers still need to be able to toggle on and off a visibility state, so that they can apply an override
             * to their children.
             * Returns if children of this layer should be visible or not.
             */
            virtual bool AreLayerChildrenVisible() = 0;

            /**
             * Requests the layer to mark its children as visible or not.
             * \param visible True to allow children to be visible, false to force children to not be visible.
             */
            virtual void SetLayerChildrenVisibility(bool visible) = 0;

            /**
             * Collects what else must be saved to safely save this layer.
             * \param allLayersToSave A set of layer entity IDs that will be saved,
             *          this is updated with the current layer's dependencies.
             * \param mustSaveLevel Set to true if the current level must be saved
             *          to safely save this layer.
             */
            virtual bool GatherSaveDependencies(
                AZStd::unordered_set<AZ::EntityId>& allLayersToSave,
                bool& mustSaveLevel) = 0;

            /**
             * Marks the passed in layer Entity ID as a save dependency for this layer.
             * This is necessary when an entity switches parents, to make sure that both layers are saved.
             * \param layerSaveDependency The layer that must be saved when this layer is saved.
             */
            virtual void AddLayerSaveDependency(const AZ::EntityId& layerSaveDependency) = 0;

            /**
             * Marks the level as a save dependency for this layer.
             * This is necessary when an entity switches parents, to make sure that the level is saved with this layer.
             */
            virtual void AddLevelSaveDependency() = 0;
        };
        using EditorLayerComponentRequestBus = AZ::EBus<EditorLayerComponentRequests>;

        /**
         * This bus is a single bus with multiple listeners. All layers listen in on this bus,
         * it's used for checking for layers that share the same file name.
         */
        class EditorLayerInfoRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus
            
            /**
             * Gathers all layers that have the given layer filename. Used to find duplicate layers.
             * \param layerName The name of the layer to search for.
             * \param layerEntities An output parameter containing all layers that have the same name.
             */
            virtual void GatherLayerEntitiesWithName(const AZStd::string& layerName, AZStd::unordered_set<AZ::EntityId>& layerEntities) = 0;
        };
        using EditorLayerInfoRequestsBus = AZ::EBus<EditorLayerInfoRequests>;

        /**
        * This is a single bus with multiple listeners, for allowing systems to listen when specific layer events occur.
        */
        class EditorLayerCreationNotification
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus

            /**
             * Called when a new layer is created. Run custom logic you need for new layers on this bus, including
             * adding components to the list of components to add to your layer, if you need custom layer components.
             * \param entityId The EntityId of the new layer entity.
             * \param componentsToAdd An output list of components to add to the entity. Gathered this way to
             *                        allow all components to be added at once, instead of deactivating and re-activating
             *                        the layer for each listener on this bus adding components.
             */
            virtual void OnNewLayerEntity(const AZ::EntityId& entityId, AZStd::vector<AZ::Component*>& componentsToAdd) = 0;
        };
        using EditorLayerCreationBus = AZ::EBus<EditorLayerCreationNotification>;

        /**
        * This is a single bus with multiple listeners, for allowing systems to listen when specific layer component events occur.
        */
        class EditorLayerComponentNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple; // multi listener
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; //single bus

            /**
             * Called when a layer component is activated on a layer entity
             * \param entityId The id of the entity whose layer component has been activated.
             */
            virtual void OnLayerComponentActivated(AZ::EntityId entityId) = 0;
            /**
             * Called when a layer component is deactivated on a layer entity
             * \param entityId The id of the entity whose layer component has been deactivated.
             */
            virtual void OnLayerComponentDeactivated(AZ::EntityId entityId) = 0;
        };
        using EditorLayerComponentNotificationBus = AZ::EBus<EditorLayerComponentNotifications>;
    }
}
