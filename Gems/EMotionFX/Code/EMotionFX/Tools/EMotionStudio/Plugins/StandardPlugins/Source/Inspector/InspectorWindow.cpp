/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Component/ComponentApplicationBus.h>

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Inspector/InspectorWindow.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Inspector/NoSelectionWidget.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Inspector/ContentWidget.h>

#include <QDockWidget>
#include <QScrollArea>
#include <QVBoxLayout>

namespace EMStudio
{
    InspectorWindow::~InspectorWindow()
    {
        InspectorRequestBus::Handler::BusDisconnect();

        if (m_scrollArea)
        {
            m_scrollArea->takeWidget();

            delete m_contentWidget;
            m_contentWidget = nullptr;

            delete m_noSelectionWidget;
            m_noSelectionWidget = nullptr;
        }

    }

    bool InspectorWindow::Init()
    {
        m_scrollArea = new QScrollArea();
        m_scrollArea->setWidgetResizable(true);
        m_scrollArea->setFrameShape(QFrame::NoFrame);

        m_dock->setWidget(m_scrollArea);

        m_contentWidget = new ContentWidget(m_dock);
        m_contentWidget->hide();

        m_noSelectionWidget = new NoSelectionWidget(m_dock);
        InternalShow(m_noSelectionWidget);

        InspectorRequestBus::Handler::BusConnect();
        return true;
    }

    void InspectorWindow::Update(QWidget* widget)
    {
        if (!widget)
        {
            Clear();
            return;
        }

        m_objectEditorCardPool.ReturnAllCards();
        m_contentWidget->Update(widget);
        InternalShow(m_contentWidget);
    }

    void InspectorWindow::UpdateWithHeader(const QString& headerTitle, const QString& iconFilename, QWidget* widget)
    {
        if (!widget)
        {
            Clear();
            return;
        }

        m_objectEditorCardPool.ReturnAllCards();
        m_contentWidget->UpdateWithHeader(headerTitle, iconFilename, widget);
        InternalShow(m_contentWidget);
    }

    void InspectorWindow::UpdateWithRpe(const QString& headerTitle, const QString& iconFilename, const AZStd::vector<CardElement>& cardElements)
    {
        m_objectEditorCardPool.ReturnAllCards();

        QWidget* containerWidget = new QWidget();
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        containerWidget->setLayout(vLayout);

        for (const CardElement& cardElement : cardElements)
        {
            if (cardElement.m_object && !cardElement.m_objectTypeId.IsNull())
            {
                ObjectEditorCard* objectEditorCard = m_objectEditorCardPool.GetFree(GetSerializeContext(), containerWidget);
                objectEditorCard->Update(cardElement.m_cardName, cardElement.m_objectTypeId, cardElement.m_object);
                vLayout->addWidget(objectEditorCard);
            }
            else if (cardElement.m_customWidget)
            {
                vLayout->addWidget(cardElement.m_customWidget);
            }
        }

        m_contentWidget->UpdateWithHeader(headerTitle, iconFilename, containerWidget);
        InternalShow(m_contentWidget);
        containerWidget->show();
    }

    void InspectorWindow::InternalShow(QWidget* widget)
    {
        if (m_scrollArea->widget() != widget)
        {
            // Get back ownership of the cached widgets to avoid recreating it each time.
            if (m_scrollArea->widget())
            {
                m_scrollArea->widget()->hide();
            }
            m_scrollArea->takeWidget();

            // Set the no selection widget and destroy the previous one.
            m_scrollArea->setWidget(widget);
        }
    }

    void InspectorWindow::Clear()
    {
        m_objectEditorCardPool.ReturnAllCards();

        m_contentWidget->Clear();
        InternalShow(m_noSelectionWidget);
    }

    void InspectorWindow::ClearIfShown(QWidget* widget)
    {
        if (m_contentWidget->GetWidget() == widget)
        {
            Clear();
        }
    }

    AZ::SerializeContext* InspectorWindow::GetSerializeContext()
    {
        if (!m_serializeContext)
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Error("EMotionFX", m_serializeContext, "Can't get serialize context from component application.");
        }

        return m_serializeContext;
    }
} // namespace EMStudio
