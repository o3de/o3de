/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/ButtonGroup.h>
#include <AzToolsFramework/ViewportUi/TextField.h>
#include <AzToolsFramework/ViewportUi/ViewportUiDisplayLayout.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QToolButton>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QGridLayout>
AZ_POP_DISABLE_WARNING

class QPoint;

namespace AzToolsFramework::ViewportUi::Internal
{
    //! Used to track info for each widget in the Viewport UI.
    struct ViewportUiElementInfo
    {
        AZStd::shared_ptr<QWidget> m_widget; //!< Reference to the widget.
        ViewportUiElementId m_viewportUiElementId; //!< Corresponding ViewportUiElementId of the widget.
        bool m_anchored = true; //!< Whether the widget is anchored to one position or moves with camera/entity.
        AZ::Vector3 m_worldPosition; //!< If not anchored, use this to project widget position to screen space.

        bool IsValid() const
        {
            return m_viewportUiElementId != InvalidViewportUiElementId;
        }
    };

    using ViewportUiElementIdInfoLookup = AZStd::unordered_map<ViewportUiElementId, ViewportUiElementInfo>;

    //! Helper function to give a widget a transparent background
    void SetTransparentBackground(QWidget* widget);

    //! Creates a transparent widget over a viewport render overlay, and adds/manages other Qt widgets
    //! to display on top of the viewport.
    class ViewportUiDisplay
    {
    public:
        ViewportUiDisplay(QWidget* parent, QWidget* renderOverlay);
        ~ViewportUiDisplay();

        void AddCluster(AZStd::shared_ptr<ButtonGroup> buttonGroup, Alignment alignment);
        void AddClusterButton(ViewportUiElementId clusterId, Button* button);
        void SetClusterButtonLocked(ViewportUiElementId clusterId, ButtonId buttonId, bool isLocked);
        void SetClusterButtonTooltip(ViewportUiElementId clusterId, ButtonId buttonId, const AZStd::string& tooltip);
        void RemoveClusterButton(ViewportUiElementId clusterId, ButtonId buttonId);
        void UpdateCluster(const ViewportUiElementId clusterId);

        void AddSwitcher(AZStd::shared_ptr<ButtonGroup> buttonGroup, Alignment alignment);
        void AddSwitcherButton(ViewportUiElementId switcherId, Button* button);
        void RemoveSwitcherButton(ViewportUiElementId switcherId, ButtonId buttonId);
        void UpdateSwitcher(ViewportUiElementId switcherId);
        void SetSwitcherActiveButton(ViewportUiElementId switcherId, ButtonId buttonId);

        void AddTextField(AZStd::shared_ptr<TextField> textField);
        void UpdateTextField(ViewportUiElementId textFieldId);

        //! After removing, can no longer be accessed by its ViewportUiElementId unless it is re-added.
        void RemoveViewportUiElement(ViewportUiElementId elementId);

        //! Moves the Viewport UI over the Render Overlay, projects new positions of non-anchored elements,
        //! and sets Viewport UI geometry to include only areas populated by Viewport UI Elements.
        void Update();

        const QMainWindow* GetUiMainWindow() const;
        const QWidget* GetUiOverlay() const;
        const QGridLayout* GetUiOverlayLayout() const;

        //! Initializes UI main window and overlay by setting attributes such as transparency and visibility.
        void InitializeUiOverlay();

        void ShowViewportUiElement(ViewportUiElementId elementId);
        void HideViewportUiElement(ViewportUiElementId elementId);

        AZStd::shared_ptr<QWidget> GetViewportUiElement(ViewportUiElementId elementId);
        bool IsViewportUiElementVisible(ViewportUiElementId elementId);

        void CreateViewportBorder(const AZStd::string& borderTitle, AZStd::optional<ViewportUiBackButtonCallback> backButtonCallback);
        void RemoveViewportBorder();

    private:
        void PrepareWidgetForViewportUi(QPointer<QWidget> widget);

        ViewportUiElementId AddViewportUiElement(AZStd::shared_ptr<QWidget> widget);
        ViewportUiElementId GetViewportUiElementId(QPointer<QWidget> widget);

        void PositionViewportUiElementFromWorldSpace(ViewportUiElementId elementId, const AZ::Vector3& pos);
        void PositionViewportUiElementAnchored(ViewportUiElementId elementId, const Qt::Alignment alignment);
        void PositionUiOverlayOverRenderViewport();

        bool UiDisplayEnabled() const;
        void SetUiOverlayContents(QPointer<QWidget> widget);
        void SetUiOverlayContentsAnchored(QPointer<QWidget>, Qt::Alignment aligment);
        void UpdateUiOverlayGeometry();

        ViewportUiElementInfo GetViewportUiElementInfo(const ViewportUiElementId elementId);

        QMainWindow m_uiMainWindow; //!< The window which contains the UI Overlay.
        QWidget m_uiOverlay; //!< The UI Overlay which displays Viewport UI Elements.
        QGridLayout m_fullScreenLayout; //!< The layout which extends across the full screen.
        ViewportUiDisplayLayout m_uiOverlayLayout; //!< The layout used for optionally anchoring Viewport UI Elements.
        QLabel m_viewportBorderText; //!< The text used for the viewport highlight border.
        QToolButton m_viewportBorderBackButton; //!< The button to return from the viewport highlight border (only displayed if callback provided).
        //! The optional callback for when the viewport highlight border back button is pressed.
        AZStd::optional<ViewportUiBackButtonCallback> m_viewportBorderBackButtonCallback;

        QWidget* m_renderOverlay;
        QPointer<QWidget> m_fullScreenWidget; //!< Reference to the widget attached to m_fullScreenLayout if any.
        int64_t m_numViewportElements = 0;
        int m_viewportId = 0;

        ViewportUiElementIdInfoLookup m_viewportUiElements;
    };
} // namespace AzToolsFramework::ViewportUi::Internal
