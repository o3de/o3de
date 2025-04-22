/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QFont>
#include <QSortFilterProxyModel>
AZ_POP_DISABLE_WARNING

namespace GraphCanvas
{
    template<typename ValueType>
    class GraphCanvasListComboBoxModel
        : public QAbstractListModel
        , public ComboBoxItemModelInterface
    {
    private:
        typedef size_t StringHash;

        // This makes use of implicit initializer lists. Any changes to this must be reflected in all of the constructor places.
        struct DisplayPair
        {
            QString m_displayString;
            StringHash    m_stringHash;
        };

    public:
    
        AZ_CLASS_ALLOCATOR(GraphCanvasListComboBoxModel, AZ::SystemAllocator);

        enum ColumnIndex
        {
            Name = 0
        };        

        void AddElement(ValueType value, const QString& displayName)
        {
            auto displayIter = m_valueToDisplayMap.find(value);
            
            if (displayIter == m_valueToDisplayMap.end())
            {
                StringHash hash = GetHash(displayName);
                
                // Implicit structure initializer list. Order must match order of declared elements.
                m_valueToDisplayMap[value] = { displayName, hash};
                m_nameHashToValueMap[hash] = value;

                beginInsertRows(QModelIndex(), static_cast<int>(m_displayElements.size()), static_cast<int>(m_displayElements.size()));
                m_displayElements.emplace_back(displayName);
                endInsertRows();
            }
        }

        void RemoveElement(ValueType value)
        {
            auto valueIter = m_valueToDisplayMap.find(value);
            
            if (valueIter != m_valueToDisplayMap.end())
            {
                const auto& displayPair = valueIter->second;
                
                RemoveElement(FindIndexForName(displayPair.m_displayString));
            }
        }
        
        void RemoveElement(const QModelIndex& index)
        {
            if (index.isValid())
            {
                QString name = GetNameForIndex(index);
                
                StringHash hash = m_stringHasher(name.toUtf8().data());
                
                auto hashLookupIter = m_nameHashToValueMap.find(hash);
                
                if (hashLookupIter != m_nameHashToValueMap.end())
                {
                    m_valueToDisplayMap.erase(hashLookupIter->second);
                    m_nameHashToValueMap.erase(hashLookupIter);
                }
                
                beginRemoveRows(QModelIndex(), index.row(), index.row());
                m_displayElements.erase(m_displayElements.begin() + index.row());
                endRemoveRows();
            }
        }

        void ClearElements()
        {
            beginResetModel();
            m_valueToDisplayMap.clear();
            m_nameHashToValueMap.clear();
            m_displayElements.clear();
            endResetModel();
        }

        // QAbstractListModel
        int rowCount(const QModelIndex& /*parent*/) const override
        {
            return static_cast<int>(m_displayElements.size());
        }

        QVariant data(const QModelIndex& index, int role) const override
        {
            switch (role)
            {
            case Qt::DisplayRole:
            {
                return GetNameForIndex(index);
            }
            case Qt::FontRole:
            {
                QFont sizedFont;
                int pointSize = sizedFont.pointSize();
                if (pointSize >= 0)
                {
                    sizedFont.setPointSizeF(pointSize * m_fontScale);
                }
                return sizedFont;
            }
            default:
                break;
            }

            return GetRoleData(index, role);
        }
        ////

        const QString& GetNameForValue(const ValueType& valueType) const
        {
            auto valueIter = m_valueToDisplayMap.find(valueType);

            if (valueIter != m_valueToDisplayMap.end())
            {
                return valueIter->second.m_displayString;
            }

            static QString k_emptyString;
            return k_emptyString;
        }

        QModelIndex GetIndexForValue(const ValueType& valueType) const
        {
            return FindIndexForName(GetNameForValue(valueType));
        }

        ValueType GetValueForIndex(QModelIndex index) const
        {
            return GetValueForName(GetNameForIndex(index));
        }

        ValueType GetValueForName(const QString& name) const
        {
            StringHash hash = GetHash(name);

            auto valueIter = m_nameHashToValueMap.find(hash);

            if (valueIter != m_nameHashToValueMap.end())
            {
                return valueIter->second;
            }

            return ValueType{};
        }

        // ComboBoxModelInterface
        void SetFontScale(qreal fontScale) override
        {
            m_fontScale = fontScale;

            layoutChanged();
        }

        QString GetNameForIndex(const QModelIndex& index) const override
        {
            if (!index.isValid()
                || index.row() < 0 
                || index.row() >= m_displayElements.size())
            {
                return "";
            }

            return m_displayElements[index.row()];
        }

        QModelIndex FindIndexForName(const QString& name) const override
        {
            for (int i = 0; i < m_displayElements.size(); ++i)
            {
                if (m_displayElements[i] == name)
                {
                    return createIndex(i, ColumnIndex::Name, nullptr);
                }
            }

            return QModelIndex();
        }

        QModelIndex GetDefaultIndex() const override
        {
            if (m_displayElements.empty())
            {
                return QModelIndex();
            }

            return createIndex(0, ColumnIndex::Name, nullptr);
        }

        QAbstractItemModel* GetDropDownItemModel() override
        {
            return this;
        }

        int GetSortColumn() const override
        {
            return ColumnIndex::Name;
        }

        int GetFilterColumn() const override
        {
            return ColumnIndex::Name;
        }

        QModelIndex GetNextIndex(const QModelIndex& modelIndex) const override
        {
            if (!modelIndex.isValid())
            {
                return QModelIndex();
            }

            int nextRow = modelIndex.row() + 1;        

            if (nextRow == rowCount(modelIndex.parent()))
            {
                nextRow = 0;
            }

            if (nextRow == rowCount(modelIndex.parent()))
            {
                return QModelIndex();
            }

            return index(nextRow, ColumnIndex::Name);
        }

        QModelIndex GetPreviousIndex(const QModelIndex& modelIndex) const override
        {
            if (!modelIndex.isValid())
            {
                return QModelIndex();
            }

            int nextRow = modelIndex.row() - 1;

            if (nextRow < 0)
            {
                nextRow = rowCount(modelIndex.parent()) - 1;
            }

            if (nextRow < 0)
            {
                return QModelIndex();
            }

            return index(nextRow, ColumnIndex::Name);
        }

        QAbstractListModel* GetCompleterItemModel() override
        {
            return this;
        }

        int GetCompleterColumn() const override
        {
            return ColumnIndex::Name;
        }        
        ////
        
    protected:

        QVariant GetRoleData(const QModelIndex& index, int role) const
        {
            AZ_UNUSED(index);
            AZ_UNUSED(role);

            return QVariant();
        }

    private:

        StringHash GetHash(const QString& qString) const
        {
            AZStd::string stdString = qString.toUtf8().data();
            return m_stringHasher(stdString);
        }
        
        AZStd::hash<AZStd::string> m_stringHasher;        
        
        // Maps Enum Value -> String
        AZStd::unordered_map< ValueType, DisplayPair > m_valueToDisplayMap;

        // Maps Name -> Enum Value
        AZStd::unordered_map<StringHash, ValueType> m_nameHashToValueMap;
        
        AZStd::vector< QString > m_displayElements;

        qreal m_fontScale = 1.0;
    };

    class GraphCanvasSortFilterComboBoxProxyModel
        : public QSortFilterProxyModel
        , public ComboBoxItemModelInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphCanvasSortFilterComboBoxProxyModel, AZ::SystemAllocator);

        void SetModelInterface(ComboBoxItemModelInterface* modelInterface)
        {
            if (m_modelInterface == nullptr)
            {
                m_modelInterface = modelInterface;

                beginResetModel();
                setSourceModel(modelInterface->GetDropDownItemModel());
                endResetModel();
            }
        }

        // ComboBoxModelInterface
        void SetFontScale(qreal fontScale) override
        {
            m_modelInterface->SetFontScale(fontScale);
        }

        QString GetNameForIndex(const QModelIndex& index) const override
        {
            QModelIndex sourceIndex = RemapToSourceIndex(index);
            return m_modelInterface->GetNameForIndex(sourceIndex);
        }

        QModelIndex FindIndexForName(const QString& name) const override
        {
            QModelIndex sourceIndex = m_modelInterface->FindIndexForName(name);            
            return RemapFromSourceIndex(sourceIndex);
        }

        QModelIndex GetDefaultIndex() const override
        {
            if (rowCount() > 0)
            {
                return index(0, 0);
            }

            return QModelIndex();
        }

        QAbstractItemModel* GetDropDownItemModel() override
        {
            return this;
        }

        int GetSortColumn() const override
        {
            return m_modelInterface->GetSortColumn();
        }

        int GetFilterColumn() const override
        {
            return m_modelInterface->GetFilterColumn();
        }

        QModelIndex GetNextIndex(const QModelIndex& modelIndex) const override
        {
            if (!modelIndex.isValid())
            {
                return QModelIndex();
            }

            int nextRow = modelIndex.row() + 1;

            if (nextRow == rowCount(modelIndex.parent()))
            {
                nextRow = 0;
            }

            if (nextRow == rowCount(modelIndex.parent()))
            {
                return QModelIndex();
            }

            return index(nextRow, GetSortColumn());
        }

        QModelIndex GetPreviousIndex(const QModelIndex& modelIndex) const override
        {
            if (!modelIndex.isValid())
            {
                return QModelIndex();
            }

            int nextRow = modelIndex.row() - 1;

            if (nextRow < 0)
            {
                nextRow = rowCount(modelIndex.parent()) - 1;
            }

            if (nextRow < 0)
            {
                return QModelIndex();
            }

            return index(nextRow, GetSortColumn());
        }

        void OnDropDownAboutToShow() override
        {
            beginResetModel();
            setSourceModel(m_modelInterface->GetDropDownItemModel());
            endResetModel();

            invalidate();
        }

        void OnDropDownHidden() override
        {
            beginResetModel();
            setSourceModel(nullptr);
            endResetModel();

            invalidate();
        }

        QAbstractListModel* GetCompleterItemModel() override
        {
            return m_modelInterface->GetCompleterItemModel();
        }

        int GetCompleterColumn() const override
        {
            return m_modelInterface->GetCompleterColumn();
        }
        ////

    protected:

        QModelIndex RemapToSourceIndex(const QModelIndex& proxyIndex) const
        {
            if (sourceModel())
            {
                return mapToSource(proxyIndex);
            }
            else
            {
                return proxyIndex;
            }
        }

        QModelIndex RemapFromSourceIndex(const QModelIndex& modelIndex) const
        {
            if (sourceModel())
            {
                return mapFromSource(modelIndex);
            }
            else
            {
                return modelIndex;
            }
        }

    private:

        ComboBoxItemModelInterface* m_modelInterface = nullptr;
    };
}
