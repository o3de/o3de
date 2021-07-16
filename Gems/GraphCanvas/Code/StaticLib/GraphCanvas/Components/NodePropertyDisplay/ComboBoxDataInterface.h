/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QMimeData>
#include <QString>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/Components/NodePropertyDisplay/DataInterface.h>
#include <GraphCanvas/Widgets/ComboBox/ComboBoxItemModelInterface.h>

namespace GraphCanvas
{
    class ComboBoxDataInterface
        : public DataInterface
    {
    public:
        // Returns the EnumModel used to populate the DropDown and AutoCompleter Menu
        virtual ComboBoxItemModelInterface* GetItemInterface() = 0;

        // Interfaces for manipulating the values. Indexes will refer to the elements within the ComboBoxModel
        virtual void AssignIndex(const QModelIndex& index) = 0;
        virtual QModelIndex GetAssignedIndex() const = 0;

        // Returns the string used to display the currently selected value[Used in the non-editable format]
        virtual QString GetDisplayString() const
        {
            static QString k_emptyString;
            return k_emptyString;
        }

        virtual void OnShowContextMenu(QWidget* nodePropertyDisplay, const QPoint& pos)
        {
            AZ_UNUSED(nodePropertyDisplay);
            AZ_UNUSED(pos);
        }
    };
}
