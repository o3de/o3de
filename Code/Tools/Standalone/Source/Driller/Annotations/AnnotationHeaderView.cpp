/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "AnnotationHeaderView.hxx"
#include "AnnotationsDataView.hxx"
#include "Annotations.hxx"
#include <QtWidgets/QGridLayout>

namespace Driller
{
    static const int k_contractedSize = 20;
    static const int k_textWidth = 153;

    AnnotationHeaderView::AnnotationHeaderView(AnnotationsProvider* ptrAnnotations, QWidget* parent, Qt::WindowFlags flags)
        : QWidget(parent, flags)
        , m_ptrAnnotations(ptrAnnotations)
    {
        setupUi(this);

        m_State.m_EndFrame = -1;
        m_State.m_FramesInView = 10;
        m_State.m_FrameOffset = 0;

        annotationDataView->RegisterAnnotationHeaderView(this, m_ptrAnnotations);
        annotationDataView->setAutoFillBackground(true);

        connect(configureAnnotations, SIGNAL(pressed()), this, SIGNAL(OnOptionsClick()));
        connect(annotationDataView, SIGNAL(InformOfMouseOverAnnotation(const Annotation&)), this, SIGNAL(InformOfMouseOverAnnotation(const Annotation&)));
        connect(annotationDataView, SIGNAL(InformOfClickAnnotation(const Annotation&)), this, SIGNAL(InformOfClickAnnotation(const Annotation&)));
        connect(m_ptrAnnotations, SIGNAL(AnnotationDataInvalidated()), this, SLOT(RefreshView()));

        annotationDataView->update();

        QSize size = annotationDataView->size();
        int nextHeight = size.height();

        (void)nextHeight;
    }

    QSize AnnotationHeaderView::sizeHint() const
    {
        return QSize(0, k_contractedSize);
    }


    AnnotationHeaderView::~AnnotationHeaderView()
    {
    }

    void AnnotationHeaderView::RefreshView()
    {
        annotationDataView->update();
    }

    void AnnotationHeaderView::SetEndFrame(FrameNumberType frameNum)
    {
        QSize size = annotationDataView->size();
        int nextHeight = size.height();

        (void)nextHeight;

        m_State.m_EndFrame = frameNum;
        annotationDataView->update();
    }

    void AnnotationHeaderView::SetSliderOffset(FrameNumberType frameNum)
    {
        m_State.m_FrameOffset = frameNum;
        annotationDataView->update();
    }

    void AnnotationHeaderView::SetDataPointsInView(int count)
    {
        m_State.m_FramesInView = count;
        annotationDataView->update();
    }
}

#include <Source/Driller/Annotations/moc_AnnotationHeaderView.cpp>
