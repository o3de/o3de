/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"
#include <AzToolsFramework/Slice/SliceUtilities.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Slice/SliceTransaction.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/UI/Slice/SlicePushWidget.hxx>

#include <QtWidgets/QWidget>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QErrorMessage>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QThread>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/Tools/UiSystemToolsBus.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

//////////////////////////////////////////////////////////////////////////
UiSliceManager::UiSliceManager(AzFramework::EntityContextId entityContextId)
    : m_entityContextId(entityContextId)
{
    UiEditorEntityContextNotificationBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
UiSliceManager::~UiSliceManager()
{
    UiEditorEntityContextNotificationBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::OnSliceInstantiationFailed(const AZ::Data::AssetId&, const AzFramework::SliceInstantiationTicket&)
{
    QMessageBox::warning(QApplication::activeWindow(),
        QStringLiteral("Cannot Instantiate UI Slice"),
        QString("Slice cannot be instantiated. Check that it is a slice containing UI elements."),
        QMessageBox::Ok);
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::InstantiateSlice(const AZ::Data::AssetId& assetId, AZ::Vector2 viewportPosition, int childIndex)
{
    AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
    sliceAsset.Create(assetId, true);

    EBUS_EVENT_ID(m_entityContextId, UiEditorEntityContextRequestBus, InstantiateEditorSliceAtChildIndex, sliceAsset, viewportPosition, childIndex);
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::InstantiateSliceUsingBrowser([[maybe_unused]] HierarchyWidget* hierarchy, AZ::Vector2 viewportPosition)
{
    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Slice");
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (!selection.IsValid())
    {
        return;
    }

    auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
    AZ_Assert(product, "Selection is invalid.");

    InstantiateSlice(product->GetAssetId(), viewportPosition);
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::MakeSliceFromSelectedItems(HierarchyWidget* hierarchy, bool inheritSlices)
{
    QTreeWidgetItemRawPtrQList selectedItems(hierarchy->selectedItems());

    HierarchyItemRawPtrList items = SelectionHelpers::GetSelectedHierarchyItems(hierarchy,
        selectedItems);

    AzToolsFramework::EntityIdList selectedEntities;
    for (auto item : items)
    {
        selectedEntities.push_back(item->GetEntityId());
    }

    MakeSliceFromEntities(selectedEntities, inheritSlices);
}

bool UiSliceManager::IsRootEntity([[maybe_unused]] const AZ::Entity& entity) const
{
    // This is only used by IsNodePushable. For the UI system, we allow the root slice
    // to be pushed updates, so we always return false here to allow that. If the UI
    // system ever wants to leverage NotPushableOnSliceRoot, we'll need to revisit this.
    return false;
}

AZ::SliceComponent* UiSliceManager::GetRootSlice() const
{
    AZ::SliceComponent* rootSlice = nullptr;
    EBUS_EVENT_ID_RESULT(rootSlice, m_entityContextId, UiEditorEntityContextRequestBus, GetUiRootSlice);
    return rootSlice;
}

//////////////////////////////////////////////////////////////////////////
// PRIVATE MEMBER FUNCTIONS
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::MakeSliceFromEntities(AzToolsFramework::EntityIdList& entities, bool inheritSlices)
{
    // expand the list of entities to include all child entities
    AzToolsFramework::EntityIdSet entitiesAndDescendants = GatherEntitiesAndAllDescendents(entities);

    const AZStd::string slicesAssetsPath = "@projectroot@/UI/Slices";

    if (!gEnv->pFileIO->Exists(slicesAssetsPath.c_str()))
    {
        gEnv->pFileIO->CreatePath(slicesAssetsPath.c_str());
    }

    char path[AZ_MAX_PATH_LEN] = { 0 };
    gEnv->pFileIO->ResolvePath(slicesAssetsPath.c_str(), path, AZ_MAX_PATH_LEN);

    MakeNewSlice(entitiesAndDescendants, path, inheritSlices);
}

//////////////////////////////////////////////////////////////////////////
bool UiSliceManager::MakeNewSlice(
    const AzToolsFramework::EntityIdSet& entities,
    const char* targetDirectory,
    bool inheritSlices,
    AZ::SerializeContext* serializeContext)
{
    AZ_PROFILE_FUNCTION(AzToolsFramework);

    if (entities.empty())
    {
        return false;
    }

    if (!serializeContext)
    {
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve application serialize context.");
    }

    // Save a reference to our currently active window since it will be
    // temporarily null after QFileDialogs close, which we need in order to
    // be able to parent our message dialogs properly
    QWidget* activeWindow = QApplication::activeWindow();

    //
    // Check for entity references outside of selected entities - we don't allow this in UI slices
    //
    AzToolsFramework::EntityIdSet entitiesToInclude = entities;
    {
        AzToolsFramework::EntityIdSet allReferencedEntities;
        bool hasExternalReferences = false;
        AzToolsFramework::SliceUtilities::GatherAllReferencedEntitiesAndCompare(entitiesToInclude, allReferencedEntities, hasExternalReferences, *serializeContext);

        if (hasExternalReferences)
        {
            const AZStd::string message = AZStd::string::format(
                "Some of the selected entities reference entities not contained in the selection and its children.\n"
                "UI slices cannot contain references to outside of the slice.\n");

            QMessageBox::warning(activeWindow, QStringLiteral("Create Slice"),
                QString(message.c_str()), QMessageBox::Ok);

            return false;
        }
    }

    //
    // Verify single root and generate an ordered entity list
    //
    AzToolsFramework::EntityIdList orderedEntityList;
    AZ::Entity* insertBefore = nullptr;
    AZ::Entity* commonParent = nullptr;
    {
        commonParent = ValidateSingleRootAndGenerateOrderedEntityList(entitiesToInclude, orderedEntityList, insertBefore);
        if (!commonParent)
        {
            QMessageBox::warning(activeWindow,
                QStringLiteral("Cannot Create UI Slice"),
                QString("The slice cannot be created because there is no single element in the selection that is parent "
                    "to all other elements in the selection."
                    "Please make sure your slice contains only one root entity.\n\n"
                    "You may want to create a new entity, and assign it as the parent of your existing root entities."),
                QMessageBox::Ok);
            return false;
        }

        AZ_Assert(!orderedEntityList.empty(), "Empty orderedEntityList during UI slice creation!");
    }

    //
    // Determine slice asset file name/path - default to name of root entity, ask user
    //
    AZStd::string sliceName;
    AZStd::string sliceFilePath;
    {
        AZStd::string suggestedName = "UISlice";
        UiElementBus::EventResult(suggestedName, orderedEntityList[0], &UiElementBus::Events::GetName);
        if (!AzToolsFramework::SliceUtilities::QueryUserForSliceFilename(suggestedName, targetDirectory, AZ_CRC("UISliceUserSettings", 0x4f30f608), activeWindow, sliceName, sliceFilePath))
        {
            // User cancelled slice creation or error prevented continuation (related warning dialog boxes, if necessary, already done at this point)
            return false;
        }
    }

    //
    // Setup and execute transaction for the new slice.
    //
    {
        AZ_PROFILE_SCOPE(AzToolsFramework, "UiSliceManager::MakeNewSlice:SetupAndExecuteTransaction");

        using AzToolsFramework::SliceUtilities::SliceTransaction;

        // PostSaveCallback for slice creation: kick off async replacement of source entities with an instance of the new slice.
        SliceTransaction::PostSaveCallback postSaveCallback =
            [this, &entitiesToInclude, &commonParent, &insertBefore]
        (SliceTransaction::TransactionPtr transaction, const char* fullPath, const SliceTransaction::SliceAssetPtr& /*asset*/) -> void
        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "UiSliceManager::MakeNewSlice:PostSaveCallback");
            // Once the asset is processed and ready, we can replace the source entities with an instance of the new slice.
            UiEditorEntityContextRequestBus::Event(m_entityContextId,
                &UiEditorEntityContextRequestBus::Events::QueueSliceReplacement,
                fullPath, transaction->GetLiveToAssetEntityIdMap(), entitiesToInclude, commonParent, insertBefore);
        };

        SliceTransaction::TransactionPtr transaction = SliceTransaction::BeginNewSlice(nullptr, serializeContext);

        // Add entities
        {
            AZ_PROFILE_SCOPE(AzToolsFramework, "UiSliceManager::MakeNewSlice:SetupAndExecuteTransaction:AddEntities");
            for (const AZ::EntityId& entityId : orderedEntityList)
            {
                SliceTransaction::Result addResult = transaction->AddEntity(entityId, !inheritSlices ? SliceTransaction::SliceAddEntityFlags::DiscardSliceAncestry : 0);
                if (!addResult)
                {
                    QMessageBox::warning(activeWindow, QStringLiteral("Slice Save Failed"),
                        QString(addResult.GetError().c_str()), QMessageBox::Ok);
                    return false;
                }
            }
        }

        SliceTransaction::Result result = transaction->Commit(
            sliceFilePath.c_str(),
            nullptr,
            postSaveCallback,
            AzToolsFramework::SliceUtilities::SliceTransaction::SliceCommitFlags::DisableUndoCapture);

        if (!result)
        {
            QMessageBox::warning(activeWindow, QStringLiteral("Slice Save Failed"),
                QString(result.GetError().c_str()), QMessageBox::Ok);
            return false;
        }

        return true;
    }
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::GetTopLevelEntities(const AZ::SliceComponent::EntityList& entities, AZ::SliceComponent::EntityList& topLevelEntities)
{
    AZStd::unordered_set<AZ::Entity*> allEntities;
    allEntities.insert(entities.begin(), entities.end());

    for (auto entity : entities)
    {
        // if this entities parent is not in the set then it is a top-level
        AZ::Entity* parentElement = nullptr;
        EBUS_EVENT_ID_RESULT(parentElement, entity->GetId(), UiElementBus, GetParent);

        if (parentElement)
        {
            if (allEntities.count(parentElement) == 0)
            {
                topLevelEntities.push_back(entity);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// This is similar to ToolsApplicationRequests::GatherEntitiesAndAllDescendents
// except that function assumes that the entities are supporting the the AZ::TransformBus
// for hierarchy. This UI-specific version uses the UiElementBus
AzToolsFramework::EntityIdSet UiSliceManager::GatherEntitiesAndAllDescendents(const AzToolsFramework::EntityIdList& inputEntities)
{
    AzToolsFramework::EntityIdSet output;
    AzToolsFramework::EntityIdList tempList;
    for (const AZ::EntityId& id : inputEntities)
    {
        output.insert(id);

        LyShine::EntityArray descendants;
        EBUS_EVENT_ID(id, UiElementBus, FindDescendantElements, [](const AZ::Entity*) { return true; }, descendants);

        for (auto descendant : descendants)
        {
            output.insert(descendant->GetId());
        }
    }

    return output;
}

//////////////////////////////////////////////////////////////////////////
// PreSaveCallback for SliceTransactions in Slice Pushes
// Fails pushes if: 
// - referenced entities are not included in the slice
// - added entities in push are not referenced as children of entities in slice
// - any entities have become orphaned with selected push options
// - there's more than one root entity
AzToolsFramework::SliceUtilities::SliceTransaction::Result SlicePreSaveCallbackForUiEntities(
    AzToolsFramework::SliceUtilities::SliceTransaction::TransactionPtr transaction,
    [[maybe_unused]] const char* fullPath,
    AzToolsFramework::SliceUtilities::SliceTransaction::SliceAssetPtr& asset)
{
    AZ_PROFILE_SCOPE(AzToolsFramework, "SlicePreSaveCallbackForUiEntities");

    // we want to ensure that "bad" data never gets pushed to a slice
    // This mostly relates to the m_childEntityIdOrder array since this is something that
    // the UI Editor manages closely and requires to be consistent.

    AZ::SerializeContext* serializeContext = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    AZ_Assert(serializeContext, "Failed to retrieve application serialize context.");

    auto& assetDb = AZ::Data::AssetManager::Instance();
    AZ::Data::Asset<AZ::SliceAsset> currentAsset =
        assetDb.FindAsset<AZ::SliceAsset>(transaction->GetTargetAsset().GetId(), AZ::Data::AssetLoadBehavior::Default);

    AZ::SliceComponent* clonedSliceComponent = asset.Get()->GetComponent();
    AZ::SliceComponent* currentSliceComponent = currentAsset.Get()->GetComponent();

    AZ::SliceComponent::EntityList clonedEntities;
    clonedSliceComponent->GetEntities(clonedEntities);

    AZ::SliceComponent::EntityList currentEntities;
    currentSliceComponent->GetEntities(currentEntities);

    // store a set of pairs which are the EntityId being referenced and the Entity that is referencing it
    using ReferencedEntityPair = AZStd::pair<AZ::EntityId, AZ::Entity*>;
    AZStd::unordered_set<ReferencedEntityPair> referencedEntities;

    AZStd::unordered_set<AZ::EntityId> referencedChildEntities;
    AZStd::unordered_set<AZ::EntityId> clonedEntityIds;
    AZStd::unordered_set<AZ::EntityId> addedEntities;

    for (auto clonedEntity : clonedEntities)
    {
        clonedEntityIds.insert(clonedEntity->GetId());

        auto iter = AZStd::find_if(currentEntities.begin(), currentEntities.end(),
            [clonedEntity](AZ::Entity* entity) -> bool
            {
                return entity->GetId() == clonedEntity->GetId();
            });

        if (iter == currentEntities.end())
        {
            // this clonedEntity is an addition to the slice
            addedEntities.insert(clonedEntity->GetId());
        }

        AZ::EntityUtils::EnumerateEntityIds(clonedEntity,
            [clonedEntity, &referencedEntities, &referencedChildEntities]
        (const AZ::EntityId& id, bool isEntityId, const AZ::SerializeContext::ClassElement* elementData) -> void
            {
                if (!isEntityId && id.IsValid())
                {
                    // Include this id.
                    referencedEntities.insert({ id, clonedEntity });

                    // Check if this is a child reference. We can detect that because the EntityId is in the "ChildEntityId"
                    // member of the ChildEntityIdOrderEntry struct.
                    if (elementData && !elementData->m_editData)
                    {
                        if (strcmp(elementData->m_name, "ChildEntityId") == 0)
                        {
                            referencedChildEntities.insert(id);
                        }
                    }
                }
            }, serializeContext);
    }

    // Issue a warning if any referenced entities are not in the slice being created
    for (auto referencedEntityPair : referencedEntities)
    {
        const AZ::EntityId& referencedEntityId = referencedEntityPair.first;

        if (clonedEntityIds.count(referencedEntityId) == 0)
        {
            const AZ::SliceComponent::EntityIdToEntityIdMap& entityIdMap = transaction->GetLiveToAssetEntityIdMap();

            AZ::Entity* referencingEntity = referencedEntityPair.second;
            AZ::EntityId referencingEntityId = referencingEntity->GetId();
            // in order to get the hierarchical name of the referencing entity we need to find the live version of the entity
            // this requires a reverse look up in the entityIdMap
            AZ::EntityId liveReferencingEntityId;
            for (auto entry : entityIdMap)
            {
                if (entry.second == referencingEntityId)
                {
                    liveReferencingEntityId = entry.first;
                    break;
                }
            }

            AZStd::string referencingEntityName;
            if (liveReferencingEntityId.IsValid())
            {
                referencingEntityName = EntityHelpers::GetHierarchicalElementName(liveReferencingEntityId);
            }
            else
            {
                // this should not happen, if it does just use the non-hierarchical name
                referencingEntityName = referencingEntity->GetName();
            }

            // Ideally we could find a hierarchical field name like "UiButtonComponent/State Actions/Hover[2]/Color/Target" but
            // this just finds "Target" in that example.
            AZStd::string fieldName;
            AZ::EntityUtils::EnumerateEntityIds(referencingEntity,
                [&referencedEntityId, &fieldName]
            (const AZ::EntityId& id, bool isEntityId, const AZ::SerializeContext::ClassElement* elementData) -> void
                {
                    if (!isEntityId && id.IsValid() && id == referencedEntityId)
                    {
                        // We have found the reference to this external or deleted EntityId
                        if (elementData)
                        {
                            if (elementData->m_editData)
                            {
                                fieldName = elementData->m_editData->m_name;
                            }
                            else
                            {
                                fieldName = elementData->m_name;
                            }
                        }
                        else
                        {
                            fieldName = "<Unknown>";
                        }
                    }
                }, serializeContext);

            // see if the entity has been deleted
            AZ::Entity* referencedEntity = nullptr;
            EBUS_EVENT_RESULT(referencedEntity, AZ::ComponentApplicationBus, FindEntity, referencedEntityId);

            if (referencedEntity)
            {
                AZStd::string referencedEntityName = EntityHelpers::GetHierarchicalElementName(referencedEntityId);
                return AZ::Failure(AZStd::string::format("There are external references. "
                    "Entity '%s' in the slice being pushed references another entity that will not be in the slice after the push. "
                    "Referenced entity is '%s'. The name of the property field referencing it is '%s'.",
                    referencingEntityName.c_str(), referencedEntityName.c_str(), fieldName.c_str()));
            }
            else
            {
                return AZ::Failure(AZStd::string::format("There are external references. "
                    "Entity '%s' in the slice being pushed references another entity that will not be in the slice after the push. "
                    "Referenced entity no longer exists, it's ID was '%s'. The name of the property field referencing it is '%s'.",
                    referencingEntityName.c_str(), referencedEntityId.ToString().c_str(), fieldName.c_str()));
            }
        }
    }

    // Issue a warning if there are any added entities that are not referenced as children of entities in the slice
    for (auto entityId : addedEntities)
    {
        if (referencedChildEntities.count(entityId) == 0)
        {
            AZStd::string name = EntityHelpers::GetHierarchicalElementName(entityId);
            return AZ::Failure(AZStd::string::format("There are added entities that are unreferenced. "
                "An entity is being added to the slice but it is not referenced as "
                "the child of another entity in the slice."
                "The added entity that is unreferenced is '%s'.", name.c_str()));
        }
    }

    // Check for any entities in the slice that have become orphaned. This can happen is a remove if pushed
    // but the entity removal is unchecked while the removal from the m_childEntityIdOrder array is checked
    int parentlessEntityCount = 0;
    for (auto entityId : clonedEntityIds)
    {
        if (referencedChildEntities.count(entityId) == 0)
        {
            // this entity is not a child of any entity
            ++parentlessEntityCount;
        }
    }

    // There can only be one "root" entity in a slice - i.e. one entity which is not referenced as a child of another
    // entity in the slice
    if (parentlessEntityCount > 1)
    {
        return AZ::Failure(AZStd::string::format("There is more than one root entity. "
            "Possibly a child reference is being removed in this push but the child entity is not."));
    }

    return AZ::Success();
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::PushEntitiesModal(const AzToolsFramework::EntityIdList& entities,
    AZ::SerializeContext* serializeContext)
{
    // Use same SlicePushWidget as world entities do
    AzToolsFramework::SlicePushWidgetConfigPtr config = AZStd::make_shared<AzToolsFramework::SlicePushWidgetConfig>();
    config->m_defaultAddedEntitiesCheckState = true;
    config->m_defaultRemovedEntitiesCheckState = true;
    config->m_rootSlice = GetRootSlice();
    AZ_Warning("UiSlicePush", config->m_rootSlice != nullptr, "Could not find root slice for Slice Push!");
    config->m_preSaveCB = SlicePreSaveCallbackForUiEntities;
    config->m_postSaveCB = nullptr;
    config->m_deleteEntitiesCB = [this](const AzToolsFramework::EntityIdList& entitiesToRemove) -> void
    {
        EBUS_EVENT_ID(this->GetEntityContextId(), UiEditorEntityContextRequestBus, DeleteElements, entitiesToRemove);
    };
    config->m_isRootEntityCB = [this](const AZ::Entity* entity) -> bool
    {
        return this->IsRootEntity(*entity);
    };

    QDialog* dialog = new QDialog();
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    AzToolsFramework::SlicePushWidget* widget = new AzToolsFramework::SlicePushWidget(entities, config, serializeContext);
    mainLayout->addWidget(widget);
    dialog->setWindowTitle(widget->tr("Save Slice Overrides - Advanced"));
    dialog->setMinimumSize(QSize(800, 300));
    dialog->resize(QSize(1200, 600));
    dialog->setLayout(mainLayout);

    QWidget::connect(widget, &AzToolsFramework::SlicePushWidget::OnFinished, dialog,
        [dialog]()
        {
            dialog->accept();
        }
    );

    QWidget::connect(widget, &AzToolsFramework::SlicePushWidget::OnCanceled, dialog,
        [dialog]()
        {
            dialog->reject();
        }
    );

    dialog->exec();
    delete dialog;
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::DetachSliceEntities(const AzToolsFramework::EntityIdList& entities)
{
    if (!entities.empty())
    {
        QString title;
        QString body;
        if (entities.size() == 1)
        {
            title = QObject::tr("Detach Slice Entity");
            body = QObject::tr("A detached entity will no longer receive pushes from its slice. The entity will be converted into a non-slice entity. This action cannot be undone.\n\n"
                "Are you sure you want to detach the selected entity?");
        }
        else
        {
            title = QObject::tr("Detach Slice Entities");
            body = QObject::tr("Detached entities no longer receive pushes from their slices. The entities will be converted into non-slice entities. This action cannot be undone.\n\n"
                "Are you sure you want to detach the selected entities and their descendants?");
        }

        if (ConfirmDialog_Detach(title, body))
        {
            EBUS_EVENT_ID(m_entityContextId, UiEditorEntityContextRequestBus, DetachSliceEntities, entities);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::DetachSliceInstances(const AzToolsFramework::EntityIdList& entities)
{
    if (!entities.empty())
    {
        // Get all slice instances for given entities
        AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;
        for (const AZ::EntityId& entityId : entities)
        {
            AZ::SliceComponent::SliceInstanceAddress sliceAddress;
            AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entityId,
                &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

            if (sliceAddress.IsValid())
            {
                if (sliceInstances.end() == AZStd::find(sliceInstances.begin(), sliceInstances.end(), sliceAddress))
                {
                    sliceInstances.push_back(sliceAddress);
                }
            }
        }

        QString title;
        QString body;
        if (sliceInstances.size() == 1)
        {
            title = QObject::tr("Detach Slice Instance");
            body = QObject::tr("A detached instance will no longer receive pushes from its slice. All entities in the slice instance will be converted into non-slice entities. This action cannot be undone.\n\n"
                "Are you sure you want to detach the selected instance?");
        }
        else
        {
            title = QObject::tr("Detach Slice Instances");
            body = QObject::tr("Detached instances no longer receive pushes from their slices. All entities in the slice instances will be converted into non-slice entities. This action cannot be undone.\n\n"
                "Are you sure you want to detach the selected instances?");
        }

        if (ConfirmDialog_Detach(title, body))
        {
            // Get all instantiated entities for the slice instances
            AzToolsFramework::EntityIdList entitiesToDetach = entities;
            for (const AZ::SliceComponent::SliceInstanceAddress& sliceInstance : sliceInstances)
            {
                const AZ::SliceComponent::InstantiatedContainer* instantiated = sliceInstance.GetInstance()->GetInstantiated();
                if (instantiated)
                {
                    for (AZ::Entity* entityInSlice : instantiated->m_entities)
                    {
                        entitiesToDetach.push_back(entityInSlice->GetId());
                    }
                }
            }

            // Detach the entities
            EBUS_EVENT_ID(m_entityContextId, UiEditorEntityContextRequestBus, DetachSliceEntities, entitiesToDetach);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
AZ::Entity* UiSliceManager::ValidateSingleRootAndGenerateOrderedEntityList(const AzToolsFramework::EntityIdSet& liveEntities, AzToolsFramework::EntityIdList& outOrderedEntityList, AZ::Entity*& insertBefore)
{
    // The low-level slice component code has no limit on there being a single root element
    // in a slice. It does make it simpler to do so though. Also this is the same limitation
    // that we had with the old Prefabs in the UI Editor.
    AZStd::unordered_set<AZ::EntityId> childrenOfCommonParent;
    AZ::Entity* commonParent = nullptr;
    for (auto entity : liveEntities)
    {
        AZ::Entity* parentElement = nullptr;
        EBUS_EVENT_ID_RESULT(parentElement, entity, UiElementBus, GetParent);

        if (parentElement)
        {
            // if this entities parent is not in the set then it is a top-level
            if (liveEntities.count(parentElement->GetId()) == 0)
            {
                // this is a top level element
                if (commonParent)
                {
                    if (commonParent != parentElement)
                    {
                        // we have already found a parent
                        return nullptr;
                    }
                    else
                    {
                        childrenOfCommonParent.insert(entity);
                    }
                }
                else
                {
                    commonParent = parentElement;
                    childrenOfCommonParent.insert(entity);
                }
            }
        }
    }

    // At present there must be a single UI element that is the root element of the slice
    // This means that there should only be one child of the commonParent (the commonParent is always outside
    // of the slice)
    if (childrenOfCommonParent.size() != 1)
    {
        return nullptr;
    }

    // ensure that the top level entities are in the order that they are children of the common parent
    // without this check they would be in the order that they were selected
    outOrderedEntityList.clear();

    LyShine::EntityArray allChildrenOfCommonParent;
    EBUS_EVENT_ID_RESULT(allChildrenOfCommonParent, commonParent->GetId(), UiElementBus, GetChildElements);

    bool justFound = false;
    for (auto entity : allChildrenOfCommonParent)
    {
        // if this child is in the set of top level elements to go in the prefab
        // then add it to the vectors so that we have an ordered list in child order
        if (childrenOfCommonParent.count(entity->GetId()) > 0)
        {
            outOrderedEntityList.push_back(entity->GetId());

            // we are actually only supporting one child of the common parent
            // If this is it, set a flag so we can record the child immediately after it.
            // This is used later to insert the slice instance before this child
            justFound = true;
        }
        else
        {
            if (justFound)
            {
                insertBefore = entity;
                justFound = false;
            }
        }
    }

    // now add the rest of the entities (that are not top-level) to the list in any order
    for (auto entity : liveEntities)
    {
        if (childrenOfCommonParent.count(entity) == 0)
        {
            outOrderedEntityList.push_back(entity);
        }
    }

    return commonParent;
}

//////////////////////////////////////////////////////////////////////////
void UiSliceManager::SetEntityContextId(AzFramework::EntityContextId entityContextId)
{
    m_entityContextId = entityContextId;
}

//////////////////////////////////////////////////////////////////////////
AZ::Outcome<void, AZStd::string> UiSliceManager::PushEntitiesBackToSlice(const AzToolsFramework::EntityIdList& entityIdList, const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset)
{
    return AzToolsFramework::SliceUtilities::PushEntitiesBackToSlice(entityIdList, sliceAsset, SlicePreSaveCallbackForUiEntities);
}

//////////////////////////////////////////////////////////////////////////
AZ::Outcome<void, AZStd::string> UiSliceManager::QuickPushSliceInstance(const AZ::SliceComponent::SliceInstanceAddress& sliceAddress,
    const AzToolsFramework::EntityIdList& entityIdList)
{
    // we cannot use SliceUtilities::PushEntitiesBackToSlice because that does not handle adds or deletes

    using AzToolsFramework::SliceUtilities::SliceTransaction;

    const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset = sliceAddress.GetReference()->GetSliceAsset();
    if (!sliceAsset)
    {
        return AZ::Failure(AZStd::string::format("Asset \"%s\" with id %s is not loaded, or is not a slice.",
            sliceAsset.GetHint().c_str(),
            sliceAsset.GetId().ToString<AZStd::string>().c_str()));
    }

    // Not all entities in the list need to be part of the slice instance being pushed (sliceAddress) since we could
    // be pushing a new instance into the slice. However, it is an error if there is a second instance of the same slice
    // asset that we are pushing to in the entity set
    for (AZ::EntityId entityId : entityIdList)
    {
        AZ::SliceComponent::SliceInstanceAddress entitySliceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(entitySliceAddress, entityId,
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

        if (entitySliceAddress.IsValid() && entitySliceAddress.GetReference()->GetSliceAsset() == sliceAsset)
        {
            if (entitySliceAddress != sliceAddress)
            {
                // error there is a second instance of the same slice asset in the set
                return AZ::Failure(AZStd::string::format("Entity with id %s is part of a different slice instance of the same slice asset. A slice cannot contain an instance of itself.",
                    entityId.ToString().c_str()));
            }
        }
    }

    // Check for any invalid slices
    bool cancelPush = false;
    AZ::SliceComponent* assetComponent = sliceAsset.Get()->GetComponent();
    if (assetComponent)
    {
        // If there are any invalid slices, warn the user and allow them to choose the next step.
        const AZ::SliceComponent::SliceList& invalidSlices = assetComponent->GetInvalidSlices();
        if (invalidSlices.size() > 0)
        {
            // Assume an invalid slice count of 1 because this is a quick push, which only has one target.
            AzToolsFramework::SliceUtilities::InvalidSliceReferencesWarningResult invalidSliceCheckResult = AzToolsFramework::SliceUtilities::DisplayInvalidSliceReferencesWarning(QApplication::activeWindow(),
                /*invalidSliceCount*/ 1,
                invalidSlices.size(),
                /*showDetailsButton*/ true);

            switch (invalidSliceCheckResult)
            {
            case AzToolsFramework::SliceUtilities::InvalidSliceReferencesWarningResult::Details:
            {
                cancelPush = true;
                PushEntitiesModal(entityIdList, nullptr);
            }
            break;
            case AzToolsFramework::SliceUtilities::InvalidSliceReferencesWarningResult::Save:
            {
                cancelPush = false;
            }
            break;
            case AzToolsFramework::SliceUtilities::InvalidSliceReferencesWarningResult::Cancel:
            default:
            {
                cancelPush = true;
            }
            break;
            }
        }
    }

    if (cancelPush)
    {
        return AZ::Success();
    }

    // Make a transaction targeting the specified slice and add all the entities in this set.
    SliceTransaction::TransactionPtr transaction = SliceTransaction::BeginSlicePush(sliceAsset);
    if (transaction)
    {
        AzToolsFramework::EntityIdList entitiesBeingAdded;

        for (AZ::EntityId entityId : entityIdList)
        {
            AZ::SliceComponent::SliceInstanceAddress entitySliceAddress;
            AzFramework::SliceEntityRequestBus::EventResult(entitySliceAddress, entityId,
                &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

            // Check if this slice is in the slice instance being pushed
            if (entitySliceAddress == sliceAddress)
            {
                const SliceTransaction::Result result = transaction->UpdateEntity(entityId);
                if (!result)
                {
                    return AZ::Failure(AZStd::string::format("Failed to add entity with Id %s to slice transaction for \"%s\". Slice push aborted.\n\nError:\n%s",
                        entityId.ToString().c_str(),
                        sliceAsset.GetHint().c_str(),
                        result.GetError().c_str()));
                }
            }
            else
            {
                // This entity is not in a slice, treat it as an add
                SliceTransaction::Result result = transaction->AddEntity(entityId, SliceTransaction::SliceAddEntityFlags::DiscardSliceAncestry);
                if (!result)
                {
                    return AZ::Failure(AZStd::string::format("Failed to add entity with Id %s to slice transaction for \"%s\". Slice push aborted.\n\nError:\n%s",
                        entityId.ToString().c_str(),
                        sliceAsset.GetHint().c_str(),
                        result.GetError().c_str()));
                }

                entitiesBeingAdded.push_back(entityId);
            }
        }

        // Check for any entity removals
        // We know the slice instance details, compare the entities it contains to the entities
        // contained in the underlying asset. If it's missing any entities that exist in the asset,
        // we can removal the entity from the base slice.
        AZStd::unordered_set<AZ::EntityId> uniqueRemovedEntities;
        AZ::SliceComponent::EntityAncestorList ancestorList;
        AZ::SliceComponent::EntityList assetEntities;
        const AZ::SliceComponent::SliceInstanceAddress& instanceAddr = sliceAddress;
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
                    // obviously aren't maintained in any maps.
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
                    }
                }

                for (AZ::EntityId entityToRemove : uniqueRemovedEntities)
                {
                    SliceTransaction::Result result = transaction->RemoveEntity(entityToRemove);
                    if (!result)
                    {
                        return AZ::Failure(AZStd::string::format("Failed to add entity with Id %s to slice transaction for \"%s\" for removal. Slice push aborted.\n\nError:\n%s",
                            entityToRemove.ToString().c_str(),
                            sliceAsset.GetHint().c_str(),
                            result.GetError().c_str()));
                        break;
                    }
                }
            }
        }

        const SliceTransaction::Result result = transaction->Commit(
            sliceAsset.GetId(),
            SlicePreSaveCallbackForUiEntities,
            nullptr);

        if (result)
        {
            // Successful commit
            // Remove any entities that were succesfully pushed into a slice (since they'll be brought to life through slice reloading)
            EBUS_EVENT_ID(this->GetEntityContextId(), UiEditorEntityContextRequestBus, DeleteElements, entitiesBeingAdded);
        }
        else
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

//////////////////////////////////////////////////////////////////////////
AZStd::string UiSliceManager::MakeTemporaryFilePathForSave(const char* targetFilename)
{
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    AZ_Assert(fileIO, "File IO is not initialized.");

    AZStd::string devAssetPath = fileIO->GetAlias("@projectroot@");
    AZStd::string userPath = fileIO->GetAlias("@user@");
    AZStd::string tempPath = targetFilename;
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, devAssetPath);
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, userPath);
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, tempPath);
    AzFramework::StringFunc::Replace(tempPath, "@projectroot@", devAssetPath.c_str());
    AzFramework::StringFunc::Replace(tempPath, devAssetPath.c_str(), userPath.c_str());
    tempPath.append(".slicetemp");

    return tempPath;
}

//////////////////////////////////////////////////////////////////////////
bool UiSliceManager::ConfirmDialog_Detach(const QString& title, const QString& text)
{
    QMessageBox questionBox(QApplication::activeWindow());
    questionBox.setIcon(QMessageBox::Question);
    questionBox.setWindowTitle(title);
    questionBox.setText(text);
    QAbstractButton* detachButton = questionBox.addButton(QObject::tr("Detach"), QMessageBox::YesRole);
    questionBox.addButton(QObject::tr("Cancel"), QMessageBox::NoRole);
    questionBox.exec();
    return questionBox.clickedButton() == detachButton;
}
