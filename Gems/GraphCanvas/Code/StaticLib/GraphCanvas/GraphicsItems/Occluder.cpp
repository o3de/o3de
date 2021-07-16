/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    
    void Occluder::paint([[maybe_unused]] QPainter* painter, [[maybe_unused]] const QStyleOptionGraphicsItem* option, [[maybe_unused]] QWidget* widget)
    {
        painter->fillRect(boundingRect(), m_renderColor);
    }
}
