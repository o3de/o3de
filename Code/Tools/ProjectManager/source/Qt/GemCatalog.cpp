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

#include <Qt/GemCatalog.h>

#include <Qt/ui_GemCatalog.h>

namespace O3DE::ProjectManager
{
    GemCatalog::GemCatalog(ProjectManagerWindow* window)
        : ScreenWidget(window)
        , m_ui(new Ui::GemCatalogClass())
    {
        m_ui->setupUi(this);

        Setup();
    }

    GemCatalog::~GemCatalog()
    {
    }

    void GemCatalog::ConnectSlotsAndSignals()
    {
        QObject::connect(m_ui->backButton, &QPushButton::pressed, this, &GemCatalog::HandleBackButton);
        QObject::connect(m_ui->confirmButton, &QPushButton::pressed, this, &GemCatalog::HandleConfirmButton);
    }

    void GemCatalog::HandleBackButton()
    {
        m_projectManagerWindow->ChangeToScreen(ProjectManagerScreen::NewProjectSettings);
    }
    void GemCatalog::HandleConfirmButton()
    {
        m_projectManagerWindow->ChangeToScreen(ProjectManagerScreen::ProjectsHome);
    }

} // namespace O3DE::ProjectManager
