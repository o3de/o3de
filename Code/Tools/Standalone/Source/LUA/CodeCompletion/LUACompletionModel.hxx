/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef LUAEDITOR_LUACOMPLETIONMODEL_H
#define LUAEDITOR_LUACOMPLETIONMODEL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <QtCore/QStringListModel>
#include <Source/LUA/LUAEditorStyleMessages.h>
#include <QRegularExpression>
#endif

#pragma once

namespace LUAEditor
{
    class CompletionModel 
        : public QAbstractItemModel
        , private HighlightedWordNotifications::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(CompletionModel, AZ::SystemAllocator, 0);

        CompletionModel(QObject *pParent = nullptr);
        virtual ~CompletionModel();

        QVariant data(const QModelIndex& index, int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        QModelIndex parent(const QModelIndex& index) const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    public slots:
        void OnScopeNamesUpdated(const QStringList& scopeNames);

    private:
        const char* c_luaSplit{R"([.:])"};

        void LUALibraryFunctionsUpdated() override;
        void UpdateKeywords();

        struct LUAName
        {
            const QString* m_name{ nullptr };
            LUAName* m_parent{ nullptr };
            QMap<QString, LUAName> m_children;
            QList<LUAName*> m_fastLookup;
            void AddName(const QStringList& nameList, int index = 0);
            void GenerateFastLookup();
            void Reset();
        };

        QRegularExpression luaSplitRegex{R"([.:])"};

        LUAName m_root;
        LUAName m_builtIns;
        QStringList m_keywords;
    };
}

#endif