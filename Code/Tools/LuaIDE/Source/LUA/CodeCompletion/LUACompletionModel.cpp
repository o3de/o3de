/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LUACompletionModel.hxx"
#include <Source/LUA/LUAEditorStyleMessages.h>

#include <Source/LUA/CodeCompletion/moc_LUACompletionModel.cpp>

namespace LUAEditor
{
    void CompletionModel::LUAName::Reset()
    {
        m_children.clear();
        m_fastLookup.clear();
    }

    void CompletionModel::LUAName::AddName(const QStringList& nameList, int index)
    {
        if (index >= nameList.size())
        {
            return;
        }

        auto iter = m_children.find(nameList[index]);
        if (iter == m_children.end())
        {
            m_children.insert(nameList[index], LUAName());
            iter = m_children.find(nameList[index]);
            iter->m_name = nullptr;
            iter->m_parent = nullptr;
        }
        iter->AddName(nameList, index + 1);
    }

    void CompletionModel::LUAName::GenerateFastLookup()
    {
        m_fastLookup.clear();
        for (auto iter = m_children.begin(); iter != m_children.end(); ++iter)
        {
            iter->m_name = &iter.key();
            iter->m_parent = this;
            m_fastLookup.push_back(&iter.value());
            iter->GenerateFastLookup();
        }
    }

    CompletionModel::CompletionModel(QObject* pParent)
        : QAbstractItemModel(pParent)
    {
        HighlightedWordNotifications::Handler::BusConnect();
        UpdateKeywords();
    }

    CompletionModel::~CompletionModel()
    {
        HighlightedWordNotifications::Handler::BusDisconnect();
    }

    void CompletionModel::UpdateKeywords()
    {
        const HighlightedWords::LUAKeywordsType* keywords = nullptr;
        HighlightedWords::Bus::BroadcastResult(keywords, &HighlightedWords::Bus::Events::GetLUAKeywords);
        const HighlightedWords::LUAKeywordsType* libraryFuncs = nullptr;
        HighlightedWords::Bus::BroadcastResult(libraryFuncs, &HighlightedWords::Bus::Events::GetLUALibraryFunctions);

        m_keywords.clear();
        m_builtIns.Reset();

        for (const auto& keyword : *keywords)
        {
            m_keywords.push_back(keyword.c_str());
        }
        for (const auto& keyword : *libraryFuncs)
        {
            m_keywords.push_back(keyword.c_str());
        }

        for (const auto& keyword : m_keywords)
        {
            m_builtIns.AddName(keyword.split(luaSplitRegex));
        }
        OnScopeNamesUpdated(QStringList());
    }

    void CompletionModel::OnScopeNamesUpdated(const QStringList& scopeNames)
    {
        beginResetModel();
        m_root = m_builtIns;
        for (const auto& scopeName : scopeNames)
        {
            m_root.AddName(scopeName.split(luaSplitRegex));
        }
        m_root.GenerateFastLookup();
        endResetModel();
    }

    QVariant CompletionModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        if (!(role == Qt::DisplayRole || role == Qt::EditRole))
        {
            return QVariant();
        }

        LUAName* name = static_cast<LUAName*>(index.internalPointer());
        if (!name)
        {
            return QVariant();
        }

        return QVariant(*name->m_name);
    }

    Qt::ItemFlags CompletionModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return Qt::ItemFlags();
        }

        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    QVariant CompletionModel::headerData([[maybe_unused]] int section, [[maybe_unused]] Qt::Orientation orientation, [[maybe_unused]] int role) const
    {
        return QVariant();
    }

    QModelIndex CompletionModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (column != 0)
        {
            return QModelIndex();
        }

        const LUAName* name = nullptr;

        if (!parent.isValid())
        {
            name = &m_root;
        }
        else
        {
            name = static_cast<LUAName*>(parent.internalPointer());
        }

        if (!name)
        {
            return QModelIndex();
        }

        if (row < 0 || row >= name->m_children.size())
        {
            return QModelIndex();
        }

        return createIndex(row, 0, name->m_fastLookup[row]);
    }

    QModelIndex CompletionModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return QModelIndex();
        }

        LUAName* name = static_cast<LUAName*>(index.internalPointer());
        if (!name)
        {
            return QModelIndex();
        }

        if (name->m_parent == &m_root || !name->m_parent)
        {
            return QModelIndex();
        }

        AZ_Assert(name->m_parent->m_parent, "invalid code completion tree");
        int row = 0;
        for (auto childPtr : name->m_parent->m_parent->m_fastLookup)
        {
            if (childPtr == name->m_parent)
            {
                break;
            }
            ++row;
        }
        AZ_Assert(row >= 0 && row < name->m_parent->m_parent->m_children.size(), "invalid code completion tree");

        return createIndex(row, 0, name->m_parent);
    }

    int CompletionModel::rowCount(const QModelIndex& parent) const
    {
        if (!parent.isValid())
        {
            return m_root.m_children.size();
        }

        LUAName* name = static_cast<LUAName*>(parent.internalPointer());
        if (!name)
        {
            return m_root.m_children.size();
        }

        return name->m_children.size();
    }

    int CompletionModel::columnCount(const QModelIndex&) const
    {
        return 1;
    }

    void CompletionModel::LUALibraryFunctionsUpdated()
    {
        UpdateKeywords();
    }
}
