/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/typetraits/conditional.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzTest/AzTest.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AZTestShared/Utils/Utils.h>
#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>
#include <AzToolsFramework/ActionManager/ActionManagerSystemComponent.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInternalInterface.h>
#include <AzToolsFramework/ActionManager/ToolBar/ToolBarManagerInternalInterface.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Entity/EditorEntityTransformBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorDefaultSelection.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>
#include <AzToolsFramework/SourceControl/PerforceConnection.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>
#include <QMainWindow>
#endif // !defined(Q_MOC_RUN)

#include <ostream>

// MSVC: warning suppressed: constant used in conditional expression
// CLANG: warning suppressed: implicit conversion loses integer precision
AZ_PUSH_DISABLE_WARNING(4127, "-Wshorten-64-to-32")
#include <QtTest/QtTest>
AZ_POP_DISABLE_WARNING

#define AUTO_RESULT_IF_SETTING_TRUE(_settingName, _result)  \
    {                                                       \
        bool settingValue = true;                           \
        if (auto* registry = AZ::SettingsRegistry::Get())   \
        {                                                   \
            registry->Get(settingValue, _settingName);      \
        }                                                   \
                                                            \
        if (settingValue)                                   \
        {                                                   \
            EXPECT_TRUE(_result);                           \
            return;                                         \
        }                                                   \
    }

// Helper unit test hook for any tools unit tests that rely on a QApplication being created (e.g. any tests that need to create a QWidget)
#define AZ_TOOLS_UNIT_TEST_HOOK_ENV(TEST_ENV)                                                           \
    AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)                                     \
    {                                                                                                   \
        ::testing::InitGoogleMock(&argc, argv);                                                         \
        AzQtComponents::PrepareQtPaths();                                                               \
        QApplication app(argc, argv);                                                                   \
        if (AZ_TRAIT_AZTEST_ATTACH_RESULT_LISTENER)                                                     \
        {                                                                                               \
            ::testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();   \
            listeners.Append(new AZ::Test::OutputEventListener);                                        \
        }                                                                                               \
        AZ::Test::ApplyGlobalParameters(&argc, argv);                                                   \
        AZ::Test::printUnusedParametersWarning(argc, argv);                                             \
        AZ::Test::addTestEnvironments({TEST_ENV});                                                      \
        int result = RUN_ALL_TESTS();                                                                   \
        if (::testing::UnitTest::GetInstance()->test_to_run_count() == 0)                               \
        {                                                                                               \
            std::cerr << "No tests were found for last suite ran!" << std::endl;                        \
            result = 1;                                                                                 \
        }                                                                                               \
        return result;                                                                                  \
    }

#define AZ_TOOLS_UNIT_TEST_HOOK(TEST_ENV) \
    AZ_TOOLS_UNIT_TEST_HOOK_ENV(TEST_ENV) \
    AZ_BENCHMARK_HOOK() \
    IMPLEMENT_TEST_EXECUTABLE_MAIN()

namespace UnitTest
{
    constexpr AZStd::string_view prefabSystemSetting = "/Amazon/Preferences/EnablePrefabSystem";

    /// Performs a mouse press and move event on the provided widget.
    /// @param widget The widget to perform the mouse press and move on.
    /// @param initialPositionWidget The position of the mouse relative to the widget (will be remapped to a global position internally).
    /// @param mouseDelta How far to move the mouse.
    /// @param mouseButton The button to be used during the press and move.
    void MousePressAndMove(
        QWidget* widget, const QPoint& initialPositionWidget, const QPoint& mouseDelta, Qt::MouseButton mouseButton = Qt::LeftButton);

    /// Performs a mouse move event on the provided widget.
    /// @param widget The widget to perform the mouse move on.
    /// @param initialPositionWidget The position of the mouse relative to the widget (will be remapped to a global position internally).
    /// @param mouseDelta How far to move the mouse (note: mouseDelta may be zero and the mouse will only be moved to initialPosition).
    /// @param mouseButton The button to be held during the move.
    void MouseMove(QWidget* widget, const QPoint& initialPosition, const QPoint& mouseDelta, Qt::MouseButton mouseButton = Qt::NoButton);

    /// Performs a full series (begin, update, end) of mouse wheel events on the provided widget.
    /// @param widget The widget to perform the mouse wheel events on.
    /// @param localEventPosition The position of the mouse relative to the widget (will be remapped to a global position internally).
    /// @param wheelDelta How far to move the mouse (note: mouseDelta may be zero and the mouse will only be moved to initialPosition).
    /// @param mouseButtons Optional mouse buttons to include during the wheel events, defaults to Qt::NoButton
    /// @param keyboardModifiers Optional keyboard modifiers to include during the wheel events, defaults to Qt::NoModifier
    void MouseScroll(QWidget* widget, QPoint localEventPosition, QPoint wheelDelta,
        Qt::MouseButtons mouseButtons = Qt::NoButton, Qt::KeyboardModifiers keyboardModifiers = Qt::NoModifier);

    /// Convert a Qt::Key + optional modifiers to the printable text of the key sequence
    /// @param key The widget to perform the mouse wheel event on.
    /// @param modifiers Optional keyboard modifiers to include during the wheel events, defaults to Qt::NoModifier
    AZStd::string QtKeyToAzString(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    //! Test implementation of the ViewportSettingsRequestBus.
    //! @note Can be used to customize viewport settings during test execution.
    class ViewportSettingsTestImpl : public AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Handler
    {
    public:
        void Connect(const AzFramework::ViewportId viewportId)
        {
            AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Handler::BusConnect(viewportId);
        }

        void Disconnect()
        {
            AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Handler::BusDisconnect();
        }

        // ViewportSettingsRequestBus overrides ...
        bool GridSnappingEnabled() const override;
        float GridSize() const override;
        bool ShowGrid() const override;
        bool AngleSnappingEnabled() const override;
        float AngleStep() const override;
        float ManipulatorLineBoundWidth() const override;
        float ManipulatorCircleBoundWidth() const override;
        bool StickySelectEnabled() const override;
        AZ::Vector3 DefaultEditorCameraPosition() const override;
        AZ::Vector2 DefaultEditorCameraOrientation() const override;
        bool IconsVisible() const override;
        bool HelpersVisible() const override;
        bool OnlyShowHelpersForSelectedEntities() const override;

        float m_gridSize = 1.0f;
        float m_angularStep = 0.0f;
        bool m_gridSnapping = false;
        bool m_angularSnapping = false;
        bool m_stickySelect = true;
        bool m_iconsVisible = true;
        bool m_helpersVisible = true;
        bool m_onlyShowForSelectedEntities = false;
    };

    /// Test widget to store QActions generated by EditorTransformComponentSelection.
    class TestWidget : public QWidget
    {
        Q_OBJECT
    public:
        TestWidget()
            : QWidget()
        {
            // ensure TestWidget can intercept and filter any incoming events itself
            installEventFilter(this);
        }

        bool eventFilter(QObject* watched, QEvent* event) override;
    };

    /// Widget used to trigger a viewport interaction event while a focus change is happening.
    class FocusInteractionWidget : public QWidget
    {
        Q_OBJECT
    public:
        FocusInteractionWidget(QWidget* parent = nullptr)
            : QWidget(parent)
        {
        }

        bool event(QEvent* event) override;
    };

    /// Records mouse move events and stores the local and global position of the cursor.
    /// @note To use, install as an event filter for the widget being interacted with
    /// e.g. m_testWidget->installEventFilter(&m_mouseMoveDetector);
    class MouseMoveDetector : public QObject
    {
        Q_OBJECT
    public:
        MouseMoveDetector(QWidget* parent = nullptr);

        bool eventFilter([[maybe_unused]] QObject* watched, QEvent* event) override;

        QPoint m_mouseGlobalPosition;
        QPoint m_mouseLocalPosition;
    };

    /// Stores actions registered for either normal mode (regular viewport) editing and
    /// component mode editing.
    class TestEditorActions
        : private AzToolsFramework::EditorActionRequestBus::Handler
        , private AzToolsFramework::ViewportEditorModeNotificationsBus::Handler
    {
        // EditorActionRequestBus ...
        void AddActionViaBus(int id, QAction* action) override;
        void AddActionViaBusCrc(AZ::Crc32 id, QAction* action) override;
        void RemoveActionViaBus(QAction* action) override;
        void EnableDefaultActions() override;
        void DisableDefaultActions() override;
        void AttachOverride(QWidget* /*object*/) override {}
        void DetachOverride() override {}

        // ViewportEditorModeNotificationsBus overrides ...
        void OnEditorModeActivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;
        void OnEditorModeDeactivated(
            const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode) override;

    public:
        void Connect();
        void Disconnect();

        TestWidget m_componentModeWidget; /// Widget to hold component mode actions.
        TestWidget m_defaultWidget; /// Widget to hold normal viewport/editor actions.
    };

    //! Used to intercept various messages which get printed to the console during the startup of a tools application
    //! but are not relevant for testing.
    class ToolsApplicationMessageHandler
    {
    public:
        ToolsApplicationMessageHandler();
    private:
        AZStd::unique_ptr<ErrorHandler> m_enginePathMessageHandler;
        AZStd::unique_ptr<ErrorHandler> m_skippingDriveMessageHandler;
        AZStd::unique_ptr<ErrorHandler> m_storageDriveMessageHandler;
        AZStd::unique_ptr<ErrorHandler> m_jsonComponentErrorHandler;
    };

    /// Base fixture for ToolsApplication editor tests.
    template<bool CheckForLeaksOnDestruction = true>
    class ToolsApplicationFixture
        : public AZStd::conditional_t<CheckForLeaksOnDestruction, LeakDetectionFixture, testing::Test>
    {
        using Base = AZStd::conditional_t<CheckForLeaksOnDestruction, LeakDetectionFixture, testing::Test>;
    public:
        void SetUp() override final
        {
            using AzToolsFramework::GetEntityContextId;
            using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

            Base::SetUp();

            if (!GetApplication())
            {
                // Create & Start a new ToolsApplication if there's no existing one
                m_app = CreateTestApplication();
                AZ::ComponentApplication::StartupParameters startupParameters;
                startupParameters.m_loadAssetCatalog = false;
                startupParameters.m_loadSettingsRegistry = false;
                m_app->Start(AzFramework::Application::Descriptor(), startupParameters);
            }

            // without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            m_editorActions.Connect();

            const auto viewportHandlerBuilder =
                [this](const AzToolsFramework::EditorVisibleEntityDataCacheInterface* entityDataCache,
                    [[maybe_unused]] AzToolsFramework::ViewportEditorModeTrackerInterface* viewportEditorModeTracker)
            {
                // create the default viewport (handles ComponentMode)
                AZStd::unique_ptr<AzToolsFramework::EditorDefaultSelection> defaultSelection =
                    AZStd::make_unique<AzToolsFramework::EditorDefaultSelection>(entityDataCache, viewportEditorModeTracker);

                // override the phantom widget so we can use out custom test widget
                defaultSelection->SetOverridePhantomWidget(&m_editorActions.m_componentModeWidget);

                return defaultSelection;
            };

            // setup default editor interaction model with the phantom widget overridden
            EditorInteractionSystemViewportSelectionRequestBus::Event(
                GetEntityContextId(), &EditorInteractionSystemViewportSelectionRequestBus::Events::SetHandler,
                viewportHandlerBuilder);

            // We need to register the MainWindowActionContextIdentifier action context to a dummy QMainWindow for the Action Manager
            // so that any actions/shortcuts registered to it from tools will work. This is typically only initialized in the Editor itself,
            // so it doesn't get registered in the ToolsApplicationFixture, but some tools we are testing rely on it (e.g. viewport interactions).
            m_defaultMainWindow = new QMainWindow();

            auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
            auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();

            AzToolsFramework::ActionContextProperties contextProperties;
            contextProperties.m_name = "O3DE Editor";

            actionManagerInterface->RegisterActionContext(
                EditorIdentifiers::MainWindowActionContextIdentifier, contextProperties);

            hotKeyManagerInterface->AssignWidgetToActionContext(EditorIdentifiers::MainWindowActionContextIdentifier, m_defaultMainWindow);

            AzToolsFramework::ActionManagerSystemComponent::TriggerRegistrationNotifications();

            SetUpEditorFixtureImpl();
        }

        void TearDown() override final
        {
            using AzToolsFramework::GetEntityContextId;
            using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;
            
            // Reset the Action Manager between test runs.
            auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();

            hotKeyManagerInterface->RemoveWidgetFromActionContext(
                EditorIdentifiers::MainWindowActionContextIdentifier, m_defaultMainWindow);

            if (auto actionManagerInternalInterface = AZ::Interface<AzToolsFramework::ActionManagerInternalInterface>::Get())
            {
                actionManagerInternalInterface->Reset();
            }
            if (auto hotKeyManagerInternalInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInternalInterface>::Get())
            {
                hotKeyManagerInternalInterface->Reset();
            }
            if (auto menuManagerInternalInterface = AZ::Interface<AzToolsFramework::MenuManagerInternalInterface>::Get())
            {
                menuManagerInternalInterface->Reset();
            }
            if (auto toolBarManagerInternalInterface = AZ::Interface<AzToolsFramework::ToolBarManagerInternalInterface>::Get())
            {
                toolBarManagerInternalInterface->Reset();
            }

            // Reset back to Default Handler to prevent having a handler with dangling "this" pointer
            EditorInteractionSystemViewportSelectionRequestBus::Event(
                GetEntityContextId(), &EditorInteractionSystemViewportSelectionRequestBus::Events::SetDefaultHandler);

            TearDownEditorFixtureImpl();

            if (m_defaultMainWindow)
            {
                delete m_defaultMainWindow;
                m_defaultMainWindow = nullptr;
            }

            m_editorActions.Disconnect();

            // Stop & delete the Application created by this fixture, hence not using GetApplication() here
            if (m_app)
            {
                m_app->Stop();
                m_app.reset();
            }

            Base::TearDown();
        }

        QMainWindow* m_defaultMainWindow = nullptr;

        virtual void SetUpEditorFixtureImpl() {}
        virtual void TearDownEditorFixtureImpl() {}

        AzToolsFramework::ToolsApplication* GetApplication()
        {
            if (m_app)
            {
                return m_app.get();
            }

            // Fallback to ToolsApplication created externally
            AZ::ComponentApplication* app = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(app, &AZ::ComponentApplicationBus::Events::GetApplication);

            // It's a valid case if GetApplication returns null here, it means there's no externally created Application exists.
            auto* toolsApp = azdynamic_cast<AzToolsFramework::ToolsApplication*>(app);
            AZ_Assert(!app || toolsApp, "ComponentApplication must be the ToolsApplication here. Unsuccessful dynamic_cast.");
            return toolsApp;
        }

        //! It is possible to override this in classes deriving from ToolsApplicationFixture to provide alternate
        //! implementations of the DebugDisplayRequests interface (e.g. TestDebugDisplayRequests).
        virtual AZStd::shared_ptr<AzFramework::DebugDisplayRequests> CreateDebugDisplayRequests()
        {
            return AZStd::make_shared<NullDebugDisplayRequests>();
        }

    protected:
        TestEditorActions m_editorActions;
        ToolsApplicationMessageHandler m_messageHandler; // used to suppress trace messages in test output

        // Override this if your test fixture needs to use a custom TestApplication
        virtual AZStd::unique_ptr<ToolsTestApplication> CreateTestApplication()
        {
            return AZStd::make_unique<ToolsTestApplication>("ToolsApplication");
        }

    private:
        AZStd::unique_ptr<ToolsTestApplication> m_app;
    };

    class EditorEntityComponentChangeDetector
        : private AzToolsFramework::PropertyEditorEntityChangeNotificationBus::Handler
        , private AzToolsFramework::EditorTransformChangeNotificationBus::Handler
        , private AzToolsFramework::ToolsApplicationNotificationBus::Handler
    {
    public:
        explicit EditorEntityComponentChangeDetector(const AZ::EntityId entityId);
        ~EditorEntityComponentChangeDetector();

        bool ChangeDetected() const { return !m_componentIds.empty(); }
        bool PropertyDisplayInvalidated() const { return m_propertyDisplayInvalidated; }

        AZStd::vector<AZ::ComponentId> m_componentIds;
        AzToolsFramework::EntityIdList m_entityIds;

    private:
        // PropertyEditorEntityChangeNotificationBus ...
        void OnEntityComponentPropertyChanged(AZ::ComponentId componentId) override;

        // EditorTransformChangeNotificationBus ...
        void OnEntityTransformChanged(const AzToolsFramework::EntityIdList& entityIds) override;

        // ToolsApplicationNotificationBus ...
        void InvalidatePropertyDisplay(AzToolsFramework::PropertyModificationRefreshLevel level) override;

        bool m_propertyDisplayInvalidated = false;
    };

    // Moves the 'Editor' (ToolsApplication) into Component Mode for the given Component type.
    // @note Entities must be selected before using EnterComponentMode to ensure
    // Component Mode is entered correctly.
    template<typename ComponentT>
    void EnterComponentMode()
    {
        using AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus;
        ComponentModeSystemRequestBus::Broadcast(
            &ComponentModeSystemRequestBus::Events::AddSelectedComponentModesOfType,
            AZ::AzTypeInfo<ComponentT>::Uuid());
    }

    struct MockPerforceCommand
        : AzToolsFramework::PerforceCommand
    {
        void ExecuteCommand() override;

        void ExecuteRawCommand() override;

        bool m_persistFstatResponse = false;
        AZStd::string m_fstatResponse;
        AZStd::string m_fstatErrorResponse;
        AZStd::function<void(AZStd::string)> m_addCallback;
        AZStd::function<void(AZStd::string)> m_editCallback;
        AZStd::function<void(AZStd::string)> m_moveCallback;
        AZStd::function<void(AZStd::string)> m_deleteCallback;
    };

    struct MockPerforceConnection
        : AzToolsFramework::PerforceConnection
    {
        explicit MockPerforceConnection(MockPerforceCommand& command) : PerforceConnection(command)
        {
        }
    };

    void WaitForSourceControl(AZStd::binary_semaphore& waitSignal);

    struct SourceControlTest
        : AzToolsFramework::SourceControlNotificationBus::Handler
    {
        SourceControlTest();
        ~SourceControlTest();

        void ConnectivityStateChanged(const AzToolsFramework::SourceControlState state) override;
        void EnableSourceControl();

        bool m_connected = false;
        AZStd::binary_semaphore m_connectSignal;
        MockPerforceCommand m_command;
    };

    /// Create an Entity as it would appear in the Editor.
    /// Optional second parameter of Entity pointer if required.
    AZ::EntityId CreateDefaultEditorEntity(const char* name, AZ::Entity** outEntity = nullptr);

    /// Create a Layer Entity as it would appear in the Editor.
    /// Optional second parameter of Entity pointer if required.
    AZ::EntityId CreateEditorLayerEntity(const char* name, AZ::Entity** outEntity = nullptr);

    /// Create a component for a given entity if the component is missing.
    template<class ComponentType>
    void CreateComponentIfMissing(AZ::Entity* entity)
    {
        ASSERT_TRUE(entity != nullptr) << "Cannot create a component for a null entity.";

        if (!(entity->FindComponent<ComponentType>()))
        {
            entity->CreateComponent<ComponentType>();
        }
    }
    
    using SliceAssets = AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::SliceAsset>>;

    /// This function transfers the ownership of all the entity pointers - do not delete or use them afterwards.
    AZ::Data::AssetId SaveAsSlice(
        AZStd::vector<AZ::Entity*> entities, AzToolsFramework::ToolsApplication* toolsApplication,
        SliceAssets& inoutSliceAssets);

    /// Instantiate the entities from the saved slice asset.
    AZ::SliceComponent::EntityList InstantiateSlice(
        AZ::Data::AssetId sliceAssetId, const SliceAssets& sliceAssets);

    /// Destroy all the created slice assets.
    void DestroySlices(SliceAssets& sliceAssets);

    //! Child class of ViewportUiManager which exposes the protected button group and viewport display
    class ViewportUiManagerTestable : public AzToolsFramework::ViewportUi::ViewportUiManager
    {
    public:
        using ViewportUiDisplay = AzToolsFramework::ViewportUi::Internal::ViewportUiDisplay;
        using ButtonGroup = AzToolsFramework::ViewportUi::Internal::ButtonGroup;

        ViewportUiManagerTestable() = default;
        ~ViewportUiManagerTestable() override = default;

        const AZStd::unordered_map<AzToolsFramework::ViewportUi::ClusterId, AZStd::shared_ptr<ButtonGroup>>& GetClusterMap();

        ViewportUiDisplay* GetViewportUiDisplay();
    };

    //! Provides support for ViewportUi request calls.
    class ViewportManagerWrapper
    {
    public:
        void Create();
        void Destroy();

        ViewportUiManagerTestable* GetViewportManager();

        QWidget* GetMockRenderOverlay();

    private:
        AZStd::unique_ptr<ViewportUiManagerTestable> m_viewportManager;
        AZStd::unique_ptr<QWidget> m_parentWidget;
        AZStd::unique_ptr<QWidget> m_mockRenderOverlay;
    };

} // namespace UnitTest
