/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include <QApplication>
#include <QSettings>
#include <QTemporaryDir>

#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <Lib/Tests/IEditorMock.h>
#include <QtViewPaneManager.h>
#include <Undo/Undo.h>

#include <Maestro/Cinematics/AnimSequence.h>
#include <Maestro/Editor/MaestroEditorServices.h>
#include <Maestro/Editor/MaestroEditorSystemComponent.h>
#include <Maestro/Editor/TrackView/TrackViewSequenceManager.h>

namespace
{
    class MaestroEditorTestEnvironment
        : public UnitTest::TraceBusHook
    {
    };

    class MaestroEditorServicesTests
        : public ::testing::Test
        , private AzToolsFramework::EditorRequests::Bus::Handler
    {
    protected:
        void SetUp() override
        {
            ON_CALL(m_editor, GetUndoManager()).WillByDefault(::testing::Return(&m_undoManager));
            SetIEditor(&m_editor);
            AzToolsFramework::EditorRequests::Bus::Handler::BusConnect();
            m_component.Activate();
            AzToolsFramework::EditorEvents::Bus::Broadcast(
                &AzToolsFramework::EditorEvents::NotifyIEditorAvailable, &m_editor);
        }

        void TearDown() override
        {
            m_component.Deactivate();
            AzToolsFramework::EditorRequests::Bus::Handler::BusDisconnect();
            SetIEditor(nullptr);
        }

        void BrowseForAssets(AzToolsFramework::AssetBrowser::AssetSelectionModel&) override {}

        void UnregisterViewPane(const char* name) override
        {
            QtViewPaneManager::instance()->UnregisterPane(name);
        }

        ::testing::NiceMock<CEditorMock> m_editor;
        CUndoManager m_undoManager;
        Maestro::MaestroEditorSystemComponent m_component;
    };

    class CompetingEditorSequenceSystemLifecycle
        : public AzToolsFramework::IEditorSequenceSystemLifecycle
    {
    public:
        void OnEditorGameEngineInitialized(bool) override {}
        void OnEditorGameEngineAttached() override {}
        void OnEditorGameEngineShutdown() override {}
        void OnEditorUpdate() override {}
        void OnBeginSceneOpen() override {}
        void OnBeginGameMode() override {}
        void OnBeforePlayInEditorStart() override {}
        void OnAfterPlayInEditorStop() override {}
        void OnEndGameMode(bool) override {}
        void OnSimulationModeChanging(bool) override {}
    };
}

TEST(MaestroEditorApiTests, MaestroRegistersAsTheSingleEditorSequenceLifecycleProvider)
{
    ASSERT_EQ(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);

    Maestro::MaestroEditorSystemComponent component;
    component.Activate();
    EXPECT_NE(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);

    CompetingEditorSequenceSystemLifecycle competingProvider;
    EXPECT_FALSE(AzToolsFramework::EditorSequenceSystemLifecycleInterface::Register(&competingProvider).IsSuccess());

    component.Deactivate();
    EXPECT_EQ(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);
}

TEST(MaestroEditorApiTests, RepeatedActivationDoesNotLeaveAnInterfaceOrEditorEventHandler)
{
    ASSERT_EQ(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);
    const size_t initialHandlerCount = AzToolsFramework::EditorEvents::Bus::GetTotalNumOfEventHandlers();
    Maestro::MaestroEditorSystemComponent component;
    for (int iteration = 0; iteration < 20; ++iteration)
    {
        component.Activate();
        EXPECT_NE(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);
        EXPECT_EQ(AzToolsFramework::EditorEvents::Bus::GetTotalNumOfEventHandlers(), initialHandlerCount + 1);
        component.Deactivate();
        EXPECT_EQ(AzToolsFramework::IEditorSequenceSystemLifecycle::Get(), nullptr);
        EXPECT_EQ(Maestro::Editor::GetAnimation(), nullptr);
        EXPECT_EQ(Maestro::Editor::GetSequenceManager(), nullptr);
        EXPECT_EQ(AzToolsFramework::EditorEvents::Bus::GetTotalNumOfEventHandlers(), initialHandlerCount);
        AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::NotifyRegisterViews);
    }
}

TEST_F(MaestroEditorServicesTests, DeactivationDestroysRemainingSequences)
{
    CTrackViewSequenceManager* sequenceManager = Maestro::Editor::GetSequenceManager();
    ASSERT_NE(sequenceManager, nullptr);
    AZStd::intrusive_ptr<IAnimSequence> sequence = aznew Maestro::CAnimSequence();
    sequenceManager->OnCreateSequenceComponent(sequence);
    ASSERT_EQ(sequenceManager->GetCount(), 1);

    m_component.Deactivate();

    EXPECT_EQ(Maestro::Editor::GetSequenceManager(), nullptr);
    EXPECT_EQ(Maestro::Editor::GetAnimation(), nullptr);
}

namespace Maestro
{
    TEST_F(MaestroEditorServicesTests, DeactivationDestroysPaneWidgetsBeforeServices)
    {
        QtViewPaneManager* paneManager = QtViewPaneManager::instance();
        const char* paneName = Editor::TrackViewPaneName;
        paneManager->RegisterPane(paneName, "Tools", [](QWidget* parent) { return new QWidget(parent); });
        const QtViewPane* pane = paneManager->OpenPane(paneName, QtViewPane::OpenMode::UseDefaultState);
        ASSERT_NE(pane, nullptr);
        QPointer<DockWidget> dockWidget = pane->m_dockWidget;
        QPointer<QDockWidget> additionalDockWidget = paneManager->InstancePane(paneName);
        ASSERT_NE(additionalDockWidget, nullptr);
        m_component.m_viewRegistered = true;

        QPointer<QWidget> contentWidget = pane->Widget();
        bool destroyedBeforeUnregister = false;
        QObject::connect(contentWidget, &QObject::destroyed, paneManager, [&]()
            {
                destroyedBeforeUnregister = paneManager->IsPaneRegistered(paneName);
                EXPECT_NE(Editor::GetAnimation(), nullptr);
                EXPECT_NE(Editor::GetSequenceManager(), nullptr);
            });

        m_component.Deactivate();

        EXPECT_TRUE(dockWidget.isNull());
        EXPECT_TRUE(additionalDockWidget.isNull());
        EXPECT_TRUE(contentWidget.isNull());
        EXPECT_TRUE(destroyedBeforeUnregister);
        EXPECT_FALSE(paneManager->IsPaneRegistered(paneName));
        EXPECT_EQ(Editor::GetAnimation(), nullptr);
        EXPECT_EQ(Editor::GetSequenceManager(), nullptr);

        if (contentWidget)
        {
            QObject::disconnect(contentWidget, nullptr, paneManager, nullptr);
        }
        if (dockWidget)
        {
            QObject::disconnect(dockWidget, nullptr, paneManager, nullptr);
        }
        if (additionalDockWidget)
        {
            QObject::disconnect(additionalDockWidget, nullptr, paneManager, nullptr);
        }
        delete dockWidget.data();
        delete additionalDockWidget.data();
    }
}

AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    QApplication application(argc, argv);
    AzQtComponents::StyleManager styleManager(&application);
    styleManager.initialize(&application, {});
    QTemporaryDir temporaryDirectory;
    QSettings settings(temporaryDirectory.filePath("panes.ini"), QSettings::IniFormat);
    AzQtComponents::DockMainWindow mainWindow;
    QtViewPaneManager::instance()->SetMainWindow(&mainWindow, &settings, {});
    AZ::Test::ApplyGlobalParameters(&argc, argv);
    AZ::Test::printUnusedParametersWarning(argc, argv);
    AZ::Test::addTestEnvironments({ new MaestroEditorTestEnvironment });
    const int result = RUN_ALL_TESTS();
    return result;
}

IMPLEMENT_TEST_EXECUTABLE_MAIN()
