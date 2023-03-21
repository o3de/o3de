/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QStandardItemModel>
#include <QTableWidget>
#endif

#include <Data/ShaderVariantStatisticData.h>

namespace ShaderManagementConsole
{
    class ShaderManagementConsoleStatisticView
        : public QTableWidget
    {
    public:
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleStatisticView, AZ::SystemAllocator);
        ShaderManagementConsoleStatisticView(ShaderVariantStatisticData statisticData, QWidget* parent);
        ~ShaderManagementConsoleStatisticView();

        void BuildTable();
        void ShowContextMenu(const QPoint& pos);
        void ShowMaterialList(AZ::Name optionName);

        ShaderVariantStatisticData m_statisticData;
    };
}
