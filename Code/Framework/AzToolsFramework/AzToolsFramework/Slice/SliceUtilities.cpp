/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/stack.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Style.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/EditorEntitySortComponent.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/UI/Slice/SlicePushWidget.hxx>
#include <AzToolsFramework/UI/Slice/SliceRelationshipBus.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/Slice/SliceTransaction.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

#include <AzToolsFramework/Slice/SliceMetadataEntityContextBus.h>
AZ_PUSH_DISABLE_WARNING(4251 4244, "-Wunknown-warning-option") // 4251: class '...' needs to have dll-interface to be used by clients of class '...'
                                                               // 4244: 'argument': conversion from 'int' to 'float', possible loss of data
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtWidgets/QWidgetAction>
#include <QtWidgets/QMenu>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QErrorMessage>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QThread>
#include <QDialogButtonBox>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    namespace SliceUtilities
    {

        namespace Internal {

            using SliceInstanceList = AZStd::vector<const AZ::SliceComponent::SliceInstance*>;

            /**
            * Gets the path to the icon used for representing the inability to save.
            * \return The icon path.
            */
            QString GetNoSaveableChangesIconPath() { return ":/PropertyEditor/Resources/no_save_available_icon.png"; }

            /**
            * Gets the default width of rows representing slices in menus.
            */
            AZ::u32 GetSliceItemDefaultWidth() { return 200; }


            enum class SliceSaveResult
            {
                Continue,
                Retry,
                Cancel
            };
            /**
            * Checks if the passed in slice path is a valid place to save assets for the current project.
            * \param activeWindow the Qt widget to parent any warning popups to.
            * \param slicePath the path to verify is a safe place to save a slice.
            * param retrySavePath an output parameter of a path to retry the save at, so the user doesn't have to find it.
            * \return Continue if the save is valid, Retry if the user wants to try again, Cancel if the user wants to stop.
            */
            SliceSaveResult IsSlicePathValidForAssets(QWidget* activeWindow, QString slicePath, AZStd::string &retrySavePath);

            /**
            * Convenience function for retrieving the specified entity's chain of ancestors and their associated assets along the slice data hierarchy
            * \param entityId - ID of an entity in a live slice instance
            * \param ancestors - [out] Output list of ancestors
            */
            void GetSliceEntityAncestors(const AZ::EntityId& entityId, AZ::SliceComponent::EntityAncestorList& ancestors);

            /**
            * Convenience function for retrieving the specified entity's chain of ancestors, specifically subslices
            * \param entityId - ID of an entity in a live slice instance
            * \param ancestors - [out] Output list of ancestors
            */
            void GetSubsliceEntityAncestors(const AZ::EntityId& entityId, AZ::SliceComponent::EntityAncestorList& ancestors);

            /**
             * Convenience function for identifying the entities and subslices requiring detachment in order to perform a NonTrivialReparent
             * \param rootEntity The root entity of the hierarchy being reparented
             * \param subslicesToDetach List to be filled with the subslices requiring detachment in order to perform a NonTrivialReparent
             * \param entitiesToDetach List to be filled with the entities requiring detachment in order to perform a NonTrivialReparent
            */
            void PartitionEntityHierarchyForNonTrivialReparent(const AZ::EntityId& rootEntity,
                AZ::SliceComponent::SliceInstanceEntityIdRemapList& subslicesToDetach,
                AzToolsFramework::EntityIdList& entitiesToDetach);

            /**
            * Convenience function for retrieving the specified entity's slice instance history
            * \param entityId - ID of an entity in a live slice instance
            * \param sliceInstanceAncestors - [out] Output list of ancestor slice instances
            */
            void GetSliceInstanceAncestry(const AZ::EntityId& entityId, SliceInstanceList& sliceInstanceAncestors);

            /**
            * Generates filename string based on entity names of passed-in entities (limits size, converts spaces to underscores)
            * \param entities - entities to generate name from
            * \param outName - [out] generated file name string
            */
            void GenerateSuggestedSliceFilenameFromEntities(const AzToolsFramework::EntityIdList& entities, AZStd::string& outName);

            /**
            * Generates path string based on name and directory chosen, handling duplicates by appending increasing numbers
            * If /mypath/ and SliceName are passed in, and SliceName_001.slice already exists in that directory, it outputs
            * /mypath/SliceName_002.slice
            * \param sliceName - desired filename
            * \param targetDirectory - desired directory for file to go in
            * \param suggestedFullPath - [out] generated full path string
            */
            void GenerateSuggestedSlicePath(const AZStd::string& sliceName, const AZStd::string& targetDirectory, AZStd::string& suggestedFullPath);

            /**
             * Sets slice save location to path for given CRC id of SliceUserSettings
             */
            void SetSliceSaveLocation(const AZStd::string& path, AZ::u32 settingsId);

            /**
             * Gets slice save location path for given CRC id of SliceUserSettings, returns true whether it existed or not
             */
            bool GetSliceSaveLocation(AZStd::string& path, AZ::u32 settingsId);

            /**
             * Calculate the centroid bottom level of a group of entities to be made into a slice
             */
            AZ::Vector3 GetSliceRootPosition(const AZ::EntityId commonRoot, const AzToolsFramework::EntityList& selectionRootEntities);

            /**
             * Calculates the differences between a live entity and a comparison entity (typically a slice ancestor).
             * Optionally, function can determine if a specific field differs, vs. all differences across the entity.
             */
            AZ::u32 CountDifferencesVersusSlice(AZ::EntityId entityId, AZ::Entity* compareTo, AZ::SerializeContext& serializeContext, const InstanceDataHierarchy::Address* fieldAddress = nullptr);

            /**
            * \brief Checks if any of the passed in entities have invalid slice references.
            *        If any are found, display a dialog warning the user that this push will remove those references.
            * \param parent The widget to parent a warning message to, if a warning message is generated.
            * \param sliceAsset The quick push target to check for invalid slice references.
            * \returns The action to take. Save if no invalid references were found or if the user chose to overwrite them,
            *          Cancel if the user decides to stop this operation, and Details if the user wants to open the advanced
            *          slice push widget to view additional details.
            */
            InvalidSliceReferencesWarningResult CheckForInvalidSliceReferences(
                QWidget* parent,
                AZ::Data::Asset<AZ::SliceAsset> sliceAsset);

            /**
            * \brief Flattens the ancestry of a slice to a selected point
            * \param toFlatten The ancestral level being flattened to
            * \param toFlattenImmediateAncestor The next ancestor from toFlatten in the direction of the root base ancestor
            */
            void FlattenAncestry(const AZ::SliceComponent::Ancestor& toFlatten, const AZ::SliceComponent::Ancestor& toFlattenImmediateAncestor);

            /**
            * \brief Populates out_PushableChangesPerAsset with how many pushable changes exist per slice to be displayed.
            * \param pushableChangesPerAsset Output parameter populated with a map of IDs to pushable changes.
            * \param sliceDisplayOrder The list of slices to use to generate the pushable change counts.
            * \param serializeContext An input/output parameter for serialization information for the slice.
            * \param assetEntityAncestorMap The entity ancestor list per slice in the sliceDisplayOrder list.
            * \param numRelevantEntitiesInSlices How many entities are in the slice.
            * \param fieldAddress The address used to count how many differences exist in the in-scene entity against the slice.
            */
            bool CalculatePushableChangesPerAsset(
                AZStd::unordered_map<AZ::Data::AssetId, int>& pushableChangesPerAsset,
                AZ::SerializeContext& serializeContext,
                const AZStd::vector<AZ::Data::AssetId>& sliceDisplayOrder,
                const AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<EntityAncestorPair>>& assetEntityAncestorMap,
                const size_t& numRelevantEntitiesInSlices,
                const InstanceDataNode::Address* fieldAddress);

            /**
            * \brief Get the selected entities belong to slice instances.
            * \param selectedEntities The selected entities.
            * \param entitiesInSlices [OUT] The selected entities belong to slice instances.
            * \param sliceInstances [OUT] The slice instances affected by the selected entity set.
            */
            void GetEntitiesInSlices(const AzToolsFramework::EntityIdList& selectedEntities, AZ::u32& entitiesInSlices, AZ::SliceComponent::SliceInstanceAddressSet& sliceInstances);

            /**
            * Resaves the slice entity to the slice file on disk.
            * \param sliceEntity the slice entity to save.
            * \param fullFilePath the path to save the slice entity to.
            */
            void ResaveSlice(AZStd::shared_ptr<AZ::Entity> sliceEntity, const AZStd::string& fullFilePath);

            /**
            * Analyse slice ancestry to check whether given EntityId can be safely pushed.
            * \param entityId the entity to check push safety for.
            * \param entitySliceAddress slice that owns this entity.
            * \param transformAncestorSliceAddress slice that owns ancestor to check push safety against.
            * \param sliceAncestryToPushTo Entity Instance Ancestry for slice to attempt to push to.
            * \param unpushableEntityIdsPerAsset list of each entity that can't be pushed to each asset in the ancestry.
            * \param newChildEntityIdAncestorPairs all new children paired with the ancestor they can be pushed to.
            * \param rootAncestorPushList all discovered root ancestors, used to block pushes to multiple slices at once which is hard to check for cyclic dependencies.
            */
            void AnalyseAncestoryForPushableEntities(const AZ::EntityId& entityId,
                AZ::SliceComponent::SliceInstanceAddress entitySliceAddress,
                AZ::SliceComponent::SliceInstanceAddress& transformAncestorSliceAddress,
                AZ::SliceComponent::EntityAncestorList& sliceAncestryToPushTo,
                AZStd::unordered_map<AZ::Data::AssetId, EntityIdSet>& unpushableEntityIdsPerAsset,
                AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>>& newChildEntityIdAncestorPairs,
                AZStd::vector< AZ::Data::AssetId>& rootAncestorPushList);

            //! Helper method to load a slice Entity from a given assetId
            AZStd::shared_ptr<AZ::Entity> GetSliceEntityForAssetId(const AZ::Data::AssetId& assetId);

        } // namespace Internal

        //=========================================================================
        bool QueryAndPruneMissingExternalReferences(AzToolsFramework::EntityIdSet& entities, AzToolsFramework::EntityIdSet& selectedAndReferencedEntities,
                    bool& useReferencedEntities, bool defaultMoveExternalRefs = false)
        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::MakeNewSlice:HandleNotIncludedReferences");
            useReferencedEntities = false;

            AZStd::string includedEntities;
            AZStd::string referencedEntities;
            AzToolsFramework::EntityIdList missingEntityIds;
            for (const AZ::EntityId& id : selectedAndReferencedEntities)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
                if (entity)
                {
                    if (entities.find(id) != entities.end())
                    {
                        includedEntities.append("    ");
                        includedEntities.append(entity->GetName());
                        includedEntities.append("\r\n");
                    }
                    else
                    {
                        referencedEntities.append("    ");
                        referencedEntities.append(entity->GetName());
                        referencedEntities.append("\r\n");
                    }
                }
                else
                {
                    missingEntityIds.push_back(id);
                }
            }

            if(!referencedEntities.empty())
            {
                if (!defaultMoveExternalRefs)
                {
                    AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::MakeNewSlice:HandleNotIncludedReferences:UserDialog");

                    const AZStd::string message = AZStd::string::format(
                        "Entity references may not be valid if the entity IDs change or if the entities do not exist when the slice is instantiated.\r\n\r\nSelected Entities\n%s\nReferenced Entities\n%s\n",
                        includedEntities.c_str(),
                        referencedEntities.c_str());

                    QWidget* mainWindow = nullptr;
                    AzToolsFramework::EditorRequests::Bus::BroadcastResult(
                        mainWindow,
                        &AzToolsFramework::EditorRequests::Bus::Events::GetMainWindow);

                    QMessageBox msgBox(mainWindow);
                    msgBox.setWindowTitle("External Entity References");
                    msgBox.setText("The slice contains references to external entities that are not selected.");
                    msgBox.setInformativeText("You can move the referenced entities into this slice or retain the external references.");
                    QAbstractButton* moveButton = (QAbstractButton*)msgBox.addButton("Move", QMessageBox::YesRole);
                    QAbstractButton* retainButton = (QAbstractButton*)msgBox.addButton("Retain", QMessageBox::NoRole);
                    msgBox.setStandardButtons(QMessageBox::Cancel);
                    msgBox.setDefaultButton(QMessageBox::Yes);
                    msgBox.setDetailedText(message.c_str());
                    msgBox.exec();

                    if (msgBox.clickedButton() == moveButton)
                    {
                        useReferencedEntities = true;
                    }
                    else if (msgBox.clickedButton() != retainButton)
                    {
                        return false;
                    }
                }
                else
                {
                    useReferencedEntities = true;
                }
            }

            for (const AZ::EntityId& missingEntityId : missingEntityIds)
            {
                entities.erase(missingEntityId);
                selectedAndReferencedEntities.erase(missingEntityId);
            }
            return true;
        }

        //=========================================================================
        bool QueryUserForSliceFilename(const AZStd::string& suggestedName,
                    const char* initialTargetDirectory,
                    AZ::u32 sliceUserSettingsId,
                    QWidget* activeWindow,
                    AZStd::string& outSliceName,
                    AZStd::string& outSliceFilePath)
        {
            AZStd::string saveAsInitialSuggestedDirectory;
            if (!Internal::GetSliceSaveLocation(saveAsInitialSuggestedDirectory, sliceUserSettingsId))
            {
                saveAsInitialSuggestedDirectory = initialTargetDirectory;
            }

            AZStd::string saveAsInitialSuggestedFullPath;
            Internal::GenerateSuggestedSlicePath(suggestedName, saveAsInitialSuggestedDirectory, saveAsInitialSuggestedFullPath);

            QString saveAs;
            AZStd::string targetPath;
            QFileInfo sliceSaveFileInfo;
            QString sliceName;
            while (true)
            {
                {
                    AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::MakeNewSlice:SaveAsDialog");
                    saveAs = QFileDialog::getSaveFileName(nullptr, QString("Save As..."), saveAsInitialSuggestedFullPath.c_str(), QString("Slices (*.slice)"));
                }

                sliceSaveFileInfo = saveAs;
                sliceName = sliceSaveFileInfo.baseName();
                if (saveAs.isEmpty())
                {
                    return false;
                }

                targetPath = saveAs.toUtf8().constData();
                if (AzFramework::StringFunc::Utf8::CheckNonAsciiChar(targetPath))
                {
                    QMessageBox::warning(activeWindow,
                        QStringLiteral("Slice Creation Failed."),
                        QString("Unicode file name is not supported. \r\n"
                            "Please use ASCII characters to name your slice."),
                        QMessageBox::Ok);
                    return false;
                }

                Internal::SliceSaveResult saveResult = Internal::IsSlicePathValidForAssets(activeWindow, saveAs, saveAsInitialSuggestedFullPath);
                if (saveResult == Internal::SliceSaveResult::Cancel)
                {
                    // The error was already reported if this failed.
                    return false;
                }
                else if (saveResult == Internal::SliceSaveResult::Continue)
                {
                    // The slice save name is valid, continue with the save attempt.
                    break;
                }
            }

            // If the slice already exists, we instead want to *push* the entities to the existing
            // asset, as opposed to creating a new one.
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (fileIO && fileIO->Exists(targetPath.c_str()))
            {
                const AZStd::string message = AZStd::string::format(
                        "You are attempting to overwrite an existing slice: \"%s\".\r\n\r\n"
                        "This will damage instances or cascades of this slice. \r\n\r\n"
                        "Instead, either push entities/fields to the slice, or save to a different location.",
                        targetPath.c_str());

                QMessageBox::warning(activeWindow, QStringLiteral("Unable to Overwrite Slice"),
                    QString(message.c_str()), QMessageBox::Ok, QMessageBox::Ok);

                return false;
            }

            // We prevent users from creating a new slice with the same relative path that's already
            // been used by an existing slice in other places (e.g. Gems) because the AssetProcessor
            // generates asset ids based on relative paths. This is unnecessary once AssetProcessor
            // starts to generate UUID to every asset regardless of paths.
            {
                AZStd::string sliceRelativeName;
                bool relativePathFound;
                AssetSystemRequestBus::BroadcastResult(relativePathFound, &AssetSystemRequestBus::Events::GetRelativeProductPathFromFullSourceOrProductPath, targetPath, sliceRelativeName);

                AZ::Data::AssetId sliceAssetId;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, sliceRelativeName.c_str(), AZ::Data::s_invalidAssetType, false);
                if (sliceAssetId.IsValid())
                {
                    const AZStd::string message = AZStd::string::format(
                        "A slice with the relative path \"%s\" already exists in the Asset Database. \r\n\r\n"
                        "Overriding it will damage instances or cascades of this slice. \r\n\r\n"
                        "Instead, either push entities/fields to the slice, or save to a different location.",
                        sliceRelativeName.c_str());

                    QMessageBox::warning(activeWindow, QStringLiteral("Unable to Overwrite Slice"),
                        QString(message.c_str()), QMessageBox::Ok, QMessageBox::Ok);

                    return false;
                }
            }

            AZStd::string saveDir(sliceSaveFileInfo.absoluteDir().absolutePath().toUtf8().constData());
            Internal::SetSliceSaveLocation(saveDir, sliceUserSettingsId);

            outSliceName = sliceName.toUtf8().constData();
            outSliceFilePath = targetPath.c_str();

            return true;
        }

        //=========================================================================
        void GatherAllReferencedEntities(AzToolsFramework::EntityIdSet& entitiesWithReferences,
                                         AZ::SerializeContext& serializeContext)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZStd::vector<AZ::EntityId> floodQueue;
            floodQueue.reserve(entitiesWithReferences.size());

            auto visitChild = [&floodQueue, &entitiesWithReferences](const AZ::EntityId& childId) -> void
            {
                if (entitiesWithReferences.insert(childId).second)
                {
                    floodQueue.push_back(childId);
                }
            };

            // Seed with all provided entity Ids
            for (const AZ::EntityId& entityId : entitiesWithReferences)
            {
                floodQueue.push_back(entityId);
            }

            // Flood-fill via outgoing entity references and gather all unique visited entities.
            while (!floodQueue.empty())
            {
                const AZ::EntityId id = floodQueue.back();
                floodQueue.pop_back();

                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);

                if (entity)
                {
                    AZStd::vector<const AZ::SerializeContext::ClassData*> parentStack;
                    parentStack.reserve(30);
                    auto beginCB = [&](void* ptr, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* elementData) -> bool
                    {
                        parentStack.push_back(classData);

                        AZ::u32 sliceFlags = GetSliceFlags(elementData ? elementData->m_editData : nullptr, classData ? classData->m_editData : nullptr);

                        // Skip any class or element marked as don't gather references
                        if (0 != (sliceFlags & AZ::Edit::SliceFlags::DontGatherReference))
                        {
                            return false;
                        }

                        if (classData->m_typeId == AZ::SerializeTypeInfo<AZ::EntityId>::GetUuid())
                        {
                            if (!parentStack.empty() && parentStack.back()->m_typeId == AZ::SerializeTypeInfo<AZ::Entity>::GetUuid())
                            {
                                // Ignore the entity's actual Id field. We're only looking for references.
                            }
                            else
                            {
                                AZ::EntityId* entityIdPtr = (elementData->m_flags & AZ::SerializeContext::ClassElement::FLG_POINTER) ?
                                    *reinterpret_cast<AZ::EntityId**>(ptr) : reinterpret_cast<AZ::EntityId*>(ptr);
                                if (entityIdPtr)
                                {
                                    const AZ::EntityId id = *entityIdPtr;
                                    if (id.IsValid())
                                    {
                                        visitChild(id);
                                    }
                                }
                            }
                        }

                        // Keep recursing.
                        return true;
                    };

                    auto endCB = [&]() -> bool
                    {
                        parentStack.pop_back();
                        return true;
                    };

                    AZ::SerializeContext::EnumerateInstanceCallContext callContext(
                        beginCB,
                        endCB,
                        &serializeContext,
                        AZ::SerializeContext::ENUM_ACCESS_FOR_READ,
                        nullptr
                    );

                    serializeContext.EnumerateInstanceConst(
                        &callContext,
                        entity,
                        azrtti_typeid<AZ::Entity>(),
                        nullptr,
                        nullptr
                    );
                }
            }
        }

        void GatherAllReferencedEntitiesAndCompare(const AzToolsFramework::EntityIdSet& entities,
                                                   AzToolsFramework::EntityIdSet& entitiesAndReferencedEntities,
                                                   bool& hasExternalReferences,
                                                   AZ::SerializeContext& serializeContext)
        {
            entitiesAndReferencedEntities.clear();
            entitiesAndReferencedEntities = entities;
            GatherAllReferencedEntities(entitiesAndReferencedEntities, serializeContext);

            // NOTE: that AZStd::unordered_set equality operator only returns true if they are in the same order
            // (which appears to deviate from the standard). So we have to do the comparison ourselves.
            hasExternalReferences = (entitiesAndReferencedEntities.size() > entities.size());
            if (!hasExternalReferences)
            {
                for (const AZ::EntityId& id : entitiesAndReferencedEntities)
                {
                    if (entities.find(id) == entities.end())
                    {
                        hasExternalReferences = true;
                        break;
                    }
                }
            }
        }

        //=========================================================================
        AZStd::unique_ptr<AZ::Entity> CloneSliceEntityForComparison(const AZ::Entity& sourceEntity,
                                                                    const AZ::SliceComponent::SliceInstance& instance,
                                                                    AZ::SerializeContext& serializeContext)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZ_Assert(instance.GetEntityIdMap().find(sourceEntity.GetId()) != instance.GetEntityIdMap().end(), "Provided source entity is not a member of the provided slice instance.");

            AZ::Entity* clone = serializeContext.CloneObject<AZ::Entity>(&sourceEntity);

            // Prior to comparison, remap asset entity's Id references to the instance values, so we don't see instance-remapped Ids as differences.
            const AZ::SliceComponent::EntityIdToEntityIdMap& assetToInstanceIdMap = instance.GetEntityIdMap();
            AZ::IdUtils::Remapper<AZ::EntityId>::RemapIds(clone,
                [&assetToInstanceIdMap](const AZ::EntityId& originalId, bool isEntityId, const AZStd::function<AZ::EntityId()>&) -> AZ::EntityId
                {
                    if (!isEntityId)
                    {
                        auto findIter = assetToInstanceIdMap.find(originalId);
                        if (findIter != assetToInstanceIdMap.end())
                        {
                            return findIter->second;
                        }
                    }

                    return originalId;
                },
                &serializeContext, false);

            return AZStd::unique_ptr<AZ::Entity>(clone);
        }

        //=========================================================================
        AZ::Outcome<void, AZStd::string> PushEntitiesBackToSlice(const AzToolsFramework::EntityIdList& entityIdList,
            const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset, SliceTransaction::PreSaveCallback preSaveCallback)
        {
            if (!sliceAsset)
            {
                return AZ::Failure(AZStd::string::format("Asset %s is not loaded, or is not a slice.",
                    sliceAsset.ToString<AZStd::string>().c_str()));
            }

            // Make a transaction targeting the specified slice and add all the entities in this set.
            SliceTransaction::TransactionPtr transaction = SliceTransaction::BeginSlicePush(sliceAsset);
            if (transaction)
            {
                for (AZ::EntityId entityId : entityIdList)
                {
                    const SliceTransaction::Result result = transaction->UpdateEntity(entityId);
                    if (!result)
                    {
                        return AZ::Failure(AZStd::string::format("Failed to add entity with Id %s to slice transaction for \"%s\". Slice push aborted.\n\nError:\n%s",
                            entityId.ToString().c_str(),
                            sliceAsset.ToString<AZStd::string>().c_str(),
                            result.GetError().c_str()));
                    }
                }

                ScopedUndoBatch undoBatch("Slice Push (Quick)");

                const SliceTransaction::Result result = transaction->Commit(
                    sliceAsset.GetId(),
                    preSaveCallback,
                    nullptr);

                if (!result)
                {
                    AZStd::string sliceAssetPath;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceAsset.GetId());

                    return AZ::Failure(AZStd::string::format("Failed to to save slice \"%s\". Slice push aborted.\n\nError:\n%s",
                        sliceAssetPath.c_str(),
                        result.GetError().c_str()));
                }
            }

            return AZ::Success();
        }

        //=========================================================================
        AZ::Outcome<void, AZStd::string> PushEntitiesIncludingAdditionAndSubtractionBackToSlice(
            const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
            const AZStd::unordered_set<AZ::EntityId>& entitiesToUpdate,
            const AZStd::unordered_set<AZ::EntityId>& entitiesToAdd,
            const AZStd::unordered_set<AZ::EntityId>& entitiesToRemove,
            SliceTransaction::PreSaveCallback preSaveCallback,
            SliceTransaction::PostSaveCallback postSaveCallback)
        {
            using AzToolsFramework::SliceUtilities::SliceTransaction;
            // Make a transaction targeting the specified slice and add all the entities in this set.
            SliceTransaction::TransactionPtr transaction = SliceTransaction::BeginSlicePush(sliceAsset);
            if (transaction)
            {
                // Update existing entities
                for (AZ::EntityId entityId : entitiesToUpdate)
                {
                    const SliceTransaction::Result result = transaction->UpdateEntity(entityId);
                    if (!result)
                    {
                        return AZ::Failure(AZStd::string::format("Failed to add entity with Id %s to slice transaction for \"%s\". Slice push aborted.\n\nError:\n%s",
                            entityId.ToString().c_str(),
                            sliceAsset.ToString<AZStd::string>().c_str(),
                            result.GetError().c_str()));
                    }
                }

                // Add new entites
                for (AZ::EntityId entityId : entitiesToAdd)
                {
                    SliceTransaction::Result result = transaction->AddEntity(entityId);
                    if (!result)
                    {
                        return AZ::Failure(AZStd::string::format("Failed to add entity with Id %s to slice transaction for \"%s\". Slice push aborted.\n\nError:\n%s",
                            entityId.ToString().c_str(),
                            sliceAsset.GetHint().c_str(),
                            result.GetError().c_str()));
                    }
                }

                // Remove existing entities
                AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;
                for (AZ::EntityId entityId : entitiesToUpdate)
                {
                    AZ::SliceComponent::SliceInstanceAddress entitySliceAddress;
                    AzFramework::SliceEntityRequestBus::EventResult(entitySliceAddress, entityId,
                        &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                    if (entitySliceAddress.IsValid())
                    {
                        if (sliceInstances.end() == AZStd::find(sliceInstances.begin(), sliceInstances.end(), entitySliceAddress))
                        {
                            sliceInstances.push_back(entitySliceAddress);
                        }
                    }
                }

                // We know the slice instance details, compare the entities it contains to the entities
                // contained in the underlying asset. If it's missing any entities that exist in the asset,
                // we can remove the entity from the base slice.
                for (const AZ::SliceComponent::SliceInstanceAddress& instanceAddr : sliceInstances)
                {
                    const AZ::SliceComponent::SliceReference* sliceReference = instanceAddr.GetReference();
                    const AZ::SliceComponent::SliceInstance* sliceInstance = instanceAddr.GetInstance();
                    if (sliceReference == nullptr || sliceInstance == nullptr)
                    {
                        continue;
                    }

                    for (AZ::EntityId entityToRemove : entitiesToRemove)
                    {
                        AZ::SliceComponent::EntityAncestorList ancestors;
                        AZ::SliceComponent::SliceInstanceAddress assetInstanceAddress = sliceReference->GetSliceAsset().Get()->GetComponent()->FindSlice(entityToRemove);
                        if (assetInstanceAddress.IsValid())
                        {
                            assetInstanceAddress.GetReference()->GetInstanceEntityAncestry(entityToRemove, ancestors);
                        }
                        else
                        {
                            // This is a loose entity of the slice
                            sliceReference->GetInstanceEntityAncestry(entityToRemove, ancestors);
                        }

                        AZ::EntityId removalEntity = entityToRemove;
                        for (AZ::SliceComponent::Ancestor& ancestor : ancestors)
                        {
                            if (ancestor.m_sliceAddress.GetReference()->GetSliceAsset().GetId() == sliceAsset.GetId())
                            {
                                removalEntity = ancestor.m_entity->GetId();
                            }
                        }

                        SliceTransaction::Result result = transaction->RemoveEntity(removalEntity);
                        if (!result)
                        {
                            return AZ::Failure(AZStd::string::format("Failed to add entity with Id %s to slice transaction for \"%s\" for removal. Slice push aborted.\n\nError:\n%s",
                                removalEntity.ToString().c_str(),
                                sliceAsset.GetHint().c_str(),
                                result.GetError().c_str()));
                        }
                    }
                }

                {
                    ScopedUndoBatch undoBatch("Slice Push");

                    const SliceTransaction::Result result = transaction->Commit(
                        sliceAsset.GetId(),
                        preSaveCallback,
                        postSaveCallback);

                    if (!result)
                    {
                        AZStd::string sliceAssetPath;
                        AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceAsset.GetId());

                        return AZ::Failure(AZStd::string::format("Failed to to save slice \"%s\". Slice push aborted.\n\nError:\n%s",
                            sliceAssetPath.c_str(),
                            result.GetError().c_str()));
                    }
                }
            }

            return AZ::Success();
        }


        AZStd::unordered_set<AZ::EntityId> GetPushableNewChildEntityIds(
            const AzToolsFramework::EntityIdList& entityIdList,
            AZStd::unordered_map<AZ::Data::AssetId, EntityIdSet>& unpushableEntityIdsPerAsset,
            AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>& sliceAncestryMapping,
            AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>>& newChildEntityIdAncestorPairs,
            EntityIdSet& newEntityIds)
        {
            AZStd::unordered_set<AZ::EntityId> pushableNewChildEntityIds;
            AZStd::vector< AZ::Data::AssetId> rootAncestorPushList;

            for (const AZ::EntityId& entityId : entityIdList)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

                if (!entity)
                {
                    continue;
                }

                AZ::SliceComponent::SliceInstanceAddress entitySliceAddress;
                AzFramework::SliceEntityRequestBus::EventResult(entitySliceAddress, entityId,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                // Determine which slice ancestry this entity should be considered for addition to, currently determined by the nearest
                // transform ancestor entity in the current selection of entities.
                AZ::SliceComponent::EntityAncestorList& sliceAncestryToPushTo = sliceAncestryMapping[entityId];
                AZ::SliceComponent::SliceInstanceAddress transformAncestorSliceAddress;
                AZ::EntityId parentId;
                AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, entityId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
                while (parentId.IsValid())
                {
                    // If we find a transform ancestor that's not part of the selected entities
                    // before we find a transform ancestor that has a relevant slice to consider
                    // pushing this entity to, we skip the consideration of this entity for addition
                    // because that would mean trying to add the entity to something we don't have selected
                    if (AZStd::find(entityIdList.begin(), entityIdList.end(), parentId) == entityIdList.end())
                    {
                        break;
                    }

                    AZ::Entity* parentEntity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(parentEntity, &AZ::ComponentApplicationRequests::FindEntity, parentId);
                    if (!parentEntity)
                    {
                        break;
                    }

                    AzFramework::SliceEntityRequestBus::EventResult(transformAncestorSliceAddress, parentId,
                        &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                    if (transformAncestorSliceAddress.IsValid() && transformAncestorSliceAddress != entitySliceAddress)
                    {
                        transformAncestorSliceAddress.GetReference()->GetInstanceEntityAncestry(parentId, sliceAncestryToPushTo);
                        if (newEntityIds.find(entityId) == newEntityIds.end())
                        {
                            newEntityIds.insert(entityId);
                        }
                        //check each parent in the ancestry, as entity might be pushable to some but not others.
                        Internal::AnalyseAncestoryForPushableEntities(entityId,
                            entitySliceAddress,
                            transformAncestorSliceAddress,
                            sliceAncestryToPushTo,
                            unpushableEntityIdsPerAsset,
                            newChildEntityIdAncestorPairs,
                            rootAncestorPushList);
                    }
                    AZ::SliceEntityHierarchyRequestBus::EventResult(parentId, parentId, &AZ::SliceEntityHierarchyRequestBus::Events::GetSliceEntityParentId);
                }
            }

            return pushableNewChildEntityIds;
        }

        AZStd::unordered_set<AZ::EntityId> GetUniqueRemovedEntities(
            const AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sliceInstances,
            IdToEntityMapping& assetEntityIdtoAssetEntityMapping,
            IdToInstanceAddressMapping& assetEntityIdtoInstanceAddressMapping)
        {
            // For all slice instances we encountered, compare the entities it contains to the entities
            // contained in the underlying asset. If it's missing any entities that exist in the asset,
            // we can add fields to allow removal of the entity from the base slice.
            AZStd::unordered_set<AZ::EntityId> uniqueRemovedEntities;
            AZ::SliceComponent::EntityAncestorList ancestorList;
            AZ::SliceComponent::EntityList assetEntities;
            for (const AZ::SliceComponent::SliceInstanceAddress& instanceAddr : sliceInstances)
            {
                if (instanceAddr.IsValid() && instanceAddr.GetReference()->GetSliceAsset() &&
                    instanceAddr.GetInstance()->GetInstantiated())
                {
                    const AZ::SliceComponent::EntityList& instanceEntities = instanceAddr.GetInstance()->GetInstantiated()->m_entities;
                    assetEntities.clear();
                    instanceAddr.GetReference()->GetSliceAsset().Get()->GetComponent()->GetEntities(assetEntities);
                    if (assetEntities.size() > instanceEntities.size())
                    {
                        // The removed entity is already gone from the instance's map, so we have to do a reverse-lookup
                        // to pin down which specific entities have been removed in the instance vs the asset.
                        for (auto assetEntityIter = assetEntities.begin(); assetEntityIter != assetEntities.end(); ++assetEntityIter)
                        {
                            AZ::Entity* assetEntity = (*assetEntityIter);
                            const AZ::EntityId assetEntityId = assetEntity->GetId();

                            if (uniqueRemovedEntities.end() != uniqueRemovedEntities.find(assetEntityId))
                            {
                                continue;
                            }

                            // Iterate over the entities left in the instance and if none of them have this
                            // asset entity as its ancestor, then we want to remove it.
                            // \todo - Investigate ways to make this non-linear time. Tricky since removed entities
                            // obviously aren't maintained in any maps. (LY-88218)
                            bool foundAsAncestor = false;
                            for (const AZ::Entity* instanceEntity : instanceEntities)
                            {
                                ancestorList.clear();
                                instanceAddr.GetReference()->GetInstanceEntityAncestry(instanceEntity->GetId(), ancestorList, 1);
                                if (!ancestorList.empty() && ancestorList.begin()->m_entity == assetEntity)
                                {
                                    foundAsAncestor = true;
                                    break;
                                }
                            }

                            if (!foundAsAncestor)
                            {
                                // Grab ancestors, which determines which slices the removal can be pushed to.
                                uniqueRemovedEntities.insert(assetEntityId);
                                assetEntityIdtoAssetEntityMapping[assetEntityId] = assetEntity;
                                assetEntityIdtoInstanceAddressMapping[assetEntityId] = instanceAddr;
                            }
                        }
                    }
                }
            }

            return uniqueRemovedEntities;
        }

        //=========================================================================
        AZ::Outcome<void, AZStd::string> PushEntityFieldBackToSlice(AZ::EntityId entityId, const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset,
            const InstanceDataNode::Address& fieldAddress, SliceTransaction::PreSaveCallback preSaveCallback)
        {
            if (!sliceAsset)
            {
                return AZ::Failure(AZStd::string::format("Asset \"%s\" with id %s is not loaded, or is not a slice.",
                    sliceAsset.GetHint().c_str(),
                    sliceAsset.GetId().ToString<AZStd::string>().c_str()));
            }

            // Make a transaction targeting the specified slice and add the target entity/field.
            SliceTransaction::TransactionPtr transaction = SliceTransaction::BeginSlicePush(sliceAsset);
            if (!transaction)
            {
                return AZ::Failure(AZStd::string("Failed to begin a transaction for pushing changes to an existing slice asset."));
            }

            SliceTransaction::Result result = transaction->UpdateEntityField(entityId, fieldAddress);
            if (!result)
            {
                return AZ::Failure(AZStd::string::format("Failed to update field for entity with Id %s in slice transaction for \"%s\". Slice push aborted.\n\nError:\n%s",
                    entityId.ToString().c_str(),
                    sliceAsset.GetHint().c_str(),
                    result.GetError().c_str()));
            }

            bool undoSliceOverrideValue;
            AzToolsFramework::EditorRequests::Bus::BroadcastResult(undoSliceOverrideValue, &AzToolsFramework::EditorRequests::GetUndoSliceOverrideSaveValue);
            AZ::u32 sliceCommitFlags = 0;

            if (!undoSliceOverrideValue)
            {
                sliceCommitFlags = SliceTransaction::SliceCommitFlags::DisableUndoCapture;
            }

            result = transaction->Commit(
                sliceAsset.GetId(),
                preSaveCallback,
                nullptr,
                sliceCommitFlags);

            if (!result)
            {
                AZStd::string sliceAssetPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetPath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, sliceAsset.GetId());

                return AZ::Failure(AZStd::string::format("Failed to to save slice \"%s\". Slice push aborted.\n\nError:\n%s",
                    sliceAssetPath.c_str(),
                    result.GetError().c_str()));
            }

            return AZ::Success();
        }

        /**
        * \brief Applies standard root entity transform logic to slice, used for PreSave callbacks on world entity slices
        * \param targetSliceAsset Slice asset to check/modify
        */
        SliceTransaction::Result VerifyAndApplySliceWorldTransformRules(SliceTransaction::SliceAssetPtr& targetSliceAsset)
        {
            AZ::SliceComponent::EntityList sliceEntities;
            targetSliceAsset.Get()->GetComponent()->GetEntities(sliceEntities);

            AZ::EntityId commonRoot;
            AzToolsFramework::EntityList sliceRootEntities;

            bool entitiesHaveCommonRoot = false;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(entitiesHaveCommonRoot, &AzToolsFramework::ToolsApplicationRequests::FindCommonRootInactive,
                                                                             sliceEntities, commonRoot, &sliceRootEntities);

            // Slices must have a single root entity
            if (!entitiesHaveCommonRoot)
            {
                return AZ::Failure(AZStd::string::format("No common root for entities"));
            }

            if (sliceRootEntities.size() != 1)
            {
                return AZ::Failure(AZStd::string::format("Must have single root entity"));
            }

            // Root entities cannot have a parent and must be located at the origin
            for (AZ::Entity* rootEntity : sliceRootEntities)
            {
                AzToolsFramework::Components::TransformComponent* transformComponent = rootEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();
                if (transformComponent)
                {
                    // Position and rotation are only marked as NotPushableOnSliceRoot for the editor specific
                    // m_translate and m_rotate fields.
                    // The transformation matrix will have those properties applied, and it needs to be cleared at this point
                    // to avoid pushing them to the slice.
                    // Only scale is preserved on the root entity of a slice.
                    transformComponent->SetParent(AZ::EntityId());
                    transformComponent->SetWorldTranslation(AZ::Vector3::CreateZero());
                    transformComponent->SetLocalRotation(AZ::Vector3::CreateZero());
                }
            }

            // Clear cached world transforms for all asset entities
            // Not a hard requirement, just they don't make too much sense without context (slices can be instantiated anywhere)
            // and the cached world transform isn't pushable (so once it's set, it won't be changed in assets)
            for (AZ::Entity* entity : sliceEntities)
            {
                AzToolsFramework::Components::TransformComponent* transformComponent = entity->FindComponent<AzToolsFramework::Components::TransformComponent>();
                if (transformComponent)
                {
                    transformComponent->ClearCachedWorldTransform();
                }
            }

            return AZ::Success();
        }

        //=========================================================================
        SliceTransaction::Result SlicePreSaveCallbackForWorldEntities(SliceTransaction::TransactionPtr transaction, const char* fullPath, SliceTransaction::SliceAssetPtr& asset)
        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "SliceUtilities::SlicePreSaveCallbackForWorldEntities");

            // Apply standard root transform rules. Zero out root entity translation, ensure single root, ensure slice root has no parent in slice.
            SliceTransaction::Result worldTransformRulesResult = VerifyAndApplySliceWorldTransformRules(asset);
            if (!worldTransformRulesResult)
            {
                return AZ::Failure(AZStd::string::format("Transform root entity rules for slice save to asset\n\"%s\"\ncould not be enforced:\n%s", fullPath, worldTransformRulesResult.GetError().c_str()));
            }

            return AZ::Success();
        }

        //=========================================================================
        bool CheckSliceAdditionCyclicDependencySafe(const AZ::SliceComponent::SliceInstanceAddress& instanceToAdd,
                                                    const AZ::SliceComponent::SliceInstanceAddress& targetInstanceToAddTo)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZ_Assert(instanceToAdd.IsValid(), "Invalid instanceToAdd passed to CheckSliceADditionCyclicDependencySafe.");

            // Ensure that parent target instance is valid.
            if (!targetInstanceToAddTo.IsValid())
            {
                return false;
            }

            // Cannot add a slice instance to the very same instance
            if (instanceToAdd == targetInstanceToAddTo)
            {
                return false;
            }

            // Cannot add an asset reference to itself - "directly cyclic" check
            if (instanceToAdd.GetReference()->GetSliceAsset().GetId() == targetInstanceToAddTo.GetReference()->GetSliceAsset().GetId())
            {
                return false;
            }

            // If the instanceToAdd already has a dependency on the targetInstanceToAddTo's asset before adding, if we added it,
            // the targetInstanceToAddTo would then depend on instanceToAdd would depend on targetInstanceToAddTo, and on, cyclic!
            AZ::SliceComponent::AssetIdSet referencedSliceAssetIds;
            instanceToAdd.GetReference()->GetSliceAsset().Get()->GetComponent()->GetReferencedSliceAssets(referencedSliceAssetIds);
            if (referencedSliceAssetIds.find(targetInstanceToAddTo.GetReference()->GetSliceAsset().GetId()) != referencedSliceAssetIds.end())
            {
                return false;
            }

            return true;
        }

        //=========================================================================
        bool IsRootEntity(const AZ::Entity& entity)
        {
            auto* transformComponent = entity.FindComponent<AzToolsFramework::Components::TransformComponent>();
            return (transformComponent && !transformComponent->GetParentId().IsValid());
        }

        //=========================================================================
        bool IsSliceOrSubsliceRootEntity(const AZ::EntityId& id)
        {
            bool isSliceEntity = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceEntity, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceEntity);
            if (!isSliceEntity)
            {
                return false;
            }

            bool isSliceRoot = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceRoot, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceRoot);
            bool isSubsliceRoot = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSubsliceRoot, id, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSubsliceRoot);

            return isSliceRoot || isSubsliceRoot;
        }

        //=========================================================================
        AZ::u32 GetSliceFlags(const AZ::Edit::ElementData* editData, const AZ::Edit::ClassData* classData)
        {
            AZ::u32 sliceFlags = 0;

            if (editData)
            {
                AZ::Edit::Attribute* slicePushAttribute = editData->FindAttribute(AZ::Edit::Attributes::SliceFlags);
                if (slicePushAttribute)
                {
                    AZ::u32 elementSliceFlags = 0;
                    AzToolsFramework::PropertyAttributeReader reader(nullptr, slicePushAttribute);
                    reader.Read<AZ::u32>(elementSliceFlags);
                    sliceFlags |= elementSliceFlags;
                }
            }

            const AZ::Edit::ElementData* classEditData = classData ? classData->FindElementData(AZ::Edit::ClassElements::EditorData) : nullptr;
            if (classEditData)
            {
                AZ::Edit::Attribute* slicePushAttribute = classEditData->FindAttribute(AZ::Edit::Attributes::SliceFlags);
                if (slicePushAttribute)
                {
                    AZ::u32 classSliceFlags = 0;
                    AzToolsFramework::PropertyAttributeReader reader(nullptr, slicePushAttribute);
                    reader.Read<AZ::u32>(classSliceFlags);
                    sliceFlags |= classSliceFlags;
                }
            }

            return sliceFlags;
        }

        AZ::u32 GetNodeSliceFlags(const InstanceDataNode& node)
        {
            const AZ::Edit::ElementData* editData = node.GetElementEditMetadata();

            AZ::u32 sliceFlags = 0;

            if (node.GetClassMetadata())
            {
                sliceFlags = GetSliceFlags(editData, node.GetClassMetadata()->m_editData);
            }

            return sliceFlags;
        }

        //=========================================================================
        bool IsNodePushable(const InstanceDataNode& node, bool isRootEntity /*= false*/)
        {
            const AZ::u32 sliceFlags = GetNodeSliceFlags(node);

            if (0 != (sliceFlags & AZ::Edit::SliceFlags::NotPushable))
            {
                return false;
            }

            if (isRootEntity && 0 != (sliceFlags & AZ::Edit::SliceFlags::NotPushableOnSliceRoot))
            {
                return false;
            }

            return true;
        }

        void PopulateSliceSubMenus(QMenu& outerMenu, const AzToolsFramework::EntityIdList& inputEntities, SliceSelectedCallback sliceSelectedCallback, SliceSelectedCallback sliceRelationshipViewCallback)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);
            // The find slice menu only works with a single entity selected.
            if (inputEntities.size() != 1)
            {
                outerMenu.addSeparator();
                return;
            }
            const AZ::EntityId selectedEntity = inputEntities[0];

            AZ::SliceComponent::EntityAncestorList ancestors;
            AZ::SliceComponent::SliceInstanceAddress sliceAddress;
            AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, selectedEntity,
                &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

            // No need to make the find slice menu if this entity has no slice association.
            if (!sliceAddress.IsValid())
            {
                outerMenu.addSeparator();
                return;
            }

            sliceAddress.GetReference()->GetInstanceEntityAncestry(selectedEntity, ancestors);

            // No need to make the find slice menu if this entity has no slice ancestry.
            if (ancestors.size() <= 0)
            {
                outerMenu.addSeparator();
                return;
            }

            const AZ::SliceComponent::SliceReference* sliceReference = ancestors.at(0).m_sliceAddress.GetReference();

            PopulateFindSliceMenu(outerMenu, selectedEntity, ancestors, sliceSelectedCallback);

            if (sliceReference)
            {
                PopulateSliceRelationshipViewMenu(outerMenu, selectedEntity, ancestors, sliceRelationshipViewCallback);
            }

            outerMenu.addSeparator();
        }

        QWidgetAction* MakeSliceMenuItem(const AZ::EntityId& /*selectedEntity*/, const AZ::SliceComponent::Ancestor& ancestor, QMenu* menu, int indentation, const QPixmap icon, QString tooltip, AZ::Data::AssetId& sliceAssetId)
        {
            const AZ::SliceComponent::SliceReference* sliceReference = ancestor.m_sliceAddress.GetReference();
            if (!sliceReference)
            {
                return nullptr;;
            }
            sliceAssetId = sliceReference->GetSliceAsset().GetId();

            // Grab the path to the asset so the name can be found.
            AZStd::string sliceAssetPath;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                sliceAssetPath,
                &AZ::Data::AssetCatalogRequestBus::Handler::GetAssetPathById,
                sliceAssetId);

            // Grab the file name of the slice to use as the slice's title in the menu.
            AZStd::string sliceAssetName;
            AzFramework::StringFunc::Path::GetFullFileName(sliceAssetPath.c_str(), sliceAssetName);
            if (sliceAssetName.empty())
            {
                AZ_Warning("PopulateFindSliceMenu", false, "Failed to determine path/name for slice with id %s", sliceAssetId.ToString<AZStd::string>().c_str());
                return nullptr;
            }

            // Build the menu item. A QWidgetAction is used instead of a QAction to allow the ancestry
            // hierarchy to be represented by indenting each ancestor under the previous.
            // The layout for each row is: [QLabel Indent][QLabel Slice Icon][QLabel Slice Name]

            // Create the container for the row: A WidgetAction to attach to the menu,
            // the base Widget to contain the horizontal layout, and the horizontal layout.
            QWidgetAction* findAction = new QWidgetAction(menu);
            QWidget* sliceLayoutWidget = new QWidget(menu);
            sliceLayoutWidget->setObjectName("SliceHierarchyMenuItem");
            // Add class to fix hover state styling for WidgetAction
            AzQtComponents::Style::addClass(sliceLayoutWidget, "WidgetAction");
            QHBoxLayout* sliceLayout = new QHBoxLayout(sliceLayoutWidget);
            findAction->setDefaultWidget(sliceLayoutWidget);

            // A label with a fixed size is used to indent instead of margins on the
            // QHBoxLayout because this matches the QuickPush menu.
            QLabel* indentLabel = new QLabel(menu);
            indentLabel->setFixedSize(indentation, GetSliceItemHeight());
            sliceLayout->addWidget(indentLabel);

            // Use the SliceIcon to visually reinforce that this is a slice file.
            QLabel* iconLabel = new QLabel(menu);
            iconLabel->setPixmap(icon);
            iconLabel->setFixedSize(GetSliceItemIconSize());
            sliceLayout->addWidget(iconLabel);

            // Use the filename without the path as the label for this menu icon, to match the QuickPush menu's behavior.
            QLabel* sliceLabel = new QLabel(sliceAssetName.c_str(), menu);
            sliceLabel->setToolTip(tooltip);
            sliceLayout->addWidget(sliceLabel);

            return findAction;
        }

        void PopulateFindSliceMenu(QMenu& outerMenu, const AZ::EntityId& selectedEntity, const AZ::SliceComponent::EntityAncestorList& ancestors, SliceSelectedCallback sliceSelectedCallback)
        {
            (void)selectedEntity;
            QMenu* findSliceMenu = new QMenu(&outerMenu);
            QPixmap sliceItemIcon(GetSliceItemIconPath());

            // Track how many ancestors deep the loop is, so the hierarchy can be visually represented.
            AZ::u32 indentation = 0;
            for (const AZ::SliceComponent::Ancestor& ancestor : ancestors)
            {
                AZ::Data::AssetId sliceAssetId;
                QWidgetAction* action = MakeSliceMenuItem(selectedEntity, ancestor, findSliceMenu, indentation, sliceItemIcon, QObject::tr("Selects this slice in the Asset Browser."), sliceAssetId);

                if (action)
                {
                    // Connect the action to select this slice in the AssetBrowser when it is clicked.
                    QObject::connect(action, &QAction::triggered,
                        [sliceSelectedCallback, sliceAssetId]
                    {
                        sliceSelectedCallback();
                        AzToolsFramework::AssetBrowser::AssetBrowserViewRequestBus::Broadcast(
                            &AzToolsFramework::AssetBrowser::AssetBrowserViewRequestBus::Events::ClearFilter);

                        AzToolsFramework::AssetBrowser::AssetBrowserViewRequestBus::Broadcast(
                            &AzToolsFramework::AssetBrowser::AssetBrowserViewRequestBus::Events::SelectProduct,
                            sliceAssetId);
                    });
                    findSliceMenu->addAction(action);

                    // Grow the indentation size for the next step in the hierarchy.
                    indentation += GetSliceHierarchyMenuIdentationPerLevel();
                }
            }

            findSliceMenu->setTitle(QObject::tr("Find slice in Asset Browser"));
            outerMenu.addMenu(findSliceMenu);
        }

        void PopulateSliceRelationshipViewMenu(QMenu& outerMenu, const AZ::EntityId& selectedEntity, const AZ::SliceComponent::EntityAncestorList& ancestors, SliceSelectedCallback sliceSelectedCallback)
        {
            (void)selectedEntity;
            QMenu* findSliceMenu = new QMenu(&outerMenu);
            QPixmap sliceItemIcon(GetSliceItemIconPath());

            // Track how many ancestors deep the loop is, so the hierarchy can be visually represented.
            AZ::u32 indentation = 0;
            for (const AZ::SliceComponent::Ancestor& ancestor : ancestors)
            {
                AZ::Data::AssetId sliceAssetId;
                QWidgetAction* action = MakeSliceMenuItem(selectedEntity, ancestor, findSliceMenu, indentation, sliceItemIcon, QObject::tr("Opens this slice in the Slice Relationship View"), sliceAssetId);

                if (action)
                {
                    // Connect the action to select this slice in the AssetBrowser when it is clicked.
                    QObject::connect(action, &QAction::triggered,
                        [sliceSelectedCallback, sliceAssetId]
                    {
                        sliceSelectedCallback();
                        AzToolsFramework::SliceRelationshipRequestBus::Broadcast(&AzToolsFramework::SliceRelationshipRequests::OnSliceRelationshipViewRequested, sliceAssetId);
                    });
                    findSliceMenu->addAction(action);

                    // Grow the indentation size for the next step in the hierarchy.
                    indentation += GetSliceHierarchyMenuIdentationPerLevel();
                }
            }

            findSliceMenu->setTitle(QObject::tr("Open in Slice Relationship View"));
            outerMenu.addMenu(findSliceMenu);
        }

        //=========================================================================
        DetachMenuActionWidget::DetachMenuActionWidget(QWidget* parent, const int& indentation, const AZStd::string& sliceAssetName, const bool& isLastAncestor)
            :QWidget(parent)
            , m_toLabel(nullptr)
            , m_sliceLabel(nullptr)
        {
            setObjectName("SliceHierarchyMenuItem");
            // Add class to fix hover state styling for WidgetAction
            AzQtComponents::Style::addClass(this, "WidgetAction");
            QHBoxLayout* sliceLayout = new QHBoxLayout(parent);
            setLayout(sliceLayout);

            QPixmap lShapeIcon(GetLShapeIconPath());
            int LShapeIconWidgetWidth = sliceLayout->contentsMargins().left() + GetLShapeIconSize().width() + sliceLayout->contentsMargins().right();

            if (indentation > 0)
            {
                QLabel* indentLabel = new QLabel(parent);
                indentLabel->setFixedSize(GetSliceHierarchyMenuIdentationPerLevel() + (indentation - 1) * LShapeIconWidgetWidth, GetSliceItemHeight());
                sliceLayout->addWidget(indentLabel);

                // Add the L shape icon to show the slice hierarchy
                QLabel* lShapeIconLabel = new QLabel(parent);
                lShapeIconLabel->setPixmap(lShapeIcon);
                lShapeIconLabel->setFixedSize(GetLShapeIconSize());
                sliceLayout->addWidget(lShapeIconLabel);
            }

            QPixmap sliceItemIcon(GetSliceItemIconPath());

            // Use the SliceIcon to visually reinforce that this is a slice file.
            QLabel* iconLabel = new QLabel(parent);
            iconLabel->setPixmap(sliceItemIcon);
            iconLabel->setFixedSize(GetSliceItemIconSize());
            sliceLayout->addWidget(iconLabel);

            // Use the filename without the path as the label for this menu icon, to match the QuickPush menu's behavior.
            m_sliceLabel = new QLabel((sliceAssetName + " ").c_str(), parent);
            sliceLayout->addWidget(m_sliceLabel, Qt::Alignment::enum_type::AlignLeft);

            // If we are not the last ancestor we will be a selectable menu item
            if (isLastAncestor)
            {
                // The last ancestor is not a selectable option as no ancestry would be flattened
                setEnabled(false);

                // Label denoting the base ancestor and where we're flattening from
                QLabel* fromLabel = new QLabel(QObject::tr("<i>From</i>"), parent);
                fromLabel->setObjectName("ContextLabelEnabled");
                sliceLayout->addWidget(fromLabel);
                sliceLayout->setAlignment(fromLabel, Qt::Alignment::enum_type::AlignRight);
            }
            else
            {
                // Label denoting the base ancestor and where we're flattening from
                m_toLabel = new QLabel(QObject::tr("<i>To</i>"), parent);
                m_toLabel->setObjectName("ContextLabelEnabled");
                sliceLayout->addWidget(m_toLabel);
                sliceLayout->setAlignment(m_toLabel, Qt::Alignment::enum_type::AlignRight);
                m_toLabel->hide();
            }
        }

        void DetachMenuActionWidget::enterEvent(QEvent* event)
        {
            if (m_toLabel)
            {
                m_toLabel->show();
            }

            setsliceLabelTextColor(detachMenuItemHoverColor);

            QWidget::enterEvent(event);
        }

        void DetachMenuActionWidget::leaveEvent(QEvent* event)
        {
            if (m_toLabel)
            {
                m_toLabel->hide();
            }

            setsliceLabelTextColor(detachMenuItemDefaultColor);

            QWidget::leaveEvent(event);
        }

        void DetachMenuActionWidget::setsliceLabelTextColor(QString color)
        {
            m_sliceLabel->setStyleSheet(QString("QLabel{color : %1;}").arg(color));
        }

        //=========================================================================
        MoveToSliceLevelConfirmation::MoveToSliceLevelConfirmation(QWidget* parent, const AZStd::string& currentSlice, const AZStd::string& destinationSlice)
            :QDialog(parent)
        {
            QVBoxLayout* confirmationDialogLayout = new QVBoxLayout(parent);

            QWidget* warningWidget = new QWidget(parent);
            QHBoxLayout* warningWidgetLayout = new QHBoxLayout(parent);

            QLabel* iconLabel = new QLabel(parent);
            QPixmap warningIcon(GetWarningIconPath());
            // Use the same size for the no saveable changes icon as the slice icon.
            iconLabel->setFixedSize(GetWarningIconSize());
            iconLabel->setPixmap(warningIcon);
            warningWidgetLayout->addWidget(iconLabel, Qt::AlignLeft);

            QLabel* warningTextLabel = new QLabel(QObject::tr("You are about to reassign entities. This operation cannot be undone. Do you want to continue?"), parent);
            warningTextLabel->setMinimumWidth(GetWarningLabelMinimumWidth());
            warningTextLabel->setWordWrap(true);
            warningWidgetLayout->addWidget(warningTextLabel);

            warningWidget->setLayout(warningWidgetLayout);

            confirmationDialogLayout->addWidget(warningWidget);

            QWidget* detailWidget = new QWidget(parent);
            detailWidget->setStyleSheet(QString("background-color:%1;").arg(detailWidgetBackgroundColor));
            QGridLayout* detailWidgetLayout = new QGridLayout(parent);
            detailWidgetLayout->setColumnStretch(0, 0);
            detailWidgetLayout->setColumnStretch(1, 1);

            detailWidgetLayout->addWidget(new QLabel("From: ", parent), 0, 0, Qt::AlignRight);
            detailWidgetLayout->addWidget(new QLabel(currentSlice.c_str(), parent), 0, 1, Qt::AlignLeft);
            detailWidgetLayout->addWidget(new QLabel("To: ", parent), 1, 0, Qt::AlignRight);
            detailWidgetLayout->addWidget(new QLabel(destinationSlice.c_str(), parent), 1, 1, Qt::AlignLeft);
            detailWidget->setLayout(detailWidgetLayout);
            confirmationDialogLayout->addWidget(detailWidget);

            QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                | QDialogButtonBox::Cancel, parent);
            connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
            confirmationDialogLayout->addWidget(buttonBox);

            setLayout(confirmationDialogLayout);
        }

        //=========================================================================
        bool DoEntitiesHaveOverrides(const AzToolsFramework::EntityIdList& inputEntities)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("Slice", false, "Could not retrieve application serialize context.");
                return false;
            }

            AZ::SliceComponent::EntityAncestorList tempAncestors;
            for (AZ::EntityId entityId : inputEntities)
            {
                AZ::SliceComponent::SliceInstanceAddress sliceAddress;
                AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entityId,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                if (sliceAddress.IsValid())
                {
                    tempAncestors.clear();

                    // We only want the immediate ancestor so pass 1 for max levels
                    sliceAddress.GetReference()->GetInstanceEntityAncestry(entityId, tempAncestors, 1);

                    for (const AZ::SliceComponent::Ancestor& entityAncestor : tempAncestors)
                    {
                        AZStd::unique_ptr<AZ::Entity> compareClone = CloneSliceEntityForComparison(*entityAncestor.m_entity, *sliceAddress.GetInstance(), *serializeContext);

                        const AZ::u32 numDifferences = Internal::CountDifferencesVersusSlice(entityId, compareClone.get(), *serializeContext, nullptr);
                        if (numDifferences > 0)
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        //=========================================================================
        bool IsReparentNonTrivial(const AZ::EntityId& entityId, const AZ::EntityId& newParentId)
        {
            AZ_PROFILE_FUNCTION(AzToolsFramework);

            AZ::EntityId oldParentId;
            AZ::TransformBus::EventResult(oldParentId, entityId, &AZ::TransformBus::Events::GetParentId);

            bool isSliceEntity = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceEntity, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceEntity);

            bool isSliceRoot = false;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSliceRoot, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSliceRoot);

            bool isNonTrivial = false;

            if (isSliceEntity && !isSliceRoot)
            {
                AZ::SliceComponent::SliceInstanceAddress oldParentSliceAddress;
                AzFramework::SliceEntityRequestBus::EventResult(oldParentSliceAddress, oldParentId,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                AZ::SliceComponent::SliceInstanceAddress newParentSliceAddress;
                AzFramework::SliceEntityRequestBus::EventResult(newParentSliceAddress, newParentId,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                // additional checks are necessary to determine if the entity hierarchy should be cloned when re-ordering the same slice instance
                if (oldParentSliceAddress.IsValid() && newParentSliceAddress.IsValid() && (oldParentSliceAddress.GetInstance() == newParentSliceAddress.GetInstance()))
                {
                    bool isSubsliceEntity = false;
                    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSubsliceEntity, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSubsliceEntity);

                    bool isOldParentSubsliceEntity = false;
                    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isOldParentSubsliceEntity, oldParentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSubsliceEntity);

                    bool isNewParentSubsliceEntity = false;
                    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isNewParentSubsliceEntity, newParentId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSubsliceEntity);

                    // moving into or out of a subslice
                    if (isOldParentSubsliceEntity != isNewParentSubsliceEntity)
                    {
                        isNonTrivial = true;
                    }
                    // moving between subslices
                    else if (isSubsliceEntity && isOldParentSubsliceEntity && isNewParentSubsliceEntity)
                    {
                        Internal::SliceInstanceList sliceHistory;
                        Internal::GetSliceInstanceAncestry(entityId, sliceHistory);

                        Internal::SliceInstanceList newParentSliceHistory;
                        Internal::GetSliceInstanceAncestry(newParentId, newParentSliceHistory);

                        for (auto& sliceInstance : sliceHistory)
                        {
                            if (AZStd::find(newParentSliceHistory.begin(), newParentSliceHistory.end(), sliceInstance) == newParentSliceHistory.end())
                            {
                                isNonTrivial = true;
                                break;
                            }
                        }
                    }
                }
                // otherwise, the entity hierarchy should always be cloned if the parents are part of different slice instances
                else
                {
                    isNonTrivial = true;
                }
            }

            return isNonTrivial;
        }

        InvalidSliceReferencesWarningResult DisplayInvalidSliceReferencesWarning(
            QWidget* parent,
            size_t invalidSliceCount,
            size_t invalidReferenceCount,
            bool showDetailsButton)
        {
            QMessageBox warningMessageBox(parent);
            warningMessageBox.setObjectName("SliceUtilities.warningMessageBox");
            warningMessageBox.setWindowTitle(QObject::tr("Remove invalid references"));
            // Qt's .arg(firstArg, secondArg) changes behavior on the data type of firstArg and secondArg.
            // If firstArg and secondArg are both QStrings, then secondArg will actually replace %2.
            // If secondArg is an integer value, then Qt will use a different override for arg, where secondArg
            // is treated as a width value. See http://doc.qt.io/archives/qt-4.8/qstring.html#arg and
            // http://doc.qt.io/archives/qt-4.8/qstring.html#arg-2
            warningMessageBox.setText(
                QObject::tr("This change will remove %1 invalid reference(s) from %2 slice file(s). You can't undo this change.")
                    .arg(invalidReferenceCount)
                    .arg(invalidSliceCount));

            // Add the two buttons that are always available: Cancel and Confirm.
            warningMessageBox.setStandardButtons(QMessageBox::Cancel);
            QAbstractButton* saveButton = warningMessageBox.addButton(QObject::tr("Confirm"), QMessageBox::AcceptRole);

            // Add the details button if it needs to be there.
            QAbstractButton* detailsButton = nullptr;
            if (showDetailsButton)
            {
                detailsButton = warningMessageBox.addButton(QObject::tr("More details"), QMessageBox::ActionRole);
            }

            QIcon linkStateError = QIcon(":/PropertyEditor/Resources/error_link_state.png");
            warningMessageBox.setIconPixmap(linkStateError.pixmap(linkStateError.availableSizes().first()));

            // Open the message box and wait for the user to make a choice.
            int warningMessageResult = warningMessageBox.exec();

            // Check which button the user clicked on, and return the result.
            QAbstractButton* clickedButton = warningMessageBox.clickedButton();

            if (detailsButton != nullptr && clickedButton == detailsButton)
            {
                return InvalidSliceReferencesWarningResult::Details;
            }
            if (clickedButton == saveButton)
            {
                return InvalidSliceReferencesWarningResult::Save;
            }
            switch (warningMessageResult)
            {
            case QMessageBox::Cancel:
                return InvalidSliceReferencesWarningResult::Cancel;
            default:
                AZ_Error("InvalidSliceReferencesPopup",
                    false,
                    "Invalid slice references warning popup dismissed with result %d. "
                    "This result will be treated as cancel, try again if you did not wish to cancel.",
                    warningMessageResult);
                return InvalidSliceReferencesWarningResult::Cancel;
            }

        }

        bool CountPushableChangesToSlice(const AzToolsFramework::EntityIdList& inputEntities,
            const InstanceDataNode::Address* fieldAddress,
            AZStd::unordered_set<AZ::EntityId>& entitiesToAdd,
            AZStd::unordered_set<AZ::EntityId>& entitiesToRemove,
            size_t& numRelevantEntitiesInSlices,
            AZStd::unordered_map<AZ::Data::AssetId, int>& numPushableChangesPerAsset,
            AZStd::vector<AZ::Data::AssetId>& sliceDisplayOrder,
            AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<EntityAncestorPair>>& assetEntityAncestorMap,
            AZStd::unordered_map<AZ::Data::AssetId, EntityIdSet>& unpushableEntityIdsPerAsset)
        {
            bool haveNewOrDeletedEntities = false;

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("Slice", false, "Could not retrieve application serialize context.");
                return false;
            }

            numRelevantEntitiesInSlices = 0;
            AZStd::unordered_map<AZ::EntityId, AZ::SliceComponent::EntityAncestorList> sliceAncestryMapping;
            AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>> newChildEntityIdAncestorPairs;

            AZStd::unordered_set<AZ::EntityId> pushableNewChildEntityIds = GetPushableNewChildEntityIds(inputEntities,
                unpushableEntityIdsPerAsset,
                sliceAncestryMapping,
                newChildEntityIdAncestorPairs,
                entitiesToAdd);

            AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;
            for (AZ::EntityId entityId : inputEntities)
            {
                AZ::SliceComponent::SliceInstanceAddress entitySliceAddress;
                AzFramework::SliceEntityRequestBus::EventResult(entitySliceAddress, entityId,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                if (entitySliceAddress.IsValid())
                {
                    if (sliceInstances.end() == AZStd::find(sliceInstances.begin(), sliceInstances.end(), entitySliceAddress))
                    {
                        sliceInstances.push_back(entitySliceAddress);
                    }
                }
            }

            IdToEntityMapping assetEntityIdtoAssetEntityMapping;
            IdToInstanceAddressMapping assetEntityIdtoInstanceAddressMapping;
            entitiesToRemove = GetUniqueRemovedEntities(sliceInstances, assetEntityIdtoAssetEntityMapping, assetEntityIdtoInstanceAddressMapping);

            haveNewOrDeletedEntities = !entitiesToRemove.empty() || !entitiesToAdd.empty();

            AZ::SliceComponent::EntityAncestorList tempAncestors;

            for (AZ::EntityId entityId : inputEntities)
            {
                AZ::SliceComponent::SliceInstanceAddress sliceAddress;
                AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entityId,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                if (sliceAddress.IsValid())
                {
                    if (entitiesToAdd.find(entityId) != entitiesToAdd.end())
                    {
                        continue;
                    }

                    tempAncestors.clear();
                    sliceAddress.GetReference()->GetInstanceEntityAncestry(entityId, tempAncestors);

                    for (const AZ::SliceComponent::Ancestor& ancestor : tempAncestors)
                    {
                        const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset = ancestor.m_sliceAddress.GetReference()->GetSliceAsset();
                        AZStd::vector<EntityAncestorPair>& entityAncestors = assetEntityAncestorMap[sliceAsset.GetId()];
                        AZStd::unique_ptr<AZ::Entity> compareClone = CloneSliceEntityForComparison(*ancestor.m_entity, *ancestor.m_sliceAddress.GetInstance(), *serializeContext);
                        AZ_Error("Slice", compareClone.get(), "Failed to clone entity for slice comparison.");
                        if (compareClone)
                        {
                            entityAncestors.emplace_back(entityId, compareClone.release());
                        }

                        // Maintain a display-order array of slice assets.
                        if (sliceDisplayOrder.end() == AZStd::find(sliceDisplayOrder.begin(), sliceDisplayOrder.end(), sliceAsset.GetId()))
                        {
                            sliceDisplayOrder.push_back(sliceAsset.GetId());
                        }
                    }

                    ++numRelevantEntitiesInSlices;
                }
            }

            bool pushableChangesAvailable = Internal::CalculatePushableChangesPerAsset(
                numPushableChangesPerAsset,
                *serializeContext,
                sliceDisplayOrder,
                assetEntityAncestorMap,
                numRelevantEntitiesInSlices,
                fieldAddress);

            return pushableChangesAvailable || haveNewOrDeletedEntities;
        }

        bool IsDynamic(const AZ::Data::AssetId& assetId)
        {
            AZStd::shared_ptr<AZ::Entity> sliceEntity = Internal::GetSliceEntityForAssetId(assetId);
            AZ::SliceComponent* sliceComponent = sliceEntity ? sliceEntity->FindComponent<AZ::SliceComponent>() : nullptr;
            if (sliceComponent)
            {
                return sliceComponent->IsDynamic();
            }
            else
            {
                AZ_Warning("Slice", false, "Asset %s does not contain a slice component", assetId.ToString<AZStd::string>().c_str());
            }

            return false;
        }

        void SetIsDynamic(const AZ::Data::AssetId& assetId, bool isDynamic)
        {
            AZStd::shared_ptr<AZ::Entity> sliceEntity = Internal::GetSliceEntityForAssetId(assetId);
            AZ::SliceComponent* sliceComponent = sliceEntity ? sliceEntity->FindComponent<AZ::SliceComponent>() : nullptr;
            if (sliceComponent)
            {
                AZStd::string relativePath, fullPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(relativePath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);

                bool fullPathFound = false;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fullPathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relativePath, fullPath);
                if (fullPathFound)
                {
                    sliceComponent->SetIsDynamic(isDynamic);
                    Internal::ResaveSlice(sliceEntity, fullPath);
                }
            }
            else
            {
                AZ_Warning("Slice", false, "Asset %s does not contain a slice component", assetId.ToString<AZStd::string>().c_str());
            }
        }

        void CreateSliceAssetContextMenu(QMenu* menu, const AZStd::string& fullFilePath)
        {
            if (!menu)
            {
                return;
            }

            AZStd::string relativePath;
            bool relativePathFound = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(relativePathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetRelativeProductPathFromFullSourceOrProductPath, fullFilePath, relativePath);

            AZ::Data::AssetId sliceAssetId;
            if (relativePathFound)
            {
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(sliceAssetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, relativePath.c_str(), AZ::Data::s_invalidAssetType, false);
            }

            if (!sliceAssetId.IsValid())
            {
                return;
            }

            // For slices, we provide the option to toggle the dynamic flag.
            bool isDynamic = IsDynamic(sliceAssetId);
            QString sliceOptions[] = { QObject::tr("Set Dynamic Slice"), QObject::tr("Unset Dynamic Slice") };
            QString optionLabel = isDynamic ? sliceOptions[1] : sliceOptions[0];
            menu->addAction(optionLabel, [sliceAssetId, isDynamic]()
            {
                SetIsDynamic(sliceAssetId, !isDynamic);
            });
        }

        void RemoveInvalidChildOrderArrayEntries(const AZStd::vector<AZ::EntityId>& originalOrderArray,
            AZStd::vector<AZ::EntityId>& prunedOrderArray,
            const AZ::Data::Asset <AZ::SliceAsset>& targetSlice,
            WillPushEntityCallback willPushEntityCallback)
        {
            // Build prunedOrderArray as a copy of originalOrderArray but with only valid entity ids.
            for (const AZ::EntityId& childId : originalOrderArray)
            {
                if (willPushEntityCallback(childId, targetSlice))
                {
                    prunedOrderArray.push_back(childId);
                }
            }
        }

        AZ::DataStream::StreamType GetSliceStreamFormat()
        {
            return AZ::DataStream::ST_XML;
        }

        namespace Internal
        {
            SliceSaveResult IsSlicePathValidForAssets(QWidget* activeWindow, QString slicePath, AZStd::string &retrySavePath)
            {
                bool assetSetFoldersRetrieved = false;
                AZStd::vector<AZStd::string> assetSafeFolders;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    assetSetFoldersRetrieved,
                    &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetSafeFolders,
                    assetSafeFolders);

                if (!assetSetFoldersRetrieved)
                {
                    // If the asset safe list couldn't be retrieved, don't block the user but warn them.
                    AZ_Warning("Slice", false, "Unable to verify that the slice file to create is in a valid path.");
                }
                else
                {
                    AZ::IO::FixedMaxPath lexicallyNormalPath = AZ::IO::PathView(slicePath.toUtf8().constData()).LexicallyNormal();

                    bool isPathSafeForAssets = false;
                    for (const AZStd::string& assetSafeFolder : assetSafeFolders)
                    {
                        AZ::IO::PathView assetSafeFolderView(assetSafeFolder);
                        // Check if the slice path is relative to the safe asset directory.
                        // The Path classes are being used to make this check case insensitive.
                        if (lexicallyNormalPath.IsRelativeTo(assetSafeFolderView))
                        {
                            isPathSafeForAssets = true;
                            break;
                        }
                    }

                    if (!isPathSafeForAssets)
                    {
                        // Put an error in the console, so the log files have info about this error, or the user can look up the error after dismissing it.
                        AZStd::string errorMessage = "You can save slices only to your game project folder or the Gems folder. Update the location and try again.\n\n";
                        AZ_Error("Slice", false, errorMessage.c_str());

                        // Display a pop-up, the logs are easy to miss. This will make sure a user who encounters this error immediately knows their slice save has failed.
                        QMessageBox msgBox(activeWindow);
                        msgBox.setIcon(QMessageBox::Icon::Warning);
                        msgBox.setTextFormat(Qt::RichText);
                        msgBox.setWindowTitle(QObject::tr("Invalid save location"));
                        msgBox.setText(QString("%1").arg(QObject::tr(errorMessage.c_str())));
                        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Retry);
                        msgBox.setDefaultButton(QMessageBox::Retry);
                        const int response = msgBox.exec();
                        switch (response)
                        {
                        case QMessageBox::Retry:
                            // If the user wants to retry, they probably want to save to a valid location,
                            // so set the suggested save path to a known valid location.
                            if (assetSafeFolders.size() > 0)
                            {
                                QStringList strList = slicePath.split("/");
                                if (strList.size() > 0)
                                {
                                    retrySavePath = assetSafeFolders[0] + ("/" + strList[strList.size() - 1]).toUtf8().data();
                                }
                                else
                                {
                                    retrySavePath = assetSafeFolders[0];
                                }

                            }
                            return SliceSaveResult::Retry;
                        case QMessageBox::Cancel:
                        default:
                            return SliceSaveResult::Cancel;
                        }
                    }
                }
                // Valid slice save location, continue with the save attempt.
                return SliceSaveResult::Continue;
            }

            //=========================================================================
            void GetSliceEntityAncestors(const AZ::EntityId& entityId, AZ::SliceComponent::EntityAncestorList& ancestors)
            {
                AZ::SliceComponent::SliceInstanceAddress sliceAddress;
                AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entityId,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                if (sliceAddress.IsValid())
                {
                    sliceAddress.GetReference()->GetInstanceEntityAncestry(entityId, ancestors);
                }
            }

            //=========================================================================
            void GetSubsliceEntityAncestors(const AZ::EntityId& entityId, AZ::SliceComponent::EntityAncestorList& ancestors)
            {
                GetSliceEntityAncestors(entityId, ancestors);
                if (!ancestors.empty())
                {
                    // Pop the first ancestor as we only care about sub-slices
                    ancestors.erase(ancestors.begin());
                }
            }

            //=========================================================================
            void PartitionEntityHierarchyForNonTrivialReparent(const AZ::EntityId& rootEntity,
                AZ::SliceComponent::SliceInstanceEntityIdRemapList& subslicesToDetach,
                AzToolsFramework::EntityIdList& entitiesToDetach)
            {
                // Acquire the owning slice of the rootEntity hierarchy
                AZ::SliceComponent::SliceInstanceAddress sourceSliceInstance;
                AzFramework::SliceEntityRequestBus::EventResult(sourceSliceInstance, rootEntity,
                    &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                if (!sourceSliceInstance.IsValid())
                {
                    AZ_Warning("SliceUtilities::Internal::PartitionEntityHierarchyForNonTrivialReparent",
                        false,
                        "Passed in Root Entity with Id: %s has no owning slice. Unable to detach its hierarchy",
                        rootEntity.ToString().c_str());

                    return;
                }

                // Acquire the hierarchy found below the rootEntity
                AzToolsFramework::EntityIdSet entityHierarchy;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(entityHierarchy, &AzToolsFramework::ToolsApplicationRequestBus::Events::GatherEntitiesAndAllDescendents, AzToolsFramework::EntityIdList{ rootEntity });

                // Sort the hierarchy
                AzToolsFramework::EntityIdList entityHierarchySortedByDepthAndOrder(entityHierarchy.begin(), entityHierarchy.end());
                AzToolsFramework::SortEntitiesByLocationInHierarchy(entityHierarchySortedByDepthAndOrder);

                entitiesToDetach.reserve(entityHierarchySortedByDepthAndOrder.size());

                AZ::SliceComponent::SliceInstanceAddressSet subslices;
                for (const AZ::EntityId& currentEntityId : entityHierarchySortedByDepthAndOrder)
                {
                    // Get the owning slice of the current entity
                    AZ::SliceComponent::SliceInstanceAddress currentSliceAddress;
                    AzFramework::SliceEntityRequestBus::EventResult(currentSliceAddress, currentEntityId,
                        &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                    // Only need to detach entities owned by the same slice instance of the entity being reparented.
                    if (currentSliceAddress.GetInstance() != sourceSliceInstance.GetInstance())
                    {
                        continue;
                    }

                    bool isSubsliceEntity = false;
                    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSubsliceEntity, currentEntityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSubsliceEntity);

                    // If it's not a subslice entity we can directly remove the entity from the owning slice
                    if (!isSubsliceEntity)
                    {
                        entitiesToDetach.emplace_back(currentEntityId);
                        continue;
                    }

                    // If the current entity is a subslice entity we need to determine if we should detach it and its children from its current owning slice
                    AZ::SliceComponent::EntityAncestorList currentEntityAncestry;
                    currentSliceAddress.GetReference()->GetInstanceEntityAncestry(currentEntityId, currentEntityAncestry, std::numeric_limits<uint32_t>::max());

                    if (currentEntityAncestry.empty())
                    {
                        subslicesToDetach.clear();
                        entitiesToDetach.clear();

                        AZ_Error("SliceUtilities::Internal::PartitionEntityHierarchyForNonTrivialReparent",
                            false,
                            "Entity with Id: %s marked as a Subslice Root entity but has no slice ancestry, unable to proceed",
                            currentEntityId.ToString().c_str());

                        return;
                    }

                    bool subsliceEntityHandled = false;
                    AZ::SliceComponent::SliceInstanceAddress subsliceRootAncestorAddress;
                    AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> subsliceAncestry;

                    // Walk the ancestry until we either find an ancestor whose entity is a root (subslice root)
                    // or we find the ancestor's slice address in our handled subslices set
                    for (const AZ::SliceComponent::Ancestor& subsliceAncestor : currentEntityAncestry)
                    {
                        if (subslices.find(subsliceAncestor.m_sliceAddress) != subslices.end())
                        {
                            subsliceEntityHandled = true;
                            break;
                        }

                        if (subsliceAncestor.m_entity && IsRootEntity(*subsliceAncestor.m_entity))
                        {
                            subsliceRootAncestorAddress = subsliceAncestor.m_sliceAddress;
                            break;
                        }

                        subsliceAncestry.emplace_back(subsliceAncestor.m_sliceAddress);
                    }

                    // Verify we have not handled this subslice entity or a subslice owning this entity already
                    if (subsliceEntityHandled)
                    {
                        continue;
                    }

                    bool isSubsliceRoot = false;
                    AzToolsFramework::EditorEntityInfoRequestBus::EventResult(isSubsliceRoot, currentEntityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsSubsliceRoot);

                    if (!isSubsliceRoot)
                    {
                        // If it's not a subslice root we can directly remove the entity from the owning slice without worrying about children beneath it
                        entitiesToDetach.emplace_back(currentEntityId);
                        continue;
                    }

                    // Remove the owning slice from the ancestry list
                    subsliceAncestry.erase(subsliceAncestry.begin());

                    // Acquire a mapping from the asset id of the subslice to the live id of the source slice
                    // This will allow us to determine the specific list of entities associated with the subslice to extract from the source slice instance
                    AZ::SliceComponent::EntityIdToEntityIdMap liveToSubsliceIdMapping;
                    AZ::SliceComponent::GetMappingBetweenSubsliceAndSourceInstanceEntityIds(currentSliceAddress.GetInstance(),
                        subsliceAncestry,
                        subsliceRootAncestorAddress,
                        liveToSubsliceIdMapping,
                        true);

                    // Filter out MetaData entities from the list
                    // We will provide new ones for the subslice when we detach it into a standalone slice instance
                    AZStd::vector<AZ::EntityId> metaDataEntities;
                    for (const AZStd::pair<AZ::EntityId, AZ::EntityId>& liveToSubslicePair : liveToSubsliceIdMapping)
                    {
                        bool isMetaDataEntity = false;
                        AzToolsFramework::SliceMetadataEntityContextRequestBus::BroadcastResult(isMetaDataEntity, &AzToolsFramework::SliceMetadataEntityContextRequestBus::Events::IsSliceMetadataEntity, liveToSubslicePair.first);

                        if (isMetaDataEntity)
                        {
                            metaDataEntities.emplace_back(liveToSubslicePair.first);
                        }
                    }

                    for (AZ::EntityId metaDataEntity : metaDataEntities)
                    {
                        liveToSubsliceIdMapping.erase(metaDataEntity);
                    }

                    // Mark this instanceAddress as handled and add it to our detach list
                    subslices.emplace(subsliceRootAncestorAddress);
                    subslicesToDetach.emplace_back(AZStd::make_pair(subsliceRootAncestorAddress, liveToSubsliceIdMapping));
                }
            }

            //=========================================================================
            void GetSliceInstanceAncestry(const AZ::EntityId& entityId, SliceInstanceList& sliceInstanceAncestors)
            {
                AZ::SliceComponent::EntityAncestorList ancestors;
                GetSliceEntityAncestors(entityId, ancestors);

                for (const AZ::SliceComponent::Ancestor& ancestor : ancestors)
                {
                    if (ancestor.m_sliceAddress.IsValid())
                    {
                        sliceInstanceAncestors.push_back(ancestor.m_sliceAddress.GetInstance());
                    }
                }
            }

            //=========================================================================
            void GenerateSuggestedSliceFilenameFromEntities(const AzToolsFramework::EntityIdList& entities, AZStd::string& outName)
            {
                AZ_PROFILE_FUNCTION(AzToolsFramework);

                // Determine suggested save name for slice based on entity names
                // For example, with entities Entity0, Entity1, and Entity2, we would end up with
                // Entity0Entity1Entity2

                AZStd::string sliceName;
                size_t sliceNameCutoffLength = 32; ///< When naming a slice after its entities, we stop appending additional names once we've reached this cutoff length

                AzToolsFramework::EntityIdSet usedNameEntities;
                auto appendToSliceName = [&sliceName, sliceNameCutoffLength, &usedNameEntities](const AZ::EntityId& id) -> bool
                {
                    if (usedNameEntities.find(id) == usedNameEntities.end())
                    {
                        AZ::Entity* entity = nullptr;
                        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
                        if (entity)
                        {
                            AZStd::string entityNameFiltered = entity->GetName();

                            // Convert spaces in entity names to underscores
                            for (size_t i = 0; i < entityNameFiltered.size(); ++i)
                            {
                                char& character = entityNameFiltered.at(i);
                                if (character == ' ')
                                {
                                    character = '_';
                                }
                            }

                            sliceName.append(entityNameFiltered);
                            usedNameEntities.insert(id);
                            if (sliceName.size() > sliceNameCutoffLength)
                            {
                                return false;
                            }
                        }
                    }
                    return true;
                };

                bool sliceNameFull = false;

                if (!sliceNameFull)
                {
                    for (const AZ::EntityId& id : entities)
                    {
                        if (!appendToSliceName(id))
                        {
                            sliceNameFull = true;
                            break;
                        }
                    }
                }

                if (sliceName.size() == 0)
                {
                    sliceName = "NewSlice";
                }
                else if (AzFramework::StringFunc::Utf8::CheckNonAsciiChar(sliceName))
                {
                    sliceName = "NewSlice";
                }

                outName = sliceName;
            }

            //=========================================================================
            void GenerateSuggestedSlicePath(const AZStd::string& sliceName, const AZStd::string& targetDirectory, AZStd::string& suggestedFullPath)
            {
                AZ_PROFILE_FUNCTION(AzToolsFramework);

                // Generate full suggested path from sliceName - if given NewSlice as sliceName,
                // NewSlice_001.slice would be tried, and if that already existed we would suggest
                // the first unused number value (NewSlice_002.slice etc.)
                suggestedFullPath = targetDirectory;
                if (suggestedFullPath.size() > 0 &&
                    suggestedFullPath.at(suggestedFullPath.size() - 1) != '/')
                {
                    suggestedFullPath += "/";
                }

                // Convert spaces in entity names to underscores
                AZStd::string sliceNameFiltered = sliceName;
                for (size_t i = 0; i < sliceNameFiltered.size(); ++i)
                {
                    char& character = sliceNameFiltered.at(i);
                    if (character == ' ')
                    {
                        character = '_';
                    }
                }

                auto settings = AZ::UserSettings::CreateFind<SliceUserSettings>(AZ_CRC_CE("SliceUserSettings"), AZ::UserSettings::CT_LOCAL);
                if (settings->m_autoNumber)
                {
                    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
                    AZStd::string possiblePath;

                    const AZ::u32 maxSliceNumber = 1000;
                    for (AZ::u32 sliceNumber = 1; sliceNumber < maxSliceNumber; ++sliceNumber)
                    {
                        possiblePath = AZStd::string::format("%s%s_%3.3u%s", suggestedFullPath.c_str(), sliceNameFiltered.c_str(), sliceNumber, GetSliceFileExtension().c_str());
                        if (!fileIO || !fileIO->Exists(possiblePath.c_str()))
                        {
                            suggestedFullPath = possiblePath;
                            break;
                        }
                    }
                }
                else
                {
                    // use the entity name as the file name regardless of it already existing, the OS will ask the user to overwrite the file in that case.
                    suggestedFullPath = AZStd::string::format("%s%s%s", suggestedFullPath.c_str(), sliceNameFiltered.c_str(), GetSliceFileExtension().c_str());
                }
            }

            //=========================================================================
            void SetSliceSaveLocation(const AZStd::string& path, AZ::u32 settingsId)
            {
                auto settings = AZ::UserSettings::CreateFind<SliceUserSettings>(settingsId, AZ::UserSettings::CT_LOCAL);
                settings->m_saveLocation = path;
            }

            //=========================================================================
            bool GetSliceSaveLocation(AZStd::string& path, AZ::u32 settingsId)
            {
                auto settings = AZ::UserSettings::Find<SliceUserSettings>(settingsId, AZ::UserSettings::CT_LOCAL);
                if (settings)
                {
                    path = settings->m_saveLocation;
                    return true;
                }

                return false;
            }

            //=========================================================================
            AZ::Vector3 GetSliceRootPosition(const AZ::EntityId commonRoot, const AzToolsFramework::EntityList& selectionRootEntities)
            {
                AZ::Vector3 sliceRootTranslation = AZ::Vector3::CreateZero();
                float sliceZmin = std::numeric_limits<float>::max();

                int count = 0;
                for (AZ::Entity* selectionRootEntity : selectionRootEntities)
                {
                    if (selectionRootEntity)
                    {
                        AzToolsFramework::Components::TransformComponent* transformComponent =
                            selectionRootEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();

                        if (transformComponent)
                        {
                            count++;
                            AZ::Vector3 currentPosition;
                            if (commonRoot.IsValid())
                            {
                                currentPosition = transformComponent->GetLocalTranslation();
                            }
                            else
                            {
                                currentPosition = transformComponent->GetWorldTranslation();
                            }

                            sliceRootTranslation += currentPosition;
                            sliceZmin = AZ::GetMin<float>(sliceZmin, currentPosition.GetZ());
                        }
                    }
                }

                sliceRootTranslation = (sliceRootTranslation / static_cast<float>(count));

                sliceRootTranslation.SetZ(sliceZmin);

                return sliceRootTranslation;
            }

            //=========================================================================
            AZ::u32 CountDifferencesVersusSlice(AZ::EntityId entityId, AZ::Entity* compareTo, AZ::SerializeContext& serializeContext, const InstanceDataHierarchy::Address* fieldAddress /*= nullptr*/)
            {
                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);

                if (!entity || !compareTo)
                {
                    return 0;
                }

                const bool isRootEntity = IsRootEntity(*compareTo);

                InstanceDataHierarchy source;
                source.AddRootInstance<AZ::Entity>(entity);
                source.Build(&serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

                InstanceDataHierarchy target;
                target.AddRootInstance<AZ::Entity>(const_cast<AZ::Entity*>(compareTo));
                target.Build(&serializeContext, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);

                AZStd::unordered_set<const InstanceDataNode*> differentNodes;

                AZStd::function<void(const InstanceDataNode*)> nodeChanged =
                    [&differentNodes, isRootEntity, fieldAddress](const InstanceDataNode* node)
                {
                    if (node)
                    {
                        if (fieldAddress)
                        {
                            const InstanceDataHierarchy::Address nodeAddress = node->ComputeAddress();

                            if (fieldAddress->size() > nodeAddress.size())
                            {
                                return;
                            }

                            // If nodeAddress ends with the field's address, the filter matches.
                            auto nodeIter = nodeAddress.begin();
                            for (auto filterIter = fieldAddress->begin(); filterIter != fieldAddress->end(); ++filterIter, ++nodeIter)
                            {
                                if (*filterIter != *nodeIter)
                                {
                                    return;
                                }
                            }
                        }

                        // If the node has any un-hidden parent, count it as a difference.
                        while (node)
                        {
                            if (!SliceUtilities::IsNodePushable(*node, isRootEntity))
                            {
                                break;
                            }

                            const AzToolsFramework::NodeDisplayVisibility visibility = CalculateNodeDisplayVisibility(*node, true);
                            if (visibility == AzToolsFramework::NodeDisplayVisibility::Visible)
                            {
                                differentNodes.insert(node);
                                break;
                            }

                            node = node->GetParent();
                        }
                    }
                };

                InstanceDataHierarchy::NewNodeCB newCallback =
                    [&nodeChanged](InstanceDataNode* targetNode, AZStd::vector<AZ::u8>& /*data*/)
                {
                    nodeChanged(targetNode);
                };

                InstanceDataHierarchy::RemovedNodeCB removedCallback =
                    [&nodeChanged](const InstanceDataNode* sourceNode, InstanceDataNode* /*targetNodeParent*/)
                {
                    nodeChanged(sourceNode);
                };

                InstanceDataHierarchy::ChangedNodeCB changedCallback =
                    [&nodeChanged](const InstanceDataNode* sourceNode, InstanceDataNode* /*targetNode*/, AZStd::vector<AZ::u8>& /*sourceData*/, AZStd::vector<AZ::u8>& /*targetData*/)
                {
                    nodeChanged(sourceNode);
                };

                InstanceDataHierarchy::CompareHierarchies(&source, &target,
                    InstanceDataHierarchy::DefaultValueComparisonFunction,
                    &serializeContext,
                    newCallback, removedCallback, changedCallback);

                return static_cast<AZ::u32>(differentNodes.size());
            }

            InvalidSliceReferencesWarningResult CheckForInvalidSliceReferences(QWidget* parent, AZ::Data::Asset<AZ::SliceAsset> sliceAsset)
            {
                AZ::SliceComponent* assetComponent = sliceAsset.Get()->GetComponent();
                if (assetComponent == nullptr)
                {
                    // The sliceComponent couldn't be found, let the later quick slice push logic handle this.
                    return InvalidSliceReferencesWarningResult::Save;
                }

                // If there are any invalid slices, warn the user and allow them to choose the next step.
                const AZ::SliceComponent::SliceList& invalidSlices = assetComponent->GetInvalidSlices();
                if (invalidSlices.size() > 0)
                {
                    // Assume an invalid slice count of 1 because this is a quick push, which only has one target.
                    return DisplayInvalidSliceReferencesWarning(parent,
                        /*invalidSliceCount*/ 1,
                        invalidSlices.size(),
                        /*showDetailsButton*/ true);
                }

                // If there were no invalid slices, then continue with the save.
                return InvalidSliceReferencesWarningResult::Save;
            }

            void FlattenAncestry(const AZ::SliceComponent::Ancestor& toFlatten, const AZ::SliceComponent::Ancestor& toFlattenImmediateAncestor)
            {
                if (!toFlatten.m_sliceAddress.IsValid() || !toFlattenImmediateAncestor.m_sliceAddress.IsValid())
                {
                    AZ_Warning("Slice", false, "Failed to flatten slice ancestry, slice ancestry invalid");
                    return;
                }

                AZ::SerializeContext* serializeContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

                if (!serializeContext)
                {
                    AZ_Error("Slice", false, "Could not retrieve application serialize context.");
                    return;
                }

                // We get the reference and asset of what's being flattened to generate a clone of its slice component
                const AZ::SliceComponent::SliceReference* toFlattenReference = toFlatten.m_sliceAddress.GetReference();
                const AZ::Data::Asset<AZ::SliceAsset> toFlattenAsset = toFlattenReference->GetSliceAsset();

                // We get the asset of our immediate ancestor being flattened into us to help locate the cloned ancestors root
                const AZ::SliceComponent::SliceReference* ancestorReference = toFlattenImmediateAncestor.m_sliceAddress.GetReference();
                const AZ::Data::Asset<AZ::SliceAsset> ancestorAsset = ancestorReference->GetSliceAsset();

                // Clone the component so we can safely manipulate it
                AZ::SliceComponent::SliceInstanceToSliceInstanceMap cloneMap;
                AZ::SliceComponent* toFlattenComponentClone = toFlattenAsset.Get()->GetComponent()->Clone(*serializeContext, &cloneMap);

                if (!toFlattenComponentClone)
                {
                    AZ_Error("Slice", false, "Failed to clone Slice Component during FlattenAncestry");
                    return;
                }

                // Confirm our component is instantiated so we operate on the slice post data patching
                toFlattenComponentClone->Instantiate();

                // Get the cloned address of the ancestor we are flattening into ourselves
                auto findIt = cloneMap.find(toFlattenImmediateAncestor.m_sliceAddress);
                if (findIt == cloneMap.end() || !findIt->second.IsValid())
                {
                    AZ_Error("Slice", false, "Failed to recover instance address from Slice Component clone in FlattenSlice");
                    return;
                }
                AZ::SliceComponent::SliceInstanceAddress ancestorAddressClone = findIt->second;

                // Get the root entity of what we're flattening into ourselves
                AZ::EntityId ancestorRootEntityClone;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(ancestorRootEntityClone, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetRootEntityIdOfSliceInstance, ancestorAddressClone);

                // Flatten all ancestry of toFlatten into toFlatten breaking its ancestral dependencies
                bool flattenSuccess = toFlattenComponentClone->FlattenSlice(ancestorAddressClone.GetReference(), ancestorRootEntityClone);

                // Flattening failed, cannot complete operation, no need for error message as FlattenSlice covers that
                if (!flattenSuccess)
                {
                    return;
                }

                // Commit the flattened clone into the actual slice asset
                SliceTransaction::TransactionPtr transaction = SliceTransaction::BeginSliceOverwrite(toFlattenAsset, *toFlattenComponentClone, serializeContext);

                if (!transaction)
                {
                    AZ_Error("Slice", false, "Failed to create a transaction for FlattenAncestry");
                    return;
                }

                transaction->Commit(toFlattenAsset.GetId(), nullptr, nullptr, SliceTransaction::SliceCommitFlags::DisableUndoCapture);


                // Flush undo for this operation because we cannot undo it and going back further is undefined
                AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::FlushUndo);
            }

            bool CalculatePushableChangesPerAsset(
                AZStd::unordered_map<AZ::Data::AssetId, int>& pushableChangesPerAsset,
                AZ::SerializeContext& serializeContext,
                const AZStd::vector<AZ::Data::AssetId>& sliceDisplayOrder,
                const AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<EntityAncestorPair>>& assetEntityAncestorMap,
                const size_t& numRelevantEntitiesInSlices,
                const InstanceDataNode::Address* fieldAddress)
            {

                const AZ::u32 kMaxEntitiesForOverrideCalculation = 5;    // Max # of entities for which we'll do a full hierarchy comparison (to preview # of overrides).

                // Track and return if any pushable changes are available. The quick push menu
                // will use this result to check if it should display a message telling the user no quick saves area available.
                bool pushableChangesAvailable = false;
                for (const AZ::Data::AssetId& sliceAssetId : sliceDisplayOrder)
                {
                    AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<EntityAncestorPair>>::const_iterator ancestorIter =
                        assetEntityAncestorMap.find(sliceAssetId);
                    if (ancestorIter == assetEntityAncestorMap.end())
                    {
                        continue;
                    }

                    const AZStd::vector<EntityAncestorPair>& entityAncestors = ancestorIter->second;
                    if (entityAncestors.size() != numRelevantEntitiesInSlices)
                    {
                        continue;
                    }

                    pushableChangesPerAsset[sliceAssetId] = 0;

                    if (numRelevantEntitiesInSlices <= kMaxEntitiesForOverrideCalculation)
                    {
                        for (const EntityAncestorPair& entityAncestor : entityAncestors)
                        {
                            pushableChangesPerAsset[sliceAssetId] += Internal::CountDifferencesVersusSlice(
                                entityAncestor.first,
                                entityAncestor.second.get(),
                                serializeContext,
                                fieldAddress);
                        }
                    }
                    if (!pushableChangesAvailable && pushableChangesPerAsset[sliceAssetId] > 0)
                    {
                        pushableChangesAvailable = true;
                    }
                }
                return pushableChangesAvailable;
            }

            void GetEntitiesInSlices(const AzToolsFramework::EntityIdList& selectedEntities, AZ::u32& entitiesInSlices, AZ::SliceComponent::SliceInstanceAddressSet& sliceInstances)
            {
                // Identify all slice instances affected by the selected entity set.
                entitiesInSlices = 0;
                for (const AZ::EntityId& entityId : selectedEntities)
                {
                    AZ::SliceComponent::SliceInstanceAddress sliceAddress;
                    AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entityId,
                        &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                    if (sliceAddress.IsValid())
                    {
                        sliceInstances.insert(sliceAddress);
                        ++entitiesInSlices;
                    }
                }
            }

            void ResaveSlice(AZStd::shared_ptr<AZ::Entity> sliceEntity, const AZStd::string& fullFilePath)
            {
                AZStd::string tmpFileName;
                bool tmpFilesaved = false;
                // here we are saving the slice to a temp file instead of the original file and then copying the temp file to the original file.
                // This ensures that AP will not a get a file change notification on an incomplete slice file causing it to fail processing. Temp files are ignored by AP.
                if (AZ::IO::CreateTempFileName(fullFilePath.c_str(), tmpFileName))
                {
                    AZ::IO::FileIOStream fileStream(tmpFileName.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);

                    if (fileStream.IsOpen())
                    {
                        tmpFilesaved = AZ::Utils::SaveObjectToStream<AZ::Entity>(fileStream, GetSliceStreamFormat(), sliceEntity.get());
                    }

                    using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
                    SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit, fullFilePath.c_str(), true,
                        [fullFilePath, tmpFileName, tmpFilesaved](bool /*success*/, const AzToolsFramework::SourceControlFileInfo& info)
                    {
                        if (!info.IsReadOnly())
                        {
                            if (tmpFilesaved && AZ::IO::SmartMove(tmpFileName.c_str(), fullFilePath.c_str()))
                            {
                                // Bump the slice asset up in the asset processor's queue.
                                AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, fullFilePath.c_str());
                            }
                        }
                        else
                        {
                            QWidget* mainWindow = nullptr;
                            AzToolsFramework::EditorRequests::Bus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
                            QMessageBox::warning(mainWindow, QObject::tr("Unable to Modify Slice"),
                                QObject::tr("File is not writable."), QMessageBox::Ok, QMessageBox::Ok);
                        }
                    });
                }
                else
                {
                    QWidget* mainWindow = nullptr;
                    AzToolsFramework::EditorRequests::Bus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);
                    QMessageBox::warning(mainWindow, QObject::tr("Unable to Modify Slice"),
                        QObject::tr("Unable to Modify Slice (%1). Cannot create a temporary file for writing data in the same folder.").arg(fullFilePath.c_str()),
                        QMessageBox::Ok, QMessageBox::Ok);
                }
            }

            void AnalyseAncestoryForPushableEntities(const AZ::EntityId& entityId,
                AZ::SliceComponent::SliceInstanceAddress entitySliceAddress,
                AZ::SliceComponent::SliceInstanceAddress& transformAncestorSliceAddress,
                AZ::SliceComponent::EntityAncestorList& sliceAncestryToPushTo,
                AZStd::unordered_map<AZ::Data::AssetId, EntityIdSet>& unpushableEntityIdsPerAsset,
                AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityAncestorList>>& newChildEntityIdAncestorPairs,
                AZStd::vector< AZ::Data::AssetId>& rootAncestorPushList)
            {
                if (entitySliceAddress.IsValid())
                {
                    if (!sliceAncestryToPushTo.empty())
                    {
                        // trivial reject, don't allow pushes to multiple slices at once due to complexity in cyclic dependency check.
                        if (!sliceAncestryToPushTo.empty())
                        {
                            AZ::Data::AssetId rootAssetId = sliceAncestryToPushTo.back().m_sliceAddress.GetReference()->GetSliceAsset().GetId();

                            for (AZ::Data::AssetId pushAncestor : rootAncestorPushList)
                            {
                                if (pushAncestor != rootAssetId)
                                {
                                    // Pushing addition of multiple slice-owned entities is currently disabled due to
                                    // the complexity of detecting cycles in slice hierarchy.
                                    EntityIdSet* unpushableEntityIds = &unpushableEntityIdsPerAsset[rootAssetId];
                                    if (AZStd::find(unpushableEntityIds->begin(), unpushableEntityIds->end(), entityId) == unpushableEntityIds->end())
                                    {
                                        unpushableEntityIds->insert(entityId);
                                    }

                                    // this entity can't be pushed to this ancestry, no further checking to do
                                    return;
                                }
                            }
                        }
                    }

                    // This is an entity that already belongs to a slice, need to verify it's a valid add
                    if (entitySliceAddress == transformAncestorSliceAddress)
                    {
                        // Entity shares slice instance address with transform ancestor, so it doesn't need to be added - it's already there!
                        return;
                    }

                    // Otherwise, this is a slice-owned entity that we want to push.
                    // At this point we have verified that it'd be safe to push to the immediate slice instance,
                    // but in the PushWidget the user will have the option of pushing to any slice asset
                    // in the sliceAncestryToPushTo. We need to check each ancestry entry and cull out any
                    // pushes that would result in cyclic asset dependencies.
                    // Example: Slice1 contains Slice2. I have a separate instance of Slice2, call it Slice2b.
                    // It is valid to push Slice2b to Slice1, since Slice1 would then have two instances of Slice2.
                    // But it would be invalid to push the addition of Slice2b to Slice2, since then Slice2 would
                    // reference itself.

                    bool canPush = false;
                    for (auto ancestorIt = sliceAncestryToPushTo.begin(); ancestorIt != sliceAncestryToPushTo.end(); ++ancestorIt)
                    {
                        const AZ::SliceComponent::SliceInstanceAddress& targetInstanceAddress = ancestorIt->m_sliceAddress;
                        if (SliceUtilities::CheckSliceAdditionCyclicDependencySafe(entitySliceAddress, targetInstanceAddress))
                        {
                            canPush = true;

                            newChildEntityIdAncestorPairs.emplace_back(entityId, sliceAncestryToPushTo);
                        }
                        else
                        {
                            //remember that this entity is unpushble to this slice asset
                            EntityIdSet* unpushableEntityIds = &unpushableEntityIdsPerAsset[targetInstanceAddress.GetReference()->GetSliceAsset().GetId()];
                            if (AZStd::find(unpushableEntityIds->begin(), unpushableEntityIds->end(), entityId) == unpushableEntityIds->end())
                            {
                                unpushableEntityIds->insert(entityId);
                            }
                            break;// Once you find one invalid ancestor, all the rest will be as well
                        }
                    }

                    if (canPush)
                    {
                        //remember we're trying to push to this root, so we don't try to push to any others
                        size_t ancestrySize = sliceAncestryToPushTo.size();
                        rootAncestorPushList.push_back(sliceAncestryToPushTo[ancestrySize-1].m_sliceAddress.GetReference()->GetSliceAsset().GetId());
                    }
                }
                else
                {
                    // This is an entity that doesn't belong to a slice yet, consider it for addition
                    newChildEntityIdAncestorPairs.emplace_back(entityId, AZStd::move(sliceAncestryToPushTo));
                }
            }

            AZStd::shared_ptr<AZ::Entity> GetSliceEntityForAssetId(const AZ::Data::AssetId& assetId)
            {
                if (!assetId.IsValid())
                {
                    AZ_Warning("Slice", false, "AssetId is invalid for asset %s", assetId.ToString<AZStd::string>().c_str());
                    return nullptr;
                }

                AZStd::string relativePath, fullPath;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(relativePath, &AZ::Data::AssetCatalogRequests::GetAssetPathById, assetId);

                if (relativePath.empty())
                {
                    AZ_Warning("Slice", false, "No relative path found for asset %s", assetId.ToString<AZStd::string>().c_str());
                    return nullptr;
                }

                bool fullPathFound = false;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(fullPathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relativePath, fullPath);

                if (fullPathFound)
                {
                    AZStd::shared_ptr<AZ::Entity> sliceEntity(AZ::Utils::LoadObjectFromFile<AZ::Entity>(fullPath, nullptr,
                        AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading)));

                    return sliceEntity;
                }
                else
                {
                    AZ_Warning("Slice", false, "Could not find full path for asset %s", assetId.ToString<AZStd::string>().c_str());
                }

                return nullptr;
            }


        } // namespace Internal

        void SliceUserSettings::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SliceUserSettings>()
                    ->Version(1)
                    ->Field("m_saveLocation", &SliceUserSettings::m_saveLocation)
                    ->Field("m_autoNumber", &SliceUserSettings::m_autoNumber);
            }
        }

        void Reflect(AZ::ReflectContext* context)
        {
            SliceUserSettings::Reflect(context);
        }

        QString GetSliceItemIconPath() { return ":/PropertyEditor/Resources/slice_item.png"; }

        QString GetSliceItemChangedIconPath() { return ":/PropertyEditor/Resources/Slice_Handle_Modified.svg"; }

        QString GetLShapeIconPath() { return ":/PropertyEditor/Resources/l_shape.png"; }

        QString GetSliceEntityIconPath() { return ":/PropertyEditor/Resources/Slice_Entity.svg"; }

        QString GetWarningIconPath() { return ":/PropertyEditor/Resources/warning.png"; }

        AZ::u32 GetSliceItemHeight() { return 16; }

        QSize GetSliceItemIconSize() { return QSize(20, GetSliceItemHeight()); }

        QSize GetLShapeIconSize() { return QSize(12, GetSliceItemHeight()); }

        QSize GetWarningIconSize() { return QSize(26, 25); }

        int GetWarningLabelMinimumWidth() { return 300; }

        AZ::u32 GetSliceHierarchyMenuIdentationPerLevel() { return 5; }

        int GetSliceSelectFontSize() { return 10; }

        AZStd::string GetSliceFileExtension() { return ".slice"; }
    } // namespace SliceUtilities
} // AzToolsFramework
