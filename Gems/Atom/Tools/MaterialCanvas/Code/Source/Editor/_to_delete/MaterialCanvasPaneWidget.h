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
    //! Editor-hosted MaterialCanvasMainWindow: supplies the tool id and graph view settings RegisterViewPane can't pass.
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
