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

#ifndef ANNOTATION_HEADER_VIEW_EVENTS_HXX
#define ANNOTATION_HEADER_VIEW_EVENTS_HXX

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtWidgets/QWidget>
#include <Woodpecker/Driller/DrillerNetworkMessages.h>

#include "Woodpecker/Driller/DrillerDataTypes.h"
#endif

namespace Charts
{
    class Axis;
}

namespace Driller
{
    class AnnotationsProvider;
    class AnnotationsDataView_Events;
    class Annotation;

    /** This version of the annotations header view sits above the per-frame events widget (near the bottom of hte main view).
    * Its job is to show annotations that happen within a single frame (on an event-by-event basis!)
    * It can actually work on any track thats willing to provide it with an axis.
    */

    class AnnotationHeaderView_Events : public QWidget
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(AnnotationHeaderView_Events,AZ::SystemAllocator,0);
        AnnotationHeaderView_Events(QWidget* parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~AnnotationHeaderView_Events(void);

        void AttachToAxis(AnnotationsProvider* ptrAnnotations, Charts::Axis *target);
signals:
        void InformOfMouseOverAnnotation(const Annotation& annotation);
        void InformOfClickAnnotation(const Annotation& annotation);
        
public slots:
        void RefreshView();
        void ControllerSizeChanged(QSize newSize);
        void OnScrubberFrameUpdate(FrameNumberType newFrame);
    private:
        AnnotationsProvider *m_ptrAnnotations;
        AnnotationsDataView_Events *m_ptrDataView;
        
        virtual QSize sizeHint() const;
    
    };

}


#endif // ANNOTATION_HEADER_VIEW_HXX
