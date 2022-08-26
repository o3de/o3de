/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>
#endif

namespace AzQtComponents
{
    class StyledBusyLabel;
}

class QPushButton;

class SceneSettingsCardHeader : public AzQtComponents::CardHeader
{
    Q_OBJECT
public:
    SceneSettingsCardHeader(QWidget* parent = nullptr);

private:
    void triggerCloseButton();

    AzQtComponents::StyledBusyLabel* m_busyLabel = nullptr;
    QPushButton* m_closeButton = nullptr;
};

class SceneSettingsCard : public AzQtComponents::Card
{
    Q_OBJECT
public:
    SceneSettingsCard(QWidget* parent = nullptr);
};
