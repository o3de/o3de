/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QTableView>

#include <AzCore/Memory/SystemAllocator.h>

#include <ScriptCanvas/Data/Data.h>
#endif

namespace ScriptCanvasEditor
{
    class DataTypePaletteModel
        : public QAbstractTableModel
    {
        Q_OBJECT
    public:

        enum ColumnIndex
        {
            Pinned,
            Type,
            Count
        };

        AZ_CLASS_ALLOCATOR(DataTypePaletteModel, AZ::SystemAllocator);

        DataTypePaletteModel(QObject* parent = nullptr);

        // QAbstractTableModel
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        ////

        QItemSelectionRange GetSelectionRangeForRow(int row);

        void ClearTypes();        
        void PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes);

        void AddDataType(const AZ::TypeId& dataType);
        void RemoveDataType(const AZ::TypeId& dataType);

        bool HasType(const AZ::TypeId& dataType) const;

        AZ::TypeId FindTypeIdForIndex(const QModelIndex& index) const;
        AZ::TypeId FindTypeIdForTypeName(const AZStd::string& typeName) const;

        QModelIndex FindIndexForTypeId(const AZ::TypeId& typeId) const;
        AZStd::string FindTypeNameForTypeId(const AZ::TypeId& typeId) const;

        void TogglePendingPinChange(const AZ::Uuid& azVarType);
        const AZStd::unordered_set< AZ::Uuid >& GetPendingPinChanges() const;
        void SubmitPendingPinChanges();

        const AZStd::vector<AZ::TypeId>& GetVariableTypes() const;

    private:

        void AddDataTypeImpl(const AZ::TypeId& dataType);

        QIcon   m_pinIcon;

        AZStd::unordered_set< AZ::Uuid >     m_pinningChanges;

        AZStd::vector<AZ::TypeId> m_variableTypes;
        AZStd::unordered_map<AZStd::string, AZ::TypeId> m_typeNameMapping;        
    };
    
    class DataTypePaletteSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(DataTypePaletteSortFilterProxyModel, AZ::SystemAllocator);

        DataTypePaletteSortFilterProxyModel(QObject* parent = nullptr);
        
        // QSortFilterProxyModel
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
        ////
        
        void SetFilter(const QString& filter);

    private:
        QString m_filter;
        QRegExp m_testRegex;
    };
}
