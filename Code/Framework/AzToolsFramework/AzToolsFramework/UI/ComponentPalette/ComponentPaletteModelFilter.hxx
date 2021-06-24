/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QSortFilterProxyModel>
#endif

namespace AzToolsFramework
{
    class ComponentPaletteModelFilter : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        ComponentPaletteModelFilter(QObject* parent = nullptr);

        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    protected:
        QRegExp m_filterRegExp;
    };
}
