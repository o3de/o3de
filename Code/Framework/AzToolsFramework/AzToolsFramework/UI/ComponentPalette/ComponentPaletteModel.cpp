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

#include "AzToolsFramework_precompiled.h"
#include "ComponentPaletteModel.hxx"

namespace AzToolsFramework
{
    ComponentPaletteModel::ComponentPaletteModel(QObject* parent)
        : QStandardItemModel(parent)
    {
    }

    ComponentPaletteModel::~ComponentPaletteModel()
    {
    }

    QVariant ComponentPaletteModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && section == 0 && orientation == Qt::Horizontal)
        {
            return tr("Components");
        }

        return QStandardItemModel::headerData(section, orientation, role);
    }
}

#include "UI/ComponentPalette/moc_ComponentPaletteModel.cpp"
