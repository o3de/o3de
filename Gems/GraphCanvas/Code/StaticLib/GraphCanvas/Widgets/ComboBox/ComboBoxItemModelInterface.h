/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QAbstractItemModel>
#include <QString>
AZ_POP_DISABLE_WARNING

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace GraphCanvas
{
    class ComboBoxItemModelInterface
    {
    public:
        ComboBoxItemModelInterface() = default;
        ~ComboBoxItemModelInterface() = default;

        virtual void SetFontScale(qreal fontScale) = 0;

        virtual QString GetNameForIndex(const QModelIndex& modelIndex) const = 0;
        virtual QModelIndex FindIndexForName(const QString& name) const = 0;

        virtual QModelIndex GetDefaultIndex() const = 0;

        // Methods for the display list
        virtual QAbstractItemModel* GetDropDownItemModel() = 0;
        virtual int GetSortColumn() const = 0;
        virtual int GetFilterColumn() const = 0;

        virtual QModelIndex GetNextIndex(const QModelIndex& modelIndex) const = 0;
        virtual QModelIndex GetPreviousIndex(const QModelIndex& modelIndex) const = 0;

        virtual void OnDropDownAboutToShow() {};
        virtual void OnDropDownHidden() {};
        ////

        // Methods for the Autocompleter
        virtual QAbstractListModel* GetCompleterItemModel() = 0;
        virtual int GetCompleterColumn() const = 0;
        ////
    };
}
