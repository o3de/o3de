/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <GradientSignal/Ebuses/GradientPreviewRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <UI/GradientPreviewWidget.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>

#include <QWidget>

namespace GradientSignal
{
    class GradientPreviewWidget;

    class GradientPreviewDataWidget
        : public QWidget
        , public LmbrCentral::DependencyNotificationBus::Handler
        , public GradientPreviewRequestBus::Handler
    {
    public:
        GradientPreviewDataWidget(QWidget* parent = nullptr);
        ~GradientPreviewDataWidget() override;

        void SetGradientSampler(const GradientSampler& sampler);
        void SetGradientSampleFilter(GradientPreviewWidget::SampleFilterFunc sampleFunc);
        void SetGradientEntity(const AZ::EntityId& id);

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus::Handler
        void OnCompositionChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // GradientPreviewRequestBus::Handler
        void Refresh() override;
        AZ::EntityId CancelRefresh() override;

        void PreventRefresh(bool preventRefresh);

    private:
        GradientPreviewWidget::SampleFilterFunc m_sampleFilterFunc;
        GradientSampler m_sampler;
        GradientPreviewWidget* m_preview = nullptr;
        GradientPreviewWidget* m_previewWindow = nullptr;

        AZ::EntityId m_observerEntityStub;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        bool m_refreshInProgress = false;
        bool m_preventRefresh = false;
        bool m_refreshQueued = false;
    };

    class GradientPreviewDataWidgetHandler
        : public AzToolsFramework::GenericPropertyHandler<GradientPreviewDataWidget>
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientPreviewDataWidgetHandler, AZ::SystemAllocator);

        AZ::u32 GetHandlerName() const override;
        bool ReadValueIntoGUI(size_t index, GradientPreviewDataWidget* GUI, void* value, const AZ::Uuid& propertyType) override;
        void ConsumeAttribute(GradientPreviewDataWidget* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        QWidget* CreateGUI(QWidget* pParent) override;
        void PreventRefresh(QWidget* widget, bool shouldPrevent) override;

        static void Register();
        static void Unregister();
    };
}
