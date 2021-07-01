/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "AnnotationsHeaderView_Events.hxx"
#include "AnnotationsDataView_Events.hxx"
#include "Annotations.hxx"
#include <QtWidgets/QGridLayout>

namespace Driller
{
    static const int k_eventContractedSize = 18;

    AnnotationHeaderView_Events::AnnotationHeaderView_Events(QWidget* parent, Qt::WindowFlags flags)
        : QWidget(parent, flags)
        , m_ptrAnnotations(NULL)
    {
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        this->setFixedHeight(k_eventContractedSize);
        this->setAutoFillBackground(true);

        QHBoxLayout* mainLayout = new QHBoxLayout(this);
        this->setLayout(mainLayout);

        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
    }

    void AnnotationHeaderView_Events::OnScrubberFrameUpdate(FrameNumberType newFrame)
    {
        if (m_ptrDataView)
        {
            m_ptrDataView->OnScrubberFrameUpdate(newFrame);
        }
    }

    QSize AnnotationHeaderView_Events::sizeHint() const
    {
        return QSize(0, k_eventContractedSize);
    }

    void AnnotationHeaderView_Events::ControllerSizeChanged(QSize newSize)
    {
        (void)newSize;
    }

    AnnotationHeaderView_Events::~AnnotationHeaderView_Events()
    {
    }

    void AnnotationHeaderView_Events::AttachToAxis(AnnotationsProvider* ptrAnnotations, Charts::Axis* target)
    {
        m_ptrAnnotations = ptrAnnotations;
        m_ptrDataView = aznew AnnotationsDataView_Events(this, ptrAnnotations);

        connect(m_ptrDataView, SIGNAL(InformOfMouseOverAnnotation(const Annotation&)), this, SIGNAL(InformOfMouseOverAnnotation(const Annotation&)));
        connect(m_ptrDataView, SIGNAL(InformOfClickAnnotation(const Annotation&)), this, SIGNAL(InformOfClickAnnotation(const Annotation&)));
        connect(m_ptrAnnotations, SIGNAL(AnnotationDataInvalidated()), this, SLOT(RefreshView()));

        layout()->addWidget(m_ptrDataView);
        m_ptrDataView->AttachToAxis(target);
    }

    void AnnotationHeaderView_Events::RefreshView()
    {
        m_ptrDataView->update();
    }
}

#include <Source/Driller/Annotations/moc_AnnotationsHeaderView_Events.cpp>
