/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // class 'QScopedPointer<QStandardItemPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QStandardItem'
#include <QStandardItemModel>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    class ComponentPaletteModel
        : public QStandardItemModel
    {
        Q_OBJECT

    public:
        ComponentPaletteModel(QObject* parent = 0);
        ~ComponentPaletteModel() override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    };
}
