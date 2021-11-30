/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentDataModel.h"

#include "Include/IObjectManager.h"
#include "Objects/SelectionGroup.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/ComponentMimeData.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Commands/EntityStateCommand.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

#include <Editor/IEditor.h>
#include <Editor/Viewport.h>
#include <Editor/ViewManager.h>
#include <CryCommon/MathConversion.h>

#include <AzQtComponents/DragAndDrop/ViewportDragAndDrop.h>

#include <QMimeData>

namespace
{
    // This is a helper function that given an object that derives from QAbstractItemModel, 
    // it will request the model's "ClassDataRole" class data for an entry and use that 
    // information to create a new entity with the selected components.
    AZ::EntityId CreateEntityFromSelection(const QModelIndexList& selection, QAbstractItemModel* model)
    {
        AZ::Vector3 position = AZ::Vector3::CreateZero();
        CViewport *view = GetIEditor()->GetViewManager()->GetGameViewport();
        int width, height;
        view->GetDimensions(&width, &height);
        position = LYVec3ToAZVec3(view->ViewToWorld(QPoint(width / 2, height / 2)));

        AZ::EntityId newEntityId;
        EBUS_EVENT_RESULT(newEntityId, AzToolsFramework::EditorRequests::Bus, CreateNewEntityAtPosition, position, AZ::EntityId());
        if (newEntityId.IsValid())
        {
            // Add all the selected components.
            AZ::ComponentTypeList componentsToAdd;
            for (auto index : selection)
            {
                // We only need to consider the first column, it's important that the data() function that 
                // returns ComponentDataModel::ClassDataRole also does so for the first column.
                if (index.column() != 0)
                {
                    continue;
                }

                QVariant classDataVariant = model->data(index, ComponentDataModel::ClassDataRole);
                if (classDataVariant.isValid())
                {
                    const AZ::SerializeContext::ClassData* classData = reinterpret_cast<const AZ::SerializeContext::ClassData*>(classDataVariant.value<void*>());
                    componentsToAdd.push_back(classData->m_typeId);
                }
            }

            AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, AzToolsFramework::EntityIdList{ newEntityId }, componentsToAdd);

            return newEntityId;
        }

        return AZ::EntityId();
    }
}

namespace ComponentDataUtilities
{
    // This is a helper function to add the specified components to the selected entities, it relies on the provided
    // QAbstractItemModel to determine the appropriate ClassData to use to create the components (given that some widgets
    // may provide proxy models that alter the order).
    void AddComponentsToSelectedEntities(const QModelIndexList& selectedComponents, QAbstractItemModel* model)
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
        if (selectedEntities.empty())
        {
            return;
        }

        // Add all the selected components.
        AZ::ComponentTypeList componentsToAdd;
        for (auto index : selectedComponents)
        {
            // We only need to consider the first column, it's important that the data() function that 
            // returns ComponentDataModel::ClassDataRole also does so for the first column.
            if (index.column() != 0)
            {
                continue;
            }

            QVariant classDataVariant = model->data(index, ComponentDataModel::ClassDataRole);
            if (classDataVariant.isValid())
            {
                const AZ::SerializeContext::ClassData* classData = reinterpret_cast<const AZ::SerializeContext::ClassData*>(classDataVariant.value<void*>());
                componentsToAdd.push_back(classData->m_typeId);
            }
        }

        AzToolsFramework::EntityCompositionRequestBus::Broadcast(&AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, selectedEntities, componentsToAdd);
    }
}


// ComponentDataModel
//////////////////////////////////////////////////////////////////////////

ComponentDataModel::ComponentDataModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    AZ::SerializeContext* serializeContext = nullptr;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

    serializeContext->EnumerateDerived<AZ::Component>([this](const AZ::SerializeContext::ClassData* classData, const AZ::Uuid&) -> bool
    {
        bool allowed = false;
        bool hidden = false;
        AZStd::string category = "Miscellaneous";

        if (classData->m_editData)
        {
            for (const AZ::Edit::ElementData& element : classData->m_editData->m_elements)
            {
                if (element.m_elementId == AZ::Edit::ClassElements::EditorData)
                {
                    AZStd::string iconPath;
                    AzToolsFramework::EditorRequestBus::BroadcastResult(iconPath, &AzToolsFramework::EditorRequests::GetComponentTypeEditorIcon, classData->m_typeId);
                    if (!iconPath.empty())
                    {
                        m_componentIcons[classData->m_typeId] = QIcon(iconPath.c_str());
                    }

                    for (const AZ::Edit::AttributePair& attribPair : element.m_attributes)
                    {
                        if (attribPair.first == AZ::Edit::Attributes::AppearsInAddComponentMenu)
                        {
                            if (auto data = azdynamic_cast<AZ::Edit::AttributeData<AZ::Crc32>*>(attribPair.second))
                            {
                                if (data->Get(nullptr) == AZ_CRC("Game"))
                                {
                                    allowed = true;
                                }
                            }
                        }
                        else if (attribPair.first == AZ::Edit::Attributes::AddableByUser)
                        {
                            // skip this component if user is not allowed to add it directly
                            if (auto data = azdynamic_cast<AZ::Edit::AttributeData<bool>*>(attribPair.second))
                            {
                                if (!data->Get(nullptr))
                                {
                                    hidden = true;
                                }
                            }
                        }
                        else if (attribPair.first == AZ::Edit::Attributes::Category)
                        {
                            if (auto data = azdynamic_cast<AZ::Edit::AttributeData<const char*>*>(attribPair.second))
                            {
                                category = data->Get(nullptr);
                            }
                        }
                    }

                    break;
                }
            }
        }

        if (allowed && !hidden)
        {
            m_componentList.push_back(classData);
            m_componentMap[category].push_back(classData);
            m_categories.insert(category);
        }

        return true;
    });

    // we'd like viewport events
    AzQtComponents::DragAndDropEventsBus::Handler::BusConnect(AzQtComponents::DragAndDropContexts::EditorViewport);
}

ComponentDataModel::~ComponentDataModel()
{
    AzQtComponents::DragAndDropEventsBus::Handler::BusDisconnect();
}

Qt::ItemFlags ComponentDataModel::flags([[maybe_unused]] const QModelIndex &index) const
{
    return Qt::ItemFlags(
        Qt::ItemIsEnabled |
        Qt::ItemIsDragEnabled |
        Qt::ItemIsDropEnabled |
        Qt::ItemIsSelectable);
}

const AZ::SerializeContext::ClassData* ComponentDataModel::GetClassData(const QModelIndex& index) const
{
    int row = index.row();
    if (row < 0 || row >= m_componentList.size())
    {
        return nullptr;
    }

    return m_componentList[row];
}

const char* ComponentDataModel::GetCategory(const AZ::SerializeContext::ClassData* classData)
{
    if (classData)
    {
        if (auto editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
        {
            if (auto categoryAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Category))
            {
                if (auto categoryData = azdynamic_cast<AZ::Edit::AttributeData<const char*>*>(categoryAttribute))
                {
                    const char* result = categoryData->Get(nullptr);
                    if (result)
                    {
                        return result;
                    }
                }
            }
        }
    }

    return "";
}


QModelIndex ComponentDataModel::index(int row, int column, const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (row >= rowCount(parent) || column >= columnCount(parent))
    {
        return QModelIndex();
    }

    return createIndex(row, column, (void*)(m_componentList[row]));
}

QModelIndex ComponentDataModel::parent([[maybe_unused]] const QModelIndex &child) const
{
    return QModelIndex();
}

int ComponentDataModel::rowCount([[maybe_unused]] const QModelIndex &parent /*= QModelIndex()*/) const
{
    return static_cast<int>(m_componentList.size());
}

int ComponentDataModel::columnCount([[maybe_unused]] const QModelIndex &parent /*= QModelIndex()*/) const
{
    return ColumnIndex::Count;
}

QVariant ComponentDataModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
    if (index.isValid())
    {
        const AZ::SerializeContext::ClassData* classData = m_componentList[index.row()];
        if (!classData)
        {
            return QVariant();
        }

        switch (role)
        {
        case ClassDataRole:
            if (index.column() == 0) // Only get data for one column
            {
                return QVariant::fromValue<void*>(reinterpret_cast<void*>(const_cast<AZ::SerializeContext::ClassData*>(classData)));
            }
            break;

        case Qt::DisplayRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                return QVariant(classData->m_editData->m_name);
            }
            else
                if (index.column() == ColumnIndex::Category)
                {
                    if (auto editorDataElement = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
                    {
                        if (auto categoryAttribute = editorDataElement->FindAttribute(AZ::Edit::Attributes::Category))
                        {
                            if (auto categoryData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(categoryAttribute))
                            {
                                return QVariant(categoryData->Get(nullptr));
                            }
                        }
                    }
                }

        }
        break;

        case Qt::ToolTipRole:
        {
            return QVariant(classData->m_editData->m_description);
        }

        case Qt::DecorationRole:
        {
            if (index.column() == ColumnIndex::Icon)
            {
                auto iconIterator = m_componentIcons.find(classData->m_typeId);
                if (iconIterator != m_componentIcons.end())
                {
                    return iconIterator->second;
                }
            }
        }
        break;

        default:
            break;
        }
    }

    return QVariant();
}

QMimeData* ComponentDataModel::mimeData(const QModelIndexList& indices) const
{
    QModelIndexList list;

    // Filter out columns we are not interested in.
    for (const QModelIndex& index : indices)
    {
        if (index.column() == 0)
        {
            list.push_back(index);
        }
    }

    AZStd::vector<const AZ::SerializeContext::ClassData*> sortedList;
    for (QModelIndex index : list)
    {
        QVariant classDataVariant = index.data(ComponentDataModel::ClassDataRole);
        if (classDataVariant.isValid())
        {
            const AZ::SerializeContext::ClassData* classData = reinterpret_cast<const AZ::SerializeContext::ClassData*>(classDataVariant.value<void*>());
            sortedList.push_back(classData);
        }
    }

    QMimeData* mimeData = nullptr;
    if (!sortedList.empty())
    {
        mimeData = AzToolsFramework::ComponentTypeMimeData::Create(sortedList).release();
    }

    return mimeData;
}

bool ComponentDataModel::CanAcceptDragAndDropEvent(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) const
{
    using namespace AzToolsFramework;
    using namespace AzQtComponents;

    // if a listener with a higher priority already claimed this event, do not touch it.
    if ((!event) || (event->isAccepted()) || (!event->mimeData()))
    {
        return false;
    }

    ViewportDragContext* contextVP = azrtti_cast<ViewportDragContext*>(&context);
    if (!contextVP)
    {
        // not a viewport event.  This is for some other GUI such as the main window itself.
        return false;
    }

    AZStd::vector<const AZ::SerializeContext::ClassData*> componentClassDataList;
    return AzToolsFramework::ComponentTypeMimeData::Get(event->mimeData(), componentClassDataList);
}

void ComponentDataModel::DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& context)
{
    if (CanAcceptDragAndDropEvent(event, context))
    {
        event->setDropAction(Qt::CopyAction);
        event->setAccepted(true);
        // opportunities to show special highlights, or ghosted entities or previews here.
    }
}

void ComponentDataModel::DragMove(QDragMoveEvent* event, AzQtComponents::DragAndDropContextBase& context)
{
    if (CanAcceptDragAndDropEvent(event, context))
    {
        event->setDropAction(Qt::CopyAction);
        event->setAccepted(true);
        // opportunities to update special highlights, or ghosted entities or previews here.
    }
}

void ComponentDataModel::DragLeave(QDragLeaveEvent* /*event*/)
{
    // opportunities to remove ghosted entities or previews here.
}

void ComponentDataModel::Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context)
{
    using namespace AzToolsFramework;
    using namespace AzQtComponents;

    // ALWAYS CHECK - you are not the only one connected to this bus, and someone else may have already
    // handled the event or accepted the drop - it might not contain types relevant to you.
    // you still get informed about the drop event in case you did some stuff in your gui and need to clean it up.
    if (!CanAcceptDragAndDropEvent(event, context))
    {
        return;
    }

    // note that the above call already checks all the pointers such as event, or whether context is a VP context, mimetype, etc
    ViewportDragContext* contextVP = azrtti_cast<ViewportDragContext*>(&context);

    // we don't get given this action by Qt unless we already returned accepted from one of the other ones (such as drag move of drag enter)
    event->setDropAction(Qt::CopyAction);
    event->setAccepted(true);

    AzToolsFramework::ScopedUndoBatch undo("Create entity from components");
    const AZStd::string name = AZStd::string::format("Entity%d", GetIEditor()->GetObjectManager()->GetObjectCount());

    AZ::Entity* newEntity = aznew AZ::Entity(name.c_str());
    if (newEntity)
    {
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequests::AddRequiredComponents, *newEntity);
        auto* transformComponent = newEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();
        if (transformComponent)
        {
            transformComponent->SetWorldTM(AZ::Transform::CreateTranslation(contextVP->m_hitLocation));
        }

        // Add the entity to the editor context, which activates it and creates the sandbox object.
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequests::AddEditorEntity, newEntity);

        // Prepare undo command last so it captures the final state of the entity.
        AzToolsFramework::EntityCreateCommand* command = aznew AzToolsFramework::EntityCreateCommand(static_cast<AZ::u64>(newEntity->GetId()));
        command->Capture(newEntity);
        command->SetParent(undo.GetUndoBatch());

        // Only need to add components to the new entity
        AzToolsFramework::EntityIdList entities = { newEntity->GetId() };

        AZStd::vector<const AZ::SerializeContext::ClassData*> componentClassDataList;
        AzToolsFramework::ComponentTypeMimeData::Get(event->mimeData(), componentClassDataList);

        AZ::ComponentTypeList componentsToAdd;
        for (auto classData : componentClassDataList)
        {
            if (!classData)
            {
                continue;
            }

            componentsToAdd.push_back(classData->m_typeId);
        }

        AzToolsFramework::EntityCompositionRequests::AddComponentsOutcome addedComponentsResult = AZ::Failure(AZStd::string("Failed to call AddComponentsToEntities on EntityCompositionRequestBus"));
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(addedComponentsResult, &AzToolsFramework::EntityCompositionRequests::AddComponentsToEntities, entities, componentsToAdd);

        ToolsApplicationRequests::Bus::Broadcast(&ToolsApplicationRequests::AddDirtyEntity, newEntity->GetId());
        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, entities);
    }
}

AZ::EntityId ComponentDataProxyModel::NewEntityFromSelection(const QModelIndexList& selection)
{
    return CreateEntityFromSelection(selection, this);
}

AZ::EntityId ComponentDataModel::NewEntityFromSelection(const QModelIndexList& selection)
{
    return CreateEntityFromSelection(selection, this);
}

bool ComponentDataProxyModel::filterAcceptsRow(int sourceRow, [[maybe_unused]] const QModelIndex &sourceParent) const
{
    if (m_selectedCategory.empty() && !filterRegExp().isValid())
        return true;

    ComponentDataModel* dataModel = static_cast<ComponentDataModel*>(sourceModel());
    if (sourceRow < 0 || sourceRow >= dataModel->GetComponents().size())
    {
        return false;
    }

    const AZ::SerializeContext::ClassData* classData = dataModel->GetComponents()[sourceRow];
    if (!classData)
    {
        return false;
    }

    // Get Category
    if (!m_selectedCategory.empty())
    {
        AZStd::string currentCateogry = ComponentDataModel::GetCategory(classData);

        if (AzFramework::StringFunc::Find(currentCateogry.c_str(), m_selectedCategory.c_str()))
        {
            return false;
        }
    }

    if (filterRegExp().isValid())
    {
        QString componentName = QString::fromUtf8(classData->m_editData->m_name);
        return componentName.contains(filterRegExp());
    }

    return true;
}

void ComponentDataProxyModel::SetSelectedCategory(const AZStd::string& category)
{
    m_selectedCategory = category;
    invalidate();
}

void ComponentDataProxyModel::ClearSelectedCategory()
{
    m_selectedCategory.clear();
    invalidate();
}

#include "UI/ComponentPalette/moc_ComponentDataModel.cpp"
