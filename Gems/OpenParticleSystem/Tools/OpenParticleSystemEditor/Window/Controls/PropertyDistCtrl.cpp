/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <QVBoxLayout>
#include <QCoreApplication>

#include <Window/Controls/PropertyDistCtrl.h>
#include <Window/Controls/CommonDefs.h>

namespace OpenParticleSystemEditor
{
    inline AZ::Vector4 QColorToAZVector4(const QColor& color)
    {
        return AZ::Vector4(
            static_cast<float>(color.redF()),
            static_cast<float>(color.greenF()),
            static_cast<float>(color.blueF()),
            static_cast<float>(color.alphaF())
        );
    }

    inline QColor AZVector4ToQColor(const AZ::Vector4& color)
    {
        return QColor::fromRgbF(
            static_cast<qreal>(color.GetX()),
            static_cast<qreal>(color.GetY()),
            static_cast<qreal>(color.GetZ()),
            static_cast<qreal>(color.GetW())
        );
    }

    AZStd::vector<AzToolsFramework::PropertyHandlerBase*> g_propertyHandlers;
    AZStd::string g_distUsedwidgetName;
    void RegisterDistCtrlHandlers()
    {
        using namespace AzToolsFramework;

        ValueObjFloatDistPropertyHandler* handlerValueObjFloat = aznew ValueObjFloatDistPropertyHandler();
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType, handlerValueObjFloat);

        ValueObjVec2DistPropertyHandler* handlerValueObjVec2 = aznew ValueObjVec2DistPropertyHandler();
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType, handlerValueObjVec2);
        
        ValueObjVec3DistPropertyHandler* handlerValueObjVec3 = aznew ValueObjVec3DistPropertyHandler();
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType, handlerValueObjVec3);
        
        ValueObjVec4DistPropertyHandler* handlerValueObjVec4 = aznew ValueObjVec4DistPropertyHandler();
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType, handlerValueObjVec4);

        ValueObjColorDistPropertyHandler* handlerValueObjColor = aznew ValueObjColorDistPropertyHandler();
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType, handlerValueObjColor);

        ValueObjLinearDistPropertyHandler* handlerValueObjLinear = aznew ValueObjLinearDistPropertyHandler();
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType, handlerValueObjLinear);

        g_propertyHandlers.push_back(handlerValueObjFloat);
        g_propertyHandlers.push_back(handlerValueObjVec2);
        g_propertyHandlers.push_back(handlerValueObjVec3);
        g_propertyHandlers.push_back(handlerValueObjVec4);
        g_propertyHandlers.push_back(handlerValueObjColor);
        g_propertyHandlers.push_back(handlerValueObjLinear);
    }

    void UnregisterDistCtrlHandlers()
    {
        using namespace AzToolsFramework;
        for (PropertyHandlerBase* handler : g_propertyHandlers)
        {
            PropertyTypeRegistrationMessages::Bus::Broadcast(
                &PropertyTypeRegistrationMessages::Bus::Handler::UnregisterPropertyType, handler);
            delete handler;
        }
        g_propertyHandlers.clear();
    }

    void SetDistCtrlBusIDName(AZStd::string widgetName)
    {
        if (widgetName != "")
        {
            g_distUsedwidgetName = widgetName;
        }
    }

    PropertyDistCtrl::PropertyDistCtrl(const AZ::TypeId& id, QWidget* parent)
        : QWidget(parent)
    {
        m_valueTypeId = id;
        m_elementCount = GetElementCount(m_valueTypeId);
        SetupUI();
        ConnectWidget();
        ConnectKeyPointWidget();
    }

    PropertyDistCtrl::~PropertyDistCtrl()
    {
    }

    void PropertyDistCtrl::SetupUI()
    {
        int margin = 2;
        int minWidth = 60;
        int fixedHeight = 120;
        int spacing = 6;
        QVBoxLayout* pLayout = new QVBoxLayout(this);
        pLayout->setContentsMargins(margin, 0, margin, 0);

        m_comboType = new QComboBox(this);
        if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            m_comboType->addItem(tr("Constant"));
            m_comboType->addItem(tr("Curve"));
        }
        else
        {
            m_comboType->addItem(tr("Constant"));
            m_comboType->addItem(tr("Random"));
            m_comboType->addItem(tr("Curve"));
        }

        m_randomTickMode = new QComboBox(this);
        m_randomTickMode->addItem(tr("Update once"));
        m_randomTickMode->addItem(tr("Update per frame"));
        m_randomTickMode->addItem(tr("Update per spawn"));
        connect(m_randomTickMode, SIGNAL(currentIndexChanged(int)), this, SLOT(OnRandomTickModeChanged(int)));

        m_curveTickMode = new QComboBox(this);
        m_curveTickMode->addItem(tr("Emit duration"));
        m_curveTickMode->addItem(tr("Particle lifetime"));
        connect(m_curveTickMode, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCurveTickModeChanged(int)));

        if (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            m_uniformButton = new QComboBox(this);
            m_uniformButton->addItem(tr("Non-uniform"));
            m_uniformButton->addItem(tr("Uniform"));
            connect(m_uniformButton, SIGNAL(currentIndexChanged(int)), this, SLOT(OnUniformChanged(int)));
        }

        m_editConstant = new InfoLineEdit("", m_valueTypeId, this);
        if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            m_randomMin = new InfoLineEdit(tr("Min"), azrtti_typeid<AZ::Vector3>(), this);
            m_randomMax = new InfoLineEdit(tr("Max"), azrtti_typeid<AZ::Vector3>(), this);
        }
        else
        {
            m_randomMin = new InfoLineEdit(tr("Min"), m_valueTypeId, this);
            m_randomMax = new InfoLineEdit(tr("Max"), m_valueTypeId, this);
        }

        m_valueFactor = new InfoLineEdit(tr("Max Value"), azrtti_typeid<float>(), this);
        m_curveSelector = new InfoRadioButton(tr("Curves"), m_elementCount, this);

        // init curve key point editor
        auto keyPointEditorLayout = new QHBoxLayout(this);
        keyPointEditorLayout->setContentsMargins(0, 0, 0, 0);
        m_keyPointXEditor = new InfoLineEdit(tr("Current point X"), azrtti_typeid<float>(), this);
        m_keyPointXEditor->SetMinimum(0);
        m_keyPointXEditor->SetMaximum(1);
        m_keyPointYEditor = new InfoLineEdit(tr("Current point Y"), azrtti_typeid<float>(), this);
        m_keyPointYEditor->SetMinimum(0);
        m_keyPointYEditor->SetMaximum(1);
        keyPointEditorLayout->addWidget(m_keyPointXEditor);
        keyPointEditorLayout->addWidget(m_keyPointYEditor);

        m_curveEditor = new CurveEditor(g_distUsedwidgetName, this);
        m_curveEditor->setMinimumWidth(minWidth);
        m_curveEditor->setFixedHeight(fixedHeight);
        m_curveEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_gradientColorCtrl = new PropertyGradientColorCtrl(this);
        m_gradientColorCtrl->setFixedHeight(20);
        m_gradientColorCtrl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        pLayout->addSpacing(spacing);
        pLayout->addWidget(m_comboType);
        pLayout->addWidget(m_randomTickMode);
        pLayout->addWidget(m_curveTickMode);
        if (m_uniformButton)
        {
            pLayout->addWidget(m_uniformButton);
        }
        pLayout->addWidget(m_randomMin);
        pLayout->addWidget(m_randomMax);
        pLayout->addWidget(m_valueFactor);
        pLayout->addWidget(m_curveSelector);
        pLayout->addWidget(m_curveEditor);
        pLayout->addWidget(m_gradientColorCtrl);
        pLayout->addWidget(m_editConstant);
        pLayout->addLayout(keyPointEditorLayout);
        pLayout->addSpacing(spacing);

        setLayout(pLayout);

        m_comboType->setCurrentIndex(DIST_SELECTION_CONSTANT);
        emit m_comboType->currentIndexChanged(DIST_SELECTION_CONSTANT);
        connect(m_comboType, SIGNAL(currentIndexChanged(int)), this, SLOT(OnDistChanged(int)));
    }

    void PropertyDistCtrl::ConnectWidget()
    {
        // constant changed
        connect(m_editConstant, &InfoLineEdit::valueChanged, this, [this]()
            {
                emit valueChanged();
                EBUS_EVENT_ID(g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
            });
        // random changed
        connect(m_randomMin, &InfoLineEdit::valueChanged, this, &PropertyDistCtrl::RandomValueChanged);
        connect(m_randomMax, &InfoLineEdit::valueChanged, this, &PropertyDistCtrl::RandomValueChanged);
        // value factor changed
        connect(m_valueFactor, &InfoLineEdit::valueChanged, this, [this]()
            {
                m_curveEditor->SetValueFactor(m_valueFactor->GetValue<float>());
                EBUS_EVENT_ID(g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
            });
        connect(m_curveSelector, &InfoRadioButton::radioButtonChecked, this, [this](int index)
            {
                m_curveEditor->SetCurrentCurve(static_cast<int>(GetDistIndex(OpenParticle::DistributionType::CURVE, index)));
                m_valueFactor->SetValue(m_curveEditor->GetValueFactor());
                m_curveTickMode->setCurrentIndex(m_curveEditor->GetTickMode());
                m_curveEditor->repaint();

                OpenParticle::ParticleSourceData* sourceData = nullptr;
                EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
                if (sourceData)
                {
                    const AZStd::string key = sourceData->GetModuleKey(m_moduleId, m_paramId);
                    sourceData->m_distribution.curveCaches[key].activeAxis = index;
                }
            });
        connect(m_gradientColorCtrl, &PropertyGradientColorCtrl::valueChanged, this, &PropertyDistCtrl::GradientValueChanged);
    }

    void PropertyDistCtrl::ConnectKeyPointWidget()
    {
        // when choose another point, set hte keyPointEditorX and Y to its value
        connect(m_curveEditor, &CurveEditor::mouseRelease, this, [this]()
            {
                m_keyPointXEditor->SetValue(m_curveEditor->GetCurrentKeyPointTime());
            });
        connect(m_curveEditor, &CurveEditor::mouseRelease, this, [this]()
            {
                m_keyPointYEditor->SetValue(m_curveEditor->GetCurrentKeyPointValue());
            });
        connect(m_curveEditor, &CurveEditor::afterCurveSet, this, [this]()
            {
                // after curve is set, we need to click on the default point so we can edit it
                auto curTime = m_curveEditor->GetCurrentKeyPointTime();
                auto curValue = m_curveEditor->GetCurrentKeyPointValue();
                m_curveEditor->SimulateLeftButtonPressDragRelease(curTime, curValue, curTime, curValue, false);
            });

        // when edit in the keyPointEditorX or Y
        connect(m_keyPointXEditor, &InfoLineEdit::valueChanged, this, [this]()
            {
                m_curveEditor->SimulateLeftButtonPressDragRelease(m_curveEditor->GetCurrentKeyPointTime(),
                    m_curveEditor->GetCurrentKeyPointValue(), m_keyPointXEditor->GetValue<float>(), m_curveEditor->GetCurrentKeyPointValue());
            });
        connect(m_keyPointYEditor, &InfoLineEdit::valueChanged, this, [this]()
            {
                m_curveEditor->SimulateLeftButtonPressDragRelease(m_curveEditor->GetCurrentKeyPointTime(),
                    m_curveEditor->GetCurrentKeyPointValue(), m_curveEditor->GetCurrentKeyPointTime(), m_keyPointYEditor->GetValue<float>());
            });
    }

    template<typename ValueType>
    void PropertyDistCtrl::RandomValueChangedFunc(OpenParticle::ParticleSourceData* sourceData) const
    {
        ValueType min = m_randomMin->GetValue<ValueType>();
        ValueType max = m_randomMax->GetValue<ValueType>();

        const AZStd::string key = sourceData->GetModuleKey(m_moduleId, m_paramId);
        sourceData->m_distribution.randomCaches[key].clear();
        for (int i = 0; i < m_elementCount; i++)
        {
            int randomIndex = static_cast<int>(GetDistIndex(OpenParticle::DistributionType::RANDOM, i));
            if (randomIndex != 0 && randomIndex <= sourceData->m_distribution.randoms.size())
            {
                auto* random = sourceData->m_distribution.randoms[randomIndex - 1];
                random->min = min.dataValue.GetElement(i);
                random->max = max.dataValue.GetElement(i);

                sourceData->m_distribution.randomCaches[key].emplace_back(random);
            }
        }
    }

    template<>
    void PropertyDistCtrl::RandomValueChangedFunc<OpenParticle::ValueObjLinear>(OpenParticle::ParticleSourceData* sourceData) const
    {
        (void)sourceData;
        OpenParticle::ValueObjLinear* value = static_cast<OpenParticle::ValueObjLinear*>(m_valuePtr);
        value->dataValue.minValue = m_randomMin->GetValue<AZ::Vector3>();
        value->dataValue.maxValue = m_randomMax->GetValue<AZ::Vector3>();
    }

    void PropertyDistCtrl::RandomValueChanged() const
    {
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData == nullptr)
        {
            return;
        }
        if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            int randomIndex = static_cast<int>(GetDistIndex(OpenParticle::DistributionType::RANDOM));
            if (randomIndex != 0 && randomIndex <= sourceData->m_distribution.randoms.size())
            {
                auto* random = sourceData->m_distribution.randoms[randomIndex - 1];
                random->min = m_randomMin->GetValue<float>();
                random->max = m_randomMax->GetValue<float>();

                const AZStd::string key = sourceData->GetModuleKey(m_moduleId, m_paramId);
                sourceData->m_distribution.randomCaches[key].clear();
                sourceData->m_distribution.randomCaches[key].emplace_back(random);
            }
        }
        else if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjVec3>())
        {
            RandomValueChangedFunc<OpenParticle::ValueObjVec3>(sourceData);
        }
        else if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            RandomValueChangedFunc<OpenParticle::ValueObjColor>(sourceData);
        }
        else if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            // UpdateSizeByVelocity Constant value changed
            RandomValueChangedFunc<OpenParticle::ValueObjLinear>(sourceData);
        }
        EBUS_EVENT_ID(g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void PropertyDistCtrl::GradientValueChanged(QGradientStops stops) const
    {
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
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
            auto time = stop.first;
            AZ::Vector4 vec(QColorToAZVector4(stop.second));
            for (int j = 0; j < ELEMENTCOUNT_COLOR; j++)
            {
                int distIndex = static_cast<int>(GetDistIndex(OpenParticle::DistributionType::CURVE, j));
                if (distIndex != 0 && distIndex <= sourceData->m_distribution.curves.size())
                {
                    OpenParticle::KeyPoint key;
                    key.time = static_cast<float>(time);
                    key.value = vec.GetElement(j);
                    sourceData->m_distribution.curves[distIndex - 1]->keyPoints.emplace_back(key);
                }
            }
        }
        EBUS_EVENT_ID(g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void PropertyDistCtrl::OnDistChanged(int index)
    {
        bool constant = false;
        bool randomMin = false;
        bool randomMax = false;
        bool randomTickMode = false;
        bool valueFactor = false;
        bool curveSelector = false;
        bool curveEditor = false;
        bool curveTickMode = false;
        bool gradientColorCtrl = false;
        bool sourceDataModified = false;
        bool isUniform = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjFloat>());

        switch (index)
        {
        case DIST_SELECTION_CONSTANT:
            constant = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjLinear>());
            randomMin = (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>());
            randomMax = (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>());
            sourceDataModified = UpdateConstant();
            break;
        case DIST_SELECTION_RANDOM:
            randomMin = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjLinear>());
            randomMax = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjLinear>());
            randomTickMode = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjLinear>());
            if ((m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjLinear>()))
            {
                // UpdateSizeByVelocity doesn't have Random.
                sourceDataModified = UpdateRandom();
                break;
            }
        case DIST_SELECTION_CURVE:
            valueFactor = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjColor>());
            curveSelector = (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjVec3>()) ||
                (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>());
            curveEditor = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjColor>());
            gradientColorCtrl = (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjColor>());
            curveTickMode = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjColor>());
            sourceDataModified = UpdateCurve();
            break;
        default:
            constant = true;
            break;
        }
        if (m_uniformButton)
        {
            m_uniformButton->setVisible(isUniform);
        }
        bool uniform = DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId);
        m_editConstant->setVisible(constant);
        m_editConstant->SetUniformVisibility(uniform);
        m_randomMin->setVisible(randomMin);
        m_randomMin->SetUniformVisibility(uniform);
        m_randomMax->setVisible(randomMax);
        m_randomMax->SetUniformVisibility(uniform);
        m_randomTickMode->setVisible(randomTickMode);
        m_valueFactor->setVisible(valueFactor);
        m_curveSelector->setVisible(curveSelector && !uniform);
        m_curveEditor->setVisible(curveEditor);
        m_curveTickMode->setVisible(curveTickMode);
        m_gradientColorCtrl->setVisible(gradientColorCtrl);
        m_keyPointXEditor->setVisible(curveEditor);
        m_keyPointYEditor->setVisible(curveEditor);

        if (sourceDataModified)
        {
            EBUS_EVENT_ID(g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
        }
    }

    void PropertyDistCtrl::DistChanged(int index)
    {
        bool constant = false;
        bool randomMin = false;
        bool randomMax = false;
        bool randomTickMode = false;
        bool valueFactor = false;
        bool curveSelector = false;
        bool curveEditor = false;
        bool curveTickMode = false;
        bool gradientColorCtrl = false;
        bool isUniform = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjFloat>());

        switch (index)
        {
        case DIST_SELECTION_CONSTANT:
            constant = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjLinear>());
            randomMin = (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>());
            randomMax = (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>());
            UpdateConstant();
            break;
        case DIST_SELECTION_RANDOM:
            randomMin = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjLinear>());
            randomMax = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjLinear>());
            randomTickMode = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjLinear>());
            if ((m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjLinear>()))
            {
                // UpdateSizeByVelocity doesn't have Random.
                UpdateRandom();
                break;
            }
        case DIST_SELECTION_CURVE:
            valueFactor = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjColor>());
            curveSelector = (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjVec3>()) ||
                (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>());
            curveEditor = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjColor>());
            gradientColorCtrl = (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjColor>());
            curveTickMode = (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjColor>());
            UpdateCurve();
            break;
        default:
            constant = true;
            break;
        }
        if (m_uniformButton)
        {
            m_uniformButton->setVisible(isUniform);
        }
        bool uniform = DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId);
        m_editConstant->setVisible(constant);
        m_editConstant->SetUniformVisibility(uniform);
        m_randomMin->setVisible(randomMin);
        m_randomMin->SetUniformVisibility(uniform);
        m_randomMax->setVisible(randomMax);
        m_randomMax->SetUniformVisibility(uniform);
        m_randomTickMode->setVisible(randomTickMode);
        m_valueFactor->setVisible(valueFactor);
        m_curveSelector->setVisible(curveSelector && !uniform);
        m_curveEditor->setVisible(curveEditor);
        m_curveTickMode->setVisible(curveTickMode);
        m_gradientColorCtrl->setVisible(gradientColorCtrl);
        m_keyPointXEditor->setVisible(curveEditor);
        m_keyPointYEditor->setVisible(curveEditor);
    }

    void PropertyDistCtrl::OnRandomTickModeChanged(int index)
    {
        bool sourceDataModified = UpdateRandomTickMode(index);

        if (sourceDataModified)
        {
            EBUS_EVENT_ID(g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
        }
    }

    void PropertyDistCtrl::OnCurveTickModeChanged(int index)
    {
        // currently, only two valid modes are available.
        OpenParticle::CurveTickMode mode =
            index == 0 ? OpenParticle::CurveTickMode::EMIT_DURATION : OpenParticle::CurveTickMode::PARTICLE_LIFETIME; 
        m_curveEditor->SetTickMode(mode);
        EBUS_EVENT_ID(g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void PropertyDistCtrl::OnUniformChanged(int index)
    {
        bool sourceDataModified = UpdateUniformMode(index);

        if (sourceDataModified)
        {
            if (GetDistType() == OpenParticle::DistributionType::CURVE)
            {
                m_curveSelector->setVisible(index == 0);
                m_curveSelector->SetCurrentChecked(0);
                m_curveEditor->SetCurrentCurve(static_cast<int>(GetDistIndex(OpenParticle::DistributionType::CURVE, 0)));
                m_valueFactor->SetValue(m_curveEditor->GetValueFactor());
                m_curveTickMode->setCurrentIndex(m_curveEditor->GetTickMode());
                m_curveEditor->update();
                OpenParticle::ParticleSourceData* sourceData = nullptr;
                EBUS_EVENT_ID_RESULT(
                    sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
                if (sourceData)
                {
                    const AZStd::string key = sourceData->GetModuleKey(m_moduleId, m_paramId);
                    sourceData->m_distribution.curveCaches[key].activeAxis = index;
                }
            }

            bool uniform = DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId);
            m_editConstant->SetUniformVisibility(uniform);
            m_randomMin->SetUniformVisibility(uniform);
            m_randomMax->SetUniformVisibility(uniform);

            EBUS_EVENT_ID(g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
        }
    }

    bool PropertyDistCtrl::UpdateConstant()
    {
        bool update = false;
        if (m_valuePtr)
        {
            update = true;
            if (IsRandom())
            {
                StashCurrentRandom();
            }
            if (IsCurve())
            {
                StashCurrentCurve();
            }
            DistIndexUtil::SetDistributionType(m_valuePtr, m_valueTypeId, OpenParticle::DistributionType::CONSTANT);
            if (m_uniformButton)
            {
                m_uniformButton->setCurrentIndex(DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId) ? 1 : 0);
            }
        }
        return update;
    }

    template<typename ValueType>
    bool PropertyDistCtrl::UpdateRandomFunc()
    {
        bool update = false;
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            const AZStd::string key = sourceData->GetModuleKey(m_moduleId, m_paramId);
            auto& lastValues = sourceData->m_distribution.randomCaches;
            if (!IsRandom())
            {
                // reset distIndex and rebuild distribution(to remove unnecessarily random/curves)
                ClearDistIndex();
                sourceData->ToEditor();

                // add new random
                ValueType min = m_randomMin->GetValue<ValueType>();
                ValueType max = m_randomMax->GetValue<ValueType>();
                for (int i = 0; i < m_elementCount; i++)
                {
                    OpenParticle::Random* random =
                        lastValues.find(key) != lastValues.end() ? lastValues.at(key)[i] : aznew OpenParticle::Random();

                    sourceData->AddRandom(random);
                    int dist = static_cast<int>(sourceData->m_distribution.randoms.size());
                    SetDistIndex(OpenParticle::DistributionType::RANDOM, dist, i);
                    min.dataValue.SetElement(i, random->min);
                    max.dataValue.SetElement(i, random->max);
                    update = true;
                    if (i == 0)
                    {
                        m_randomTickMode->setCurrentIndex(OpenParticle::DataConvertor::ECast(random->tickMode));
                        if (m_uniformButton)
                        {
                            m_uniformButton->setCurrentIndex(DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId) ? 1 : 0);
                        }
                    }
                }
                if (azrtti_typeid<ValueType>() == azrtti_typeid<OpenParticle::ValueObjColor>())
                {
                    min.dataValue.SetElement(3, MAXIMUM_COLOR_VALUE);
                    max.dataValue.SetElement(3, MAXIMUM_COLOR_VALUE);
                }
                m_randomMin->SetValue(min);
                m_randomMax->SetValue(max);
            }
        }
        return update;
    }

    bool PropertyDistCtrl::IsRandom() const
    {
        return GetDistType() == OpenParticle::DistributionType::RANDOM;
    }

    bool PropertyDistCtrl::IsCurve() const
    {
        return GetDistType() == OpenParticle::DistributionType::CURVE;
    }

    bool PropertyDistCtrl::UpdateRandomTickMode(int index)
    {
        if (!IsRandom())
        {
            return false;
        }

        bool res = false;
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            for (auto iter = 0; iter < m_elementCount; ++iter)
            {
                auto randomIndex = DistIndexUtil::GetDistIndex(m_valuePtr, m_valueTypeId, OpenParticle::DistributionType::RANDOM, iter);
                if (randomIndex != 0 && randomIndex <= sourceData->m_distribution.randoms.size())
                {
                    auto randomPtr = sourceData->m_distribution.randoms[randomIndex - 1];
                    randomPtr->tickMode = static_cast<OpenParticle::RandomTickMode>(index);
                    res = true;
                }
            }
        }
        return res;
    }

    bool PropertyDistCtrl::UpdateUniformMode(int index)
    {
        if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjFloat>() || m_valuePtr == nullptr)
        {
            return false;
        }

        bool current = DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId);
        if (current == static_cast<bool>(index))
        {
            return false;
        }

        DistIndexUtil::SetUinform(m_valuePtr, m_valueTypeId, static_cast<bool>(index));
        return true;
    }

    void PropertyDistCtrl::StashCurrentRandom()
    {
        if (m_valuePtr == nullptr)
        {
            return;
        }
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            const AZStd::string key = sourceData->GetModuleKey(m_moduleId, m_paramId);
            sourceData->m_distribution.randomCaches[key].clear();
            for (auto index = 0; index < m_elementCount; ++index)
            {
                auto distIndex = DistIndexUtil::GetDistIndex(m_valuePtr, m_valueTypeId, OpenParticle::DistributionType::RANDOM, index);
                if (distIndex != 0 && distIndex <= sourceData->m_distribution.randoms.size())
                {
                    sourceData->m_distribution.randomCaches[key].emplace_back(sourceData->m_distribution.randoms[distIndex - 1]);
                    sourceData->m_distribution.stashedRandomIndexes.emplace_back(distIndex);
                }
            }

            sourceData->UpdateRandomIndexes();
        }
    }

    void PropertyDistCtrl::StashCurrentCurve()
    {
        if (m_valuePtr == nullptr)
        {
            return;
        }
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            const AZStd::string key = sourceData->GetModuleKey(m_moduleId, m_paramId);
            for (auto index = 0; index < m_elementCount; ++index)
            {
                auto distIndex = DistIndexUtil::GetDistIndex(m_valuePtr, m_valueTypeId, OpenParticle::DistributionType::CURVE, index);
                if (distIndex != 0 && distIndex <= sourceData->m_distribution.curves.size())
                {
                    sourceData->m_distribution.curveCaches[key].curves[index] = sourceData->m_distribution.curves[distIndex - 1];
                    sourceData->m_distribution.stashedCurveIndexes.emplace_back(distIndex);
                }
            }

            sourceData->UpdateCurveIndexes();
        }
    }

    bool PropertyDistCtrl::UpdateRandom()
    {
        bool update = false;
        if (IsCurve())
        {
            StashCurrentCurve();
        }
        if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            OpenParticle::ParticleSourceData* sourceData = nullptr;
            EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
            if (sourceData)
            {
                const AZStd::string key = sourceData->GetModuleKey(m_moduleId, m_paramId);
                auto& lastValues = sourceData->m_distribution.randomCaches;
                if (!IsRandom())
                {
                    OpenParticle::Random* random =
                        lastValues.find(key) != lastValues.end() ? lastValues.at(key).back() : aznew OpenParticle::Random();
                    sourceData->AddRandom(random);
                    int dist = static_cast<int>(sourceData->m_distribution.randoms.size());
                    SetDistIndex(OpenParticle::DistributionType::RANDOM, dist);

                    m_randomMin->SetValue(random->min);
                    m_randomMax->SetValue(random->max);
                    update = true;
                    m_randomTickMode->setCurrentIndex(OpenParticle::DataConvertor::ECast(random->tickMode));
                }
            }
        }
        else if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjVec3>())
        {
            update = UpdateRandomFunc<OpenParticle::ValueObjVec3>();
        }
        else if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            update = UpdateRandomFunc<OpenParticle::ValueObjColor>();
        }
        return update;
    }

    void PropertyDistCtrl::ClearDistIndex() const
    {
        if (m_valuePtr == nullptr)
        {
            AZ_WarningOnce("PropertyDistCtrl", false, "SetDistIndex failed, m_valuePtr is nullptr.");
            return;
        }
        DistIndexUtil::ClearDistIndex(m_valuePtr, m_valueTypeId);
    }

    void PropertyDistCtrl::ShowCurve() const
    {
        if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjFloat>() ||
            m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjVec2>() ||
            m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjVec3>() ||
            m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjVec4>() ||
            m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            OpenParticle::ParticleSourceData* sourceData = nullptr;
            EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
            int lastChosenAxis = 0;
            if (sourceData)
            {
                const AZStd::string key = sourceData->GetModuleKey(m_moduleId, m_paramId);
                if (sourceData->m_distribution.curveCaches.find(key) != sourceData->m_distribution.curveCaches.end())
                {
                    lastChosenAxis = sourceData->m_distribution.curveCaches[key].activeAxis;
                }
            }
            lastChosenAxis = DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId) ? 0 : lastChosenAxis;
            m_curveSelector->SetCurrentChecked(lastChosenAxis);
            m_curveEditor->SetCurrentCurve(static_cast<int>(GetDistIndex(OpenParticle::DistributionType::CURVE, lastChosenAxis)));
            m_valueFactor->SetValue(m_curveEditor->GetValueFactor());
            m_curveTickMode->setCurrentIndex(m_curveEditor->GetTickMode());
            if (m_uniformButton)
            {
                m_uniformButton->setCurrentIndex(DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId) ? 1 : 0);
            }
        }
        else if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            m_gradientColorCtrl->m_moduleId = m_moduleId;
            m_gradientColorCtrl->InitDistIndex(m_valuePtr);
        }
    }

    bool PropertyDistCtrl::UpdateCurve()
    {
        bool update = false;
        if (IsRandom())
        {
            StashCurrentRandom();
        }
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            if (!IsCurve())
            {
                update = true;
                ClearDistIndex();
                sourceData->ToEditor();

                const AZStd::string key = sourceData->GetModuleKey(m_moduleId, m_paramId);
                const auto lastValues = sourceData->m_distribution.curveCaches;
                for (int i = 0; i < m_elementCount; i++)
                {
                    OpenParticle::Curve* defaultCurve = nullptr;
                    if (lastValues.find(key) != lastValues.end())
                    {
                        defaultCurve = sourceData->m_distribution.curveCaches[key].curves[i];
                    }
                    else
                    {
                        defaultCurve = aznew OpenParticle::Curve();
                        OpenParticle::KeyPoint start;
                        OpenParticle::KeyPoint end;
                        start.time = 0;
                        start.value = 1;
                        end.time = 1;
                        end.value = 1;
                        defaultCurve->keyPoints.emplace_back(start);
                        defaultCurve->keyPoints.emplace_back(end);
                    }
                    sourceData->AddCurve(defaultCurve);
                    SetDistIndex(OpenParticle::DistributionType::CURVE, sourceData->m_distribution.curves.size(), i);
                }

                if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjColor>())
                {
                    if (lastValues.find(key) == lastValues.end())
                    {
                        QGradientStops stops;
                        stops.push_back(QPair<int, QColor>(0, QColor(Qt::white)));
                        stops.push_back(QPair<int, QColor>(1, QColor(Qt::white)));
                        m_gradientColorCtrl->SetGradientStops(stops);
                    }
                    else
                    {
                        UpdateDistControlsCurve(sourceData->m_distribution);
                    }
                }
            }
            ShowCurve();
        }
        return update;
    }

    template<typename ValueType>
    void PropertyDistCtrl::SetValue(ValueType& value)
    {
        m_valuePtr = static_cast<void*>(&value);
        m_editConstant->SetValue(value);
        UpdateDistControls();
    }

    template<>
    void PropertyDistCtrl::SetValue(OpenParticle::ValueObjLinear& value)
    {
        m_valuePtr = static_cast<void*>(&value);
        m_randomMin->SetValue(value.dataValue.minValue);
        m_randomMax->SetValue(value.dataValue.maxValue);
        UpdateDistControls();
    }

    template<typename ValueType>
    ValueType PropertyDistCtrl::GetValue()
    {
        return m_editConstant->GetValue<ValueType>();
    }

    template<>
    OpenParticle::ValueObjLinear PropertyDistCtrl::GetValue()
    {
        OpenParticle::ValueObjLinear value = m_editConstant->GetValue<OpenParticle::ValueObjLinear>();
        value.dataValue.minValue = m_randomMin->GetValue<AZ::Vector3>();
        value.dataValue.maxValue = m_randomMax->GetValue<AZ::Vector3>();
        return value;
    }

    size_t PropertyDistCtrl::GetDistIndex(const OpenParticle::DistributionType& distType, int index) const
    {
        return DistIndexUtil::GetDistIndex(m_valuePtr, m_valueTypeId, distType, index);
    }

    void PropertyDistCtrl::SetDistIndex(const OpenParticle::DistributionType& distType, size_t value, int index) const
    {
        if (m_valuePtr == nullptr)
        {
            AZ_WarningOnce("PropertyDistCtrl", false, "SetDistIndex failed, m_distIndex is nullptr.");
            return;
        }
        DistIndexUtil::SetDistIndex(m_valuePtr, m_valueTypeId, distType, value, index);
    }

    OpenParticle::DistributionType PropertyDistCtrl::GetDistType() const
    {
        return DistIndexUtil::GetDistributionType(m_valuePtr, m_valueTypeId);
    }

    void PropertyDistCtrl::SetDistType(const OpenParticle::DistributionType& distType) const
    {
        if (m_valuePtr == nullptr)
        {
            AZ_WarningOnce("PropertyDistCtrl", false, "SetDistIndex failed, m_valuePtr is nullptr.");
            return;
        }
        DistIndexUtil::SetDistributionType(m_valuePtr, m_valueTypeId, distType);
    }

    template<typename ValueType>
    void PropertyDistCtrl::UpdateDistControlsFunc(const OpenParticle::Distribution& distribution) const
    {
        ValueType min;
        ValueType max;
        OpenParticle::RandomTickMode tickMode;
        for (int i = 0; i < m_elementCount; i++)
        {
            auto randomIndex = GetDistIndex(OpenParticle::DistributionType::RANDOM, i);
            if (randomIndex != 0 && randomIndex <= distribution.randoms.size())
            {
                auto random = distribution.randoms[randomIndex - 1];
                min.dataValue.SetElement(i, random->min);
                max.dataValue.SetElement(i, random->max);
                tickMode = random->tickMode;
            }
        }
        m_randomMin->SetValue(min);
        m_randomMax->SetValue(max);

        m_randomTickMode->setCurrentIndex(OpenParticle::DataConvertor::ECast(tickMode));
        if (m_uniformButton)
        {
            m_uniformButton->setCurrentIndex(DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId) ? 1 : 0);
        }
    }

    template<>
    void PropertyDistCtrl::UpdateDistControlsFunc<OpenParticle::ValueObjLinear>(const OpenParticle::Distribution& distribution) const
    {
        OpenParticle::ValueObjLinear min;
        OpenParticle::ValueObjLinear max;
        OpenParticle::RandomTickMode tickMode;
        for (int i = 0; i < 3; i++)
        {
            auto randomIndex = GetDistIndex(OpenParticle::DistributionType::RANDOM, i);
            if (randomIndex != 0 && randomIndex <= distribution.randoms.size())
            {
                auto random = distribution.randoms[randomIndex - 1];
                min.dataValue.minValue.SetElement(i, random->min);
                max.dataValue.maxValue.SetElement(i, random->max);
                tickMode = random->tickMode;
            }
        }
        m_randomMin->SetValue(min);
        m_randomMax->SetValue(max);

        m_randomTickMode->setCurrentIndex(OpenParticle::DataConvertor::ECast(tickMode));
        if (m_uniformButton)
        {
            m_uniformButton->setCurrentIndex(DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId) ? 1 : 0);
        }
    }

    int PropertyDistCtrl::UpdateDistControlsCurve(const OpenParticle::Distribution& distribution) const
    {
        int currIndex = 0;
        int dist = static_cast<int>(GetDistIndex(OpenParticle::DistributionType::CURVE));
        if (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            currIndex = m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>() ? 1 : DIST_SELECTION_CURVE;
            m_curveSelector->SetCurrentChecked(0);
            m_curveEditor->SetCurrentCurve(dist);
            m_valueFactor->SetValue(m_curveEditor->GetValueFactor());

            auto currentCurve = distribution.curves[dist - 1];
            m_curveTickMode->setCurrentIndex(OpenParticle::DataConvertor::ECast(currentCurve->tickMode));
            if (m_valueTypeId != azrtti_typeid<OpenParticle::ValueObjFloat>() && m_uniformButton)
            {
                m_uniformButton->setCurrentIndex(DistIndexUtil::IsUinform(m_valuePtr, m_valueTypeId) ? 1 : 0);
            }
        }
        else
        {
            currIndex = DIST_SELECTION_CURVE;
            m_curveSelector->SetCurrentChecked(0);
            QGradientStops stops;

            if (dist != 0 && dist <= distribution.curves.size())
            {
                for (int i = 0; i < static_cast<int>(distribution.curves[dist - 1]->keyPoints.size()); i++)
                {
                    stops.push_back(QPair<int, QColor>(i, QColor(Qt::white)));
                }
            }
            for (int i = 0; i < ELEMENTCOUNT_COLOR; i++)
            {
                if (dist != 0 && dist + i <= distribution.curves.size())
                {
                    auto currentCurve = distribution.curves[dist - 1 + i];
                    int stopIndex = 0;
                    for (auto iter = currentCurve->keyPoints.begin(); iter != currentCurve->keyPoints.end(); iter++, stopIndex++)
                    {
                        OpenParticle::KeyPoint& key = (*iter);
                        auto curr = stops.at(stopIndex);

                        AZ::Vector4 vec(QColorToAZVector4(curr.second));
                        vec.SetElement(i, key.value * MAXIMUM_COLOR_VALUE);
                        QColor color(AZVector4ToQColor(vec));
                        stops.replace(stopIndex, QPair<float, QColor>(key.time, color));
                    }
                }
            }
            m_gradientColorCtrl->SetGradientStops(stops);
        }
        return currIndex;
    }

    void PropertyDistCtrl::UpdateDistControls()
    {
        int currIndex = DIST_SELECTION_CONSTANT;
        if (m_valuePtr == nullptr)
        {
            AZ_WarningOnce("PropertyDistCtrl", false, "UpdateDistControls failed, m_valuePtr is nullptr.");
            m_comboType->setCurrentIndex(currIndex);
            emit m_comboType->currentIndexChanged(currIndex);
            return;
        }

        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, g_distUsedwidgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            auto& distribution = sourceData->m_distribution;
            if (IsRandom())
            {
                currIndex = DIST_SELECTION_RANDOM;
                if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjFloat>())
                {
                    int dist = static_cast<int>(GetDistIndex(OpenParticle::DistributionType::RANDOM));
                    if (dist != 0 && dist <= distribution.randoms.size())
                    {
                        auto random = distribution.randoms[dist - 1];
                        m_randomMin->SetValue(random->min);
                        m_randomMax->SetValue(random->max);

                        m_randomTickMode->setCurrentIndex(OpenParticle::DataConvertor::ECast(random->tickMode));
                    }
                }
                else if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjVec3>())
                {
                    UpdateDistControlsFunc<OpenParticle::ValueObjVec3>(distribution);
                }
                else if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjColor>())
                {
                    UpdateDistControlsFunc<OpenParticle::ValueObjColor>(distribution);
                }
                else if (m_valueTypeId == azrtti_typeid<OpenParticle::ValueObjLinear>())
                {
                    UpdateDistControlsFunc<OpenParticle::ValueObjLinear>(distribution);
                }
            }
            if (IsCurve())
            {
                currIndex = UpdateDistControlsCurve(distribution);
            }
        }

        m_comboType->setCurrentIndex(currIndex);
        DistChanged(currIndex);
    }

    void PropertyDistCtrl::SetMinimum(double min)
    {
        m_editConstant->SetMinimum(min);
        m_randomMin->SetMinimum(min);
        m_randomMax->SetMinimum(min);
        m_valueFactor->SetMinimum(min);
    }

    void PropertyDistCtrl::SetMaximum(double max)
    {
        m_editConstant->SetMaximum(max);
        m_randomMin->SetMaximum(max);
        m_randomMax->SetMaximum(max);
        m_valueFactor->SetMaximum(max);
    }

    void PropertyDistCtrl::SetSuffix(const AZStd::string& suffix)
    {
        m_editConstant->SetSuffix(QCoreApplication::translate("Reflect", suffix.c_str()));
        m_randomMin->SetSuffix(QCoreApplication::translate("Reflect", suffix.c_str()));
        m_randomMax->SetSuffix(QCoreApplication::translate("Reflect", suffix.c_str()));
        m_valueFactor->SetSuffix(QCoreApplication::translate("Reflect", suffix.c_str()));
    }

    int PropertyDistCtrl::GetElementCount(AZ::TypeId id)
    {
        const int ELEMENTCOUNT_1 = 1;
        const int ELEMENTCOUNT_2 = 2;
        const int ELEMENTCOUNT_3 = 3;
        const int ELEMENTCOUNT_4 = 4;
        if (id == azrtti_typeid<OpenParticle::ValueObjFloat>())
        {
            return ELEMENTCOUNT_1;
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec2>())
        {
            return ELEMENTCOUNT_2;
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec3>() || id == azrtti_typeid<OpenParticle::ValueObjLinear>())
        {
            return ELEMENTCOUNT_3;
        }
        if (id == azrtti_typeid<OpenParticle::ValueObjVec4>() || id == azrtti_typeid<OpenParticle::ValueObjColor>())
        {
            return ELEMENTCOUNT_4;
        }
        return 0;
    }

    template<class ValueType>
    void DistHandlerCommon<ValueType>::ConsumeAttribute(
        PropertyDistCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::Min)
        {
            double value;
            if (attrValue->Read<double>(value))
            {
                GUI->SetMinimum(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Max)
        {
            double value;
            if (attrValue->Read<double>(value))
            {
                GUI->SetMaximum(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Suffix)
        {
            AZStd::string label;
            if (attrValue->Read<AZStd::string>(label))
            {
                GUI->SetSuffix(label.c_str());
            }
        }
        else
        {
            ConsumeCustomAttribute(GUI, attrib, attrValue, debugName);
        }
    }
    template<class ValueType>
    void DistHandlerCommon<ValueType>::ConsumeCustomAttribute(
        PropertyDistCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ_CRC("Id"))
        {
            AZ::TypeId id;
            if (attrValue->Read<AZ::TypeId>(id))
            {
                GUI->SetModuleId(id);
            }
        }
        if (attrib == AZ_CRC("ParamId"))
        {
            AZ::TypeId id;
            if (attrValue->Read<AZ::TypeId>(id))
            {
                GUI->SetParamId(id);
            }
        }
    }

    void ValueObjFloatDistPropertyHandler::WriteGUIValuesIntoProperty(
        size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        OpenParticle::ValueObjFloat val = GUI->GetValue<OpenParticle::ValueObjFloat>();
        instance.dataValue = val.dataValue;
    }

    bool ValueObjFloatDistPropertyHandler::ReadValuesIntoGUI(
        size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        GUI->SetValue(const_cast<property_t&>(instance));
        return false;
    }

    void ValueObjVec2DistPropertyHandler::WriteGUIValuesIntoProperty(
        size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        OpenParticle::ValueObjVec2 val = GUI->GetValue<OpenParticle::ValueObjVec2>();
        instance.dataValue = val.dataValue;
    }

    bool ValueObjVec2DistPropertyHandler::ReadValuesIntoGUI(
        size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        GUI->SetValue(const_cast<property_t&>(instance));
        return false;
    }

    void ValueObjVec3DistPropertyHandler::WriteGUIValuesIntoProperty(
        size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        OpenParticle::ValueObjVec3 val = GUI->GetValue<OpenParticle::ValueObjVec3>();
        instance.dataValue = val.dataValue;
    }

    bool ValueObjVec3DistPropertyHandler::ReadValuesIntoGUI(
        size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        GUI->SetValue(const_cast<property_t&>(instance));
        return false;
    }

    void ValueObjVec4DistPropertyHandler::WriteGUIValuesIntoProperty(
        size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        OpenParticle::ValueObjVec4 val = GUI->GetValue<OpenParticle::ValueObjVec4>();
        instance.dataValue = val.dataValue;
    }

    bool ValueObjVec4DistPropertyHandler::ReadValuesIntoGUI(
        size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        GUI->SetValue(const_cast<property_t&>(instance));
        return false;
    }

    void ValueObjColorDistPropertyHandler::WriteGUIValuesIntoProperty(
        size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        OpenParticle::ValueObjColor val = GUI->GetValue<OpenParticle::ValueObjColor>();
        instance.dataValue = val.dataValue;
    }

    bool ValueObjColorDistPropertyHandler::ReadValuesIntoGUI(
        size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        GUI->SetValue(const_cast<property_t&>(instance));
        return false;
    }

    void ValueObjLinearDistPropertyHandler::WriteGUIValuesIntoProperty(
        size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        OpenParticle::ValueObjLinear val = GUI->GetValue<OpenParticle::ValueObjLinear>();
        instance.dataValue = val.dataValue;
    }

    bool ValueObjLinearDistPropertyHandler::ReadValuesIntoGUI(
        size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);

        GUI->SetValue(const_cast<property_t&>(instance));
        return false;
    }
} // namespace OpenParticleSystemEditor
