/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include "EditorCommon.h"
#include "LyShinePassDataBus.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

#include <IFont.h>

#include <QTimer>
#endif

class RulerWidget;
class QMimeData;
class UiRenderer;
class CDraw2d;

class ViewportWidget
    : public AtomToolsFramework::RenderViewportWidget
    , private AzToolsFramework::EditorPickModeNotificationBus::Handler
    , private FontNotificationBus::Handler
    , private LyShinePassDataRequestBus::Handler
    , public AZ::RPI::ViewportContextNotificationBus::Handler
{
    Q_OBJECT

public: // types

    enum DrawElementBorders
    {
        DrawElementBorders_Unselected = 0x1,
        DrawElementBorders_Visual     = 0x2,
        DrawElementBorders_Parent     = 0x4,
        DrawElementBorders_Hidden     = 0x8,
    };

public: // member functions

    explicit ViewportWidget(EditorWindow* parent);
    virtual ~ViewportWidget();

    ViewportInteraction* GetViewportInteraction();

    bool IsDrawingElementBorders(uint32 flags) const;
    void ToggleDrawElementBorders(uint32 flags);

    void ActiveCanvasChanged();
    void EntityContextChanged();

    //! Flags the viewport display as needing a refresh
    void Refresh();

    //! Used to clear the viewport and prevent rendering until the viewport layout updates
    void ClearUntilSafeToRedraw();

    //! Set whether to render the canvas
    void SetRedrawEnabled(bool enabled);

    //! Get the canvas scale factor being used for the preview mode
    float GetPreviewCanvasScale() { return m_previewCanvasScale; }

    //! Used by ViewportInteraction for drawing
    ViewportHighlight* GetViewportHighlight() { return m_viewportHighlight.get(); }

    bool IsInObjectPickMode() { return m_inObjectPickMode; }
    void PickItem(AZ::EntityId entityId);

    QWidget* CreateViewportWithRulersWidget(QWidget* parent);
    void ShowRulers(bool show);
    bool AreRulersShown() { return m_rulersVisible; }
    void RefreshRulers();
    void SetRulerCursorPositions(const QPoint& globalPos);

    void ShowGuides(bool show);
    bool AreGuidesShown() { return m_guidesVisible; }

    void InitUiRenderer();

protected:

    void contextMenuEvent(QContextMenuEvent* e) override;

private slots:

    void UserSelectionChanged(HierarchyItemRawPtrList* items);

    void EnableCanvasRender();

    //! Called by a timer at the max frequency that we want to refresh the display
    void RefreshTick();

protected:

    //! Forwards mouse press events to ViewportInteraction.
    //!
    //! Event is NOT propagated to parent class.
    void mousePressEvent(QMouseEvent* ev) override;

    //! Forwards mouse move events to ViewportInteraction.
    //!
    //! Event is NOT propagated to parent class.
    void mouseMoveEvent(QMouseEvent* ev) override;

    //! Forwards mouse release events to ViewportInteraction.
    //!
    //! Event is NOT propagated to parent class.
    void mouseReleaseEvent(QMouseEvent* ev) override;

    //! Forwards mouse wheel events to ViewportInteraction.
    //!
    //! Event is propagated to parent class.
    void wheelEvent(QWheelEvent* ev) override;

    //! Prevents shortcuts from interfering with preview mode.
    bool eventFilter(QObject* watched, QEvent* event) override;

    //! Handle events from Qt.
    bool event(QEvent* ev) override;

    //! Key press event from Qt.
    void keyPressEvent(QKeyEvent* event) override;

    //! Key release event from Qt.
    void keyReleaseEvent(QKeyEvent* event) override;

    void focusOutEvent(QFocusEvent* ev) override;

private: // member functions
    // EditorPickModeNotificationBus
    void OnEntityPickModeStarted() override;
    void OnEntityPickModeStopped() override;

    // FontNotifications
    void OnFontsReloaded() override;
    void OnFontTextureUpdated(IFFont* font) override;
    // ~FontNotifications

    // LyShinePassDataRequestBus
    LyShine::AttachmentImagesAndDependencies GetRenderTargets() override;
    // ~LyShinePassDataRequestBus

    // AZ::TickBus::Handler
    void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    int GetTickOrder() override;
    // ~AZ::TickBus::Handler

    // AZ::RPI::ViewportContextNotificationBus::Handler overrides...
    void OnRenderTick() override;

    //! Update UI canvases when in edit mode
    void UpdateEditMode(float deltaTime);

    //! Render the viewport when in edit mode
    void RenderEditMode();

    //! Update UI canvases when in preview mode
    void UpdatePreviewMode(float deltaTime);

    //! Render the viewport when in preview mode
    void RenderPreviewMode();

    //! Fill the entire viewport area with a background color
    void RenderViewportBackground();

    //! Create shortcuts for manipulating the viewport
    void SetupShortcuts();

    //! Do the Qt stuff to hide/show the rulers
    void ApplyRulerVisibility();

private: // data

    void resizeEvent(QResizeEvent* ev) override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    bool AcceptsMimeData(const QMimeData* mimeData);

    double WidgetToViewportFactor() const
    {
        // Needed for high DPI mode on windows
        return devicePixelRatioF();
    }

    QPointF WidgetToViewport(const QPointF &point) const;

    EditorWindow* m_editorWindow;

    std::unique_ptr< ViewportInteraction > m_viewportInteraction;
    std::unique_ptr< ViewportAnchor > m_viewportAnchor;
    std::unique_ptr< ViewportHighlight > m_viewportHighlight;
    std::unique_ptr< ViewportCanvasBackground > m_viewportBackground;
    std::unique_ptr< ViewportPivot > m_viewportPivot;

    uint32 m_drawElementBordersFlags;
    bool m_refreshRequested;
    bool m_canvasRenderIsEnabled;
    QTimer m_updateTimer;

    float m_previewCanvasScale;

    bool m_inObjectPickMode = false;

    RulerWidget* m_rulerHorizontal = nullptr;
    RulerWidget* m_rulerVertical = nullptr;
    QWidget* m_rulerCorner = nullptr;
    bool     m_rulersVisible;
    bool     m_guidesVisible;
    bool     m_fontTextureHasChanged = false;

    AZStd::shared_ptr<UiRenderer> m_uiRenderer;
    AZStd::shared_ptr<CDraw2d> m_draw2d;
};
