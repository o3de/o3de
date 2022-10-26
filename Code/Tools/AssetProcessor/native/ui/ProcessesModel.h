/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QTimer>
#include <QAbstractListModel>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <utilities/Builder.h>
#endif

namespace AssetProcessor
{
    class ProcessesModel : public QAbstractListModel
    {
        Q_OBJECT

    public:
        ProcessesModel();

    public Q_SLOTS:
        void OnBuilderAdded(AZ::Uuid builderId, AZStd::shared_ptr<const AssetProcessor::Builder> builder);
        void OnBuilderRemoved(AZ::Uuid builderId);
        void OnStatusUpdate(AZ::Uuid builderId, bool process, bool connection, bool busy, QString file);

    Q_SIGNALS:
        void UtilizationUpdate(int builderCount, int busyCount);

    public:
        int rowCount(const QModelIndex& parent) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        void ShowFilename(bool show);

    private:
        void DoUpdate(AZ::Uuid builderId, bool process, bool connection, bool busy, QString file);

        using BuilderInfo = AZStd::tuple<AZ::Uuid, bool, bool, bool, QString>;

        AZStd::vector<BuilderInfo> m_builders;
        QTimer m_debounceTimer;
        AZStd::unordered_map<AZ::Uuid, BuilderInfo> m_pendingUpdates;
        bool m_showFilename = false;
    };
} // namespace AssetProcessor
