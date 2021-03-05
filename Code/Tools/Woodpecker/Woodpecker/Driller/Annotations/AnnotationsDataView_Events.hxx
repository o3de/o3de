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

#ifndef ANNOTATIONS_DATA_VIEW_EVENTS_HXX
#define ANNOTATIONS_DATA_VIEW_EVENTS_HXX

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>

#include <QtWidgets/QWidget>

#include "Woodpecker/Driller/DrillerDataTypes.h"
#endif

namespace Charts
{
    class Axis;
}

namespace Driller
{
    class AnnotationsProvider;
    class Annotation;
    class AnnotationHeaderView_Events;
    /** Annotations Data View just shows the annotations that are available in a horizontal strip with indicators for easy clickability.
    * This flavor of the view is supposed to operate on individual events instead of individual frames and is supposed to sit above the event driller track
    * But it can actually work on any track thats willing to provide it with an axis.
    */

    class AnnotationsDataView_Events : public QWidget
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(AnnotationsDataView_Events, AZ::SystemAllocator, 0);
        AnnotationsDataView_Events( AnnotationHeaderView_Events* header, AnnotationsProvider *annotations );
        virtual ~AnnotationsDataView_Events();

        void AttachToAxis(Charts::Axis *pAxis);

        virtual void paintEvent( QPaintEvent *event );
        virtual void mouseMoveEvent( QMouseEvent *event );
        virtual void mousePressEvent( QMouseEvent *event );
        virtual void mouseReleaseEvent( QMouseEvent *event );

    signals:
        void InformOfMouseOverAnnotation(const Annotation& annotation);
        void InformOfClickAnnotation(const Annotation& annotation);
    private:
        typedef AZStd::unordered_map<AZ::s64, QPainterPath> EventIndexToClickablePath;
        
        EventIndexToClickablePath m_ClickableAreas;
        AZStd::unordered_set<AZ::s64> m_eventsToHighlight;
        QPainter *m_Painter;
        Charts::Axis *m_ptrAxis;

        AnnotationsProvider *m_ptrAnnotations;
        AnnotationHeaderView_Events *m_ptrHeaderView;
        FrameNumberType m_CurrentFrameNumber;

        const Annotation* GetNearestAnnotationToMousePoint(QPoint pos) const;
    
    public slots:
        void OnAxisInvalidated();
        void OnAxisDestroyed();
        void OnScrubberFrameUpdate(FrameNumberType newFrameNumber);
    };
}

#endif // ANNOTATIONS_DATA_VIEW
