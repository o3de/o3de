/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorComponentBase.h"
#include "EditorLayerComponentBus.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

class QCryptographicHash;
class QDir;

namespace AzToolsFramework
{
    namespace Components
    {
        class TransformComponent;
    }

    namespace Layers
    {
        /// Properties on this class will save to the layer file. Properties on the component
        /// will save to the level. Tend toward saving properties here to minimize how often
        /// users need to interact with the level file.
        class LayerProperties
        {
        public:
            AZ_CLASS_ALLOCATOR(LayerProperties, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(LayerProperties, "{FA61BD6E-769D-4856-BFB5-B535E0FC57B4}");

            enum class SaveFormat 
            {
                Xml,
                Binary
            };

            static void Reflect(AZ::ReflectContext* context);

            void Clear()
            {
                m_saveAsBinary = false;
                m_color = AZ::Color::CreateOne();
                m_isLayerVisible = true;
            }

            // The color to display the layer in the outliner.
            AZ::Color m_color = AZ::Color::CreateOne();
            // Default to text files, so the save history is easier to understand in source control.
            // This attribute only effects writing layers, and is safe to store here instead of on the component.
            // When reading files off disk, Open 3D Engine figures out the correct format automatically.
            bool m_saveAsBinary = false;

            // The layer entity needs to be invisible to all other systems, so they don't show up in the viewport.
            // Visibility for layers can be toggled in the outliner, though. When layers are made invisible in the outliner,
            // it should provide an override to all children, making them invisible.
            bool m_isLayerVisible = true;
        };

        /// Helper class for saving and loading the contents of layers.
        class EditorLayer
        {
        public:
            AZ_CLASS_ALLOCATOR(EditorLayer, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(EditorLayer, "{82C661FE-617C-471D-98D5-289570137714}");

            EditorLayer() = default;
            EditorLayer(const EditorLayer&) = delete;
            EditorLayer(EditorLayer&&) = delete;
            EditorLayer& operator=(const EditorLayer&) = delete;
            EditorLayer& operator=(EditorLayer&&) = delete;
            ~EditorLayer();

            static void Reflect(AZ::ReflectContext* context);

            EntityList m_layerEntities;
            AZ::SliceComponent::SliceAssetToSliceInstancePtrs m_sliceAssetsToSliceInstances;
            LayerProperties m_layerProperties;

            // Makes it easier to recover a lost layer if the layer's entity ID was known.
            AZ::EntityId m_layerEntityId;
        };

        /// This editor component marks an entity as a layer entity.
        /// Layer entities allow the level to be split into multiple files,
        /// so content creators can work on the same level at the same time, without conflict.
        class EditorLayerComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorLayerComponentRequestBus::Handler
            , public EditorLayerInfoRequestsBus::Handler
            , public AZ::TransformNotificationBus::Handler
        {
        public:
            AZ_EDITOR_COMPONENT(EditorLayerComponent, "{976E05F0-FAC7-43B6-B621-66108AE73FD4}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            ~EditorLayerComponent();

            ////////////////////////////////////////////////////////////////////
            // EditorLayerComponentRequestBus::Handler
            LayerResult WriteLayerAndGetEntities(
                QString levelAbsoluteFolder,
                EntityList& entityList,
                AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances) override;

            void RestoreEditorData() override;

            bool HasLayer() override { return true; }

            void UpdateLayerNameConflictMapping(AZStd::unordered_map<AZStd::string, int>& nameConflictsMapping) override;

            QColor GetLayerColor() override;

            AZ::Color GetColorPropertyValue() override;

            bool IsSaveFormatBinary() override;

            bool IsLayerNameValid() override;

            AZ::Outcome<AZStd::string, AZStd::string> GetLayerBaseFileName() override;
            AZ::Outcome<AZStd::string, AZStd::string> GetLayerFullFileName() override;
            AZ::Outcome<AZStd::string, AZStd::string> GetLayerFullFilePath(const QString& levelAbsoluteFolder) override;

            void SetLayerChildrenVisibility(bool visible) override;
            bool AreLayerChildrenVisible() override { return m_editableLayerProperties.m_isLayerVisible; }

            bool HasUnsavedChanges() override;
            void MarkLayerWithUnsavedChanges() override;
            void SetOverwriteFlag(bool set) override;
            bool GetOverwriteFlag() override { return m_overwriteCheck; }

            bool DoesLayerFileExistOnDisk(const QString& levelAbsoluteFolder) override;

            bool GatherSaveDependencies(
                AZStd::unordered_set<AZ::EntityId>& allLayersToSave,
                bool& mustSaveLevel) override;

            void AddLayerSaveDependency(const AZ::EntityId& layerSaveDependency) override;

            void AddLevelSaveDependency() override;
            ////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////
            // EditorLayerInfoRequestsBus::Handler
            void GatherLayerEntitiesWithName(const AZStd::string& layerName, EntityIdSet& layerEntities) override;
            ////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AZ::TransformNotificationBus::Handler
            // Layers cannot have their parent changed.
            void CanParentChange(bool &parentCanChange, AZ::EntityId oldParent, AZ::EntityId newParent) override;
            //////////////////////////////////////////////////////////////////////////
            
            // This is not an ebus call because the layer entity is not yet fully setup at the point that entities in the layer are loaded.
            LayerResult ReadLayer(
                QString levelPakFile,
                AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
                AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities);

            // When reading in layers, GetLayerBaseFileName cannot be called because entities aren't yet responding to ebuses.
            // The system reading in the layers needs to know the file names to look for them, so used the last saved file name.
            AZStd::string GetCachedLayerBaseFileName() { return m_layerFileName; }

            // The Layer entities need to be added to the before they are cleaned up.
            void CleanupLoadedLayer();

            // Sets the layer to the passed in color.
            void SetLayerColor(AZ::Color newColor) override { m_editableLayerProperties.m_color = newColor; }

            // Sets the save format.
            void SetSaveFormat(LayerProperties::SaveFormat saveFormat);

            /**
            * Creates the right click context menu for layers in the asset browser.
            * \param menu The menu to parent this to.
            * \param fullFilePath The full file path of the slice.
            * \param levelPath The path to the level file.
            */
            static void CreateLayerAssetContextMenu(QMenu* menu, const AZStd::string& fullFilePath, QString levelPath);

            /**
            * Attempts to recover the passed in layer file. This is used when content creators
            * accidentally step on each other's toes, and a layer entity gets deleted in a scene.
            * \param fullFilePath The full path to the layer to attempt to recover.
            */
            static void RecoverLayer(const AZStd::string& fullFilePath);

            /**
            * Attempts to recover the passed in EditorLayer object. Most call sites should use the RecoverLayer
            * that takes in a path to the layer file.
            * \param editorLayer The layer object to recover.
            * \param newLayerName The name to give the layer entity when it's recovered.
            * \param layerParentId The parent for the new layer. Passed in the invalid entity ID for loose layers with no parents.
            * \return A success if the layer was recovered, an error if it was not.
            */
            static LayerResult RecoverEditorLayer(
                const AZStd::shared_ptr<Layers::EditorLayer> editorLayer,
                const AZStd::string& newLayerName,
                const AZ::EntityId& layerParentId);

            // Returns the file extension (without a .) used by the layer system.
            static const char* GetLayerExtension() { return "layer"; }

            // Returns the file extension (with a .) used by the layer system.
            static AZStd::string GetLayerExtensionWithDot() { return AZStd::string::format(".%s", GetLayerExtension()); }

            /**
            * Creates a layer entity with the given name, and returns the EntityId of the layer entity.
            * \param name Name to use for the new layer.
            * \param layerColor color of the layer in editor.
            * \param saveAsBinary save format for layer, xml or binary.
            * \param optionalEntityId optional param to specify entity ID for created layer.
            * \return a valid entity ID on successful entity creation
            */
            static AZ::EntityId CreateLayerEntity(const AZStd::string& name, const AZ::Color& layerColor, const LayerProperties::SaveFormat& saveAsBinary=LayerProperties::SaveFormat::Xml, const AZ::EntityId& optionalEntityId=AZ::EntityId());

            /**
            * Helper function for script reflection friendly override.
            * Creates a layer entity with the given name, default color
            * and xml save format. Returns the EntityId of the layer.
            * \param name Name to use for the new layer
            * \return a valid entity ID on successful entity creation
            */
            static AZ::EntityId CreateLayerEntityFromName(const AZStd::string& name);


            // Returns the format the save is currently set to.
            AzToolsFramework::Layers::LayerProperties::SaveFormat GetSaveFormat();

        protected:
            ////////////////////////////////////////////////////////////////////
            // AZ::Entity
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////
            LayerResult PrepareLayerForSaving(
                EditorLayer& layer,
                EntityList& entityList,
                AZ::SliceComponent::SliceReferenceToInstancePtrs& layerInstances);
            LayerResult WriteLayerToStream(
                const EditorLayer& layer,
                AZ::IO::ByteContainerStream<AZStd::vector<char> >& entitySaveStream);
            LayerResult WriteLayerStreamToDisk(
                QString levelAbsoluteFolder,
                const AZ::IO::ByteContainerStream<AZStd::vector<char> >& entitySaveStream);
            LayerResult CreateDirectoryAtPath(const QString& path);

            LayerResult PopulateFromLoadedLayerData(
                const EditorLayer& loadedLayer,
                AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
                AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities);

            void AddUniqueEntitiesAndInstancesFromEditorLayer(
                const EditorLayer& loadedLayer,
                AZ::SliceComponent::SliceAssetToSliceInstancePtrs& sliceInstances,
                AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& uniqueEntities);

            QString GetLayerDirectory() const { return "Layers"; }
            QString GetLayerTempDirectory() const { return "Layers_Temp"; }
            QString GetLayerTempExtension() const { return "layer_temp"; }

            // Try writing a temp file a few times, in case the initial temp file isn't writeable for some reason.
            int GetMaxTempFileWriteAttempts() const { return 5; }

            /**
            * Verifies that the passed in layer path is safe to begin a recovery attempt.
            * If so, also populates necessary info to recover this layer.
            * Also retrieves the name of the layer to use when creating the entity.
            * Checks if the ancestry of the layer is available. If not, prompts the user
            * if they want it created, or if they want to bail out of the operation.
            * \param fullFilePath The full path to the layer file.
            * \param newLayerName An output paramater that will be populated with the entity name for this layer.
            * \param layerParentId An output parameter that will be populated with the ID of the parent of the layer, if it has one.
            */
            static bool CanAttemptToRecoverLayerAndGetLayerInfo(
                const AZStd::string& fullFilePath,
                AZStd::string& newLayerName,
                AZ::EntityId& layerParentId);

            /**
            * Returns true if the passed in layer is safe to spawn in the scene, false if not.
            * This checks for duplicate entity IDs.
            * \param loadedLayer The layer to validate.
            * \param rootSlice The root slice for the level.
            * \return A success if the layer is safe to recover, otherwise an error with a message if not.
            */
            static LayerResult IsLayerDataSafeToRecover(const AZStd::shared_ptr<Layers::EditorLayer>  loadedLayer, AZ::SliceComponent& rootSlice);

            /**
            * Creates a layer entity with the given ancestor name.
            * This is not parsed for deeper ancestry, if you give this "Grandparent.Parent" and have an entity
            * in your scene already named "Grandparent", this won't create "Parent" as a child of "Grandparent".
            * \param nearestLayerAncestorName the full ancestor name.
            * \return The entity ID fo the missing ancestor.
            */
            static AZ::EntityId CreateMissingLayerAncestors(const AZStd::string& nearestLayerAcenstorName);

            /**
            * Checks if the given entity ID was already discovered. Updates the discovery list with the passed in ID.
            * Reports an error if the entity ID was already found. Returns true if it was, false if not.
            * This is used when recovering layers, to search for potential entity ID collisions, and cancel the recovery
            * if there is a collision.
            * \param discoveredIds A list of entity already found.
            * \param newEntity The entity to update the list with, and report an error if it was already found.
            * \return A success if the entity ID is safe to recover, a failure if not.
            */
            static LayerResult UpdateListOfDiscoveredEntityIds(AZStd::unordered_set<AZ::EntityId> &discoveredIds, const AZ::EntityId& newEntity);

            // Layer file names generated on each save.
            LayerResult GenerateLayerFileName();

            LayerResult GetFileHash(QString filePath, QCryptographicHash& hash);

            LayerResult GenerateCleanupFailureWarningResult(QString message, const LayerResult* currentFailure);
            LayerResult CleanupTempFileAndFolder(QString tempFile, const QDir& layerTempFolder, const LayerResult* currentFailure);
            LayerResult CleanupTempFolder(const QDir& layerTempFolder, const LayerResult* currentFailure);

            void GatherAllNonLayerNonSliceDescendants(
                EntityList& entityList,
                EditorLayer& layer,
                const AZStd::unordered_map<AZ::EntityId, AZ::Entity*>& entityIdsToEntityPtrs,
                AzToolsFramework::Components::TransformComponent& transformComponent) const;

            void SetUnsavedChanges(bool unsavedChanges);

            // Gets the marker used to include ancestry in layer files created on disk.
            // Given a layer hierarchy Grandparent, Parent, and Child, and a separator ".", the separator
            // will be used to generate a file named "Grandparent.Parent.Child".
            static AZStd::string GetLayerSeparator() { return "."; }

            EditorLayer* m_loadedLayer = nullptr;
            AZStd::string m_layerFileName;

            // Open 3D Engine's serialization system requires everything in the editor to have a serialized to disk counterpart.
            // Layers have their data split into two categories: Stuff that should save to the layer file, and stuff that should
            // save to the layer component in the level. To allow the layer component to edit the data that goes in the layer file,
            // a placeholder value is serialized. This is only used at edit time, and is copied and cleared during serialization.
            LayerProperties m_editableLayerProperties;
            LayerProperties m_cachedLayerProperties;

            bool m_hasUnsavedChanges = false;

            // Users can save individual layers. This can cause problems if an entity is moved between layers
            // and only one of those two layers is saved, it will duplicate the entity on the next load.
            // This tracks other layers that need to be saved when this layer is saved.
            AZStd::unordered_set<AZ::EntityId> m_otherLayersToSave;
            // When a new layer is created, mark it as needing to save the level the next time it saves. Existing layers
            // will set this flag to false when they load.
            bool m_mustSaveLevelWhenLayerSaves = true;
            // Setting this flag to true will ask for overwrite confirmation when saving the layer, if a layer with the same name exists on
            // disk. If default value isn't false, this will be set to true for all layers when the level is reset(for example, when saving
            // a slice), thus prompting a confirmation on further level saves for already loaded layers.
            bool m_overwriteCheck = false;
        };
    }
}
