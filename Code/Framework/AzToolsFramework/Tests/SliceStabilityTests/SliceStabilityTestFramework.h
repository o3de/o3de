/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

namespace UnitTest
{
    class SliceStabilityTest
        : public ToolsApplicationFixture,
          public AzToolsFramework::AssetSystemRequestBus::Handler,
          public AzToolsFramework::EditorRequestBus::Handler,
          public AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler
    {
    public:
        //! Creates an entity within the EditorEntityContext and supplies it required components
        //! @param entityName The name the created entity will use
        //! @param entityList The created entity will be placed at the back of the provided entityList
        //! @param parentId The EntityId to be assigned as the parent of the created entity.
        //!    Defaults to an invalid id
        //! @return The EntityId of the created Entity, or an invalid id if operation failed
        AZ::EntityId CreateEditorEntity(const char* entityName, AzToolsFramework::EntityIdList& entityList, const AZ::EntityId& parentId = AZ::EntityId());

        //! Creates a new slice asset out of the provided entityList and generates the first slice instance using the provided entityList
        //! @param sliceAssetName Name of the newly created slice Asset
        //! @param entityList Created slice will be comprised of the entity hierarchy found in entityList
        //!    entities need to exist within the EditorEntityContext and those same entities will be promoted into the first slice instance of the created slice
        //! @param sliceAddress The SliceInstanceAddress of the first slice instance generated out of the entities found in entityList
        //! @return The AssetId of the newly created slice, or an invalid id if operation failed
        AZ::Data::AssetId CreateSlice(AZStd::string sliceAssetName, AzToolsFramework::EntityIdList entityList, AZ::SliceComponent::SliceInstanceAddress& sliceAddress);

        //! Pushes a set of entities to an existing slice asset generated via CreateSlice
        //! @param sliceInstanceAddress An instance of the slice being pushed to
        //!   Note: The act of pushing to a slice destroys and remakes all existing instances sliceInstanceAddress will be updated to represent the remade slice instance.
        //!   Any copies of sliceInstanceAddress from before this operation are invalid.
        //! @param entitiesToPush A list of entities to push to the slice.
        //!    If an entity in the list is already in the slice instance then it will be pushed as an updated entity
        //!    If an entity in the list is not in the slice instance then it will be pushed as an added entity
        //! @return true if the push succeeded, or false if the operation failed
        bool PushEntitiesToSlice(AZ::SliceComponent::SliceInstanceAddress& sliceInstanceAddress, const AzToolsFramework::EntityIdList& entitiesToPush);

        //! Instantiates a slice into the EditorEntityContext using an existing slice Asset created via CreateSlice
        //! @param sliceAssetId The asset id of the slice being instantiated
        //! @param entityList All newly instantiated entities will be added to the back of entityList
        //! @param parent The EntityId to be assigned as the parent of the slice instance entities.
        //!    Defaults to an invalid id
        //! @return The SliceInstanceAddress of the new slice instance, or an invalid address if operation failed
        AZ::SliceComponent::SliceInstanceAddress InstantiateEditorSlice(AZ::Data::AssetId sliceAssetId, AzToolsFramework::EntityIdList& entityList, const AZ::EntityId& parent = AZ::EntityId());

        //! Performs a reparent of entity to newParent. Handles any slice hierarchy manipulation needed
        //! @param entity The entity being reparented
        //! @param newParent The new parent in the reparent operation
        void ReparentEntity(AZ::EntityId& entity, const AZ::EntityId& newParent);

        //! Helper that searches for an entityId within a list of entities
        //! @param entityId The EntityId being searched for
        //! @param entityList the list to search in
        //! @return The Entity* whose EntityId matches entityId and is found in entityList.
        //!    Returns nullptr if not found
        static AZ::Entity* FindEntityInList(const AZ::EntityId& entityId, const AZ::SliceComponent::EntityList& entityList);

        //! Helper that searches for an entity within the EditorEntityContext
        //! @param entityId The entityId being searched for
        //! @return The Entity* whose id matches entityId and is found in the EditorEntityContext
        //!    Returns nullptr if not found
        static AZ::Entity* FindEntityInEditor(const AZ::EntityId& entityId);

        class SliceOperationValidator
        {
        public:
            SliceOperationValidator();
            ~SliceOperationValidator();
            void SetSerializeContext(AZ::SerializeContext* serializeContext);

            //! Clones the provided entities out of the EditorEntityContext and caches them for Compare operations
            //! @param entitiesToCapture List of EntityIDs that is used to search the EditorEntityContext and clone the respective Entity* into a cache for Compare operations
            //! @return Returns whether the capture was successful
            //!    Can fail if there is already a cached capture or the entities could not be found in the EditorEntityContext
            bool Capture(const AzToolsFramework::EntityIdList& entitiesToCapture);

            //! Does a DataPatch compare of the reflected fields of a captured EntityList and the Instantiated entities found in instanceToCompare
            //! @param instanceToCompare A SliceInstanceAddress whose InstantiatedContainer will be diffed against a previously captured EntityList using DataPatch
            bool Compare(const AZ::SliceComponent::SliceInstanceAddress& instanceToCompare);

            //! Resets the current capture so a new one can be made
            void Reset();
        private:

            bool SortCapture(const AzToolsFramework::EntityList& orderToMatch);

            AZ::SerializeContext* m_serializeContext;
            AZ::SliceComponent::EntityList m_entityStateCapture;
        };

        class EntityReferenceComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_EDITOR_COMPONENT(EntityReferenceComponent, "{3628F6A3-DFAD-4C1E-B9DE-EFBB1B6915C3}");

            void Init() override {}
            void Activate() override {}
            void Deactivate() override {}

            static void Reflect(AZ::ReflectContext* reflection);

            AZ::EntityId m_entityReference;
        };
        SliceOperationValidator m_validator;

    private:

        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        /*
         * SliceEditorEntityOwnershipServiceNotificationBus
         */
        void OnSliceInstantiated(const AZ::Data::AssetId& sliceAssetId, AZ::SliceComponent::SliceInstanceAddress& sliceAddress, const AzFramework::SliceInstantiationTicket& ticket) override;
        void OnSliceInstantiationFailed(const AZ::Data::AssetId& sliceAssetId, const AzFramework::SliceInstantiationTicket& ticket) override;

        /*
        * EditorRequestBus
        */
        void CreateEditorRepresentation(AZ::Entity* entity) override;
        void BrowseForAssets(AzToolsFramework::AssetBrowser::AssetSelectionModel& selection) override { AZ_UNUSED(selection); }
        int GetIconTextureIdFromEntityIconPath(const AZStd::string& entityIconPath) override { AZ_UNUSED(entityIconPath);  return 0; }
        bool DisplayHelpersVisible() override { return false; }

        /*
        * AssetSystemRequestBus
        */
        bool GetRelativeProductPathFromFullSourceOrProductPath([[maybe_unused]] const AZStd::string& fullPath, [[maybe_unused]] AZStd::string& relativeProductPath) override { return false; }
        bool GenerateRelativeSourcePath(
            [[maybe_unused]] const AZStd::string& sourcePath, [[maybe_unused]] AZStd::string& relativePath,
            [[maybe_unused]] AZStd::string& watchFolder) override { return false; }
        bool GetFullSourcePathFromRelativeProductPath([[maybe_unused]] const AZStd::string& relPath, [[maybe_unused]] AZStd::string& fullSourcePath) override { return false; }
        bool GetAssetInfoById([[maybe_unused]] const AZ::Data::AssetId& assetId, [[maybe_unused]] const AZ::Data::AssetType& assetType, [[maybe_unused]] const AZStd::string& platformName, [[maybe_unused]] AZ::Data::AssetInfo& assetInfo, [[maybe_unused]] AZStd::string& rootFilePath) override { return false; }
        bool GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) override;
        bool GetSourceInfoBySourceUUID([[maybe_unused]] const AZ::Uuid& sourceUuid, [[maybe_unused]] AZ::Data::AssetInfo& assetInfo, [[maybe_unused]] AZStd::string& watchFolder) override { return false; }
        bool GetScanFolders([[maybe_unused]] AZStd::vector<AZStd::string>& scanFolders) override { return false; }
        bool GetAssetSafeFolders([[maybe_unused]] AZStd::vector<AZStd::string>& assetSafeFolders) override { return false; }
        bool IsAssetPlatformEnabled([[maybe_unused]] const char* platform) override { return false; }
        int GetPendingAssetsForPlatform([[maybe_unused]] const char* platform) override { return -1; }
        bool GetAssetsProducedBySourceUUID([[maybe_unused]] const AZ::Uuid& sourceUuid, [[maybe_unused]] AZStd::vector<AZ::Data::AssetInfo>& productsAssetInfo) override { return false; }

        AZStd::unique_ptr<testing::NiceMock<AZ::IO::MockFileIOBase>> m_fileIOMock;
        AZ::IO::FileIOBase* m_priorFileIO = nullptr;

        AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>> m_createdSlices;
        AZ::Data::AssetId m_newSliceId;
        AzFramework::SliceInstantiationTicket m_ticket;

        static constexpr const char* m_relativeSourceAssetRoot = "Test/";
    };
}
