/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QSet>

#include "EditorModularViewportCameraComposer.h"
#include "EditorViewportSettings.h"
#include "Undo/Undo.h"
#include "Util/PredefinedAspectRatios.h"
#include "Viewport.h"

#include <Atom/RPI.Public/SceneBus.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Scene/SceneSystemInterface.h>
#include <AzFramework/Viewport/ViewportBus.h>
#include <AzFramework/Visibility/EntityVisibilityQuery.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <MathConversion.h>
#endif

// forward declarations.
class QMenu;
class QKeyEvent;
struct ray_hit;
struct IRenderMesh;
struct IVariable;

namespace AZ::ViewportHelpers
{
    class EditorEntityNotifications;
} // namespace AZ::ViewportHelpers

namespace AtomToolsFramework
{
    class RenderViewportWidget;
    class ModularViewportCameraController;
} // namespace AtomToolsFramework

namespace AzToolsFramework
{
    class ManipulatorManager;
}

//! Viewport settings for the EditorViewportWidget
struct EditorViewportSettings : public AzToolsFramework::ViewportInteraction::ViewportSettingsRequestBus::Handler
{
    void Connect(AzFramework::ViewportId viewportId);
    void Disconnect();

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
};

//! EditorViewportWidget window
AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
class SANDBOX_API EditorViewportWidget final
    : public QtViewport
    , public AzFramework::ViewportBorderRequestBus::Handler
    , private IEditorNotifyListener
    , private IUndoManagerListener
    , private Camera::EditorCameraRequestBus::Handler
    , private Camera::CameraNotificationBus::Handler
    , private AzFramework::InputSystemCursorConstraintRequestBus::Handler
    , private AzToolsFramework::ViewportInteraction::MainEditorViewportInteractionRequestBus::Handler
    , private AzToolsFramework::ViewportInteraction::EditorEntityViewportInteractionRequestBus::Handler
    , private AzFramework::AssetCatalogEventBus::Handler
    , private AZ::RPI::SceneNotificationBus::Handler
    , private AzToolsFramework::Prefab::PrefabPublicNotificationBus::Handler
{
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    Q_OBJECT

public:
    EditorViewportWidget(const QString& name, QWidget* parent = nullptr);
    ~EditorViewportWidget() override;

    static const GUID& GetClassID()
    {
        return QtViewport::GetClassID<EditorViewportWidget>();
    }

    static EditorViewportWidget* GetPrimaryViewport();

    // Used by ViewPan in some circumstances
    void ConnectViewportInteractionRequestBus();
    void DisconnectViewportInteractionRequestBus();

    // QtViewport/IDisplayViewport/CViewport
    // These methods are made public in the derived class because they are called with an object whose static type is known to be this class
    // type.
    void SetFOV(float fov) override;
    float GetFOV() const override;

    // AzFramework::ViewportBorderRequestBus overrides ...
    AZStd::optional<AzFramework::ViewportBorderPadding> GetViewportBorderPadding() const override;

private:
    ////////////////////////////////////////////////////////////////////////
    // Private types ...

    enum class PlayInEditorState
    {
        Editor,
        Starting,
        Started,
        Stopping
    };

    enum class KeyPressedState
    {
        AllUp,
        PressedThisFrame,
        PressedInPreviousFrame,
    };

    ////////////////////////////////////////////////////////////////////////
    // Method overrides ...

    // QWidget overrides ...
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool event(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

    // QtViewport/IDisplayViewport/CViewport overrides ...
    EViewportType GetType() const override
    {
        return ET_ViewportCamera;
    }

    void SetType([[maybe_unused]] EViewportType type) override
    {
        assert(type == ET_ViewportCamera);
    };

    AzToolsFramework::ViewportInteraction::MouseInteraction BuildMouseInteraction(
        Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, const QPoint& point) override;
    void SetViewportId(int id) override;
    QPoint WorldToView(const Vec3& wp) const override;
    Vec3 WorldToView3D(const Vec3& wp, int nFlags = 0) const override;
    Vec3 ViewToWorld(
        const QPoint& vp,
        bool* collideWithTerrain = nullptr,
        bool onlyTerrain = false,
        bool bSkipVegetation = false,
        bool bTestRenderMesh = false,
        bool* collideWithObject = nullptr) const override;
    void ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const override;
    Vec3 ViewToWorldNormal(const QPoint& vp, bool onlyTerrain, bool bTestRenderMesh = false) override;
    float GetScreenScaleFactor(const Vec3& worldPoint) const override;
    float GetAspectRatio() const override;
    bool HitTest(const QPoint& point, HitContext& hitInfo) override;
    bool IsBoundsVisible(const AABB& box) const override;
    void CenterOnAABB(const AABB& aabb) override;
    void OnTitleMenu(QMenu* menu) override;
    void SetViewTM(const Matrix34& tm) override;
    const Matrix34& GetViewTM() const override;
    void Update() override;
    void UpdateContent(int flags) override;

    // SceneNotificationBus overrides ...
    void OnBeginPrepareRender() override;

    // Camera::CameraNotificationBus overrides ...
    void OnActiveViewChanged(const AZ::EntityId&) override;

    // IEditorEventListener overrides ...
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    // Callback for setting modification
    void OnDefaultCameraNearFarChanged();

    // AzToolsFramework::EditorEntityContextNotificationBus overrides ...
    // note: handler moved to cpp to resolve link issues in unity builds
    void OnStartPlayInEditor();
    void OnStopPlayInEditor();
    void OnStartPlayInEditorBegin();

    // IUndoManagerListener
    void BeginUndoTransaction() override;
    void EndUndoTransaction() override;

    // AzFramework::InputSystemCursorConstraintRequestBus overrides ...
    void* GetSystemCursorConstraintWindow() const override;

    // AzToolsFramework::MainEditorViewportInteractionRequestBus overrides ...
    bool ShowingWorldSpace() override;
    QWidget* GetWidgetForViewportContextMenu() override;

    // EditorEntityViewportInteractionRequestBus overrides ...
    void FindVisibleEntities(AZStd::vector<AZ::EntityId>& visibleEntities) override;

    // Camera::EditorCameraRequestBus overrides ...
    void SetViewFromEntityPerspective(const AZ::EntityId& entityId) override;
    AZ::EntityId GetCurrentViewEntityId() override;
    bool GetActiveCameraPosition(AZ::Vector3& cameraPos) override;
    AZStd::optional<AZ::Transform> GetActiveCameraTransform() override;
    AZStd::optional<float> GetCameraFoV() override;
    bool GetActiveCameraState(AzFramework::CameraState& cameraState) override;

    // AzToolsFramework::Prefab::PrefabPublicNotificationBus overrides ...
    void OnRootPrefabInstanceLoaded() override;

    ////////////////////////////////////////////////////////////////////////
    // Private helpers...
    void SetDefaultCameraNearFar();
    void RenderAll();

    bool RayRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const Vec3& vInDir, Vec3& vOutPos, Vec3& vOutNormal) const;

    bool AddCameraMenuItems(QMenu* menu);
    void ResizeView(int width, int height);

    void HideCursor();
    void ShowCursor();

    double WidgetToViewportFactor() const;

    bool ShouldPreviewFullscreen() const;
    void StartFullscreenPreview();
    void StopFullscreenPreview();

    void OnMenuCreateCameraEntityFromCurrentView();

    // From a series of input primitives, compose a complete mouse interaction.
    AzToolsFramework::ViewportInteraction::MouseInteraction BuildMouseInteractionInternal(
        AzToolsFramework::ViewportInteraction::MouseButtons buttons,
        AzToolsFramework::ViewportInteraction::KeyboardModifiers modifiers,
        const AzToolsFramework::ViewportInteraction::MousePick& mousePick) const;

    // Given a point in the viewport, return the pick ray into the scene.
    // note: The argument passed to parameter **point**, originating
    // from a Qt event, must first be passed to WidgetToViewport before being
    // passed to BuildMousePick.
    AzToolsFramework::ViewportInteraction::MousePick BuildMousePick(const QPoint& point) const;

    bool CheckRespondToInput() const;

    void BuildDragDropContext(
        AzQtComponents::ViewportDragContext& context, AzFramework::ViewportId viewportId, const QPoint& point) override;

    void SetAsActiveViewport();
    void PushDisableRendering();
    void PopDisableRendering();
    bool IsRenderingDisabled() const;

    void RestoreViewportAfterGameMode();

    void UpdateScene();

    void SetDefaultCamera();
    void SetSelectedCamera();
    bool IsSelectedCamera() const;
    void SetEntityAsCamera(const AZ::EntityId& entityId);
    void SetFirstComponentCamera();
    void PostCameraSet();
    // This switches the active camera to the next one in the list of (default, all custom cams).
    void CycleCamera();

    QPoint WidgetToViewport(const QPoint& point) const;
    QPoint ViewportToWidget(const QPoint& point) const;
    QSize WidgetToViewport(const QSize& size) const;

    void UnProjectFromScreen(float sx, float sy, float* px, float* py, float* pz) const;
    void ProjectToScreen(float ptx, float pty, float ptz, float* sx, float* sy) const;

    AZ::RPI::ViewPtr GetCurrentAtomView() const;

    ////////////////////////////////////////////////////////////////////////
    // Members ...
    friend class AZ::ViewportHelpers::EditorEntityNotifications;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING

    // Singleton for the primary viewport
    static EditorViewportWidget* m_pPrimaryViewport;

    // The simulation (play-game in editor) state
    PlayInEditorState m_playInEditorState = PlayInEditorState::Editor;

    // Whether we are doing a full screen game preview (play-game in editor) or a regular one
    bool m_inFullscreenPreview = false;

    // The entity ID of the current camera for this viewport, or invalid if the default editor camera
    AZ::EntityId m_viewEntityId;

    // During play game in editor, holds the editor entity ID of the last
    AZ::EntityId m_viewEntityIdCachedForEditMode;

    // The editor camera TM before switching to game mode
    Matrix34 m_preGameModeViewTM;

    // Disables rendering during some periods of time, e.g. undo/redo, resize events
    uint m_disableRenderingCount = 0;

    // Determines if the viewport needs updating (false when out of focus for example)
    bool m_bUpdateViewport = false;

    // Avoid re-entering PostCameraSet->OnActiveViewChanged->PostCameraSet
    bool m_sendingOnActiveChanged = false;

    // Legacy...
    KeyPressedState m_pressedKeyState = KeyPressedState::AllUp;

    // The name to use for the default editor camera
    const QString m_defaultViewName;

    // Reentrancy guard for on paint events
    bool m_isOnPaint = false;

    // Guard against calling UpdateVisibility multiple times a frame
    bool m_hasUpdatedVisibility = false;

    // Aspect ratios available in the title bar
    CPredefinedAspectRatios m_predefinedAspectRatios;

    // Is the cursor hidden or displayed?
    bool m_bCursorHidden = false;

    // Shim for QtViewport, which used to be responsible for visibility queries in the editor,
    // these are now forwarded to EntityVisibilityQuery
    AzFramework::EntityVisibilityQuery m_entityVisibilityQuery;

    // Handlers for snapping/editor event callbacks
    SandboxEditor::AngleSnappingChangedEvent::Handler m_angleSnappingHandler;
    SandboxEditor::CameraSpeedScaleChangedEvent::Handler m_cameraSpeedScaleHandler;
    SandboxEditor::GridShowingChangedEvent::Handler m_gridShowingHandler;
    SandboxEditor::GridSnappingChangedEvent::Handler m_gridSnappingHandler;
    SandboxEditor::NearFarPlaneChangedEvent::Handler m_nearPlaneDistanceHandler;
    SandboxEditor::NearFarPlaneChangedEvent::Handler m_farPlaneDistanceHandler;
    SandboxEditor::PerspectiveChangedEvent::Handler m_perspectiveChangeHandler;
    AZStd::unique_ptr<SandboxEditor::EditorViewportSettingsCallbacks> m_editorViewportSettingsCallbacks;

    // Used for some legacy logic which lets the widget release a grabbed keyboard at the right times
    // Unclear if it's still necessary.
    QSet<int> m_keyDown;

    // This widget holds a reference to the manipulator manage because its responsible for drawing manipulators
    AZStd::shared_ptr<AzToolsFramework::ManipulatorManager> m_manipulatorManager;

    AZStd::unique_ptr<SandboxEditor::EditorModularViewportCameraComposer> m_editorModularViewportCameraComposer;

    // Helper for getting EditorEntityNotificationBus events
    AZStd::unique_ptr<AZ::ViewportHelpers::EditorEntityNotifications> m_editorEntityNotifications;

    // The widget to which Atom will actually render
    AtomToolsFramework::RenderViewportWidget* m_renderViewport = nullptr;

    // Atom debug display
    AzFramework::DebugDisplayRequests* m_debugDisplay = nullptr;

    // Type to return current state of editor viewport settings
    EditorViewportSettings m_editorViewportSettings;

    // DO NOT USE THIS! It exists only to satisfy the signature of the base class method GetViewTm
    mutable Matrix34 m_viewTmStorage;

    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};
