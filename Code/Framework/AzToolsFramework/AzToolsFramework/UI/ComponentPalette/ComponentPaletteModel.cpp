/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
