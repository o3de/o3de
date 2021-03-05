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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITORCOMMON_DEEPFILTERPROXYMODEL_H
#define CRYINCLUDE_EDITORCOMMON_DEEPFILTERPROXYMODEL_H
#pragma once

#include <QSortFilterProxyModel>
#include <QModelIndex>
#include <QStringList>
#include "EditorCommonAPI.h"

class EDITOR_COMMON_API DeepFilterProxyModel
    : public QSortFilterProxyModel
{
public:
    DeepFilterProxyModel(QObject* parent);

    void setFilterString(const QString& filter);
    void invalidate();

    QVariant data(const QModelIndex& index, int role) const override;

    void setFilterWildcard(const QString& pattern);

    bool matchFilter(int source_row, const QModelIndex& source_parent) const;
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    bool hasAcceptedChildrenCached(int source_row, const QModelIndex& source_parent) const;
    bool hasAcceptedChildren(int source_row, const QModelIndex& source_parent) const;

    QModelIndex findFirstMatchingIndex(const QModelIndex& root);

private:
    QString m_filter;
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QStringList m_filterParts;
    typedef std::map<std::pair<QModelIndex, int>, bool> TAcceptCache;
    mutable TAcceptCache m_acceptCache;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITORCOMMON_DEEPFILTERPROXYMODEL_H
