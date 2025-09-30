/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <Window/Controls/PropertyGradientColorCtrl.h>
#include <Window/Controls/GradientColorDialog.h>
#include <Document/ParticleDocumentBus.h>
#include <Window/Controls/DistIndexUtil.h>
#include <Window/Controls/CommonDefs.h>

#include <QObject>
#include <QHBoxLayout>
#include <QPainter>

namespace OpenParticleSystemEditor
{
    PropertyGradientColorCtrlHandler* g_propertyHandler = nullptr;
    AZStd::string g_gradientColorUsedwidgetName;

    PropertyGradientColorCtrl::PropertyGradientColorCtrl(QWidget* parent)
        : QWidget(parent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        pLayout->setAlignment(Qt::AlignLeft);

        static const int minColorEditWidth = 60;
        static const int defaultHeight = 18;

        setLayout(pLayout);

        m_gradientWidget = new GradientWidget(this);
        m_gradientWidget->AddGradient(new QLinearGradient(0, 0, 1, 0), QPainter::CompositionMode::CompositionMode_Source);
        m_gradientWidget->setMinimumWidth(minColorEditWidth);
        m_gradientWidget->setMaximumHeight(defaultHeight);
        m_gradientWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        connect(m_gradientWidget, SIGNAL(Clicked()), this, SLOT(OpenGradientColorDialog()));

        pLayout->setContentsMargins(2, 0, 2, 0);
        pLayout->addWidget(m_gradientWidget);
    }

    PropertyGradientColorCtrl::~PropertyGradientColorCtrl()
    {
        if (m_gradientColorDialog != nullptr)
        {
            delete m_gradientColorDialog;
            m_gradientColorDialog = nullptr;
        }
    }

    void PropertyGradientColorCtrl::AddKeyEnabled(bool enabled)
    {
        m_addKey = enabled;
    }

    void PropertyGradientColorCtrl::OpenGradientColorDialog()
    {
        if (m_gradientColorDialog == nullptr)
        {
            m_gradientColorDialog = new GradientColorDialog(this);
        }
        m_gradientColorDialog->SetGradient(GetQGradientStops());
        m_gradientColorDialog->AddKeyEnabled(m_addKey);

        if (m_gradientColorDialog->exec() == QDialog::Accepted)
        {
            QGradientStops stops = m_gradientColorDialog->GetGradient();
            m_gradientStops = stops;
            m_gradientWidget->SetKeys(0, stops);

            GradientValueChanged(stops);
            emit valueChanged(stops);
        }
    }

    void PropertyGradientColorCtrl::SetGradientStops(QGradientStops stops)
    {
        m_gradientStops = stops;
        m_gradientWidget->SetKeys(0, stops);
    }

    size_t PropertyGradientColorCtrl::GetDistIndex(const OpenParticle::DistributionType& distType, int index) const
    {
        return DistIndexUtil::GetDistIndex(m_valuePtr, azrtti_typeid<OpenParticle::ValueObjColor>(), distType, index);
    }

    void PropertyGradientColorCtrl::SetDistIndex(const OpenParticle::DistributionType& distType, size_t value, int index) const
    {
        if (m_valuePtr == nullptr)
        {
            AZ_WarningOnce("PropertyGradientColorCtrl", false, "SetDistIndex failed, m_valuePtr is nullptr.");
            return;
        }
        DistIndexUtil::SetDistIndex(m_valuePtr, azrtti_typeid<OpenParticle::ValueObjColor>(), distType, value, index);
    }

    void PropertyGradientColorCtrl::ShowQGradientStops()
    {
        SetGradientStops(GetQGradientStops());
    }

    QGradientStops PropertyGradientColorCtrl::GetQGradientStops()
    {
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_gradientColorUsedwidgetName, ParticleDocumentRequestBus, GetParticleSourceData);
        int dist = static_cast<int>(GetDistIndex(OpenParticle::DistributionType::CURVE, 0));
        
        if (DistIndexUtil::GetDistributionType(m_valuePtr, azrtti_typeid<OpenParticle::ValueObjColor>()) ==
            OpenParticle::DistributionType::CURVE && dist != 0)
        {
            QGradientStops stops;
            if (dist <= sourceData->m_distribution.curves.size())
            {
                for (int i = 0; i < static_cast<int>(sourceData->m_distribution.curves[dist - 1]->keyPoints.size()); i++)
                {
                    stops.push_back(QPair<int, QColor>(i, QColor(Qt::white)));
                }
            }
            for (int i = 0; i < ELEMENTCOUNT_COLOR; i++)
            {
                if (dist + i <= sourceData->m_distribution.curves.size())
                {
                    auto currentCurve = sourceData->m_distribution.curves[dist - 1 + i];
                    int stopIndex = 0;
                    for (auto iter = currentCurve->keyPoints.begin(); iter != currentCurve->keyPoints.end(); iter++, stopIndex++)
                    {
                        OpenParticle::KeyPoint& key = (*iter);
                        auto& curr = stops.at(stopIndex);

                        AZ::Vector4 vec(curr.second.red(), curr.second.green(), curr.second.blue(), curr.second.alpha());
                        vec.SetElement(i, key.value * MAXIMUM_COLOR_VALUE);
                        QColor color(vec.GetX(), vec.GetY(), vec.GetZ(), vec.GetW());
                        stops.replace(stopIndex, QPair<float, QColor>(key.time, color));
                    }
                }
            }
            m_gradientStops = stops;
        }
        else
        {
            m_gradientStops.clear();
            m_gradientStops.push_back(QPair<int, QColor>(0, QColor(Qt::white)));
            m_gradientStops.push_back(QPair<int, QColor>(1, QColor(Qt::white)));
        }
        return m_gradientStops;
    }

    void PropertyGradientColorCtrl::GradientValueChanged(QGradientStops stops) const
    {
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_gradientColorUsedwidgetName, ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData == nullptr)
        {
            return;
        }

        for (int i = 0; i < ELEMENTCOUNT_COLOR; i++)
        {
            int distIndex = static_cast<int>(GetDistIndex(OpenParticle::DistributionType::CURVE, i));
            if (distIndex != 0 && distIndex <= sourceData->m_distribution.curves.size())
            {
                sourceData->m_distribution.curves[distIndex - 1]->keyPoints.clear();
            }
        }

        for (int i = 0; i < static_cast<int>(stops.size()); i++)
        {
            QGradientStop stop = stops.at(i);
            float time = stop.first;
            [[maybe_unused]] QColor value = stop.second;
            AZ::Vector4 vec(stop.second.redF(), stop.second.greenF(), stop.second.blueF(), stop.second.alphaF());
            for (int j = 0; j < ELEMENTCOUNT_COLOR; j++)
            {
                int distIndex = static_cast<int>(GetDistIndex(OpenParticle::DistributionType::CURVE, j));
                OpenParticle::KeyPoint key;
                key.time = time;
                key.value = vec.GetElement(j);
                if (distIndex != 0 && distIndex <= sourceData->m_distribution.curves.size())
                {
                    sourceData->m_distribution.curves[distIndex - 1]->keyPoints.emplace_back(key);
                }
            }
        }
    }

    void PropertyGradientColorCtrl::InitColor() const
    {
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_gradientColorUsedwidgetName, ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData->CheckModuleState(m_moduleId))
        {
            OpenParticle::KeyPoint start;
            OpenParticle::KeyPoint end;
            start.time = 0;
            start.value = 1;
            end.time = 1;
            end.value = 1;
            for (int i = 0; i < ELEMENTCOUNT_COLOR; i++)
            {
                OpenParticle::Curve curve;
                curve.tickMode = OpenParticle::CurveTickMode::PARTICLE_LIFETIME;
                curve.keyPoints.emplace_back(start);
                curve.keyPoints.emplace_back(end);
                sourceData->AddCurve(curve);
                SetDistIndex(OpenParticle::DistributionType::CURVE, static_cast<int>(sourceData->m_distribution.curves.size()), i);
            }
        }
    }

    void PropertyGradientColorCtrl::InitDistIndex(void* value)
    {
        m_valuePtr = value;
        if (m_moduleId == azrtti_typeid<OpenParticle::UpdateColor>() &&
            DistIndexUtil::GetDistributionType(m_valuePtr, azrtti_typeid<OpenParticle::ValueObjColor>()) !=
            OpenParticle::DistributionType::CURVE)
        {
            InitColor();
        }
    }

    void PropertyGradientColorCtrlHandler::ConsumeAttribute(
        PropertyGradientColorCtrl* GUI,
        AZ::u32 attrib,
        AzToolsFramework::PropertyAttributeReader* attrValue,
        const char* debugName)
    {
        AZ_UNUSED(debugName);
        if (attrib == AZ_CRC("Id"))
        {
            AZ::TypeId id;
            if (attrValue->Read<AZ::TypeId>(id))
            {
                GUI->m_moduleId = id;
            }
        }
        else if (attrib == AZ_CRC("ModifyKey"))
        {
            bool value = false;
            if (attrValue->Read<bool>(value))
            {
                GUI->AddKeyEnabled(value);
            }
        }
    }

    void PropertyGradientColorCtrlHandler::WriteGUIValuesIntoProperty(
        size_t index,
        PropertyGradientColorCtrl* GUI,
        property_t& instance,
        AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(GUI);
        AZ_UNUSED(instance);
        AZ_UNUSED(node);
    }

    bool PropertyGradientColorCtrlHandler::ReadValuesIntoGUI(
        size_t index,
        PropertyGradientColorCtrl* GUI,
        const property_t& instance,
        AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        GUI->InitDistIndex((void*)&instance);
        GUI->ShowQGradientStops();
        return false;
    }

    QWidget* PropertyGradientColorCtrlHandler::CreateGUI(QWidget* pParent)
    {
        PropertyGradientColorCtrl* newCtrl = aznew PropertyGradientColorCtrl(pParent);

        connect(newCtrl, &PropertyGradientColorCtrl::valueChanged, this, [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
            });
        return newCtrl;
    }

    PropertyGradientColorCtrlHandler* RegisterGradientColorPropertyHandler()
    {
        g_propertyHandler = aznew PropertyGradientColorCtrlHandler();
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType, g_propertyHandler);
        return g_propertyHandler;
    }

    void SetGradientColorBusIDName(AZStd::string widgetName)
    {
        if (widgetName != "")
        {
            g_gradientColorUsedwidgetName = widgetName;
        }
    }

    void UnregisterGradientColorPropertyHandler()
    {
        using namespace AzToolsFramework;
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Handler::UnregisterPropertyType, g_propertyHandler);
        delete g_propertyHandler;
        g_propertyHandler = nullptr;
    }
} // namespace OpenParticleSystemEditor
