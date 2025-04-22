/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BlendNParamWeightsHandler.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLocale>
#include <AzCore/std/containers/unordered_map.h>
#include <EMotionFX/Source/AnimGraphNode.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendNParamWeightContainerWidget, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendNParamWeightsHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendNParamWeightElementWidget, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendNParamWeightElementHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(BlendNParamWeightGuiEntry, EditorAllocator)


    const int BlendNParamWeightElementWidget::s_decimalPlaces = 2;

    BlendNParamWeightGuiEntry::BlendNParamWeightGuiEntry(AZ::u32 portId, float weightRange, const char* sourceNodeName) 
        : m_portId(portId),
          m_weightRange(weightRange),
          m_sourceNodeName(sourceNodeName)
    { }

    const char* BlendNParamWeightGuiEntry::GetSourceNodeName() const
    {
        return m_sourceNodeName;
    }

    const AZStd::string& BlendNParamWeightGuiEntry::GetTooltipText() const
    {
        return m_tooltipText;
    }

    void BlendNParamWeightGuiEntry::SetTooltipText(const AZStd::string& text)
    {
        m_tooltipText = text;
    }

    const char* BlendNParamWeightGuiEntry::GetPortLabel() const
    {
        return BlendTreeBlendNNode::GetPoseInputPortName(m_portId);
    }

    AZ::u32 BlendNParamWeightGuiEntry::GetPortId() const
    {
        return m_portId;
    }

    float BlendNParamWeightGuiEntry::GetWeightRange() const
    {
        return m_weightRange;
    }

    void BlendNParamWeightGuiEntry::SetWeightRange(float value)
    {
        m_weightRange = value;
    }

    void BlendNParamWeightGuiEntry::SetValid(bool valid)
    {
        m_isValid = valid;
    }

    bool BlendNParamWeightGuiEntry::IsValid()const
    {
        return m_isValid;
    }


    BlendNParamWeightElementWidget::BlendNParamWeightElementWidget(QWidget* parent) 
        : QWidget(parent)
    {
        QHBoxLayout* hLayout = new QHBoxLayout(this);

        hLayout->setMargin(0);
        setLayout(hLayout);
        m_sourceNodeNameLabel = new QLabel("element A", this);
        layout()->addWidget(m_sourceNodeNameLabel);
        m_weightField = new AzQtComponents::DoubleSpinBox(this);
        m_weightField->setRange(-FLT_MAX, FLT_MAX);
        m_weightField->setDecimals(s_decimalPlaces);
        layout()->addWidget(m_weightField);

        connect(m_weightField
            , qOverload<double>(&QDoubleSpinBox::valueChanged)
            , this, &BlendNParamWeightElementWidget::OnWeightRangeEdited);
    }

    BlendNParamWeightElementWidget::~BlendNParamWeightElementWidget()
    {
        if (m_parentContainerWidget)
        {
            m_parentContainerWidget->RemoveElementWidget(this);
        }
    }

    void BlendNParamWeightElementWidget::OnWeightRangeEdited([[maybe_unused]] double value)
    {
        emit DataChanged(this);
    }


    void BlendNParamWeightElementWidget::SetDataSource(const BlendNParamWeightGuiEntry& paramWeight)
    {
        m_paramWeight = &paramWeight;
    }

    float BlendNParamWeightElementWidget::GetWeightRange() const
    {
        return aznumeric_cast<float>(m_weightField->value());
    }

    void BlendNParamWeightElementWidget::UpdateGui()
    {
        m_sourceNodeNameLabel->setText(m_paramWeight->GetSourceNodeName());
        m_weightField->setValue(m_paramWeight->GetWeightRange());
        if (m_paramWeight->IsValid())
        {
            AzQtComponents::SpinBox::setHasError(m_weightField, false);
            m_weightField->setToolTip("");
        }
        else
        {
            AzQtComponents::SpinBox::setHasError(m_weightField, true);
            m_weightField->setToolTip(m_paramWeight->GetTooltipText().c_str());
        }
    }

    void BlendNParamWeightElementWidget::SetId(size_t index)
    {
        m_dataElementIndex = index;
    }

    size_t BlendNParamWeightElementWidget::GetId() const
    {
        return m_dataElementIndex;
    }


    BlendNParamWeightContainerWidget::BlendNParamWeightContainerWidget(QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout(parent);

        vLayout->setMargin(0);
        setLayout(vLayout);

        QHBoxLayout* hHeaderLayout = new QHBoxLayout(this);
        QLabel* inputNodeLabel = new QLabel("Input node", this);
        QLabel* weightRanges = new QLabel("Max weight trigger", this);

        hHeaderLayout->addWidget(inputNodeLabel);
        hHeaderLayout->addWidget(weightRanges);

        QHBoxLayout* hButtonLayout = new QHBoxLayout(this);

        QPushButton* buttonEqualize = new QPushButton("Evenly distribute", this);
        hButtonLayout->addWidget(buttonEqualize);
        
        vLayout->addLayout(hButtonLayout);
        vLayout->addLayout(hHeaderLayout);

        connect(buttonEqualize, &QPushButton::pressed, this, [this]()
        {
            EqualizeWeightRanges();
            SetAllValid();
            Update();
            emit DataChanged();
        });

        EMotionFX::AnimGraphNotificationBus::Handler::BusConnect();
    }

    BlendNParamWeightContainerWidget::~BlendNParamWeightContainerWidget()
    {
        EMotionFX::AnimGraphNotificationBus::Handler::BusDisconnect();
    }

    const AZStd::vector<BlendNParamWeightGuiEntry>& BlendNParamWeightContainerWidget::GetParamWeights() const
    {
        return m_paramWeights;
    }

    void BlendNParamWeightContainerWidget::EqualizeWeightRanges()
    {
        if (!m_paramWeights.empty())
        {
            const float first = m_paramWeights.front().GetWeightRange();
            const float last = m_paramWeights.back().GetWeightRange();
            const float min = first < last ? first : last;
            const float max = first < last ? last : first;
            EqualizeWeightRanges(min, max);
        }
    }

    void BlendNParamWeightContainerWidget::EqualizeWeightRanges(float min, float max)
    {
        if (m_paramWeights.empty())
        {
            return;
        }

        if (AZ::IsClose(min, max, AZ::Constants::FloatEpsilon))
        {
            min = 0.0f;
            max = 1.0f;
        }

        float weightRange = min;
        const size_t paramWeightsSize = m_paramWeights.size();
        const float weightStep = (max - min) / (paramWeightsSize - 1);
        m_paramWeights.back().SetWeightRange(max);
        for (size_t i = 0; i < paramWeightsSize - 1; ++i)
        {
            m_paramWeights[i].SetWeightRange(weightRange);
            weightRange += weightStep;
        }
    }

    void BlendNParamWeightContainerWidget::SetParamWeights(const AZStd::vector<BlendNParamWeight>& paramWeights, const AnimGraphNode* node)
    {
        m_paramWeights.clear();
        const size_t paramWeightsCount = paramWeights.size();
        m_paramWeights.reserve(paramWeightsCount);
        for (size_t i = 0; i < paramWeightsCount; ++i)
        {
            const auto& inputPorts = node->GetInputPorts();
            const char* sourceNodeName = "";
            for (const AnimGraphNode::Port& port : inputPorts)
            {
                if (port.m_connection)
                {
                    if (port.m_portId == paramWeights[i].GetPortId())
                    {
                        sourceNodeName = port.m_connection->GetSourceNode()->GetName();
                    }
                }
            }

            m_paramWeights.emplace_back(paramWeights[i].GetPortId(), paramWeights[i].GetWeightRange(), sourceNodeName);
        }

        UpdateDataValidation();
    }

    void BlendNParamWeightContainerWidget::HandleOnChildWidgetDataChanged(BlendNParamWeightElementWidget* elementWidget)
    {
        size_t widgetId = elementWidget->GetId();
        if (widgetId >= m_paramWeights.size())
        {
            AZ_Error("EMotionFX", false, "Weight parameter widget incorrectly initialized");
            return;
        }

        m_paramWeights[widgetId].SetWeightRange(elementWidget->GetWeightRange());
        if (CheckElementValidation(widgetId))
        {
            if (CheckAllElementsValidation())
            {
                SetAllValid();
                Update();
                emit DataChanged();
            }
        }
        else
        {
            m_paramWeights[widgetId].SetValid(false);
            AZStd::string decPlacesStr = AZStd::string::format("%u", BlendNParamWeightElementWidget::s_decimalPlaces);
            if (widgetId == 0)
            {
                m_paramWeights[widgetId].SetTooltipText(AZStd::string::format("The value has to be less than or equal %.2f", m_paramWeights[widgetId + 1].GetWeightRange()));
            }
            else if (widgetId == m_paramWeights.size() - 1)
            {
                m_paramWeights[widgetId].SetTooltipText(AZStd::string::format("The value has to be more than or equal %.2f", m_paramWeights[widgetId - 1].GetWeightRange()));
            }
            else
            {
                m_paramWeights[widgetId].SetTooltipText(AZStd::string::format("The value has to be between %.2f and %.2f", m_paramWeights[widgetId - 1].GetWeightRange(), m_paramWeights[widgetId + 1].GetWeightRange()));
            }
            elementWidget->UpdateGui();
        }
    }

    void BlendNParamWeightContainerWidget::AddElementWidget(BlendNParamWeightElementWidget* widget)
    {
        if (AZStd::find(m_elementWidgets.begin(), m_elementWidgets.end(), widget) == m_elementWidgets.end())
        {
            m_elementWidgets.push_back(widget);
        }
    }

    void BlendNParamWeightContainerWidget::RemoveElementWidget(BlendNParamWeightElementWidget* widget)
    {
        m_elementWidgets.erase(AZStd::remove(m_elementWidgets.begin(), m_elementWidgets.end(), widget), m_elementWidgets.end());
    }

    void BlendNParamWeightContainerWidget::Update()
    {
        for (BlendNParamWeightElementWidget* elementWidget : m_elementWidgets)
        {
            elementWidget->UpdateGui();
        }
    }

    void BlendNParamWeightContainerWidget::ConnectWidgetToDataSource(BlendNParamWeightElementWidget* elementWidget)
    {
        elementWidget->SetId(m_widgetBoundToDataCount++);
        size_t index = elementWidget->GetId();

        if (index >= m_paramWeights.size())
        {
            AZ_Error("EMotionFX", false, "Property widget incorrectly initialized");
            return;
        }
        elementWidget->SetDataSource(m_paramWeights[index]);
        elementWidget->UpdateGui();
        elementWidget->SetParentContainerWidget(this);
        AddElementWidget(elementWidget); // Adds it only in case it hasn't been added yet.


        connect(elementWidget, &BlendNParamWeightElementWidget::DataChanged, this, &BlendNParamWeightContainerWidget::HandleOnChildWidgetDataChanged, Qt::UniqueConnection);
    }

    void BlendNParamWeightContainerWidget::SetAllValid()
    {
        for (BlendNParamWeightGuiEntry& paramWeight : m_paramWeights)
        {
            paramWeight.SetValid(true);
        }
    }

    bool BlendNParamWeightContainerWidget::CheckAllElementsValidation()
    {
        const size_t paramWeightsSize = m_paramWeights.size();
        for (size_t i = 0; i < paramWeightsSize; ++i)
        {
            if (!CheckElementValidation(i))
            {
                return false;
            }
        }
        return true;
    }

    void BlendNParamWeightContainerWidget::UpdateDataValidation()
    {
        const size_t paramWeightsSize = m_paramWeights.size();
        for(size_t i = 0; i < paramWeightsSize; ++i)
        {
            m_paramWeights[i].SetValid(CheckElementValidation(i));
        }
    }

    bool BlendNParamWeightContainerWidget::CheckElementValidation(size_t index)
    {
        if (index > 0)
        {
            if (m_paramWeights[index].GetWeightRange() < m_paramWeights[index - 1].GetWeightRange())
            {
                return false;
            }
        }
        if (index < m_paramWeights.size() - 1)
        {
            if (m_paramWeights[index + 1].GetWeightRange() < m_paramWeights[index].GetWeightRange())
            {
                return false;
            }
        }
        return true;
    }

    bool BlendNParamWeightContainerWidget::CheckValidation()
    {
        for (BlendNParamWeightGuiEntry paramWeight : m_paramWeights)
        {
            if (!paramWeight.IsValid())
            {
                return false;
            }
        }
        return true;
    }

    void BlendNParamWeightContainerWidget::OnSyncVisualObject(AnimGraphObject* object)
    {
        if (azrtti_istypeof<EMotionFX::BlendTreeBlendNNode>(object))
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh, AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
        }
    }

    AZ::u32 BlendNParamWeightElementHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("BlendNParamWeightsElementHandler");
    }

    QWidget* BlendNParamWeightElementHandler::CreateGUI(QWidget* parent)
    {
        BlendNParamWeightElementWidget* paramWeightElementWidget = aznew BlendNParamWeightElementWidget(parent);
        return paramWeightElementWidget;
    }

    bool BlendNParamWeightElementHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, BlendNParamWeightElementWidget* GUI, [[maybe_unused]] const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        BlendNParemWeightWidgetNotificationBus::Broadcast(&BlendNParemWeightWidgetNotificationBus::Events::OnRequestDataBind, GUI);
        return true;
    }


    BlendNParamWeightsHandler::BlendNParamWeightsHandler()
    {
        BlendNParemWeightWidgetNotificationBus::Handler::BusConnect();
    }
    
    BlendNParamWeightsHandler::~BlendNParamWeightsHandler()
    {
        BlendNParemWeightWidgetNotificationBus::Handler::BusDisconnect();
    }

    AZ::u32 BlendNParamWeightsHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("BlendNParamWeightsContainerHandler");
    }

    QWidget* BlendNParamWeightsHandler::CreateGUI(QWidget* parent)
    {
        BlendNParamWeightContainerWidget* paramWeightsWidget = aznew BlendNParamWeightContainerWidget(parent);

        connect(paramWeightsWidget, &BlendNParamWeightContainerWidget::DataChanged, this, [paramWeightsWidget]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, paramWeightsWidget);
        });
        m_containerWidget = paramWeightsWidget;
        return paramWeightsWidget;
    }

    void BlendNParamWeightsHandler::ConsumeAttribute([[maybe_unused]] BlendNParamWeightContainerWidget* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ_CRC_CE("BlendTreeBlendNNodeParamWeightsElement") && attrValue)
        {
            m_node = static_cast<AnimGraphNode*>(attrValue->GetInstance());
        }
    }

    void BlendNParamWeightsHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, BlendNParamWeightContainerWidget* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        const AZStd::vector<BlendNParamWeightGuiEntry>& paramWeights = GUI->GetParamWeights();
        instance.clear();
        for (size_t i = 0; i < paramWeights.size(); ++i)
        {
            instance.emplace_back(paramWeights[i].GetPortId(), paramWeights[i].GetWeightRange());
        }
    }

    bool BlendNParamWeightsHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, BlendNParamWeightContainerWidget* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetParamWeights(instance, m_node);
        return true;
    }

    void BlendNParamWeightsHandler::OnRequestDataBind(BlendNParamWeightElementWidget* elementWidget)
    {
        // Create a QT connection between elementWidget and m_containerWidget;
        m_containerWidget->ConnectWidgetToDataSource(elementWidget);
    }
}
