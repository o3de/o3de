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
#include <PythonBindingsInterface.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <GemCatalog/GemFilterWidget.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>

//#define USE_TESTGEMDATA

namespace O3DE::ProjectManager
{
    GemCatalogScreen::GemCatalogScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        m_gemModel = new GemModel(this);
        GemSortFilterProxyModel* proxyModel = new GemSortFilterProxyModel(m_gemModel, this);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        setLayout(vLayout);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        vLayout->addLayout(hLayout);

        m_gemListView = new GemListView(proxyModel, proxyModel->GetSelectionModel(), this);
        m_gemInspector = new GemInspector(m_gemModel, this);
        m_gemInspector->setFixedWidth(320);

        // Start: Temporary gem test data
#ifdef USE_TESTGEMDATA
        QVector<GemInfo> testGemData = GenerateTestData();
        for (const GemInfo& gemInfo : testGemData)
        {
            m_gemModel->AddGem(gemInfo);
        }
#else
        // End: Temporary gem test data
        auto result = PythonBindingsInterface::Get()->GetGems();
        if (result.IsSuccess())
        {
            for (auto gemInfo : result.GetValue())
            {
                m_gemModel->AddGem(gemInfo);
            }
        }
#endif

        GemFilterWidget* filterWidget = new GemFilterWidget(proxyModel);
        filterWidget->setFixedWidth(250);

        QVBoxLayout* middleVLayout = new QVBoxLayout();
        middleVLayout->setMargin(0);
        middleVLayout->setSpacing(0);
        middleVLayout->addWidget(m_gemListView);

        hLayout->addWidget(filterWidget);
        hLayout->addLayout(middleVLayout);
        hLayout->addWidget(m_gemInspector);

        proxyModel->InvalidateFilter();
    }

    QVector<GemInfo> GemCatalogScreen::GenerateTestData()
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
        gem.m_types = (GemInfo::Code | GemInfo::Asset);
        gem.m_version = "v1.01";
        gem.m_lastUpdatedDate = "24th April 2021";
        gem.m_binarySizeInKB = 40;
        gem.m_features = QStringList({"Animation", "Assets", "Physics"});
        gem.m_gemOrigin = GemInfo::O3DEFoundation;
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

    ProjectManagerScreen GemCatalogScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::GemCatalog;
    }
} // namespace O3DE::ProjectManager
