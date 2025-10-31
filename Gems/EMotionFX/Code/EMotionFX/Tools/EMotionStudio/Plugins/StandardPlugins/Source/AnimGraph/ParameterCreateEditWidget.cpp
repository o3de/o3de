/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <Editor/InspectorBus.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditWidget.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ParameterEditorFactory.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ValueParameterEditor.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>


namespace EMStudio
{
    const int ParameterCreateEditWidget::s_parameterEditorMinWidth = 300;

    ParameterCreateEditWidget::ParameterCreateEditWidget(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
        , m_plugin(plugin)
        , m_valueTypeCombo(nullptr)
        , m_valueParameterEditor(nullptr)
    {
        setObjectName("ParameterCreateEditWidget");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_createButton = new QPushButton(tr("Create"), this);
        m_createButton->setObjectName("EMFX.ParameterCreateEditWidget.CreateApplyButton");
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // Value Type
        QHBoxLayout* valueTypeLayout = new QHBoxLayout();
        m_valueTypeLabel = new QLabel("Value type", this);
        m_valueTypeLabel->setFixedWidth(100);
        valueTypeLayout->addItem(new QSpacerItem(4, 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        valueTypeLayout->addWidget(m_valueTypeLabel);
        m_valueTypeCombo = new QComboBox(this);
        m_valueTypeCombo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        valueTypeLayout->addWidget(m_valueTypeCombo);
        valueTypeLayout->addItem(new QSpacerItem(2, 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        connect(m_valueTypeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ParameterCreateEditWidget::OnValueTypeChange);
        mainLayout->addItem(valueTypeLayout);

        // Add the RPE for the parameter
        m_parameterEditorWidget = aznew AzToolsFramework::ReflectedPropertyEditor(this);
        m_parameterEditorWidget->setObjectName("EMFX.ParameterCreateEditWidget.ReflectedPropertyEditor.ParameterEditorWidget");
        m_parameterEditorWidget->SetAutoResizeLabels(false);
        m_parameterEditorWidget->setSizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::MinimumExpanding);
        m_parameterEditorWidget->SetSizeHintOffset(QSize(0, 0));
        m_parameterEditorWidget->SetLeafIndentation(0);
        m_parameterEditorWidget->setMinimumWidth(s_parameterEditorMinWidth);
        mainLayout->addWidget(m_parameterEditorWidget);

        // Add the preview information
        QHBoxLayout* previewLayout = new QHBoxLayout();
        m_previewFrame = new QFrame(this);
        m_previewFrame->setFrameShape(QFrame::Box);
        m_previewFrame->setObjectName("previewFrame");
        m_previewFrame->setStyleSheet("QFrame#previewFrame { border: 2px dashed #979797; background-color: #85858580; }");
        m_previewFrame->setLayout(new QHBoxLayout());
        QLabel* previewLabel = new QLabel("Preview", m_previewFrame);
        previewLabel->setAutoFillBackground(false);
        previewLabel->setStyleSheet("background: transparent");
        m_previewFrame->layout()->addWidget(previewLabel);
        m_previewWidget = new AzToolsFramework::ReflectedPropertyEditor(m_previewFrame);
        m_previewWidget->SetAutoResizeLabels(false);
        m_previewWidget->SetLeafIndentation(0);
        m_previewWidget->setStyleSheet("QFrame, .QWidget, QSlider, QCheckBox { background-color: transparent }");
        m_previewFrame->layout()->addWidget(m_previewWidget);
        previewLayout->addSpacerItem(new QSpacerItem(100, 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
        previewLayout->addWidget(m_previewFrame);
        mainLayout->addItem(previewLayout);
        mainLayout->addItem(new QSpacerItem(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed));

        // create the create or apply button and the cancel button
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        QPushButton* cancelButton = new QPushButton("Cancel", this);
        buttonLayout->addWidget(m_createButton);
        buttonLayout->addWidget(cancelButton);
        mainLayout->addItem(buttonLayout);
        connect(m_createButton, &QPushButton::clicked, this, &ParameterCreateEditWidget::OnValidate);
        connect(cancelButton, &QPushButton::clicked, this, [this]()
            {
                EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::ClearIfShown, this);
            });

        setLayout(mainLayout);
    }

    ParameterCreateEditWidget::~ParameterCreateEditWidget()
    {
        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::ClearIfShown, this);

        if (m_previewWidget)
        {
            m_previewWidget->ClearInstances();
            delete m_previewWidget;
        }

        if (m_valueParameterEditor)
        {
            delete m_valueParameterEditor;
        }

        if (m_parameterEditorWidget)
        {
            m_parameterEditorWidget->ClearInstances();
            delete m_parameterEditorWidget;
        }
    }

    void ParameterCreateEditWidget::Reinit(const EMotionFX::Parameter* editParameter)
    {
        if (!editParameter)
        {
            m_parameter.reset();
            m_createButton->setText("Create");
            m_originalName.clear();
        }
        else
        {
            m_parameter.reset(MCore::ReflectionSerializer::Clone(editParameter));
            m_createButton->setText("Apply");
            m_originalName = editParameter->GetName();
        }

        const bool showType = !editParameter || (editParameter && editParameter->RTTI_IsTypeOf(azrtti_typeid<EMotionFX::ValueParameter>()));
        m_valueTypeLabel->setVisible(showType);
        m_valueTypeCombo->setVisible(showType);
        m_valueTypeCombo->setEnabled(showType);

        if (showType)
        {
            // We're editing a value parameter, allow to change the type to any other value parameter.
            const AZStd::vector<AZ::TypeId> parameterTypes = EMotionFX::ParameterFactory::GetValueParameterTypes();

            m_valueTypeCombo->blockSignals(true);
            m_valueTypeCombo->clear();

            // Register all children of EMotionFX::ValueParameter
            for (const AZ::TypeId& parameterType : parameterTypes)
            {
                AZStd::unique_ptr<EMotionFX::Parameter> param(EMotionFX::ParameterFactory::Create(parameterType));
                m_valueTypeCombo->addItem(QString(param->GetTypeDisplayName()), QString(parameterType.ToString<AZStd::string>().c_str()));
            }

            if (m_parameter)
            {
                m_valueTypeCombo->setCurrentText(m_parameter->GetTypeDisplayName());
            }

            m_valueTypeCombo->blockSignals(false);
        }

        if (!m_parameter)
        {
            OnValueTypeChange(0);
        }
        else
        {
            InitDynamicInterface(azrtti_typeid(m_parameter.get()));
        }
    }

    // init the dynamic part of the interface
    void ParameterCreateEditWidget::InitDynamicInterface(const AZ::TypeId& valueType)
    {
        if (valueType.IsNull())
        {
            return;
        }

        if (!m_parameter)
        {
            const AZStd::string uniqueParameterName = MCore::GenerateUniqueString("Parameter",
                    [&](const AZStd::string& value)
                    {
                        return (m_plugin->GetActiveAnimGraph()->FindParameterByName(value) == nullptr);
                    });
            m_parameter.reset(EMotionFX::ParameterFactory::Create(valueType));
            m_parameter->SetName(uniqueParameterName);
        }
        else
        {
            if (azrtti_typeid(m_parameter.get()) != valueType) // value type changed
            {
                const AZStd::string oldParameterName = m_parameter->GetName();
                const AZStd::string oldParameterDescription = m_parameter->GetDescription();
                m_parameter.reset(EMotionFX::ParameterFactory::Create(valueType));
                m_parameter->SetName(oldParameterName);
                m_parameter->SetDescription(oldParameterDescription);
            }
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return;
        }

        m_parameterEditorWidget->ClearInstances();
        m_parameterEditorWidget->AddInstance(m_parameter.get(), azrtti_typeid(m_parameter.get()));
        m_parameterEditorWidget->Setup(serializeContext, this, false, 100);
        m_parameterEditorWidget->show();
        m_parameterEditorWidget->ExpandAll();
        m_parameterEditorWidget->InvalidateAll();

        m_previewWidget->ClearInstances();
        if (m_valueParameterEditor)
        {
            delete m_valueParameterEditor;
            m_valueParameterEditor = nullptr;
        }

        if (azrtti_typeid(m_parameter.get()) == azrtti_typeid<EMotionFX::GroupParameter>())
        {
            m_previewFrame->setVisible(false);
        }
        else
        {
            m_valueParameterEditor = ParameterEditorFactory::Create(nullptr, static_cast<const EMotionFX::ValueParameter*>(m_parameter.get()), AZStd::vector<MCore::Attribute*>());
            m_previewWidget->AddInstance(m_valueParameterEditor, azrtti_typeid(m_valueParameterEditor));
            m_previewWidget->Setup(serializeContext, nullptr, false, 0);
            m_previewWidget->show();
            m_previewWidget->ExpandAll();
            m_previewWidget->InvalidateAll();
            m_previewFrame->setVisible(true);
        }

        adjustSize();
    }


    // when the value type changes
    void ParameterCreateEditWidget::OnValueTypeChange(int valueType)
    {
        if (m_valueTypeCombo->isEnabled() && valueType != -1)
        {
            QVariant variantData = m_valueTypeCombo->itemData(valueType);
            AZ_Assert(variantData != QVariant::Invalid, "Expected valid variant data");
            const AZStd::string typeId = FromQtString(variantData.toString());
            InitDynamicInterface(AZ::TypeId::CreateString(typeId.c_str(), typeId.size()));
        }
    }


    // check if we can create this parameter
    void ParameterCreateEditWidget::OnValidate()
    {
        EMotionFX::AnimGraph* animGraph = m_plugin->GetActiveAnimGraph();
        if (!animGraph)
        {
            MCore::LogWarning("ParameterCreateEditWidget::OnValidate() - No AnimGraph active!");
            return;
        }

        const AZStd::string& parameterName = m_parameter->GetName();

        if (parameterName.empty())
        {
            QMessageBox::warning(this, "Please Provide A Parameter Name", "The parameter name cannot be empty!");
            return;
        }

        // Check if the name has invalid characters.
        AZStd::string invalidCharacters;
        if (!EMotionFX::Parameter::IsNameValid(parameterName, &invalidCharacters))
        {
            const AZStd::string errorString = AZStd::string::format("The parameter name contains invalid characters %s", invalidCharacters.c_str());
            QMessageBox::warning(this, "Parameter Name Invalid", errorString.c_str());
            return;
        }

        // check if the name already exists
        if ((m_createButton->text() == "Create" && animGraph->FindParameterByName(parameterName))
            || (parameterName != m_originalName && animGraph->FindParameterByName(parameterName)))
        {
            const AZStd::string errorString = AZStd::string::format("Parameter with name '<b>%s</b>' already exists in anim graph '<b>%s</b>'.<br><br><i>Please use a unique parameter name.</i>",
                parameterName.c_str(), animGraph->GetFileName());
            QMessageBox::warning(this, "Parameter name is not unique", errorString.c_str());
            return;
        }

        emit accept();
    }

    void ParameterCreateEditWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode*)
    {
        m_previewWidget->InvalidateAttributesAndValues();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_ParameterCreateEditWidget.cpp>
