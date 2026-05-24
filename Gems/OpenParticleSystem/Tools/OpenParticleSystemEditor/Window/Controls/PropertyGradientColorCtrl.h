/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Serializer/ParticleBase.h>
#include <Window/Controls/GradientWidget.h>
#include <AzCore/Math/Color.h>
#endif

#include <QWidget>
#include <QLineEdit>
#include <QToolButton>
#include <QColor>
#include <QGradientStop>
#include <QGradientStops>

namespace OpenParticleSystemEditor
{
    class GradientColorDialog;

    class PropertyGradientColorCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyGradientColorCtrl, AZ::SystemAllocator, 0);

        explicit PropertyGradientColorCtrl(QWidget* parent = nullptr);
        ~PropertyGradientColorCtrl() override;
        void SetGradientStops(QGradientStops stops);
        void AddKeyEnabled(bool enabled);
        void GradientValueChanged(QGradientStops stops) const;
        void InitDistIndex(void* value);
        void InitColor() const;
        void ShowQGradientStops();

        AZ::TypeId m_moduleId;
    signals:
        void valueChanged(QGradientStops newValue);
    private slots:
        void OpenGradientColorDialog();

    private:
        QGradientStops GetQGradientStops();
        size_t GetDistIndex(const OpenParticle::DistributionType& distType, int index) const;
        void SetDistIndex(const OpenParticle::DistributionType& distType, size_t value, int index) const;

        GradientColorDialog* m_gradientColorDialog = nullptr;
        GradientWidget* m_gradientWidget = nullptr;
        QGradientStops m_gradientStops;

        bool m_addKey = true;
        void* m_valuePtr = nullptr;
    };

    class PropertyGradientColorCtrlHandler : QObject, public AzToolsFramework::PropertyHandler<OpenParticle::ValueObjColor, PropertyGradientColorCtrl>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyGradientColorCtrlHandler, AZ::SystemAllocator, 0);
        
        AZ::u32 GetHandlerName(void) const override
        {
            return AZ_CRC("GradientColor");
        }
        virtual bool AutoDelete() const override
        {
            return false;
        }

        QWidget* CreateGUI(QWidget* pParent) override;
        void ConsumeAttribute(PropertyGradientColorCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, PropertyGradientColorCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, PropertyGradientColorCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    private:
        AZStd::unordered_map<AZStd::string, PropertyGradientColorCtrl*> m_identToCtrl;
    };

    PropertyGradientColorCtrlHandler* RegisterGradientColorPropertyHandler();
    void SetGradientColorBusIDName(AZStd::string widgetName = "");
    void UnregisterGradientColorPropertyHandler();
} // namespace OpenParticleSystemEditor
