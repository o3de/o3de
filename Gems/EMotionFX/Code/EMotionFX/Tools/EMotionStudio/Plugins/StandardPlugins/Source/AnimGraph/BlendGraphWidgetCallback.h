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
