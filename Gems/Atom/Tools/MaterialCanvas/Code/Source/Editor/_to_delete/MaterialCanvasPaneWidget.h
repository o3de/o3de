/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Window/MaterialCanvasMainWindow.h>

namespace MaterialCanvas
{
    //! The Editor-hosted form of MaterialCanvasMainWindow.
    //!
    //! AzToolsFramework::RegisterViewPane<T> constructs the pane widget itself and can only pass a parent, but
    //! MaterialCanvasMainWindow takes (toolId, graphViewSettings, parent). This subclass exists purely to bridge that gap: it
    //! pulls the tool id and graph view settings from MaterialCanvasEditorSystemComponent and forwards them to the base.
    //! Landscape Canvas solves the same problem the same way, with an `explicit MainWindow(QWidget* parent = nullptr)`.
    //!
    //! It also tells the system component when it comes and goes, because the document type view factories need the window
    //! that owns the document tabs and that window only exists while the pane is open.
    //!
    //! MaterialCanvasMainWindow derives from AzQtComponents::DockMainWindow, which is a QMainWindow. Qt permits a QMainWindow
    //! as a child widget, which is what makes hosting the existing window in a pane possible without rewriting it.
    class MaterialCanvasPaneWidget : public MaterialCanvasMainWindow
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialCanvasPaneWidget, AZ::SystemAllocator);

        using Base = MaterialCanvasMainWindow;

        explicit MaterialCanvasPaneWidget(QWidget* parent = nullptr);
        ~MaterialCanvasPaneWidget() override;
    };
} // namespace MaterialCanvas
