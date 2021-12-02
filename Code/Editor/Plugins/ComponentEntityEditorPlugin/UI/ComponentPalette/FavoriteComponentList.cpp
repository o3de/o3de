/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FavoriteComponentList.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/CryEditDoc.h>
#include <Editor/ViewManager.h>

#include <QHeaderView>
#include <QMimeData>

// FavoritesList
//////////////////////////////////////////////////////////////////////////

FavoritesList::FavoritesList(QWidget* parent /*= nullptr*/) 
    : FilteredComponentList(parent)
{
}

FavoritesList::~FavoritesList()
{
    FavoriteComponentListRequestBus::Handler::BusDisconnect();
}

void FavoritesList::Init()
{
    FavoriteComponentListRequestBus::Handler::BusConnect();

    FavoritesDataModel* favoritesDataModel = new FavoritesDataModel(this);
    
    setModel(favoritesDataModel);

    horizontalHeader()->setSectionResizeMode(ComponentDataModel::ColumnIndex::Name, QHeaderView::Stretch);

    setShowGrid(false);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    setStyleSheet("QTableView { selection-background-color: rgba(255,255,255,0.2); }");
    setGridStyle(Qt::PenStyle::NoPen);
    verticalHeader()->hide();
    horizontalHeader()->hide();
    setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    setShowGrid(false);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    setColumnWidth(ComponentDataModel::ColumnIndex::Icon, 32);
    hideColumn(ComponentDataModel::ColumnIndex::Category);

    setDragDropMode(QAbstractItemView::DragDrop);
    setAcceptDrops(true);

    horizontalHeader()->setSectionResizeMode(ComponentDataModel::ColumnIndex::Icon, QHeaderView::ResizeToContents);
    setColumnWidth(ComponentDataModel::ColumnIndex::Icon, 32);

    horizontalHeader()->setSectionResizeMode(ComponentDataModel::ColumnIndex::Category, QHeaderView::Stretch);
    setColumnWidth(ComponentDataModel::ColumnIndex::Category, 90);

    // Context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &FavoritesList::ShowContextMenu);
}

void FavoritesList::ShowContextMenu(const QPoint& pos)
{
    // Only show if a level is loaded
    if (!GetIEditor() || GetIEditor()->IsInGameMode())
    {
        return;
    }

    if ( model()->rowCount() == 0)
    {
        return;
    }

    QMenu contextMenu(tr("Context menu"), this);

    QAction actionNewEntity(tr("Make entity with selected favorites"), this);
    QAction actionAddToSelection(this);
    if (GetIEditor()->GetDocument()->IsDocumentReady())
    {
        QObject::connect(&actionNewEntity, &QAction::triggered, this, [&] { ContextMenu_NewEntity(); });
        contextMenu.addAction(&actionNewEntity);

        AzToolsFramework::EntityIdList selectedEntities;
        EBUS_EVENT_RESULT(selectedEntities, AzToolsFramework::ToolsApplicationRequests::Bus, GetSelectedEntities);

        if (!selectedEntities.empty())
        {
            QString addToSelection = selectedEntities.size() > 1 ? tr("Add to selected entities") : tr("Add to selected entity");
            actionAddToSelection.setText(addToSelection);

            QObject::connect(&actionAddToSelection, &QAction::triggered, this, [&] { ContextMenu_AddToSelectedEntities(); });
            contextMenu.addAction(&actionAddToSelection);
        }

        contextMenu.addSeparator();
    }

    QAction action(tr("Remove"), this);
    QObject::connect(&action, &QAction::triggered, this, [&] { ContextMenu_RemoveSelectedFavorites(); });
    contextMenu.addAction(&action);

    contextMenu.exec(mapToGlobal(pos));
}

void FavoritesList::ContextMenu_RemoveSelectedFavorites()
{
    FavoritesDataModel* dataModel = qobject_cast<FavoritesDataModel*>(model());
    if (!selectedIndexes().empty())
    {
        dataModel->Remove(selectedIndexes());
    }
}

void FavoritesList::rowsInserted([[maybe_unused]] const QModelIndex& parent, [[maybe_unused]] int start, [[maybe_unused]] int end)
{
    resizeRowToContents(0);
}

void FavoritesList::AddFavorites(const AZStd::vector<const AZ::SerializeContext::ClassData*>& classDataContainer)
{
    for (const AZ::SerializeContext::ClassData* classData : classDataContainer)
    {
        if (classData)
        {
            FavoritesDataModel* dataModel = qobject_cast<FavoritesDataModel*>(model());
            dataModel->AddFavorite(classData);
        }
    }
}

void FavoritesList::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat(AzToolsFramework::ComponentTypeMimeData::GetMimeType()))
    {
        event->acceptProposedAction();
    }
}

void FavoritesList::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->source() == this)
    {
        event->ignore();
    }
    else
    {
        event->accept();
    }
}

// FavoritesDataModel
//////////////////////////////////////////////////////////////////////////

int FavoritesDataModel::rowCount([[maybe_unused]] const QModelIndex &parent /*= QModelIndex()*/) const
{
    return m_favorites.size();
}

int FavoritesDataModel::columnCount([[maybe_unused]] const QModelIndex &parent /*= QModelIndex()*/) const
{
    return ColumnIndex::Count;
}

void FavoritesDataModel::SaveState()
{
    AZStd::vector<AZ::Uuid> favorites;
    for (const AZ::SerializeContext::ClassData* classData : m_favorites)
    {
        favorites.push_back(classData->m_typeId);
    }
    m_settings->SetFavorites(AZStd::move(favorites));


    // Write the settings to file...
    AZ::SerializeContext* serializeContext = nullptr;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(serializeContext, "Serialize Context is null!");

    char settingsPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(ComponentPaletteSettings::GetSettingsFile(), settingsPath, AZ_MAX_PATH_LEN);

    bool result = m_provider.Save(settingsPath, serializeContext);
    (void)result;
    AZ_Warning("ComponentPaletteSettings", result, "Failed to Save the Component Palette Settings!");
}

void FavoritesDataModel::LoadState()
{
    // It is necessary to Load the settings file *before* you call UserSettings::CreateFind!
    AZ::SerializeContext* serializeContext = nullptr;
    EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(serializeContext, "Serialize Context is null!");

    char settingsPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetInstance()->ResolvePath(ComponentPaletteSettings::GetSettingsFile(), settingsPath, AZ_MAX_PATH_LEN);

    bool result = m_provider.Load(settingsPath, serializeContext);
    (void)result;


    // Create (if no file was found) or find the settings, this will populate the m_settings->m_favorites list.
    m_settings = AZ::UserSettings::CreateFind<ComponentPaletteSettings>(AZ_CRC("ComponentPaletteSettings", 0x481d355b), m_providerId);

    // Add favorites to the data model from loaded settings
    for (const AZ::Uuid& favorite : m_settings->m_favorites)
    {
        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(favorite);
        if (classData)
        {
            AddFavorite(classData, false);
        }
    }
}

void FavoritesDataModel::Remove(const QModelIndexList& indices)
{
    beginResetModel();

    auto newFavorites = m_favorites;

    // swap here
    for (auto index : indices)
    {
        // we're only dealing with columns and they're the only thing with class data anyways
        if (index.column() == 0)
        {
            QVariant classDataVariant = index.data(ComponentDataModel::ClassDataRole);
            if (classDataVariant.isValid())
            {
                const AZ::SerializeContext::ClassData* classData = reinterpret_cast<const AZ::SerializeContext::ClassData*>(classDataVariant.value<void*>());
                newFavorites.removeAll(classData);

                AZ_TracePrintf("Debug", "Removing: %s\n", classData->m_editData->m_name);
            }
        }
    }

    m_favorites.swap(newFavorites);

    endResetModel();

    SaveState();
}

QModelIndex FavoritesDataModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    if (row >= rowCount(parent) || column >= columnCount(parent))
    {
        return QModelIndex();
    }

    return createIndex(row, column, (void*)(m_favorites[row]));
}

QVariant FavoritesDataModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    const AZ::SerializeContext::ClassData* classData = m_favorites[index.row()];
    if (!classData)
    {
        return QVariant();
    }

    switch (role)
    {
    case Qt::DisplayRole:
    {
        if (index.column() == ComponentDataModel::ColumnIndex::Name)
        {
            if (m_favorites.empty())
            {
                return QVariant(tr("You have 0 favorites.\nDrag some components here."));
            }

            return QVariant(classData->m_editData->m_name);
        }
    }
    break;

    case Qt::DecorationRole:
    {
        if (index.column() == ColumnIndex::Icon)
        {
            const AZ::SerializeContext::ClassData* iconClassData = m_favorites[index.row()];
            auto iconIterator = m_componentIcons.find(iconClassData->m_typeId);
            if (iconIterator != m_componentIcons.end())
            {
                return iconIterator->second;
            }

            return QVariant();
        }
    }
    break;

    case ClassDataRole:
        if (index.column() == 0) // Only get data for one column
        {
            return QVariant::fromValue<void*>(reinterpret_cast<void*>(const_cast<AZ::SerializeContext::ClassData*>(classData)));
        }
        break;

    default:
        break;
    }

    return ComponentDataModel::data(index, role);
    
}

void FavoritesDataModel::SetSavedStateKey([[maybe_unused]] AZ::u32 key)
{
}

FavoritesDataModel::FavoritesDataModel(QWidget* parent /*= nullptr*/) 
    : ComponentDataModel(parent)
    , m_providerId(AZ_CRC("ComponentPaletteSettingsProviderId"))
{
    m_provider.Activate(m_providerId);
    LoadState();
}

FavoritesDataModel::~FavoritesDataModel()
{
    m_provider.Deactivate();
}

void FavoritesDataModel::AddFavorite(const AZ::SerializeContext::ClassData* classData, bool updateSettings)
{
    beginResetModel();

    if (m_favorites.indexOf(classData) < 0)
    {
        m_favorites.push_back(classData);
    }

    endResetModel();

    if (updateSettings)
    {
        SaveState();
    }
}

bool FavoritesDataModel::dropMimeData(const QMimeData *data, Qt::DropAction action, [[maybe_unused]] int row, [[maybe_unused]] int column, [[maybe_unused]] const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction)
    {
        return true;
    }

    if (data && data->hasFormat(AzToolsFramework::ComponentTypeMimeData::GetMimeType()))
    {
        AzToolsFramework::ComponentTypeMimeData::ClassDataContainer classDataContainer;
        AzToolsFramework::ComponentTypeMimeData::Get(data, classDataContainer);

        for (const AZ::SerializeContext::ClassData* classData : classDataContainer)
        {
            if (classData)
            {
                AddFavorite(classData);
            }
        }

        return true;
    }

    return false;
}

#include <UI/ComponentPalette/moc_FavoriteComponentList.cpp>
