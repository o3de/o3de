/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(CompletionModel, AZ::SystemAllocator);

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
