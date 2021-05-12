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

#include <GemCatalog/GemCatalog.h>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>

namespace O3DE::ProjectManager
{
    GemCatalog::GemCatalog(ProjectManagerWindow* window)
        : ScreenWidget(window)
    {
        m_gemModel = new GemModel(this);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        setLayout(vLayout);

        QHBoxLayout* hLayout = new QHBoxLayout();
        vLayout->addLayout(hLayout);

        m_gemListView = new GemListView(m_gemModel, this);
        m_gemInspector = new GemInspector(m_gemModel, this);
        m_gemInspector->setFixedWidth(320);

        // Start: Temporary gem test data
        QVector<GemInfo> testGemData = GenerateTestData();
        for (const GemInfo& gemInfo : testGemData)
        {
            m_gemModel->AddGem(gemInfo);
        }
        // End: Temporary gem test data

        hLayout->addWidget(m_gemListView);
        hLayout->addWidget(m_gemInspector);

        // Temporary back and next buttons until they are centralized and shared.
        QDialogButtonBox* backNextButtons = new QDialogButtonBox();
        vLayout->addWidget(backNextButtons);

        QPushButton* tempBackButton = backNextButtons->addButton("Back", QDialogButtonBox::RejectRole);
        QPushButton* tempNextButton = backNextButtons->addButton("Next", QDialogButtonBox::AcceptRole);
        connect(tempBackButton, &QPushButton::pressed, this, &GemCatalog::HandleBackButton);
        connect(tempNextButton, &QPushButton::pressed, this, &GemCatalog::HandleConfirmButton);

        // Select the first entry after everything got correctly sized
        QTimer::singleShot(100, [=]{
            QModelIndex firstModelIndex = m_gemListView->model()->index(0,0);
            m_gemListView->selectionModel()->select(firstModelIndex, QItemSelectionModel::ClearAndSelect);
            });
    }

    QVector<GemInfo> GemCatalog::GenerateTestData()
    {
        QVector<GemInfo> result;

        GemInfo gem("EMotion FX",
            "O3DE Foundation",
            "EMFX is a real-time character animation system. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
            (GemInfo::Android | GemInfo::iOS | GemInfo::macOS | GemInfo::Windows | GemInfo::Linux),
            true);
        gem.m_directoryLink = "C:/";
        gem.m_documentationLink = "http://www.amazon.com";
        gem.m_dependingGemUuids = QStringList({"EMotionFX", "Atom"});
        gem.m_conflictingGemUuids = QStringList({"Vegetation", "Camera", "ScriptCanvas", "CloudCanvas", "Networking"});
        gem.m_version = "v1.01";
        gem.m_lastUpdatedDate = "24th April 2021";
        gem.m_binarySizeInKB = 40;
        gem.m_features = QStringList({"Animation", "Assets", "Physics"});
        result.push_back(gem);

        gem.m_name = "Atom";
        gem.m_creator = "O3DE Seattle";
        gem.m_summary = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
        gem.m_platforms = (GemInfo::Android | GemInfo::Windows | GemInfo::Linux | GemInfo::macOS);
        gem.m_isAdded = true;
        gem.m_directoryLink = "C:/";
        gem.m_documentationLink = "https://aws.amazon.com/gametech/";
        gem.m_dependingGemUuids = QStringList({"EMotionFX", "Core", "AudioSystem", "Camera", "Particles"});
        gem.m_conflictingGemUuids = QStringList({"CloudCanvas", "NovaNet"});
        gem.m_version = "v2.31";
        gem.m_lastUpdatedDate = "24th November 2020";
        gem.m_features = QStringList({"Assets", "Rendering", "UI", "VR", "Debug", "Environment"});
        gem.m_binarySizeInKB = 2087;
        result.push_back(gem);

        gem.m_name = "Physics";
        gem.m_creator = "O3DE London";
        gem.m_summary = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
        gem.m_platforms = (GemInfo::Android | GemInfo::Linux | GemInfo::macOS);
        gem.m_isAdded = true;
        gem.m_directoryLink = "C:/";
        gem.m_documentationLink = "https://aws.amazon.com/gametech/";
        gem.m_dependingGemUuids = QStringList({"GraphCanvas", "ExpressionEvaluation", "UI Lib", "Multiplayer", "GameStateSamples"});
        gem.m_conflictingGemUuids = QStringList({"Cloud Canvas", "EMotion FX", "Streaming", "MessagePopup", "Cloth", "Graph Canvas", "Twitch Integration"});
        gem.m_version = "v1.5.102145";
        gem.m_lastUpdatedDate = "1st January 2021";
        gem.m_binarySizeInKB = 2000000;
        gem.m_features = QStringList({"Physics", "Gameplay", "Debug", "Assets"});
        result.push_back(gem);

        result.push_back(O3DE::ProjectManager::GemInfo("Certificate Manager",
            "O3DE Irvine",
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
            GemInfo::Windows,
            false));

        result.push_back(O3DE::ProjectManager::GemInfo("Cloud Gem Framework",
            "O3DE Seattle",
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
            GemInfo::iOS | GemInfo::Linux,
            false));

        result.push_back(O3DE::ProjectManager::GemInfo("Cloud Gem Core",
            "O3DE Foundation",
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
            GemInfo::Android | GemInfo::Windows | GemInfo::Linux,
            true));

        result.push_back(O3DE::ProjectManager::GemInfo("Gestures",
            "O3DE Foundation",
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
            GemInfo::Android | GemInfo::Windows | GemInfo::Linux,
            false));

        result.push_back(O3DE::ProjectManager::GemInfo("Effects System",
            "O3DE Foundation",
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
            GemInfo::Android | GemInfo::Windows | GemInfo::Linux,
            true));

        result.push_back(O3DE::ProjectManager::GemInfo("Microphone",
            "O3DE Foundation",
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus euismod ligula vitae dui dictum, a sodales dolor luctus. Sed id elit dapibus, finibus neque sed, efficitur mi. Nam facilisis ligula at eleifend pellentesque. Praesent non ex consectetur, blandit tellus in, venenatis lacus. Duis nec neque in urna ullamcorper euismod id eu leo. Nam efficitur dolor sed odio vehicula venenatis. Suspendisse nec est non velit commodo cursus in sit amet dui. Ut bibendum nisl et libero hendrerit dapibus. Vestibulum ultrices ullamcorper urna, placerat porttitor est lobortis in. Interdum et malesuada fames ac ante ipsum primis in faucibus. Integer a magna ac tellus sollicitudin porttitor. Phasellus lobortis viverra justo id bibendum. Etiam ac pharetra risus. Nulla vitae justo nibh. Nulla viverra leo et molestie interdum. Duis sit amet bibendum nulla, sit amet vehicula augue.",
            GemInfo::Android | GemInfo::Windows | GemInfo::Linux,
            false));

        return result;
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
