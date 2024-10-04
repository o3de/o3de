/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Utils.h>
#include <AzCore/ScriptCanvas/ScriptCanvasOnDemandNames.h>

#include <Editor/View/Dialogs/ContainerWizard/ContainerWizard.h>
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <Editor/View/Dialogs/ContainerWizard/ui_ContainerWizard.h>
AZ_POP_DISABLE_WARNING

#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <Editor/View/Dialogs/ContainerWizard/ContainerTypeLineEdit.h>
#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>

#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>

#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvasEditor
{
    ////////////////////
    // ContainerWizard
    ////////////////////

    ContainerWizard::ContainerWizard(QWidget* parent)
        : QDialog(parent)
        , m_serializeContext(nullptr)
        , m_validationAction(nullptr)
        , m_invalidIcon(":/ScriptCanvasEditorResources/Resources/error_icon.png")
        , m_dataTypeMenuVisibile(false)
        , m_releaseVariable(false)
        , m_variableCounter(0)
        , m_ui(new Ui::ContainerWizard())
    {
        m_ui->setupUi(this);

        QObject::connect(m_ui->containerTypeBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ContainerWizard::OnContainerTypeChanged);
        m_ui->containerTypeBox->setEditable(false);

        // Don't want to enable enter triggering through the default mechanism for the create/cancel, since it causes a bunch of accidental triggers
        // of submission while editing. Instead install an event filter and deal with this internally.
        m_ui->createButton->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
        m_ui->createButton->setAutoDefault(false);
        m_ui->createButton->setDefault(false);
        m_ui->createButton->installEventFilter(this);

        m_ui->cancelButton->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
        m_ui->cancelButton->setAutoDefault(false);
        m_ui->cancelButton->setDefault(false);
        m_ui->cancelButton->installEventFilter(this);

        QObject::connect(m_ui->createButton, &QPushButton::clicked, this, &ContainerWizard::OnCreate);
        QObject::connect(m_ui->cancelButton, &QPushButton::clicked, this, &ContainerWizard::OnCancel);

        QObject::connect(this, &QDialog::finished, this, &ContainerWizard::OnFinished);

        QObject::connect(m_ui->variableName, &QLineEdit::textChanged, this, &ContainerWizard::ValidateName);
    }

    ContainerWizard::~ContainerWizard()
    {
        ClearDisplay();

        for (ContainerTypeLineEdit* lineEdit : m_containerTypeLineEdit)
        {
            delete lineEdit;
        }
    }

    void ContainerWizard::SetActiveScriptCanvasId(const ScriptCanvas::ScriptCanvasId& scriptCanvasId)
    {
        m_activeScriptCanvasId = scriptCanvasId;
    }

    void ContainerWizard::RegisterType(const AZ::TypeId& dataType)
    {
        if (AZ::Utils::IsContainerType(dataType))
        {
            RegisterContainerType(dataType);
        }
        else
        {
            RegisterDataType(dataType);
        }
    }

    void ContainerWizard::ShowWizard(const AZ::TypeId& genericContainerType)
    {
        // Always default the wizard to the unchecked state
        m_ui->checkBox->setChecked(false);

        if (m_ui->containerTypeBox->count() != m_genericContainerTypes.size())
        {
            QSignalBlocker signalBlock(m_ui->containerTypeBox);
            std::sort(m_genericContainerTypeNames.begin(), m_genericContainerTypeNames.end(), [](const AZStd::pair< AZStd::string, AZ::TypeId>& lhs,
                const AZStd::pair< AZStd::string, AZ::TypeId>& rhs)
            {
                return AZStd::less<AZStd::string>()(lhs.first, rhs.first);
            });

            m_ui->containerTypeBox->clear();

            for (const auto& element : m_genericContainerTypeNames)
            {
                m_ui->containerTypeBox->addItem(element.first.c_str());
            }
        }

        for (int i = 0; i < m_genericContainerTypeNames.size(); ++i)
        {
            const auto& element = m_genericContainerTypeNames[i];

            if (element.second == genericContainerType)
            {
                QSignalBlocker signalBlock(m_ui->containerTypeBox);
                m_ui->containerTypeBox->setCurrentIndex(i);
            }
        }

        // Need to show before trying to initialize the display otherwise the line edits won't be cleaned up
        // correctly.
        show();
        InitializeDisplay(genericContainerType);

        m_releaseVariable = true;

        bool nameAvailable = false;
        AZStd::string variableName;

        do
        {
            SceneCounterRequestBus::EventResult(m_variableCounter, m_activeScriptCanvasId, &SceneCounterRequests::GetNewVariableCounter);

            variableName = VariableDockWidget::ConstructDefaultVariableName(m_variableCounter);

            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(nameAvailable, m_activeScriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::IsNameAvailable, variableName);
        } while (!nameAvailable);

        m_ui->variableName->setText(variableName.c_str());
        m_ui->variableName->setFocus(Qt::FocusReason::MouseFocusReason);
        m_ui->variableName->setSelection(0, m_ui->variableName->text().size());
    }

    void ContainerWizard::accept()
    {
        QDialog::accept();
    }

    void ContainerWizard::reject()
    {
        if (m_dataTypeMenuVisibile)
        {
            for (ContainerTypeLineEdit* lineEdit : m_containerTypeLineEdit)
            {
                lineEdit->HideDataTypeMenu();
            }

            m_dataTypeMenuVisibile = false;
        }
        else
        {
            QDialog::reject();
        }
    }

    void ContainerWizard::hideEvent(QHideEvent* hideEvent)
    {
        QDialog::hideEvent(hideEvent);

        for (ContainerTypeLineEdit* lineEdit : m_containerTypeLineEdit)
        {
            lineEdit->HideDataTypeMenu();
        }

        m_dataTypeMenuVisibile = false;
    }

    bool ContainerWizard::eventFilter(QObject* object, QEvent* event)
    {
        if (object == m_ui->createButton
            || object == m_ui->cancelButton)
        {
            if (event->type() == QEvent::Type::KeyRelease)
            {
                QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

                if (keyEvent->key() == Qt::Key_Enter
                    || keyEvent->key() == Qt::Key_Return)
                {
                    QPushButton* button = qobject_cast<QPushButton*>(object);

                    if (button)
                    {
                        button->click();
                    }
                }
            }
        }

        return false;
    }

    const AZStd::unordered_map<AZ::Crc32, AZ::TypeId>& ContainerWizard::GetFinalTypeMapping() const
    {
        return m_finalContainerTypeIds;
    }

    void ContainerWizard::ReparseDisplay()
    {
        AZ::Crc32 workingCrc = AZ::Crc32(m_genericType.ToString<AZStd::string>().c_str());
        
        for (int typeIndex = 0; typeIndex < m_containerTypes.size(); ++typeIndex)
        {
            const AZ::TypeId& typeId = m_containerTypes[typeIndex];
            ContainerTypeLineEdit* lineEdit = GetLineEdit(typeIndex);            

            auto dataTypeIter = m_containerDataTypeSets.find(workingCrc);
            if (dataTypeIter == m_containerDataTypeSets.end())
            {
                AZ_Error("ScriptCanvas", false, "Unknown partial type found in Container Creation. Aborting.");
                close();
                break;
            }

            lineEdit->SetDataTypes(dataTypeIter->second);

            AZ::TypeId selectedTypeId = typeId;
            
            {
                QSignalBlocker signalBlocker(lineEdit);

                if (!lineEdit->DisplayType(typeId))
                {
                    selectedTypeId = lineEdit->GetDefaultTypeId();
                    lineEdit->DisplayType(selectedTypeId);
                }
            }

            lineEdit->update();
            
            m_containerTypes[typeIndex] = selectedTypeId;
            workingCrc.Add(selectedTypeId.ToString<AZStd::string>().c_str());            
        }
    }
    
    void ContainerWizard::OnCreate()
    {
        AZ::Crc32 typeCrc = AZ::Crc32(m_genericType.ToString<AZStd::string>().c_str());

        for (const AZ::TypeId& typeId : m_containerTypes)
        {
            typeCrc.Add(typeId.ToString<AZStd::string>().c_str());
        }

        auto containerIter = m_finalContainerTypeIds.find(typeCrc);
        
        if (containerIter != m_finalContainerTypeIds.end())
        {
            m_releaseVariable = false;
            AZStd::string variableName = m_ui->variableName->text().toUtf8().data();

            if (m_ui->checkBox->isChecked())
            {
                AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC_CE("ScriptCanvasPreviewSettings"), AZ::UserSettings::CT_LOCAL);

                if (settings)
                {       
                    auto insertResult = settings->m_pinnedDataTypes.insert(containerIter->second);

                    if (insertResult.second)
                    {
                        Q_EMIT ContainerPinned(containerIter->second);
                    }
                }
            }

            Q_EMIT CreateContainerVariable(variableName, containerIter->second);
            close();
        }
        else
        {
            AZ_Warning("ScriptCanvas", false, "Unable to find Registered type with the given parameters.");
        }
    }
    
    void ContainerWizard::OnCancel()
    {
        close();
    }

    void ContainerWizard::OnFinished([[maybe_unused]] int result)
    {
        ClearDisplay();

        if (m_releaseVariable)
        {
            SceneCounterRequestBus::Event(m_activeScriptCanvasId, &SceneCounterRequests::ReleaseVariableCounter, m_variableCounter);
        }
    }

    void ContainerWizard::OnContainerTypeChanged(int index)
    {
        if (index >= 0 && index < m_genericContainerTypeNames.size())
        {
            InitializeDisplay(m_genericContainerTypeNames[index].second);
        }
    }

    void ContainerWizard::OnTypeChanged(int index, const AZ::TypeId& typeId)
    {
        if (index >= 0 && index < m_containerTypes.size())
        {
            m_containerTypes[index] = typeId;
            ReparseDisplay();
        }
    }

    void ContainerWizard::OnDataTypeMenuVisibilityChanged(bool visible)
    {
        m_dataTypeMenuVisibile = visible;
    }

    void ContainerWizard::ValidateName(const QString& newName)
    {
        if (m_validationAction)
        {
            m_ui->variableName->removeAction(m_validationAction);
            delete m_validationAction;
            m_validationAction = nullptr;
        }

        ScriptCanvas::VariableValidationOutcome validName = AZ::Failure(ScriptCanvas::GraphVariableValidationErrorCode::Unknown);

        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(validName, m_activeScriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::IsNameValid, newName.toUtf8().data());

        if (!validName || newName.isEmpty())
        {
            m_validationAction = m_ui->variableName->addAction(m_invalidIcon, QLineEdit::TrailingPosition);

            if (validName.GetError() == ScriptCanvas::GraphVariableValidationErrorCode::Invalid)
            {
                m_validationAction->setToolTip("A Variable name cannot be empty or over 200 characters.\nPlease specify a new name for the variable.");
            }
            else if (validName.GetError() == ScriptCanvas::GraphVariableValidationErrorCode::Duplicate)
            {
                m_validationAction->setToolTip("This name is already in use by\nanother variable");
            }
        }

        m_ui->createButton->setEnabled((validName && !newName.isEmpty()));
    }

    void ContainerWizard::ClearDisplay()
    {
        m_containerTypes.clear();

        while (m_ui->typeSelectionFrame->layout()->count() > 0)
        {
            m_ui->typeSelectionFrame->layout()->takeAt(0);
        }

        for (ContainerTypeLineEdit* lineEdit : m_containerTypeLineEdit)
        {
            if (lineEdit->isVisible())
            {
                lineEdit->ResetLineEdit();
                lineEdit->setVisible(false);
            }
            else
            {
                break;
            }
        }
    }

    void ContainerWizard::InitializeDisplay(const AZ::TypeId& typeId)
    {
        m_genericType = typeId;

        if (AZ::Utils::IsMapContainerType(m_genericType))
        {
            PopulateMapDisplay();
        }
        else
        {
            PopulateGeneralDisplay();
        }

        adjustSize();
    }

    void ContainerWizard::PopulateMapDisplay()
    {
        const AZStd::vector< AZStd::string > typeLabels = { "Key", "Value" };
        PopulateGeneralDisplay("Map %i", "Map", typeLabels);
    }

    void ContainerWizard::PopulateGeneralDisplay(const AZStd::string& patternFallback, const AZStd::string& singleTypeString, const AZStd::vector< AZStd::string >& typeLabels)
    {
        ClearDisplay();

        AZ::Crc32 workingCrc = AZ::Crc32(m_genericType.ToString<AZStd::string>().c_str());

        int containerIndex = 0;
        auto dataTypeIter = m_containerDataTypeSets.find(workingCrc);

        QWidget* focusWidget = m_ui->variableName;

        while (dataTypeIter != m_containerDataTypeSets.end())
        {
            DataTypeSet& dataTypeSet = dataTypeIter->second;

            ContainerTypeLineEdit* lineEdit = GetLineEdit(containerIndex);

            lineEdit->ResetLineEdit();

            if (containerIndex >= typeLabels.size())
            {
                AZStd::string containerName = AZStd::string::format(patternFallback.c_str(), containerIndex);
                lineEdit->SetDisplayName(containerName);                
            }
            else
            {
                lineEdit->SetDisplayName(typeLabels[containerIndex]);
            }

            lineEdit->SetDataTypes(dataTypeSet);            
            lineEdit->setVisible(true);

            m_ui->typeSelectionFrame->layout()->addWidget(lineEdit);

            AZ::TypeId typeId = lineEdit->GetDefaultTypeId();
            workingCrc.Add(typeId.ToString<AZStd::string>().c_str());

            m_containerTypes.emplace_back(typeId);

            {
                QSignalBlocker signalBlocker(lineEdit);
                lineEdit->DisplayType(typeId);
            }

            QWidget* nextFocus = lineEdit->GetLineEdit();
            setTabOrder(focusWidget, nextFocus);
            focusWidget = nextFocus;

            ++containerIndex;
            dataTypeIter = m_containerDataTypeSets.find(workingCrc);
        }

        setTabOrder(focusWidget, m_ui->createButton);

        if (containerIndex == 1)
        {
            ContainerTypeLineEdit* lineEdit = GetLineEdit(0);
            lineEdit->SetDisplayName(singleTypeString);
        }
    }

    void ContainerWizard::RegisterDataType(const AZ::TypeId& dataType)
    {
        m_dataTypes[dataType] = ScriptCanvasEditor::TranslationHelper::GetSafeTypeName(ScriptCanvas::Data::FromAZType(dataType));
    }

    void ContainerWizard::RegisterContainerType(const AZ::TypeId& containerType)
    {
        if (m_serializeContext == nullptr)
        {
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (m_serializeContext == nullptr)
            {
                AZ_Warning("ScriptCanvas", false, "Not given a SerializeContext and unable to find a SerializeContext to deduce generic ContainerTypes from.");
            }
        }

        AZStd::vector<AZ::Uuid> containedTypes = AZ::Utils::GetContainedTypes(containerType);

        AZ::GenericClassInfo* classInfo = m_serializeContext->FindGenericClassInfo(containerType);

        if (classInfo)
        {
            // Until we get the ability to create generic versions of these containers
            // Keep track of all of the partial matches so we can generate various lists in order to pretend
            // that we have general Key/Value support.
            //
            // Using the Partial CRC to identify a partial match so we can sort of soft fill out our template by
            // knowing all of the possible outcomes given Container X with first element Y
            AZ::Crc32 workingCrc = AZ::Crc32(classInfo->GetGenericTypeId().ToString<AZStd::string>().c_str());

            for (const AZ::Uuid& containedType : containedTypes)
            {
                DataTypeSet& dataTypeSet = m_containerDataTypeSets[workingCrc];

                if (ScriptCanvas::Data::IsNumber(containedType))
                {
                    dataTypeSet.insert(azrtti_typeid<ScriptCanvas::Data::NumberType>());
                    workingCrc.Add(containedType.ToString<AZStd::string>().c_str());
                }
                else
                {
                    dataTypeSet.insert(containedType);
                    workingCrc.Add(containedType.ToString<AZStd::string>().c_str());
                }
            }

            if (m_finalContainerTypeIds.find(workingCrc) == m_finalContainerTypeIds.end())
            {
                m_finalContainerTypeIds[workingCrc] = containerType;
            }

            // Need to populate the combo box
            AZ::TypeId genericTypeId = AZ::Utils::GetGenericContainerType(containerType);

            auto insertResult = m_genericContainerTypes.insert(genericTypeId);

            if (insertResult.second)
            {
                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);

                if (behaviorContext)
                {
                    auto mapIter = behaviorContext->m_typeToClassMap.find(containerType);

                    if (mapIter != behaviorContext->m_typeToClassMap.end())
                    {
                        AZ::Attribute* categoryAttribute = AZ::FindAttribute(AZ::Script::Attributes::Category, mapIter->second->m_attributes);

                        AZStd::string categoryName;

                        if (categoryAttribute)
                        {
                            if (AZ::AttributeReader(nullptr, categoryAttribute).Read<AZStd::string>(categoryName, *behaviorContext))
                            {
                                m_genericContainerTypeNames.emplace_back(categoryName, genericTypeId);
                            }
                            else
                            {
                                m_genericContainerTypes.erase(genericTypeId);
                            }
                        }

                        AZ::Attribute* tooltipAttribute = AZ::FindAttribute(AZ::Script::Attributes::ToolTip, mapIter->second->m_attributes);

                        if (tooltipAttribute)
                        {
                            AZStd::string containerToolTip;

                            if (AZ::AttributeReader(nullptr, tooltipAttribute).Read<AZStd::string>(containerToolTip, *behaviorContext))
                            {
                                QString toolTip = m_ui->containerLabel->toolTip();
                                
                                // We want to sort the container tool tips so the order they are added matches the order in which
                                // we display them in the combobox.
                                //
                                // I don't really have a final point at which to resolve this currently, so I'm just going to sort the elements on the
                                // fly with a ton of string manipulation to get it to align correctly.
                                QStringList previousToolTips = toolTip.split("\n");
                                if (!previousToolTips.empty())
                                {
                                    toolTip.clear();

                                    // First tooltip is always the native tool tip. So we want to maintain that
                                    toolTip.append(previousToolTips[0]);

                                    previousToolTips.removeFirst();

                                    QString newToolTip = QString("  %1 - %2").arg(categoryName.c_str()).arg(containerToolTip.c_str());

                                    previousToolTips.push_back(newToolTip);
                                    previousToolTips.sort(Qt::CaseInsensitive);

                                    for (const QString& toolTipString : previousToolTips)
                                    {
                                        toolTip.append("\n");
                                        toolTip.append(toolTipString);
                                    }

                                    m_ui->containerLabel->setToolTip(toolTip);
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            AZ_Warning("ScriptCanvas", false, "Could not find generic class info for container with TypeId(%s)", containerType.ToString<AZStd::string>().c_str());
        }
    }

    ContainerTypeLineEdit* ContainerWizard::GetLineEdit(int typeIndex)
    {
        while (m_containerTypeLineEdit.size() <= typeIndex)
        {
            ContainerTypeLineEdit* lineEdit = aznew ContainerTypeLineEdit(typeIndex, this);

            QObject::connect(lineEdit, &ContainerTypeLineEdit::TypeChanged, this, &ContainerWizard::OnTypeChanged);
            QObject::connect(lineEdit, &ContainerTypeLineEdit::DataTypeMenuVisibilityChanged, this, &ContainerWizard::OnDataTypeMenuVisibilityChanged);

            m_containerTypeLineEdit.emplace_back(lineEdit);
        }

        return m_containerTypeLineEdit[typeIndex];
    }
}

#include <Editor/View/Dialogs/ContainerWizard/moc_ContainerWizard.cpp>
