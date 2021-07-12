/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_BLENDGRAPHWIDGETCALLBACK_H
#define __EMSTUDIO_BLENDGRAPHWIDGETCALLBACK_H

// include required headers
#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include "../StandardPluginsConfig.h"
#include "GraphWidgetCallback.h"
#include "BlendGraphWidget.h"
#include <QTextOption>
#include <QFont>
#include <QFontMetrics>
#endif


namespace EMStudio
{
    // blend graph widget callback
    class BlendGraphWidgetCallback
        : public GraphWidgetCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendGraphWidgetCallback, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        BlendGraphWidgetCallback(BlendGraphWidget* widget);
        virtual ~BlendGraphWidgetCallback();

        void DrawOverlay(QPainter& painter);

    private:
        BlendGraphWidget*           mBlendGraphWidget;

        QFont                       mFont;
        QString                     mQtTempString;
        QTextOption                 mTextOptions;
        QFontMetrics*               mFontMetrics;
        AZStd::string               m_tempStringA;
        AZStd::string               m_tempStringB;
        AZStd::string               m_tempStringC;
        AZStd::string               m_mcoreTempString;
    };
}   // namespace EMStudio


#endif
