/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Widgets/ConstructPresetDialog/ConstructPresetDialog.h>
#include <GraphCanvas/Widgets/StyledItemDelegates/IconDecoratedNameDelegate.h>
#include <StaticLib/GraphCanvas/Widgets/ConstructPresetDialog/ui_ConstructPresetDialog.h>

namespace GraphCanvas
{
    ///////////////////////////////
    // ConstructPresetsTableModel
    ///////////////////////////////

    ConstructPresetsTableModel::ConstructPresetsTableModel(QObject* parent)
        : QAbstractTableModel(parent)
        , m_constructType(ConstructType::Unknown)
        , m_presetsContainer(nullptr)
    {
    }

    ConstructPresetsTableModel::~ConstructPresetsTableModel()
    {
    }

    void ConstructPresetsTableModel::SetEditorId(EditorId editorId)
    {
        m_editorId = editorId;

        AssetEditorSettingsNotificationBus::Handler::BusConnect(m_editorId);
    }

    int ConstructPresetsTableModel::columnCount(const QModelIndex &/*parent*/) const
    {
        return ColumnIndex::Count;
    }

    int ConstructPresetsTableModel::rowCount(const QModelIndex &/*parent*/) const
    {
        return aznumeric_cast<int>(m_activePresets.size());
    }

    QVariant ConstructPresetsTableModel::data(const QModelIndex &index, int role) const
    {        
        const PresetStructure* structure = FindPresetStructureForIndex(index);

        if (structure == nullptr)
        {
            return QVariant();
        }

        switch (role)
        {
        case Qt::EditRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                return QVariant(structure->m_preset->GetDisplayName().c_str());
            }
            break;
        }
        case Qt::DisplayRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                return QVariant(structure->m_preset->GetDisplayName().c_str());
            }
            break;
        }
        case Qt::DecorationRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                if (structure->m_pixmap)
                {
                    return (*structure->m_pixmap);
                }
            }
            break;
        }
        case Qt::CheckStateRole:
        {
            if (index.column() == ColumnIndex::DefaultPreset)
            {
                if (structure->m_isDefault)
                {
                    return Qt::CheckState::Checked;
                }
                else
                {
                    return Qt::CheckState::Unchecked;
                }
            }
        }
        break;
        default:
            break;
        }

        return QVariant();
    }

    QVariant ConstructPresetsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole)
        {
            if (orientation == Qt::Horizontal)
            {
                switch (section)
                {
                case ColumnIndex::Name:
                    return QString("PresetName");
                case ColumnIndex::DefaultPreset:
                    return QString("Is Default");
                default:
                    break;
                }
            }
        }

        return QVariant();
    }

    bool ConstructPresetsTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        PresetStructure* structure = ModPresetStructureForIndex(index);

        if (structure == nullptr)
        {
            return false;
        }

        emit OnPresetModificationBegin();

        bool modifiedData = false;

        switch (role)
        {
        case Qt::EditRole:
        {
            if (index.column() == ColumnIndex::Name)
            {
                modifiedData = true;

                AZStd::string newVariableName = value.toString().toUtf8().data();
                structure->m_preset->SetDisplayName(newVariableName);

                AssetEditorPresetNotificationBus::Event(m_editorId, &AssetEditorPresetNotifications::OnConstructPresetsChanged, m_constructType);
            }
        }
        break;
        case Qt::CheckStateRole:
            if (index.column() == ColumnIndex::DefaultPreset)
            {
                modifiedData = true;
                structure->m_isDefault = value.toBool();

                if (!structure->m_isDefault)
                {
                    PresetStructure* presetStructure = ModPresetStructureForRow(0);
                    presetStructure->m_isDefault = true;

                    m_presetsContainer->SetDefaultPreset(m_constructType, 0);
                    dataChanged(createIndex(0, ColumnIndex::DefaultPreset, nullptr), createIndex(0, ColumnIndex::DefaultPreset, nullptr));
                }
                else
                {
                    int oldPresetIndex = 0;
                    
                    auto constructBucket = m_presetsContainer->FindPresetBucket(m_constructType);

                    if (constructBucket)
                    {
                        oldPresetIndex = constructBucket->GetDefaultPresetIndex();
                    }                    

                    PresetStructure* previousDefaultPreset = ModPresetStructureForRow(oldPresetIndex);

                    if (previousDefaultPreset)
                    {
                        previousDefaultPreset->m_isDefault = false;
                        dataChanged(createIndex(oldPresetIndex, ColumnIndex::Name, nullptr), createIndex(oldPresetIndex, ColumnIndex::DefaultPreset, nullptr));
                    }

                    m_presetsContainer->SetDefaultPreset(m_constructType, index.row());
                }                
            }
        break;
        default:
            break;
        }

        emit OnPresetModificationEnd();

        return modifiedData;
    }

    Qt::ItemFlags ConstructPresetsTableModel::flags(const QModelIndex& index) const
    {
        Qt::ItemFlags itemFlags = Qt::ItemFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        if (index.column() == ColumnIndex::Name)
        {
            itemFlags |= Qt::ItemIsEditable;
        }
        else if (index.column() == ColumnIndex::DefaultPreset)
        {
            itemFlags |= Qt::ItemIsUserCheckable;
        }

        return itemFlags;
    }

    void ConstructPresetsTableModel::SetConstructType(const ConstructType& constructType)
    {
        m_constructType = constructType;
    }

    void ConstructPresetsTableModel::RefreshDisplay(bool refreshPresets)
    {
        layoutAboutToBeChanged();

        if (refreshPresets || m_presetsContainer == nullptr)
        {
            AssetEditorSettingsRequestBus::EventResult(m_presetsContainer, m_editorId, &AssetEditorSettingsRequests::GetConstructPresets);
        }

        m_activePresets.clear();

        if (m_presetsContainer != nullptr && m_constructType != ConstructType::Unknown)
        {
            const ConstructTypePresetBucket* presetBucket = m_presetsContainer->FindPresetBucket(m_constructType);            

            if (presetBucket)
            {
                m_activePresets.reserve(presetBucket->GetPresetCount());

                for (auto preset : presetBucket->GetPresets())
                {
                    PresetStructure presetStructure;
                    presetStructure.m_preset = preset;
                    presetStructure.m_pixmap = preset->GetDisplayIcon(m_editorId);

                    m_activePresets.emplace_back(presetStructure);
                }

                int defaultPreset = presetBucket->GetDefaultPresetIndex();

                if (defaultPreset >= 0 && defaultPreset < m_activePresets.size())
                {
                    m_activePresets[defaultPreset].m_isDefault = true;
                }
            }
        }

        layoutChanged();
    }

    void ConstructPresetsTableModel::RemoveRows(const AZStd::vector<int>& rows)
    {
        if (m_presetsContainer == nullptr)
        {
            return;
        }
        
        emit OnPresetModificationBegin();

        AZStd::vector< AZStd::shared_ptr< ConstructPreset > > removeablePresets;
        removeablePresets.reserve(rows.size());

        for (auto row : rows)
        {
            PresetStructure* presetStructure = ModPresetStructureForRow(row);

            if (presetStructure)
            {
                removeablePresets.emplace_back(presetStructure->m_preset);
            }
        }

        m_presetsContainer->RemovePresets(removeablePresets);

        emit OnPresetModificationEnd();

        RefreshDisplay();
    }

    void ConstructPresetsTableModel::OnSettingsChanged()
    {
        m_presetsContainer = nullptr;
    }

    EditorConstructPresets* ConstructPresetsTableModel::GetPresetContainer() const
    {
        return m_presetsContainer;
    }

    ConstructPresetsTableModel::PresetStructure* ConstructPresetsTableModel::ModPresetStructureForIndex(const QModelIndex& modelIndex)
    {
        if (!modelIndex.isValid())
        {
            return nullptr;
        }

        return ModPresetStructureForRow(modelIndex.row());
    }

    ConstructPresetsTableModel::PresetStructure* ConstructPresetsTableModel::ModPresetStructureForRow(int row)
    {
        if (row < 0 || row >= m_activePresets.size())
        {
            return nullptr;
        }

        return &m_activePresets[row];
    }

    const ConstructPresetsTableModel::PresetStructure* ConstructPresetsTableModel::FindPresetStructureForIndex(const QModelIndex& modelIndex) const
    {
        if (!modelIndex.isValid() || modelIndex.row() > m_activePresets.size())
        {
            return nullptr;
        }

        return &m_activePresets[modelIndex.row()];
    }

    //////////////////////////
    // ConstructPresetDialog
    //////////////////////////

    ConstructPresetDialog::ConstructPresetDialog(QWidget* parent)
        : QMainWindow(parent, Qt::WindowFlags::enum_type::WindowCloseButtonHint)
        , m_ui(new Ui::ConstructPresetDialog())        
        , m_ignorePresetChanges(false)
    {
        m_ui->setupUi(this);

        // Once we set things up as an asset. We'll need the file bar in order to manage things appropriately.
        m_ui->menubar->setVisible(false);

        m_presetsModel = aznew ConstructPresetsTableModel(this);

        AddConstructType(ConstructType::CommentNode);
        AddConstructType(ConstructType::NodeGroup);

        m_presetsModel->SetConstructType(GetActiveConstructType());

        m_ui->constructListing->setModel(m_presetsModel);
        m_ui->constructListing->horizontalHeader();
        m_ui->constructListing->horizontalHeader()->setStretchLastSection(false);
        m_ui->constructListing->horizontalHeader()->setSectionResizeMode(ConstructPresetsTableModel::ColumnIndex::Name, QHeaderView::ResizeMode::Stretch);
        m_ui->constructListing->horizontalHeader()->setSectionResizeMode(ConstructPresetsTableModel::ColumnIndex::DefaultPreset, QHeaderView::ResizeMode::Fixed);
        m_ui->constructListing->setItemDelegateForColumn(ConstructPresetsTableModel::ColumnIndex::Name, aznew GraphCanvas::IconDecoratedNameDelegate(m_ui->constructListing));
        m_ui->constructListing->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);

        m_ui->removePreset->setEnabled(false);

        QObject::connect(m_ui->removePreset, &QPushButton::pressed, this, &ConstructPresetDialog::RemoveSelected);
        QObject::connect(m_ui->okButton, &QPushButton::clicked, this, &QMainWindow::close);
        QObject::connect(m_ui->restoreDefaults, &QPushButton::clicked, this, &ConstructPresetDialog::RestoreDefaults);

        QObject::connect(m_ui->constructListing->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ConstructPresetDialog::OnSelectionChanged);

        QObject::connect(m_ui->constructTypes, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ConstructPresetDialog::OnActiveConstructTypeChanged);

        QObject::connect(m_presetsModel, &ConstructPresetsTableModel::OnPresetModificationBegin, this, &ConstructPresetDialog::OnPresetModificationBegin);
        QObject::connect(m_presetsModel, &ConstructPresetsTableModel::OnPresetModificationBegin, this, &ConstructPresetDialog::OnPresetModificationEnd);
    }

    ConstructPresetDialog::~ConstructPresetDialog()
    {
        AssetEditorPresetNotificationBus::Handler::BusDisconnect();
    }

    void ConstructPresetDialog::SetEditorId(EditorId editorId)
    {
        m_editorId = editorId;
        m_presetsModel->SetEditorId(editorId);
        
        AssetEditorPresetNotificationBus::Handler::BusConnect(m_editorId);
    }

    void ConstructPresetDialog::AddConstructType(ConstructType constructType)
    {
        AZStd::string constructName = EnumStringifier::GetConstructTypeString(constructType);
        m_ui->constructTypes->addItem(constructName.c_str());

        m_constructType.emplace_back(constructType);
    }

    void ConstructPresetDialog::OnPresetsChanged()
    {
        if (!m_ignorePresetChanges)
        {
            const bool forcePresetsRefresh = true;
            m_presetsModel->RefreshDisplay(forcePresetsRefresh);
        }
    }

    void ConstructPresetDialog::OnConstructPresetsChanged(ConstructType constructType)
    {
        if (!m_ignorePresetChanges)
        {
            if (GetActiveConstructType() == constructType)
            {
                m_presetsModel->RefreshDisplay();
            }
        }
    }

    void ConstructPresetDialog::showEvent(QShowEvent* /*showEvent*/)
    {        
        m_presetsModel->RefreshDisplay();
    }

    ConstructType ConstructPresetDialog::GetActiveConstructType() const
    {
        int activeIndex = m_ui->constructTypes->currentIndex();

        if (activeIndex < 0 || activeIndex >= m_constructType.size())
        {
            return ConstructType::Unknown;
        }

        return m_constructType[activeIndex];
    }

    void ConstructPresetDialog::SetActiveConstructType(ConstructType constructType) const
    {
        int newIndex = -1;

        for (int i = 0; i < m_constructType.size(); ++i)
        {
            if (m_constructType[i] == constructType)
            {
                newIndex = i;
                break;
            }
        }

        if (newIndex >= 0)
        {
            m_ui->constructTypes->setCurrentIndex(newIndex);
        }
    }

    void ConstructPresetDialog::OnSelectionChanged(const QItemSelection&, const QItemSelection&)
    {
        UpdateButtonStates();        
    }

    void ConstructPresetDialog::RemoveSelected()
    {
        AZStd::vector<int> rows;
        for (const auto& index : m_ui->constructListing->selectionModel()->selectedRows())
        {
            rows.push_back(index.row());
        }

        m_ui->constructListing->clearSelection();
        m_presetsModel->RemoveRows(rows);
    }

    void ConstructPresetDialog::RestoreDefaults()
    {
        OnPresetModificationBegin();
        ConstructType activeType = GetActiveConstructType();

        m_presetsModel->GetPresetContainer()->InitializeConstructType(activeType);
        AssetEditorPresetNotificationBus::Event(m_editorId, &AssetEditorPresetNotifications::OnConstructPresetsChanged, activeType);

        OnPresetModificationEnd();
        m_presetsModel->RefreshDisplay();
    }

    void ConstructPresetDialog::UpdateButtonStates()
    {
        auto selectedIndexes = m_ui->constructListing->selectionModel()->selectedIndexes();

        m_ui->removePreset->setEnabled(!selectedIndexes.empty());

        // If there's only one element we don't want to allow removal of it.
        // Mostly because we always ensure there's one preset available. 
        // So it will remove it, then add on a new one. And if you do this
        // with the base one it looks like nothing is happening.
        if (m_presetsModel->rowCount() == 1)
        {
            m_ui->removePreset->setEnabled(false);
        }
    }

    void ConstructPresetDialog::OnActiveConstructTypeChanged(int)
    {
        m_ui->constructListing->clearSelection();
        m_presetsModel->SetConstructType(GetActiveConstructType());
        m_presetsModel->RefreshDisplay();
    }

    void ConstructPresetDialog::OnPresetModificationBegin()
    {
        AssetEditorPresetNotificationBus::Handler::BusDisconnect(m_editorId);
    }

    void ConstructPresetDialog::OnPresetModificationEnd()
    {        
        AssetEditorPresetNotificationBus::Handler::BusConnect(m_editorId);
    }

#include <StaticLib/GraphCanvas/Widgets/ConstructPresetDialog/moc_ConstructPresetDialog.cpp>
}
