/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <QAbstractListModel>
#endif

namespace AssetProcessor
{
    class EnabledRelocationTypesModel : public QAbstractListModel
    {
        Q_OBJECT;

    public:
        EnabledRelocationTypesModel(QObject* parent = nullptr)
            : QAbstractListModel(parent)
        {
        }

        int rowCount(const QModelIndex& parent) const override;

        QVariant data(const QModelIndex& index, int role) const override;

        void Reset();

    protected:
        // Cache the enabled type list locally, for performance and to keep the sorting stable.
        // A vector is used instead of a set to make look up in data faster.
        AZStd::vector<AZStd::string> m_enabledTypes;
    };

} // namespace AssetProcessor
