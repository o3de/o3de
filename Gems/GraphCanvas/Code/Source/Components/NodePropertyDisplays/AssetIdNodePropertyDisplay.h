/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplay/AssetIdDataInterface.h>

class QGraphicsProxyWidget;

namespace GraphCanvas
{
    class GraphCanvasLabel;

    class AssetIdNodePropertyDisplay
        : public NodePropertyDisplay
    {

    public:
        AZ_CLASS_ALLOCATOR(AssetIdNodePropertyDisplay, AZ::SystemAllocator);
        AssetIdNodePropertyDisplay(AssetIdDataInterface* dataInterface);
        virtual ~AssetIdNodePropertyDisplay();

        // NodePropertyDisplay
        void RefreshStyle() override;
        void UpdateDisplay() override;

        QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() override;
        ////

        // DataSlotNotifications
        void OnDragDropStateStateChanged(const DragDropState& dragState) override;
        ////

    private:

        void EditStart();
        void EditFinished();
        void SubmitValue();
        void SetupProxyWidget();
        void CleanupProxyWidget();

        AssetIdDataInterface*  m_dataInterface;

        AzToolsFramework::PropertyAssetCtrl*        m_propertyAssetCtrl;
        GraphCanvasLabel*                           m_disabledLabel;
        QGraphicsProxyWidget*                       m_proxyWidget;
        GraphCanvasLabel*                           m_displayLabel;
    };
}
