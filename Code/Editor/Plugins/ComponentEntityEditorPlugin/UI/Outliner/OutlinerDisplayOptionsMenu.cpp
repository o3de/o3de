/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "OutlinerDisplayOptionsMenu.h"

#include <QIcon>


namespace EntityOutliner
{
    DisplayOptionsMenu::DisplayOptionsMenu(QWidget* parent)
        : QMenu(parent)
    {
        auto sortManually = addAction(QIcon(QStringLiteral(":/Outliner/sort_manually.svg")), tr("Sort: Manually"));
        sortManually->setData(static_cast<int>(DisplaySortMode::Manually));
        sortManually->setCheckable(true);

        auto sortAtoZ = addAction(QIcon(QStringLiteral(":/Outliner/sort_a_to_z.svg")), tr("Sort: A to Z"));
        sortAtoZ->setData(static_cast<int>(DisplaySortMode::AtoZ));
        sortAtoZ->setCheckable(true);

        auto sortZtoA = addAction(QIcon(QStringLiteral(":/Outliner/sort_z_to_a.svg")), tr("Sort: Z to A"));
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

#include <UI/Outliner/moc_OutlinerDisplayOptionsMenu.cpp>
