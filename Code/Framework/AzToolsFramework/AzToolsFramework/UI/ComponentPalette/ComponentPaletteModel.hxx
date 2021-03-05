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
