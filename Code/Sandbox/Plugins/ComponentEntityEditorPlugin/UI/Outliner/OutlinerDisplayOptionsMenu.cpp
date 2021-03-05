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
#include "ComponentEntityEditorPlugin_precompiled.h"

#include "OutlinerDisplayOptionsMenu.h"

#include <QIcon>


namespace EntityOutliner
{
    DisplayOptionsMenu::DisplayOptionsMenu(QWidget* parent)
        : QMenu(parent)
    {
        auto sortManually = addAction(QIcon(QStringLiteral(":/sort_manually.svg")), tr("Sort: Manually"));
        sortManually->setData(static_cast<int>(DisplaySortMode::Manually));
        sortManually->setCheckable(true);

        auto sortAtoZ = addAction(QIcon(QStringLiteral(":/sort_a_to_z.svg")), tr("Sort: A to Z"));
        sortAtoZ->setData(static_cast<int>(DisplaySortMode::AtoZ));
        sortAtoZ->setCheckable(true);

        auto sortZtoA = addAction(QIcon(QStringLiteral(":/sort_z_to_a.svg")), tr("Sort: Z to A"));
        sortZtoA->setData(static_cast<int>(DisplaySortMode::ZtoA));
        sortZtoA->setCheckable(true);

        addSeparator();

        auto autoScroll = addAction(tr("Scroll to Selected"));
        autoScroll->setCheckable(true);

        auto autoExpand = addAction(tr("Expand Selected"));
        autoExpand->setCheckable(true);

        auto sortGroup = new QActionGroup(this);
        sortGroup->addAction(sortManually);
        sortGroup->addAction(sortAtoZ);
        sortGroup->addAction(sortZtoA);

        sortManually->setChecked(true);
        autoScroll->setChecked(true);
        autoExpand->setChecked(true);

        connect(sortGroup, &QActionGroup::triggered, this, &DisplayOptionsMenu::OnSortModeSelected);
        connect(autoScroll, &QAction::toggled, this, &DisplayOptionsMenu::OnAutoScrollToggle);
        connect(autoExpand, &QAction::toggled, this, &DisplayOptionsMenu::OnAutoExpandToggle);
    }

    void DisplayOptionsMenu::OnSortModeSelected(QAction* action)
    {
        const auto sortMode = static_cast<DisplaySortMode>(action->data().toInt());
        emit OnSortModeChanged(sortMode);
    }

    void DisplayOptionsMenu::OnAutoScrollToggle(bool checked)
    {
        emit OnOptionToggled(DisplayOption::AutoScroll, checked);
    }

    void DisplayOptionsMenu::OnAutoExpandToggle(bool checked)
    {
        emit OnOptionToggled(DisplayOption::AutoExpand, checked);
    }
}
