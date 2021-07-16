/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
