#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QWidget>
#include <QStackedLayout>
#include <QString>
#include <QVector>
#include <AzCore/std/functional.h>
#include <QPointer>
#endif

class QDockWidget;

namespace AzQtComponents
{
    namespace Internal
    {
        class OverlayWidgetLayer;
    } // namespace Internal

    struct OverlayWidgetButton
    {
        // Optional function to call when the button is pressed
        using Callback = AZStd::function<void()>;

        // Optional status check for the button. If this returns false,
        //      the button will be disabled. A disabled button that's used for the close
        //      (cross) button of a breakout window will also prevent the window from
        //      being closed. This check fires in response to a Refresh call (see below).
        using EnabledCheck = AZStd::function<bool()>;

        QString m_text;
        Callback m_callback;
        EnabledCheck m_enabledCheck;

        // If true, pressing this button will pop the layer and close it's breakout window.
        bool m_triggersPop = false;

        // The first button with this flag will also be attached to the close (cross) button
        //      of the breakout window. This will make the close button use the callback and/or
        //      the enabled check of the button to be used.
        bool m_isCloseButton = false;
    };

    using OverlayWidgetButtonList = QVector<const OverlayWidgetButton*>;

    // Widget that allows several layers to be stacked on top of the root widget it will display when there are no layers.
    //      Each layer can also have an associated window attached to it that, when closed will also pop the layer. This
    //      widget can be used for instance to block a part of the UI while interacting with the popped up window, resulting in
    //      pseudo modal windows. Layers are stacked allowing lower layers to still be visible and by default they darken the
    //      lower layers.
    class AZ_QT_COMPONENTS_API OverlayWidget : public QWidget
    {
        friend class Internal::OverlayWidgetLayer;

        Q_OBJECT
    public:
        static const int s_invalidOverlayIndex;

        explicit OverlayWidget(QWidget* parent);
        ~OverlayWidget();

        // Sets the widget that will be shown when there are no layers active.
        void SetRoot(QWidget* root);

        // Push a new layer on top of the layer widget, with the provided widgets and buttons.
        //      centerWidget:   Widget that will be shown as part of the overlay stack. If null the overlay will be faded out and disabled.
        //      breakoutWidget: If not null, a window will be shown containing this widget and stealing the provided buttons.
        //      title:          Name for the layer, used as the title for the breakout window.
        //      buttons:        Buttons that will be shown as part of the overlay or the breakout window.
        int PushLayer(QWidget* centerWidget, QWidget* breakoutWidget, const char* title, const OverlayWidgetButtonList& buttons);
        // Remove the top layer from the stack.
        bool PopLayer();
        // Remove a specific layer from that stack. The layerId will be returned from PushLayer.
        bool PopLayer(int layerId);
        // Removes all layers.
        bool PopAllLayers();
        // Refreshes the status of all layers, causing a check of their enabled state.
        void RefreshAll();
        // Refreshes the status of a specific layer, causing a check of its enabled state.
        void RefreshLayer(int layerId);

        // Whether or not there are any layers added or the root widget is showing.
        bool IsAtRoot() const;
        // Whether or not there are any layers open that currently can't be closed.
        bool CanClose() const;

        // Push a new layer on top of the given layer widget, with the provided widgets and buttons.
        //      centerWidget:   Widget that will be shown as part of the overlay stack. If null the overlay will be faded out and disabled.
        //      breakoutWidget: If not null, a window will be shown containing this widget and stealing the provided buttons.
        //      title:          Name for the layer, used as the title for the breakout window.
        //      buttons:        Buttons that will be shown as part of the overlay or the breakout window.
        static int PushLayerToOverlay(OverlayWidget* overlay, QWidget* centerWidget, QWidget* breakoutWidget,
            const char* title, const OverlayWidgetButtonList& buttons);
        static int PushLayerToContainingOverlay(QWidget* overlayChild, QWidget* centerWidget, QWidget* breakoutWidget,
            const char* title, const OverlayWidgetButtonList& buttons);

        // Searches for the closest enclosing OverlayWidget the given widget is a child of. This can be used to find the appropriate
        //      widget to push a layer to if the containing OverlayWidget is not known.
        static OverlayWidget* GetContainingOverlay(QWidget* overlayChild);

    Q_SIGNALS:
        void LayerAdded(int layerId);
        void LayerRemoved(int layerId);

    protected:
        struct LayerInfo
        {
            Internal::OverlayWidgetLayer* m_layer;
            // Unique number to identify the layer by.
            int m_layerId;
            // The index of the layer on the stack.
            int m_layerIndex;
        };

        int PushLayer(Internal::OverlayWidgetLayer* layer);
        bool PopLayer(Internal::OverlayWidgetLayer* layer);
        void OnStackEntryRemoved(int index);

        static int PushLayer(OverlayWidget* targetOverlay, Internal::OverlayWidgetLayer* layer, bool hasBreakoutWindow);

        int m_layerIdCounter;
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QVector<LayerInfo> m_layers;
        QStackedLayout* m_stack;
        int m_rootIndex;

        Qt::DockWidgetAreas m_originalDockWidgetAreas;
        QPointer<QDockWidget> m_parentDockWidget;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    };
} // namespace AzQtComponents

