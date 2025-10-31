/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WatchesPanel.hxx"

#include "AzCore/Debug/Trace.h"
#include "LUAEditorDebuggerMessages.h"
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Script/lua/lua.h>
#include <Source/LUA/moc_WatchesPanel.cpp>

#include <QSortFilterProxyModel>
#include <QMenu>
#include <QAction>
#include <QKeyEvent>

namespace WatchesPanel
{
    static constexpr AZStd::array typeStringLUT {
        "NIL",           // LUA_TNIL
        "BOOLEAN",       // LUA_TBOOLEAN
        "LIGHTUSERDATA", // LUA_TLIGHTUSERDATA
        "NUMBER",        // LUA_TNUMBER
        "STRING",        // LUA_TSTRING
        "TABLE",         // LUA_TTABLE
        "FUNCTION",      // LUA_TFUNCTION
        "USERDATA",      // LUA_TUSERDATA
        "THREAD",        // LUA_TTHREAD
    };
}


class WatchesFilterModel
    : public QSortFilterProxyModel
{
public:
    AZ_CLASS_ALLOCATOR(WatchesFilterModel, AZ::SystemAllocator);
    WatchesFilterModel(QObject* pParent)
        : QSortFilterProxyModel(pParent)
    {
        setFilterCaseSensitivity(Qt::CaseSensitive);
        setDynamicSortFilter(false);
    }
protected:
    virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        return QSortFilterProxyModel::lessThan(left, right);
    }
};


DHWatchesWidget::DHWatchesWidget(QWidget* parent)
    : AzToolsFramework::QTreeViewWithStateSaving(parent)
{
    setFocusPolicy(Qt::StrongFocus); // required for key press handling

    setEnabled(false);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);

    SetOperatingMode(WATCHES_MODE_GENERAL);
    m_OperatingState = WATCHES_STATE_DISCONNECTED;

    m_pFilterModel = aznew WatchesFilterModel(this);
    m_pFilterModel->setSourceModel(&m_DM);

    setModel(m_pFilterModel);

    LUAEditor::LUAWatchesDebuggerMessages::Handler::BusConnect();
    LUAEditor::LUALocalsTrackerMessages::Handler::BusConnect();
    LUAEditor::LUABreakpointTrackerMessages::Handler::BusConnect();

    connectDataModelUpdate();
    connect(this, &DHWatchesWidget::doubleClicked, this, &DHWatchesWidget::OnDoubleClicked);

    auto crc = AZ::Crc32("StandaloneToolsWatchesPanel");
    InitializeTreeViewSaving(crc);

    ForceSelectNewWatch();
}

void DHWatchesWidget::disconnectDataModelUpdate()
{
    disconnect(m_dataModelDataChangedConnection);
    disconnect(m_dataModelRestConnection);
}

void DHWatchesWidget::connectDataModelUpdate()
{
    m_dataModelDataChangedConnection = connect(&m_DM, &WatchesDataModel::dataChanged, this, &DHWatchesWidget::OnItemChanged);
    m_dataModelRestConnection = connect(&m_DM, &WatchesDataModel::modelReset, this, &DHWatchesWidget::OnItemChanged);
}

DHWatchesWidget::~DHWatchesWidget()
{
    LUAEditor::LUABreakpointTrackerMessages::Handler::BusDisconnect();
    LUAEditor::LUALocalsTrackerMessages::Handler::BusDisconnect();
    LUAEditor::LUAWatchesDebuggerMessages::Handler::BusDisconnect();
}

void DHWatchesWidget::SetOperatingMode(WatchesOperatingMode newMode)
{
    m_OperatingMode = newMode;
    m_DM.SetOperatingMode(newMode);
}

void DHWatchesWidget::OnDebuggerAttached()
{
    m_OperatingState = WATCHES_STATE_CONNECTED;
    setEnabled(true);

    if (m_OperatingMode == WATCHES_MODE_GENERAL)
    {
        CaptureVariables();
    }
}

void DHWatchesWidget::OnDebuggerDetached()
{
    m_OperatingState = WATCHES_STATE_DISCONNECTED;
    LocalsClear();
    setEnabled(false);
}

void DHWatchesWidget::CaptureVariables()
{
    //AZ_TracePrintf("LUA Debug", "CaptureVariables:\n");

    if (m_OperatingState == WATCHES_STATE_CONNECTED)
    {
        // query watched variables

        int shortLoop = m_OperatingMode == WATCHES_MODE_GENERAL ? 1 : 0;
        int rowCount = m_DM.rowCount() - shortLoop; // negative offset to skip the <new watch> for the not-locals panel

        //AZ_TracePrintf("LUA Debug", "CaptureVariables( count = %d)\n",rowCount);

        for (int i = 0; i < rowCount; ++i)
        {
            QModelIndex indexChild = m_DM.index(i, 0);
            if (indexChild.isValid())
            {
                QString name;
                name = m_DM.data(indexChild).toString();
                //AZ_TracePrintf("LUA Editor", "  - RequestWatchedVariable( %s )\n", AZStd::string(name.toAscii()));
                LUAEditor::LUAWatchesRequestMessagesRequestBus::Broadcast(&LUAEditor::LUAWatchesRequestMessages::RequestWatchedVariable, AZStd::string(name.toUtf8().data()));
                // results will return via WatchesUpdate() values asynchronously
                // NB: not recursive, only top-level variable names are requested
            }
        }
    }
}

void DHWatchesWidget::BreakpointHit(const LUAEditor::Breakpoint& bp)
{
    (void)bp;
    if (m_OperatingMode == WATCHES_MODE_GENERAL)
    {
        CaptureVariables();
    }
    else // m_OperatingMode == WATCHES_MODE_LOCALS
    {
        if (isVisible())
        {
            LUAEditor::LUAEditorDebuggerMessagesRequestBus::Broadcast(&LUAEditor::LUAEditorDebuggerMessages::EnumLocals);
        }
    }
}

void DHWatchesWidget::WatchesUpdate(const AZ::ScriptContextDebug::DebugValue& topmostDebugReference)
{
    //AZ_TracePrintf("LUA Editor", "incoming WatchesUpdate( %s )\n", topmostDebugReference.m_name);
    disconnectDataModelUpdate();
    m_DM.UpdateMatchingDVs(topmostDebugReference);
    connectDataModelUpdate();
    ApplyTreeViewSnapshot();
}

void DHWatchesWidget::OnItemChanged()
{
    if (m_OperatingMode == WATCHES_MODE_GENERAL)
    {
        CaptureVariables();
    }
}

void DHWatchesWidget::OnDoubleClicked(const QModelIndex& index)
{
    //AZ_TracePrintf("BP", "OnDoubleClicked() %d, %d\n", index.row(), index.column());

    if (m_OperatingState == WATCHES_STATE_CONNECTED)
    {
        if (m_pFilterModel->mapToSource(index).column() == 2)
        {
            // double click popup only works on the TYPE column
            {
                if (m_DM.IsTypeChangeAllowed(m_pFilterModel->mapToSource(index)))
                {
                    QMenu* popupMenu = new QMenu(this);
                    {
                        QMenu* layoutMenu = new QMenu(tr("LUA Value Type"), this);
                        layoutMenu->addAction(tr("Boolean"));
                        layoutMenu->addAction(tr("Number"));
                        layoutMenu->addAction(tr("String"));
                        popupMenu->addMenu(layoutMenu);
                        QAction* act = popupMenu->exec(QCursor::pos());
                        if (act)
                        {
                            char newType = LUA_TNONE;

                            if (act->text() == tr("Boolean"))
                            {
                                newType = LUA_TBOOLEAN;
                            }
                            if (act->text() == tr("Number"))
                            {
                                newType = LUA_TNUMBER;
                            }
                            if (act->text() == tr("String"))
                            {
                                newType = LUA_TSTRING;
                            }

                            m_DM.SetType(m_pFilterModel->mapToSource(index), newType);
                        }
                    }
                }
            }
        }
    }
}

void DHWatchesWidget::LocalsUpdate(const AZStd::vector<AZStd::string>& vars)
{
    disconnectDataModelUpdate();
    if (m_OperatingMode == WATCHES_MODE_LOCALS)
    {
        LocalsClear();

        //AZ_TracePrintf("LUA Editor", "LOCALSUPDATE\n");

        for (AZStd::vector<AZStd::string>::const_iterator it = vars.begin(); it != vars.end(); ++it)
        {
            //AZ_TracePrintf("LUA Editor", "  -  LocalsUpdate( %s )\n", it->c_str());
            m_DM.AddWatch(it->c_str());
        }
    }
    connectDataModelUpdate();
}

void DHWatchesWidget::LocalsClear()
{
    //AZ_TracePrintf("LUA Editor", "LOCALS Clear\n");
    disconnectDataModelUpdate();
    if (m_OperatingMode == WATCHES_MODE_LOCALS)
    {
        if (m_DM.rowCount())
        {
            // local implementation of removeRows handles the begin/end cycle
            m_DM.removeRows(0, m_DM.rowCount());
        }
    }
    connectDataModelUpdate();
}

void DHWatchesWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_OperatingMode == WATCHES_MODE_GENERAL)
    {
        if (event->isAccepted())
        {
            if (event->key() == Qt::Key_Delete)
            {
                QModelIndexList mil = selectedIndexes();
                for (QModelIndexList::iterator iter = mil.begin(); iter != mil.end(); ++iter)
                {
                    QModelIndex toRemove =  m_pFilterModel->mapToSource(*iter);
                    m_DM.RemoveWatch(toRemove);
                }
                event->accept();
                return;
            }
            if ((event->key() == Qt::Key_Enter) || (event->key() == Qt::Key_Return))
            {
                QModelIndexList mil = selectedIndexes();
                if (mil.size())
                {
                    // edit the selected item by spoofing QT's "edit key"
                    QKeyEvent evt(event->type(), Qt::Key_F2, event->modifiers());
                    QTreeView::keyPressEvent(&evt);
                    event->accept();
                    return;
                }
                else
                {
                    // if nothing is selected then force edit to the <new watch> item
                    ForceSelectNewWatch();

                    event->ignore();
                    QTreeView::keyPressEvent(event);
                    return;
                }
            }
        }
    }

    event->ignore();
    QTreeView::keyPressEvent(event);
}

void DHWatchesWidget::ForceSelectNewWatch()
{
    if (m_OperatingMode == WATCHES_MODE_GENERAL)
    {
        int row = m_DM.rowCount() - 1;
        QModelIndex index = m_DM.index(row, 0);
        selectionModel()->select(m_pFilterModel->mapFromSource(index), QItemSelectionModel::ClearAndSelect);
        setCurrentIndex(m_pFilterModel->mapFromSource(index));
    }
}

//------------------------------------------------------------------------

WatchesDataModel::WatchesDataModel()
{
    m_parentsDirty = false;
    m_OperatingMode = WATCHES_MODE_GENERAL;
}

WatchesDataModel::~WatchesDataModel()
{
}

void WatchesDataModel::SetOperatingMode(WatchesOperatingMode newMode)
{
    beginResetModel(); // this has to be a reset because changing the operating mode can change row count.
    m_OperatingMode = newMode;
    endResetModel();
}

void WatchesDataModel::AddWatch(const AZ::ScriptContextDebug::DebugValue& newData)
{
    //beginResetModel(); // this has to be a reset because push_back can reallocate.

    beginInsertRows(QModelIndex(), (int)m_DebugValues.size(), (int)m_DebugValues.size());

    //AZ_TracePrintf("LUA Editor", "AddWatch( lr = %d rc = %d dvsize = %d )\n",m_OperatingMode,rowCount(),m_DebugValues.size());
    m_DebugValues.push_back(newData);
    m_parentsDirty = true;

    endInsertRows();
}

void WatchesDataModel::AddWatch(AZStd::string newName)
{
    //AZ_TracePrintf("LUA Editor", "AddWatch( lr = %d - %s )\n",m_OperatingMode,newName.c_str());

    AZ::ScriptContextDebug::DebugValue dv;
    dv.m_name = newName;
    dv.m_value = "<invalid>";
    dv.m_type = LUA_TNONE;
    dv.m_typeId = AZ::ScriptTypeId{};
    dv.m_flags = 0;

    AddWatch(dv);
}

void WatchesDataModel::RemoveWatch(QModelIndex& index)
{
    if (IsRealIndex(index))
    {
        const QModelIndex qmi = GetTopmostIndex(index);
        // we only delete full rows, incoming selections can hold many columns in that row
        // and so we must skip any but the first column
        if (qmi.isValid() && (qmi.column() == 0))
        {
            removeRow(qmi.row(), parent(qmi));
        }
    }
}

bool WatchesDataModel::removeRows (int row, int count, const QModelIndex& parent)
{
    (void)count;

    beginRemoveRows(parent, row, row + count - 1);

    m_DebugValues.erase(m_DebugValues.begin() + row, m_DebugValues.begin() + row + count);
    m_parentsDirty = true;

    endRemoveRows();

    return true;
}

const AZ::ScriptContextDebug::DebugValue *WatchesDataModel::GetDV(const QModelIndex & index) const
{
    if (IsRealIndex(index))
    {
        const AZ::ScriptContextDebug::DebugValue *dv = static_cast<const AZ::ScriptContextDebug::DebugValue *>(index.internalPointer());
        return dv;
    }

    return NULL;
}

const QModelIndex WatchesDataModel::GetTopmostIndex(const QModelIndex& index)
{
    if (index.parent().isValid())
    {
        return GetTopmostIndex(index.parent());
    }

    return index;
}

void WatchesDataModel::UpdateMatchingDVs(const AZ::ScriptContextDebug::DebugValue& newData)
{
    bool startedReset = false;

    for (int idx = 0; idx < m_DebugValues.size(); ++idx)
    {
        if (m_DebugValues[idx].m_name == newData.m_name)
        {
            if (!startedReset)
            {
                beginResetModel();
            }

            startedReset = true;
            ApplyNewData(m_DebugValues[idx], newData);
        }
    }

    if (startedReset)
    {
        endResetModel();
    }
}
void WatchesDataModel::ApplyNewData(AZ::ScriptContextDebug::DebugValue& original, const AZ::ScriptContextDebug::DebugValue& newvalues)
{
    //AZ_TracePrintf("LUA Editor", "ApplyNewData: name=%s to name=%s\n", original.m_name.c_str(),newvalues.m_name.c_str());

    // deep copy operator=, original is the locally maintained version and newvalues just arrived from outside
    original = newvalues;
    m_parentsDirty = true;
    //DVRecursePrint( original, 0 );
}

void WatchesDataModel::DVRecursePrint(const AZ::ScriptContextDebug::DebugValue& dv, int indent) const
{
    for (int idx = 0; idx < dv.m_elements.size(); ++idx)
    {
        AZ_TracePrintf("LUA Editor", "(%d) of (%d) - %s := %s\n", idx, dv.m_elements.size(), dv.m_name.c_str(), dv.m_value.c_str());

        DVRecursePrint(dv.m_elements[idx], indent + 1);
    }
}


void WatchesDataModel::RegenerateParentsMap() const
{
    // lazy recalculation of the map from DV* to its parent DV*
    if (m_parentsDirty)
    {
        m_parents.clear();

        for (int idx = 0; idx < m_DebugValues.size(); ++idx)
        {
            m_parents[ &m_DebugValues[idx] ] = NULL;

            RegenerateParentsMapRecurse(m_DebugValues[idx]);
        }

        m_parentsDirty = false;
    }
}

void WatchesDataModel::RegenerateParentsMapRecurse(const AZ::ScriptContextDebug::DebugValue& dv) const
{
    for (int idx = 0; idx < dv.m_elements.size(); ++idx)
    {
        m_parents[ &dv.m_elements[idx] ] = &dv;

        RegenerateParentsMapRecurse(dv.m_elements[idx]);
    }
}

int WatchesDataModel::columnCount (const QModelIndex& parent) const
{
    (void)parent;

    return 3; // name + value + type
}

QVariant WatchesDataModel::data (const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if (!IsRealIndex(index))
        {
            if (index.column() == 0)
            {
                return QVariant("<new watch>");
            }
            return QVariant();
        }

        const AZ::ScriptContextDebug::DebugValue *dv = GetDV(index);

        // retrieve the DebugValue for this index
        // return a variant built from its displayable text name
        if (dv)
        {
            switch (index.column())
            {
            case 0: // name
                return QVariant(dv->m_name.c_str());
                break;
            case 1: // value
                return QVariant(dv->m_value.c_str());
                break;
            case 2: // type lookup
                return QVariant(SafetyType(dv->m_type));
                break;
            }
        }
    }

    return QVariant();
}

Qt::ItemFlags WatchesDataModel::flags (const QModelIndex& index) const
{
    if (!index.isValid())
    {
        return Qt::ItemFlags();
    }
    const AZ::ScriptContextDebug::DebugValue *dv = GetDV(index);

    switch (index.column())
    {
    case 0: // NAME
    {
        // LUA locals do not allow the name to change, only the value
        bool RO = (m_OperatingMode == WATCHES_MODE_LOCALS);

        if (index.isValid() && !index.parent().isValid() && !RO)
        {
            // topmost
            //if(dv) AZ_TracePrintf("LUA Editor", "EDITABLE flags RO( %d ) dv( %s )\n", (int)RO, dv->m_name );
            return Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        }
        else
        {
            // children names cannot be edited
            //if(dv) AZ_TracePrintf("LUA Editor", "LOCKED flags RO( %d ) dv( %s )\n", (int)RO, dv->m_name );
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        }
    }
    break;
    case 1: // VALUE
    {
        bool RO =
            (dv == NULL)
            || (dv->m_type == LUA_TFUNCTION)
            || (static_cast<int>(dv->m_type) == LUA_TNONE)
                || (dv->m_flags & AZ::ScriptContextDebug::DebugValue::FLAG_READ_ONLY);

        if (IsRealIndex(index) && !RO)
        {
            //if(dv) AZ_TracePrintf("LUA Editor", "EDITABLE flags RO( %d ) dv( %s )\n", (int)RO, dv->m_name );
            return Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        }
        else
        {
            //if(dv) AZ_TracePrintf("LUA Editor", "LOCKED flags RO( %d ) dv( %s )\n", (int)RO, dv->m_name );
            return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        }
    }
    break;
    case 2: // TYPE LOOKUP
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
        break;
    }

    return Qt::ItemIsEnabled;
}

QModelIndex WatchesDataModel::index (int row, int column, const QModelIndex& index) const
{
    RegenerateParentsMap();

    //Given a model index for a parent item, this function allows views and delegates to access children of that item.
    //If no valid child item - corresponding to the specified row, column, and parent model index, can be found,
    //the function must return QModelIndex(), which is an invalid model index.

    if ((row < 0) || (column < 0))
    {
        return QModelIndex();
    }

    if (column > 2)
    {
        return QModelIndex();
    }

    if (!index.isValid()) // its a root element.
    {
        if (row < m_DebugValues.size())
        {
            return createIndex(row, column, (void*)&m_DebugValues[row]);
        }
        else
        {
            if ((m_OperatingMode == WATCHES_MODE_GENERAL) && (row == m_DebugValues.size()))
            {
                return createIndex(row, column, (void*)NULL);
            }
            return QModelIndex();
        }
    }

    // internal pointer is the DebugValue address
    if (IsRealIndex(index))
    {
        const AZ::ScriptContextDebug::DebugValue *dv = GetDV(index);
        if (row < dv->m_elements.size())
        {
            return createIndex(row, column, (void*)&dv->m_elements[row]);
        }
    }
    return QModelIndex();
}

QModelIndex WatchesDataModel::parent (const QModelIndex& index) const
{
    RegenerateParentsMap();

    if (!index.isValid())
    {
        return QModelIndex();
    }

    if (index.internalPointer() != NULL)
    {
        const AZ::ScriptContextDebug::DebugValue *dv = static_cast<const AZ::ScriptContextDebug::DebugValue *>(index.internalPointer());

        ParentContainer::const_iterator found = m_parents.find(dv);
        AZ_Assert(found != m_parents.end(), "Invalid node.");
        const AZ::ScriptContextDebug::DebugValue *parentDV = found->second;
        if (parentDV)
        {
            // find which index that parent is, in its own parent.
            ParentContainer::const_iterator foundParentParent = m_parents.find(parentDV);
            AZ_Assert(found != m_parents.end(), "Invalid node.");
            const AZ::ScriptContextDebug::DebugValue *parentParentDV = foundParentParent->second;
            if (!parentParentDV)
            {
                // a root element.
                for (int idx = 0; idx < m_DebugValues.size(); ++idx)
                {
                    if (&m_DebugValues[idx] == parentDV)
                    {
                        return createIndex(idx, 0, (void*)parentDV);
                    }
                }
            }
            else
            {
                // it actually has a parent.
                for (int idx = 0; idx < parentParentDV->m_elements.size(); ++idx)
                {
                    if (&parentParentDV->m_elements[idx] == parentDV)
                    {
                        return createIndex(idx, 0, (void*)parentDV);
                    }
                }
            }
        }
    }

    return QModelIndex();
}

int WatchesDataModel::rowCount (const QModelIndex& index) const
{
    RegenerateParentsMap();

    size_t count = 0;

    if (!index.isValid())
    {
        if (m_OperatingMode == WATCHES_MODE_LOCALS)
        {
            //AZ_TracePrintf("LUA Editor", "rowCount( LOCALS )\n");
            count = m_DebugValues.size();
        }
        else
        {
            //AZ_TracePrintf("LUA Editor", "rowCount( GENERAL )\n");
            count = m_DebugValues.size() + 1; // +1 offset for the <new watch> line which is only at the topmost
        }
        // invalid parent is the dummy index in QT holding the topmost rows
    }
    else if (IsRealIndex(index))
    {
        if (index.column() == 0)
        {
            //AZ_TracePrintf("LUA Editor", "rowCount( ISREAL )\n");
            const AZ::ScriptContextDebug::DebugValue *dv = static_cast<const AZ::ScriptContextDebug::DebugValue *>(index.internalPointer());
            count = dv->m_elements.size();
        }
    }

    return (int)count;
}

QVariant WatchesDataModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    (void)orientation;

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
        case 0: // name
            return QVariant(tr("Name"));
            break;
        case 1: // value
            return QVariant(tr("Value"));
            break;
        case 2: // type lookup
            return QVariant(tr("LUA Type"));
            break;
        }

        return QVariant();
    }

    return QVariant();
}

bool WatchesDataModel::setData (const QModelIndex& index, const QVariant& value, int role)
{
    //AZ_TracePrintf("LUA Editor", "SetData: val=%s\n", value.toString().toAscii());

    // false default means that we didn't allow this change for some reason
    bool result = false;

    if (role == Qt::EditRole)
    {
        switch (index.column())
        {
        case 0:
            if (!index.parent().isValid())
            {
                // column 0 := NAME.  we changed this watch to look for something completely different
                // THIS IS ONLY POSSIBLE IF WE ARE A TOPMOST PARENT, names of children should not be changed
                // true means this new data was accepted

                if (index.row() < m_DebugValues.size())
                {
                    // this is a real row
                    AZ::ScriptContextDebug::DebugValue *dv = static_cast<AZ::ScriptContextDebug::DebugValue *>(index.internalPointer());
                    if (dv)
                    {
                        AZStd::string str = value.toString().toUtf8().data();
                        dv->m_name = str.c_str();
                        emit dataChanged(index, index);
                        result = true;
                    }
                }
                else if (m_OperatingMode == WATCHES_MODE_GENERAL)
                {
                    // this is the phony <new watch> row
                    // display role string for this is synthesized, not stored
                    AZStd::string str = value.toString().toUtf8().data();
                    if (str.length())
                    {
                        beginResetModel();
                        AddWatch(str);
                        result = true;
                        endResetModel();
                    }
                }
            }
            else
            {
                AZ::ScriptContextDebug::DebugValue *dv = static_cast<AZ::ScriptContextDebug::DebugValue *>(index.internalPointer());
                if (dv)
                {
                    AZStd::string str = value.toString().toUtf8().data();
                    dv->m_name = str.c_str();
                    emit dataChanged(index, index);
                    result = true;
                }
            }

            break;

        case 1:
            // column 1 := VALUE.  we want to message the outside world of the change
            // true means this new data was accepted
            result = true;

            AZ::ScriptContextDebug::DebugValue *dv = static_cast<AZ::ScriptContextDebug::DebugValue *>(index.internalPointer());

            if (dv)
            {
                AZStd::string str = value.toString().toUtf8().data();
                dv->m_value = str.c_str();

                const QModelIndex qmi = GetTopmostIndex(index);
                const AZ::ScriptContextDebug::DebugValue *cdv = GetDV(qmi);
                LUAEditor::LUAEditorDebuggerMessagesRequestBus::Broadcast(&LUAEditor::LUAEditorDebuggerMessages::SetValue, *cdv);

                emit dataChanged(index, index);
            }
            break;
        }
    }

    return result;
}

const char* WatchesDataModel::SafetyType(char c) const
{
    if (static_cast<int>(c) <= LUA_TNONE || c > LUA_NUMTAGS)
    {
        return "<invalid>";
    }
    static_assert(WatchesPanel::typeStringLUT.size() == LUA_NUMTAGS, "number of lua tags does not match the number of typeStringLUT");
    return WatchesPanel::typeStringLUT[aznumeric_cast<int>(c)];
}

const bool WatchesDataModel::IsRealIndex(const QModelIndex& index) const
{
    if ((!index.isValid() && index.row() >= m_DebugValues.size()) || index.internalPointer() == NULL)
    {
        return false;
    }

    return true;
}

bool WatchesDataModel::IsTypeChangeAllowed(const QModelIndex& index) const
{
    const AZ::ScriptContextDebug::DebugValue *dv = GetDV(index);
    if (dv)
    {
        return 0 != (dv->m_flags & AZ::ScriptContextDebug::DebugValue::FLAG_ALLOW_TYPE_CHANGE);
    }

    return false;
}

void WatchesDataModel::SetType(const QModelIndex& index, char newType)
{
    AZ::ScriptContextDebug::DebugValue *dv = static_cast<AZ::ScriptContextDebug::DebugValue *>(index.internalPointer());
    if (dv)
    {
        dv->m_type = newType;

        const QModelIndex qmi = GetTopmostIndex(index);
        const AZ::ScriptContextDebug::DebugValue *cdv = GetDV(qmi);
        LUAEditor::LUAEditorDebuggerMessagesRequestBus::Broadcast(&LUAEditor::LUAEditorDebuggerMessages::SetValue, *cdv);

        emit dataChanged(index, index);
    }
}
