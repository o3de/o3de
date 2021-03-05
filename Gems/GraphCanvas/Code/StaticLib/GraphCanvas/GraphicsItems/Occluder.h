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
        AZ_CLASS_ALLOCATOR(Occluder, AZ::SystemAllocator, 0);
        
        Occluder(const OccluderConfiguration& occluderConfiguration);
        ~Occluder() = default;
       
        // GraphicsItem
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
        ////
        
    private:   

        QColor m_renderColor;
    };
}
