/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qaction.h>
#include <qevent.h>
#include <qheaderview.h>
#include <qitemselectionmodel.h>
#include <qscrollbar.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>

#include <Editor/View/Widgets/DataTypePalette/DataTypePaletteModel.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/QtMetaTypes.h>

#include <ScriptCanvas/Data/DataRegistry.h>

namespace ScriptCanvasEditor
{
    /////////////////////////
    // DataTypePaletteModel
    /////////////////////////
    DataTypePaletteModel::DataTypePaletteModel(QObject* parent)
        : QAbstractTableModel(parent)
        , m_pinIcon(":/ScriptCanvasEditorResources/Resources/pin.png")
    {
    }

    int DataTypePaletteModel::columnCount(const QModelIndex&) const
    {
        return ColumnIndex::Count;
    }

    int DataTypePaletteModel::rowCount(const QModelIndex&) const
    {
        return aznumeric_cast<int>(m_variableTypes.size());
    }

    QVariant DataTypePaletteModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
    {
        AZ::Uuid typeId = FindTypeIdForIndex(index);        

        switch (role)
        {
        case Qt::DisplayRole:
        {
            if (index.column() == ColumnIndex::Type)
            {
                AZStd::string typeString = FindTypeNameForTypeId(typeId);
                return QString(typeString.c_str());
            }
        }
        break;

        case Qt::DecorationRole:
        {
            if (index.column() == ColumnIndex::Type)
            {
                const QPixmap* icon = nullptr;

                if (AZ::Utils::IsContainerType(typeId) && !AZ::Utils::IsGenericContainerType(typeId))
                {
                    AZStd::vector<AZ::Uuid> dataTypes = AZ::Utils::GetContainedTypes(typeId);
                    GraphCanvas::StyleManagerRequestBus::EventResult(icon, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetMultiDataTypeIcon, dataTypes);
                }
                else
                {
                    GraphCanvas::StyleManagerRequestBus::EventResult(icon, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetDataTypeIcon, typeId);
                }

                if (icon)
                {
                    return *icon;
                }
            }
            else if (index.column() == ColumnIndex::Pinned)
            {
                AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);

                bool showPin = settings->m_pinnedDataTypes.find(typeId) != settings->m_pinnedDataTypes.end();

                if (m_pinningChanges.find(typeId) != m_pinningChanges.end())
                {
                    showPin = !showPin;
                }

                if (showPin)
                {
                    return m_pinIcon;
                }
            }
        }
        break;

        case Qt::EditRole:
        {
            if (index.column() == ColumnIndex::Type)
            {
                AZStd::string typeString = TranslationHelper::GetSafeTypeName(ScriptCanvas::Data::FromAZType(typeId));
                return QString(typeString.c_str());
            }
        }
        break;

        default:
            break;
        }

        return QVariant();
    }

    Qt::ItemFlags DataTypePaletteModel::flags(const QModelIndex&) const
    {
        return Qt::ItemFlags(
            Qt::ItemIsEnabled |
            Qt::ItemIsSelectable);
    }

    QItemSelectionRange DataTypePaletteModel::GetSelectionRangeForRow(int row)
    {
        return QItemSelectionRange(createIndex(row, 0, nullptr), createIndex(row, columnCount(), nullptr));
    }
    
    void DataTypePaletteModel::ClearTypes()
    {
        layoutAboutToBeChanged();
        m_variableTypes.clear();
        m_typeNameMapping.clear();
        layoutChanged();
    }

    void DataTypePaletteModel::PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& dataTypes)
    {
        layoutAboutToBeChanged();

        m_variableTypes.reserve(dataTypes.size() + m_variableTypes.size());
        
        for (const AZ::Uuid& typeId : dataTypes)
        {
            AddDataTypeImpl(typeId);
        }

        layoutChanged();
    }

    void DataTypePaletteModel::AddDataType(const AZ::TypeId& typeId)
    {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        AddDataTypeImpl(typeId);
        endInsertRows();        
    }

    void DataTypePaletteModel::RemoveDataType(const AZ::TypeId& typeId)
    {
        QModelIndex index = FindIndexForTypeId(typeId);

        if (index.isValid())
        {
            beginRemoveRows(QModelIndex(), index.row(), index.row());

            m_variableTypes.erase(m_variableTypes.begin() + index.row());
            
            auto mapIter = m_typeNameMapping.begin();

            while (mapIter != m_typeNameMapping.end())
            {
                if (mapIter->second == typeId)
                {
                    m_typeNameMapping.erase(mapIter);
                    break;
                }

                ++mapIter;
            }

            endRemoveRows();
        }
    }

    bool DataTypePaletteModel::HasType(const AZ::TypeId& dataType) const
    {
        auto typeIter = AZStd::find(m_variableTypes.begin(), m_variableTypes.end(), dataType);
        return typeIter != m_variableTypes.end();
    }
    
    AZ::TypeId DataTypePaletteModel::FindTypeIdForIndex(const QModelIndex& index) const
    {   
        AZ::TypeId retVal = AZ::TypeId::CreateNull();

        if (index.row() >= 0 && index.row() < m_variableTypes.size())
        {
            retVal = m_variableTypes[index.row()];
        }

        return retVal;
    }

    QModelIndex DataTypePaletteModel::FindIndexForTypeId(const AZ::TypeId& typeId) const
    {
        int row = 0;

        for (; row < m_variableTypes.size(); ++row)
        {
            if (m_variableTypes[row] == typeId)
            {
                return index(row, 0);
            }
        }

        return QModelIndex();
    }
    
    AZ::TypeId DataTypePaletteModel::FindTypeIdForTypeName(const AZStd::string& typeName) const
    {
        AZ::TypeId retVal = azrtti_typeid<void>();

        AZStd::string lowerName = typeName;
        AZStd::to_lower(lowerName.begin(), lowerName.end());

        auto mapIter = m_typeNameMapping.find(lowerName);

        if (mapIter != m_typeNameMapping.end())
        {
            retVal = mapIter->second;
        }

        return retVal;        
    }

    AZStd::string DataTypePaletteModel::FindTypeNameForTypeId(const AZ::TypeId& typeId) const
    {
        GraphCanvas::TranslationKey key;
        key << "BehaviorType" << typeId.ToString<AZStd::string>() << "details";

        GraphCanvas::TranslationRequests::Details details;

        details.m_name = TranslationHelper::GetSafeTypeName(ScriptCanvas::Data::FromAZType(typeId));

        GraphCanvas::TranslationRequestBus::BroadcastResult(details, &GraphCanvas::TranslationRequests::GetDetails, key, details);

        return details.m_name;
    }

    void DataTypePaletteModel::TogglePendingPinChange(const AZ::Uuid& azVarType)
    {
        auto pinningIter = m_pinningChanges.find(azVarType);

        if (pinningIter != m_pinningChanges.end())
        {
            m_pinningChanges.erase(pinningIter);
        }
        else
        {
            m_pinningChanges.insert(azVarType);
        }
    }

    const AZStd::unordered_set< AZ::Uuid >& DataTypePaletteModel::GetPendingPinChanges() const
    {
        return m_pinningChanges;
    }

    void DataTypePaletteModel::SubmitPendingPinChanges()
    {
        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);

        if (settings)
        {
            for (const AZ::Uuid& azVarType : m_pinningChanges)
            {
                size_t result = settings->m_pinnedDataTypes.erase(azVarType);

                if (result == 0)
                {
                    settings->m_pinnedDataTypes.insert(azVarType);
                }
                // We don't want to let individual container types exist in our data unless they are pinned.
                else if (AZ::Utils::IsContainerType(azVarType) && !AZ::Utils::IsGenericContainerType(azVarType))
                {
                    RemoveDataType(azVarType);
                }
            }

            m_pinningChanges.clear();
        }
    }

    const AZStd::vector<AZ::TypeId>& DataTypePaletteModel::GetVariableTypes() const
    {
        return m_variableTypes;
    }

    void DataTypePaletteModel::AddDataTypeImpl(const AZ::TypeId& typeId)
    {

        AZStd::string lowerName = FindTypeNameForTypeId(typeId);
        AZStd::to_lower(lowerName.begin(), lowerName.end());

        if (ScriptCanvas::Data::IsNumber(typeId))
        {
            const auto numberTypeId = azrtti_typeid<ScriptCanvas::Data::NumberType>();
            m_variableTypes.emplace_back(numberTypeId);

            lowerName = ScriptCanvas::Data::GetName(ScriptCanvas::Data::Type::Number());
            AZStd::to_lower(lowerName.begin(), lowerName.end());

            m_typeNameMapping[lowerName] = numberTypeId;
        }
        else
        {
            m_variableTypes.emplace_back(typeId);
            m_typeNameMapping[lowerName] = typeId;
        }
    }

    ////////////////////////////////////////
    // DataTypePaletteSortFilterProxyModel
    ////////////////////////////////////////

    DataTypePaletteSortFilterProxyModel::DataTypePaletteSortFilterProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }

    bool DataTypePaletteSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        if (m_filter.isEmpty())
        {
            return true;
        }
        
        DataTypePaletteModel* model = qobject_cast<DataTypePaletteModel*>(sourceModel());
        if (!model)
        {
            return false;
        }
        
        QModelIndex index = model->index(sourceRow, DataTypePaletteModel::ColumnIndex::Type, sourceParent);
        QString test = model->data(index, Qt::DisplayRole).toString();
        return (test.lastIndexOf(m_testRegex) >= 0);
    }

    bool DataTypePaletteSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        DataTypePaletteModel* model = qobject_cast<DataTypePaletteModel*>(sourceModel());
        if (!model)
        {
            return false;
        }

        bool pinnedLeft = false;
        AZ::TypeId leftDataType = model->FindTypeIdForIndex(left);

        bool pinnedRight = false;
        AZ::TypeId rightDataType = model->FindTypeIdForIndex(right);

        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);

        if (settings)
        {
            pinnedLeft = settings->m_pinnedDataTypes.find(leftDataType) != settings->m_pinnedDataTypes.end();
            pinnedRight = settings->m_pinnedDataTypes.find(rightDataType) != settings->m_pinnedDataTypes.end();
        }

        if (pinnedRight == pinnedLeft)
        {
            return QSortFilterProxyModel::lessThan(left, right);
        }
        else if (pinnedRight)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    
    void DataTypePaletteSortFilterProxyModel::SetFilter(const QString& filter)
    {
        m_filter = filter;
        m_testRegex = QRegExp(m_filter, Qt::CaseInsensitive);
        invalidateFilter();
    }   
}

#include <Editor/View/Widgets/DataTypePalette/moc_DataTypePaletteModel.cpp>
