/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientPreviewDataWidget.h"
#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>

#include <AzCore/Component/EntityId.h>

#include <QVBoxLayout>

namespace GradientSignal
{
    //
    // GradientPreviewDataWidgetHandler
    //

    AZ::u32 GradientPreviewDataWidgetHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("GradientPreviewer");
    }

    void GradientPreviewDataWidgetHandler::ConsumeAttribute(GradientPreviewDataWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        (void)debugName;

        if (attrib == AZ_CRC_CE("GradientFilter"))
        {
            GradientPreviewWidget::SampleFilterFunc filterFunc;
            if (attrValue->Read<GradientPreviewWidget::SampleFilterFunc>(filterFunc))
            {
                GUI->SetGradientSampleFilter(filterFunc);
            }
        }
        else if (attrib == AZ_CRC_CE("GradientSampler"))
        {
            GradientSampler* sampler = nullptr;
            if (attrValue->Read<GradientSampler*>(sampler) && sampler)
            {
                GUI->SetGradientSampler(*sampler);
            }
        }
        else if (attrib == AZ_CRC_CE("GradientEntity"))
        {
            AZ::EntityId id;
            if (attrValue->Read<AZ::EntityId>(id))
            {
                GUI->SetGradientEntity(id);
            }
        }
    }

    bool GradientPreviewDataWidgetHandler::ReadValueIntoGUI(size_t index, GradientPreviewDataWidget* GUI, void* value, const AZ::Uuid& propertyType)
    {
        (void)index;
        (void)value;
        (void)propertyType;

        GUI->Refresh();

        return false;
    }

    QWidget* GradientPreviewDataWidgetHandler::CreateGUI(QWidget* pParent)
    {
        return new GradientPreviewDataWidget(pParent);
    }

    void GradientPreviewDataWidgetHandler::PreventRefresh(QWidget* GUI, bool preventRefresh)
    {
        // Notify our preview widget to disable / enable itself during refreshes.  Because it uses
        // delayed, threaded logic it's possible for it to query a component during a time that the component
        // is deactivated / deleted.  This notification tells the preview widget to immediately cancel any
        // refreshes, or restart them when it's safe again.
        auto previewWidget = reinterpret_cast<GradientPreviewDataWidget*>(GUI);
        previewWidget->PreventRefresh(preventRefresh);
    }

    void GradientPreviewDataWidgetHandler::Register()
    {
        using namespace AzToolsFramework;

        // Property handlers are set to auto-delete by default, which means that we're handing off ownership of the pointer to the
        // PropertyManagerComponent, where it will get cleaned up on system shutdown.
        auto propertyHandler = aznew GradientPreviewDataWidgetHandler();
        AZ_Assert(propertyHandler->AutoDelete(),
            "GradientPreviewDataWidgetHandler is no longer set to auto-delete, it will leak memory.");
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, propertyHandler);
    }

    void GradientPreviewDataWidgetHandler::Unregister()
    {
        // We don't need to call UnregisterPropertyType here because it's an autoDelete handler.
    }

    //
    // GradientPreviewDataWidget
    //

    GradientPreviewDataWidget::GradientPreviewDataWidget(QWidget * parent)
        : QWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(QMargins());
        layout->setAlignment(Qt::AlignHCenter);

        constexpr bool enablePopout = true;
        m_preview = new GradientPreviewWidget(this, enablePopout);
        m_preview->setFixedSize(256, 256);
        layout->addWidget(m_preview);

        QObject::connect(m_preview, &GradientPreviewWidget::popoutClicked, this, [this]()
        {
            delete m_previewWindow;
            m_previewWindow = new GradientPreviewWidget;

            // Make sure our popout preview always stays on top
            m_previewWindow->setWindowFlag(Qt::WindowStaysOnTopHint, true);

            // We need to call show() once before the resize to initialize the window frame width/height,
            // so that way the resize correctly takes them into account.  We then call show() a second time
            // afterwards to cause the resize to take effect.
            m_previewWindow->show();
            m_previewWindow->resize(750, 750);
            m_previewWindow->show();
            Refresh();
        });

        //dependency monitor must be connected to an owner/observer as a target for notifications.
        //generating a place holder entity
        m_observerEntityStub = AZ::Entity::MakeId();
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(m_observerEntityStub);
    }

    GradientPreviewDataWidget::~GradientPreviewDataWidget()
    {
        GradientPreviewRequestBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        m_dependencyMonitor.Reset();
        delete m_previewWindow;
    }

    void GradientPreviewDataWidget::PreventRefresh(bool preventRefresh)
    {
        m_preventRefresh = preventRefresh;
        if (m_preventRefresh)
        {
            // If we're trying to prevent refreshes, cancel any existing or pending refreshes.
            CancelRefresh();
        }
        else
        {
            // If we're allowing refreshes again, start one up if it has been requested during
            // the time that we weren't allowing them.
            if (m_refreshQueued)
            {
                Refresh();
            }
        }
    }

    void GradientPreviewDataWidget::SetGradientSampler(const GradientSampler& sampler)
    {
        m_sampler = sampler;

        GradientPreviewRequestBus::Handler::BusDisconnect();
        GradientPreviewRequestBus::Handler::BusConnect(sampler.m_ownerEntityId);

        Refresh();
    }

    void GradientPreviewDataWidget::SetGradientSampleFilter(GradientPreviewWidget::SampleFilterFunc sampleFunc)
    {
        m_sampleFilterFunc = sampleFunc;
        Refresh();
    }

    void GradientPreviewDataWidget::SetGradientEntity(const AZ::EntityId& id)
    {
        m_sampler = {};
        m_sampler.m_gradientId = id;
        m_sampler.m_ownerEntityId = id;

        GradientPreviewRequestBus::Handler::BusDisconnect();
        GradientPreviewRequestBus::Handler::BusConnect(id);

        Refresh();
    }

    void GradientPreviewDataWidget::OnCompositionChanged()
    {
        Refresh();
    }

    void GradientPreviewDataWidget::Refresh()
    {
        // If we currently aren't allowing refreshes, just note that it's been requested so that
        // we can start it up once refreshes are allowed again.
        if (m_preventRefresh)
        {
            m_refreshQueued = true;
            return;
        }

        if (!m_refreshInProgress)
        {
            m_refreshInProgress = true;

            m_dependencyMonitor.Reset();
            m_dependencyMonitor.ConnectOwner(m_observerEntityStub);
            m_dependencyMonitor.ConnectDependency(m_sampler.m_gradientId);

            AZ::EntityId previewEntity;
            GradientPreviewContextRequestBus::BroadcastResult(previewEntity, &GradientPreviewContextRequestBus::Events::GetPreviewEntity);
            m_dependencyMonitor.ConnectDependency(previewEntity);

            for (GradientPreviewWidget* previewer : { m_preview, m_previewWindow })
            {
                if (previewer)
                {
                    previewer->SetGradientSampler(m_sampler);
                    previewer->SetGradientSampleFilter(m_sampleFilterFunc);
                    previewer->QueueUpdate();
                }
            }
            m_refreshInProgress = false;
            m_refreshQueued = false;
        }
    }

    AZ::EntityId GradientPreviewDataWidget::CancelRefresh()
    {
        bool cancelled = false;
        for (GradientPreviewWidget* previewer : { m_preview, m_previewWindow })
        {
            if (previewer)
            {
                cancelled |= previewer->OnCancelRefresh();
            }
        }

        if (cancelled)
        {
            return m_sampler.m_gradientId;
        }

        return AZ::EntityId();
    }
} //namespace GradientSignal
