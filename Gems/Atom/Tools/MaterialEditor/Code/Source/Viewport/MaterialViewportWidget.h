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
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzFramework/Windowing/WindowBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QWidget>
#include <QAbstractNativeEventFilter>
AZ_POP_DISABLE_WARNING
#endif

#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>

namespace Ui
{
    class MaterialViewportWidget;
}

namespace AZ
{
    namespace RPI
    {
        class WindowContext;
    }
}

namespace MaterialEditor
{
    class MaterialViewportRenderer;

    class MaterialViewportWidget
        : public AtomToolsFramework::RenderViewportWidget
        , public QAbstractNativeEventFilter
    {
    public:
        MaterialViewportWidget(QWidget* parent = nullptr);

        QScopedPointer<Ui::MaterialViewportWidget> m_ui;
        AZStd::unique_ptr<MaterialViewportRenderer> m_renderer;

        bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;
    };
} // namespace MaterialEditor
