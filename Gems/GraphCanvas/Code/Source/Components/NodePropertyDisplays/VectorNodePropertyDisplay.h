/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QEvent>
#include <QGraphicsWidget>
#include <QObject>

#include <AzQtComponents/Components/Widgets/VectorInput.h>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplay/VectorDataInterface.h>

#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#endif

class QGraphicsPixmapItem;
class QGraphicsLinearLayout;
class QPixmap;

namespace GraphCanvas
{
    class GraphCanvasLabel;
    class VectorNodePropertyDisplay;

    class VectorEventFilter
        : public QObject
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(VectorEventFilter, AZ::SystemAllocator);
        VectorEventFilter(VectorNodePropertyDisplay* owner);
        ~VectorEventFilter() = default;

        bool eventFilter(QObject *object, QEvent *event) override;

    private:
        VectorNodePropertyDisplay* m_owner;
    };
    
    class IconLayoutItem
        : public QGraphicsWidget{
    public:
        AZ_CLASS_ALLOCATOR(IconLayoutItem, AZ::SystemAllocator);
        IconLayoutItem(QGraphicsItem *parent = nullptr);
        ~IconLayoutItem();

        void setIcon(const QPixmap& pixmap);
    private:
        QGraphicsPixmapItem* m_pixmap;
    };

    class ReadOnlyVectorControl
        : public QGraphicsWidget
    {
    public:
        AZ_CLASS_ALLOCATOR(ReadOnlyVectorControl, AZ::SystemAllocator);
        ReadOnlyVectorControl(int index, const VectorDataInterface& dataInterface);
        ~ReadOnlyVectorControl();
        
        void RefreshStyle(const AZ::EntityId& sceneId);
        void UpdateDisplay();

        int GetIndex() const;

        GraphCanvasLabel* GetTextLabel();
        const GraphCanvasLabel* GetTextLabel() const;
        
        GraphCanvasLabel* GetValueLabel();
        const GraphCanvasLabel* GetValueLabel() const;
    
    private:    
        QGraphicsLinearLayout* m_layout;
        
        GraphCanvasLabel* m_textLabel;
        GraphCanvasLabel* m_valueLabel;
        
        int m_index;
        
        const VectorDataInterface& m_dataInterface;
    };
    
    class VectorNodePropertyDisplay
        : public NodePropertyDisplay
    {
        friend class VectorEventFilter;

    public:
        AZ_CLASS_ALLOCATOR(VectorNodePropertyDisplay, AZ::SystemAllocator);
        VectorNodePropertyDisplay(VectorDataInterface* dataInterface);
        virtual ~VectorNodePropertyDisplay();
    
        // DataSource
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
        void SetupProxyWidget();
        void CleanupProxyWidget();
        
        void SubmitValue();
    
        Styling::StyleHelper m_styleHelper;
        VectorDataInterface* m_dataInterface{};

        QWidget* m_widgetContainer{};

        GraphCanvasLabel* m_disabledLabel{};
        AzQtComponents::VectorInput* m_propertyVectorCtrl{};
        QToolButton* m_button{};
        QGraphicsProxyWidget* m_proxyWidget{};

        QGraphicsWidget* m_displayWidget{};
        IconLayoutItem* m_iconDisplay{};
        AZStd::vector<ReadOnlyVectorControl*> m_vectorDisplays{};
    };
} // namespace GraphCanvas
