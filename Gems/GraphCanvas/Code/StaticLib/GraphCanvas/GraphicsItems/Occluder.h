/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>
// qvector2d.h(131): warning C4244: 'initializing': conversion from 'int' to 'float', possible loss of data
// qevent.h(72): warning C4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QGraphicsWidget>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/GraphicsItems/GraphicsEffect.h>

namespace GraphCanvas
{
    class OccluderConfiguration
    {
    public:
        QColor m_renderColor;
        float m_opacity = 1.0f;
        
        QRectF m_bounds;

        int m_zValue = 1;
    };
    
    class Occluder
        : public GraphicsEffect<QGraphicsWidget>
    {
    public:
        AZ_CLASS_ALLOCATOR(Occluder, AZ::SystemAllocator);
        
        Occluder(const OccluderConfiguration& occluderConfiguration);
        ~Occluder() = default;
       
        // GraphicsItem
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        ////
        
    private:   

        QColor m_renderColor;
    };
}
