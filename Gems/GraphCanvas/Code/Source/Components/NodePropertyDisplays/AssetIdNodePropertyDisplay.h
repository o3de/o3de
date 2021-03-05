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
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplay/AssetIdDataInterface.h>
#endif

class QGraphicsProxyWidget;

namespace GraphCanvas
{
    class GraphCanvasLabel;

    class AssetIdNodePropertyDisplay
        : public NodePropertyDisplay
    {

    public:
        AZ_CLASS_ALLOCATOR(AssetIdNodePropertyDisplay, AZ::SystemAllocator, 0);
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
