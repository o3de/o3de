/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/EditorDockWidgetBus.h>
#endif

namespace Ui
{
    class GraphCanvasEditorDockWidget;
}

namespace GraphCanvas
{
    class GraphCanvasGraphicsView;

    class EditorDockWidget
        : public AzQtComponents::StyledDockWidget
        , public ActiveEditorDockWidgetRequestBus::Handler
        , public AssetEditorNotificationBus::Handler
        , public EditorDockWidgetRequestBus::Handler
    {
        Q_OBJECT

        friend class AssetEditorCentralDockWindow;
    public:    
        AZ_CLASS_ALLOCATOR(EditorDockWidget,AZ::SystemAllocator,0);
        EditorDockWidget(const EditorId& editorId, const QString& title, QWidget* parent);
        ~EditorDockWidget();

        // ActiveEditorDockWidgetRequestBus
        DockWidgetId GetDockWidgetId() const override;

        void ReleaseBus() override;
        ////

        const EditorId& GetEditorId() const;

        // GraphCanvasEditorDockWidgetBus
        AZ::EntityId GetViewId() const override;
        GraphCanvas::GraphId GetGraphId() const override;
        EditorDockWidget* AsEditorDockWidget() override;
        void SetTitle(const AZStd::string& title) override;
        ////

    signals:
        void OnEditorClosed(EditorDockWidget*);

    protected:
        GraphCanvasGraphicsView* GetGraphicsView() const;

    private:
        void closeEvent(QCloseEvent* closeEvent) override;
        void SignalActiveEditor();

        DockWidgetId m_dockWidgetId;
        EditorId m_editorId;
        GraphId m_graphId;
        AZ::Entity* m_sceneEntity = nullptr;

        AZ::EntityId m_assetId;

        AZStd::unique_ptr<Ui::GraphCanvasEditorDockWidget> m_ui;
    };
}
