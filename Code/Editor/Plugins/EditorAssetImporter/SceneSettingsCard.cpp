/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SceneSettingsCard.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <AzQtComponents/Components/StyledBusyLabel.h>
#include <AzQtComponents/Components/StyledDetailsTableView.h>
#include <AzQtComponents/Components/StyledDetailsTableModel.h>

SceneSettingsCardHeader::SceneSettingsCardHeader(QWidget* parent /* = nullptr */)
    : AzQtComponents::CardHeader(parent)
{
    m_busyLabel = new AzQtComponents::StyledBusyLabel(this);
    m_busyLabel->SetIsBusy(true);
    m_busyLabel->SetBusyIconSize(14);
    m_backgroundLayout->insertWidget(1, m_busyLabel);

    m_closeButton = new QPushButton(this);
    m_closeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_closeButton->setMinimumSize(24, 24);
    m_closeButton->setMaximumSize(24, 24);
    m_closeButton->setBaseSize(24, 24);

    m_backgroundLayout->addWidget(m_closeButton);
    
    connect(m_closeButton, &QPushButton::clicked, this, &SceneSettingsCardHeader::triggerCloseButton);
}

void SceneSettingsCardHeader::triggerCloseButton()
{
    parent()->deleteLater();
}

SceneSettingsCard::SceneSettingsCard(QWidget* parent /* = nullptr */)
    : AzQtComponents::Card(new SceneSettingsCardHeader(), parent)
{
    // This has to be set here, instead of in the customheader,
    // because the Card constructor forces the context menu to be visible.
    header()->setHasContextMenu(false);

    AzQtComponents::StyledDetailsTableModel* m_reportModel = new AzQtComponents::StyledDetailsTableModel();
    m_reportModel->AddColumn("Status", AzQtComponents::StyledDetailsTableModel::StatusIcon);
    m_reportModel->AddColumn("Message");
    m_reportModel->AddColumnAlias("message", "Message");
    AzQtComponents::StyledDetailsTableView* m_reportView = new AzQtComponents::StyledDetailsTableView();
    m_reportView->setModel(m_reportModel);
    setContentWidget(m_reportView);
}
