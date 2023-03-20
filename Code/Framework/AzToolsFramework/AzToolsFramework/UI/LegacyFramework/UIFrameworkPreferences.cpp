/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "UIFramework.hxx"
#include <qaction.h>
#include <qheaderview.h>

#include <QMessageBox>

// split out the preferences implementation from the main UIFramework.cpp file

namespace AzToolsFramework
{
    void Framework::PreferencesOpening(QTabWidget* panel)
    {
        // create and populate the data model
        m_View = aznew AZPreferencesView(panel);
        if (m_View)
        {
            m_Model = aznew AZPreferencesDataModel(m_View);
            if (m_Model)
            {
                PopulateTheModel(m_Model);

                panel->addTab(m_View, "Hotkeys");
                m_View->setModel(m_Model);
                m_View->horizontalHeader()->setStretchLastSection(true);
                m_View->resizeColumnsToContents();
                m_View->setColumnWidth(1, 150); // hotkey text area large enough for "ctrl+alt+shift+scroll lock" combo
            }
        }
        else
        {
            delete m_View;
            m_View = nullptr;
        }
    }
    void Framework::PreferencesAccepted()
    {
        ApplyModelToLive(m_Model);
        PreferencesClosing();
    }
    void Framework::PreferencesRejected()
    {
        PreferencesClosing();
    }
    void Framework::PreferencesRevertToDefault()
    {
        m_Model->RevertToDefault();
    }
    void Framework::PreferencesClosing()
    {
        if (m_View)
        {
            delete m_View;
            m_View = nullptr;
        }
        if (m_Model)
        {
            m_Model = nullptr;
        }
    }

    void Framework::ApplyModelToLive(AZPreferencesDataModel* model)
    {
        model->ApplyToLive();
    }

    void Framework::PopulateTheModel(AZPreferencesDataModel* model)
    {
        HotkeyDescriptorContainerType::iterator hkIter = m_hotkeyDescriptors.begin();
        while (hkIter != m_hotkeyDescriptors.end())
        {
            //AZ::Uuid &hkGUID = hkIter->first;
            HotkeyData& hkData = hkIter->second;

            model->AddItem(CreateHotkeyItem(hkData));

            ++hkIter;
        }
    }

    AZPreferencesItem* Framework::CreateHotkeyItem(HotkeyData& hotkey)
    {
        AZPreferencesItem* item = aznew AZPreferencesItem(hotkey);
        item->setCheckable(false);
        item->setDragEnabled(false);
        item->setDropEnabled(false);
        item->setSelectable(false);
        item->setToolTip("Double-click to change hotkey assignment");
        item->setEditable(false);
        item->setEnabled(true);
        return item;
    }

    //--------------------------------------------------------------------
    AZPreferencesView::AZPreferencesView(QWidget* parent)
        : QTableView(parent)
    {
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setSelectionMode(QAbstractItemView::SingleSelection);

        connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(OnDoubleClicked(const QModelIndex &)));
    }
    AZPreferencesView::~AZPreferencesView()
    {
    }

    void AZPreferencesView::OnDoubleClicked(const QModelIndex& index)
    {
        AZPreferencesDataModel* model = (AZPreferencesDataModel*)index.model();
        AZPreferencesItem* item = model->itemAtIndex(index.row());
        if (item)
        {
        }
    }

    void AZPreferencesView::CaptureHotkeyAccepted(QKeySequence keysPressed)
    {
        clickResult = keysPressed;
    }

    //------------------------------------------------------------------------
    AZPreferencesItem::AZPreferencesItem(Framework::HotkeyData& hkData)
        : hotkeyData(hkData)
        , hotkeySource(hkData)
    {
    }
    AZPreferencesItem::~AZPreferencesItem()
    {
    }

    void AZPreferencesItem::SetCurrentKey(QKeySequence& key)
    {
        hotkeyData.m_desc.m_currentKey = key.toString().toUtf8().data();
    }
    void AZPreferencesItem::ClearCurrentKey()
    {
        hotkeyData.m_desc.m_currentKey = "";
    }

    //-----------------------------------------------------------------------
    AZPreferencesDataModel::AZPreferencesDataModel(QObject* pParent)
        : QAbstractTableModel(pParent)
    {
    }
    AZPreferencesDataModel::~AZPreferencesDataModel()
    {
        vecPrefs::iterator iter = m_Items.begin();
        while (iter != m_Items.end())
        {
            // container items were allocated when first added in PopulateTheModel, here is the matched delete
            delete (*iter);
            ++iter;
        }
        m_Items.clear();
    }

    int AZPreferencesDataModel::rowCount(const QModelIndex& index) const
    {
        (void)index;
        return (int)m_Items.size();
    }
    int AZPreferencesDataModel::columnCount(const QModelIndex& index) const
    {
        (void)index;
        return 3;
    }

    Qt::ItemFlags AZPreferencesDataModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return Qt::ItemIsEnabled;
        }

        return QAbstractItemModel::flags(index);
    }

    QVariant AZPreferencesDataModel::data(const QModelIndex& index, int role) const
    {
        using namespace AZ::Debug;
        if (role == Qt::DecorationRole)
        {
            return QVariant();
        }
        else
        {
            if (role == Qt::DisplayRole)
            {
                if (index.column() == 0)
                {
                    return QVariant(m_Items[index.row()]->hotkeyData.m_desc.m_category.c_str());
                }
                else if (index.column() == 1)
                {
                    return QVariant(m_Items[index.row()]->hotkeyData.m_desc.m_currentKey.c_str());
                }
                else if (index.column() == 2)
                {
                    return QVariant(m_Items[index.row()]->hotkeyData.m_desc.m_description.c_str());
                }
            }
        }
        return QVariant();
    }

    QVariant AZPreferencesDataModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            if (section == 0)
            {
                return QVariant("Category");
            }
            else if (section == 1)
            {
                return QVariant("Current Hotkey");
            }
            else if (section == 2)
            {
                return QVariant("Description");
            }
        }

        return QVariant();
    }

    void AZPreferencesDataModel::AddItem(AZPreferencesItem* item)
    {
        vecPrefs::iterator iter = m_Items.begin();
        while (iter != m_Items.end())
        {
            if ((*iter)->hotkeyData.m_desc.m_category == item->hotkeyData.m_desc.m_category)
            {
                m_Items.insert(iter, item);
                return;
            }
            ++iter;
        }
        m_Items.push_back(item);
    }

    void AZPreferencesDataModel::RevertToDefault()
    {
        vecPrefs::iterator iter = m_Items.begin();
        while (iter != m_Items.end())
        {
            (*iter)->hotkeyData.m_desc.m_currentKey = (*iter)->hotkeyData.m_desc.m_defaultKey;
            ++iter;
        }

        QModelIndex tl = index(0, 0);
        QModelIndex br = index((int)m_Items.size(), 2);
        emit dataChanged(tl, br);
    }

    AZPreferencesItem* AZPreferencesDataModel::itemAtIndex(int index)
    {
        return m_Items[ index ];
    }

    void AZPreferencesDataModel::ApplyToLive()
    {
        vecPrefs::iterator iter = m_Items.begin();
        while (iter != m_Items.end())
        {
            (*iter)->hotkeySource = (*iter)->hotkeyData;
            (*iter)->hotkeySource.SelfBindActions();
            ++iter;
        }
    }

    void AZPreferencesDataModel::SafeApplyKeyToItem(QKeySequence& clickResult, AZPreferencesItem* item)
    {
        // first clear all existing actions using this key, within scope restrictions
        vecPrefs::iterator iter = m_Items.begin();
        while (iter != m_Items.end())
        {
            if (*iter != item)
            {
                // if the incoming key matches one already existing
                if (!qstricmp((*iter)->hotkeyData.m_desc.m_currentKey.c_str(), clickResult.toString().toUtf8().data()))
                {
                    if ((*iter)->hotkeyData.m_desc.m_scope == HotkeyDescription::SCOPE_GLOBAL)
                    {
                        // yes/no dialog to verify overriding a global hotkey with a forceful warning
                        QMessageBox msgBox;
                        msgBox.setText("Override a System global hotkey?");
                        msgBox.setInformativeText((*iter)->hotkeyData.m_desc.m_description.c_str());
                        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                        msgBox.setDefaultButton(QMessageBox::Cancel);
                        msgBox.setIcon(QMessageBox::Warning);
                        int ret = msgBox.exec();
                        if (ret == QMessageBox::Cancel)
                        {
                            // early out doesn't change any settings
                            return;
                        }
                        // clear the existing hotkey
                        (*iter)->ClearCurrentKey();
                    }
                    else if ((*iter)->hotkeyData.m_desc.m_WindowGroupID == item->hotkeyData.m_desc.m_WindowGroupID || item->hotkeyData.m_desc.m_scope == HotkeyDescription::SCOPE_GLOBAL)
                    {
                        // yes/no dialog to verify override with an advisement
                        QMessageBox msgBox;
                        msgBox.setText("Replace an existing hotkey?");
                        msgBox.setInformativeText((*iter)->hotkeyData.m_desc.m_description.c_str());
                        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                        msgBox.setDefaultButton(QMessageBox::Cancel);
                        msgBox.setIcon(QMessageBox::Information);
                        int ret = msgBox.exec();
                        if (ret == QMessageBox::Cancel)
                        {
                            // early out doesn't change any settings
                            return;
                        }
                        // clear the existing hotkey
                        (*iter)->ClearCurrentKey();
                    }
                }
            }

            ++iter;
        }

        // then set the key into the current action
        item->SetCurrentKey(clickResult);
    }

    void Framework::HotkeyData::SelfBindActions()
    {
        QKeySequence qkey(m_desc.m_currentKey.c_str());

        ActionContainerType::iterator iter = m_actionsBound.begin();
        while (iter != m_actionsBound.end())
        {
            (*iter)->setShortcut(qkey);

            // bridge between our own scope of control to QT's context setup
            switch (m_desc.m_scope)
            {
            case AzToolsFramework::HotkeyDescription::SCOPE_GLOBAL:
                (*iter)->setShortcutContext (Qt::ApplicationShortcut);
                break;
            case AzToolsFramework::HotkeyDescription::SCOPE_WINDOW:
                (*iter)->setShortcutContext (Qt::WindowShortcut);
                break;
            case AzToolsFramework::HotkeyDescription::SCOPE_WIDGET:
                (*iter)->setShortcutContext (Qt::WidgetShortcut);
                break;
            }

            ++iter;
        }
    }
}
