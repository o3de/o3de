/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QMenu>

#endif

class QAction;

namespace Ui
{
    class EntityOutlinerDisplayOptions;
}

namespace AzToolsFramework
{

    namespace EntityOutliner
    {
        enum class DisplaySortMode : unsigned char
        {
            Manually,
            AtoZ,
            ZtoA
        };

        enum class DisplayOption : unsigned char
        {
            AutoScroll,
            AutoExpand
        };

        class DisplayOptionsMenu
            : public QMenu
        {
            Q_OBJECT // AUTOMOC

        public:
            DisplayOptionsMenu(QWidget* parent = nullptr);
            ~DisplayOptionsMenu() = default;

        signals:
            void OnSortModeChanged(DisplaySortMode sortMode);
            void OnOptionToggled(DisplayOption option, bool enabled);

        private:
            void OnSortModeSelected(QAction* action);

            void OnAutoScrollToggle(bool checked);
            void OnAutoExpandToggle(bool checked);
        };
    }

}
