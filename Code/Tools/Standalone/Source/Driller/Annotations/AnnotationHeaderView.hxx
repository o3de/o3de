/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef ANNOTATION_HEADER_VIEW_HXX
#define ANNOTATION_HEADER_VIEW_HXX

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtWidgets/QWidget>

#include <Source/Driller/Annotations/ui_AnnotationHeaderView.h>
#include <Source/Driller/DrillerNetworkMessages.h>
#include <Source/Driller/DrillerDataTypes.h>
#endif


namespace Driller
{
    class AnnotationsProvider;
    class AnnotationsDataView;
    class Annotation;

    /** The annotation header view runs along the top of the channels, and shows annotation blips that can be hovered over.
    *   this gives nice hit boxes for clicking that are not a sliver thick.
    */

    class AnnotationHeaderView : public QWidget, private Ui::AnnotationHeaderView
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(AnnotationHeaderView,AZ::SystemAllocator,0);
        AnnotationHeaderView(AnnotationsProvider* ptrAnnotations, QWidget* parent = NULL, Qt::WindowFlags flags = Qt::WindowFlags());
        virtual ~AnnotationHeaderView(void);

        struct HeaderViewState
        {
            int m_EndFrame;
            int m_FramesInView;
            int m_FrameOffset;
        };

        const HeaderViewState& GetState() { return m_State; }

        void SetDataPointsInView( int count );
        void SetEndFrame( FrameNumberType frame );
        void SetSliderOffset( FrameNumberType frame );

signals:
        void OnOptionsClick();
        void InformOfMouseOverAnnotation(const Annotation& annotation);
        void InformOfClickAnnotation(const Annotation& annotation);
        public slots:
            void RefreshView();

    private:
        HeaderViewState m_State;

        AnnotationsProvider *m_ptrAnnotations;                
        
        virtual QSize sizeHint() const;
    
    };

}


#endif // ANNOTATION_HEADER_VIEW_HXX
