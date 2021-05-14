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

#include <GemCatalog/GemCatalogScreen.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

namespace O3DE::ProjectManager
{
    GemCatalogScreen::GemCatalogScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        m_gemModel = new GemModel(this);

        QVBoxLayout* vLayout = new QVBoxLayout();
        setLayout(vLayout);

        QHBoxLayout* hLayout = new QHBoxLayout();
        vLayout->addLayout(hLayout);

        QWidget* filterPlaceholderWidget = new QWidget();
        filterPlaceholderWidget->setFixedWidth(250);
        hLayout->addWidget(filterPlaceholderWidget);

        m_gemListView = new GemListView(m_gemModel, this);
        hLayout->addWidget(m_gemListView);

        QWidget* inspectorPlaceholderWidget = new QWidget();
        inspectorPlaceholderWidget->setFixedWidth(250);
        hLayout->addWidget(inspectorPlaceholderWidget);

        // Start: Temporary gem test data
        {
            m_gemModel->AddGem(GemInfo("EMotion FX",
                "O3DE Foundation",
                "EMFX is a real-time character animation system. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
                (GemInfo::Android | GemInfo::iOS | GemInfo::macOS | GemInfo::Windows | GemInfo::Linux),
                true));

            m_gemModel->AddGem(O3DE::ProjectManager::GemInfo("Atom",
                "O3DE Foundation",
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
                GemInfo::Android | GemInfo::Windows | GemInfo::Linux | GemInfo::macOS,
                true));

            m_gemModel->AddGem(O3DE::ProjectManager::GemInfo("PhysX",
                "O3DE London",
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
                GemInfo::Android | GemInfo::Linux | GemInfo::macOS,
                false));

            m_gemModel->AddGem(O3DE::ProjectManager::GemInfo("Certificate Manager",
                "O3DE Irvine",
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
                GemInfo::Windows,
                false));

            m_gemModel->AddGem(O3DE::ProjectManager::GemInfo("Cloud Gem Framework",
                "O3DE Seattle",
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
                GemInfo::iOS | GemInfo::Linux,
                false));

            m_gemModel->AddGem(O3DE::ProjectManager::GemInfo("Achievements",
                "O3DE Foundation",
                "Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
                GemInfo::Android | GemInfo::Windows | GemInfo::Linux,
                false));
        }
        // End: Temporary gem test data
    }

    ProjectManagerScreen GemCatalogScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::GemCatalog;
    }

    QString GemCatalogScreen::GetNextButtonText()
    {
        return "Create Project";
    }
} // namespace O3DE::ProjectManager
