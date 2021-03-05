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
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QPainter>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/GraphicsItems/Occluder.h>

namespace GraphCanvas
{
    /////////////
    // Occluder
    /////////////
    
    Occluder::Occluder(const OccluderConfiguration& occluderConfiguration)
        : m_renderColor(occluderConfiguration.m_renderColor)
    {
        m_renderColor.setAlpha(255);
        
        setPos(occluderConfiguration.m_bounds.topLeft());
        setPreferredSize(occluderConfiguration.m_bounds.size());
        
        setOpacity(occluderConfiguration.m_opacity);

        setZValue(occluderConfiguration.m_zValue);
    }
    
    void Occluder::paint(QPainter* painter, [[maybe_unused]] const QStyleOptionGraphicsItem* option, [[maybe_unused]] QWidget* widget)
    {
        painter->fillRect(boundingRect(), m_renderColor);
    }
}
