/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option")
#include <QWidget>
AZ_POP_DISABLE_WARNING

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Component/EntityId.h>
#include <Debugger/Bus.h>
#include <AzCore/Asset/AssetCommon.h>

#include <GraphCanvas/Components/ViewBus.h>
#endif

class QVBoxLayout;

namespace Ui
{
    class CanvasWidget;
}

namespace GraphCanvas
{
    class GraphCanvasGraphicsView;
    class MiniMapGraphicsView;
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class CanvasWidget
            : public QWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(CanvasWidget, AZ::SystemAllocator, 0);
            CanvasWidget(const AZ::Data::AssetId& assetId, QWidget* parent = nullptr);
            ~CanvasWidget() override;

            void SetDefaultBorderColor(AZ::Color defaultBorderColor);
            void ShowScene(const ScriptCanvas::ScriptCanvasId& scriptCanvasId);

            void SetAssetId(const AZ::Data::AssetId& assetId);

            const GraphCanvas::ViewId& GetViewId() const;

            void EnableView();
            void DisableView();

        protected:

            void resizeEvent(QResizeEvent *ev) override;

            void OnClicked();

            bool m_attached;

            void SetupGraphicsView();
            
            AZ::Data::AssetId m_assetId;

            AZStd::unique_ptr<Ui::CanvasWidget> ui;

            void showEvent(QShowEvent *event) override;

        private:

            enum MiniMapPosition
            {
                MM_Not_Visible,
                MM_Upper_Left,
                MM_Upper_Right,
                MM_Lower_Right,
                MM_Lower_Left,

                MM_Position_Count
            };

            void PositionMiniMap();

            AZ::Color m_defaultBorderColor;

            ScriptCanvas::ScriptCanvasId m_scriptCanvasId;

            GraphCanvas::GraphCanvasGraphicsView* m_graphicsView;
            GraphCanvas::MiniMapGraphicsView* m_miniMapView;
            MiniMapPosition m_miniMapPosition = MM_Upper_Left;

            GraphCanvas::GraphicsEffectId m_disabledOverlay;
        };
    }
}
