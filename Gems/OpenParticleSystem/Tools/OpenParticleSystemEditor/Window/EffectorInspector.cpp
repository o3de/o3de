/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Window/EffectorInspector.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <Window/ParticleItemWidget.h>
#include <Document/ParticleDocumentBus.h>
#include <Window/LevelOfDetailInspectorNotifyBus.h>
#include <QMessageBox>

namespace OpenParticleSystemEditor
{
    EffectorInspector::EffectorInspector(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::effectorInspector)
        , m_pItemWidget(nullptr)
        , m_isShowMessage(false)
        , m_detail(nullptr)
        , m_sourceData(nullptr)
    {
        m_ui->setupUi(this);

        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(m_serializeContext != nullptr, "invalid serialize context");
        m_preWarm = m_serializeContext->CreateAny(azrtti_typeid<OpenParticle::PreWarm>());

        m_ui->particleName->setStyleSheet("background:transparent;color:white");
        m_ui->particleName->installEventFilter(this);

        connect(m_ui->particleName, &QLineEdit::editingFinished, this, &EffectorInspector::OnEditingFinished);
        Init();

        OpenParticle::EditorParticleRequestsBus::Handler::BusConnect();
        ParticleDocumentNotifyBus::Handler::BusConnect();
    }

    EffectorInspector::~EffectorInspector()
    {
        OpenParticle::EditorParticleRequestsBus::Handler::BusDisconnect();
        ParticleDocumentNotifyBus::Handler::BusDisconnect();
        ResetDefaultDisplay();
    }

    bool EffectorInspector::eventFilter(QObject* obj, QEvent* event)
    {
        if (obj == m_ui->particleName)
        {
            if (event->type() == QEvent::FocusIn)
            {
                m_ui->particleName->setStyleSheet("QLineEdit{color:rgb(68,68,68);}");
            }
            else if (event->type() == QEvent::FocusOut)
            {
                m_ui->particleName->setStyleSheet("QLineEdit{background:rgb(68,68,68);border-width:0;border-style:outset;color:white;}");
            }
        }
        return QWidget::eventFilter(obj, event);
    }

    void EffectorInspector::OnEditingFinished()
    {
        if (m_sourceData == nullptr)
        {
            m_ui->particleName->setText("");
            return;
        }
        const AZStd::string text = m_ui->particleName->text().toUtf8().constData();
        if (m_detail == nullptr)
        {
            m_ui->particleName->setText("");
            return;
        }
        else
        {
            if (text == m_detail->m_name)
            {
                // It didn't change.
                return;
            }
        }

        if (m_isShowMessage)
        {
            return;
        }

        auto& emitters = m_sourceData->m_emitters;
        for (auto& iter : emitters)
        {
            if (text.compare(iter->m_name.c_str()) == 0)
            {
                QString message = tr("The name already exists! Please modify it.");
                QMessageBox messageBox(QMessageBox::Critical, QString(), message, QMessageBox::Ok);
                m_isShowMessage = true;
                messageBox.exec();
                m_ui->particleName->setFocus();
                m_ui->particleName->setText(QString::fromUtf8(m_detail->m_name.c_str()));
                m_isShowMessage = false;
                return;
            }
        }
        m_sourceData->UpdateEmitterName(m_detail->m_name, text);
        ParticleItemWidget* itemWidget = static_cast<ParticleItemWidget*>(m_pItemWidget);
        if (itemWidget != nullptr)
        {
            itemWidget->UpdateParticleName();
            EBUS_EVENT(OpenParticle::EditorParticleDocumentBusRequestsBus, OnEmitterNameChanged, m_sourceData);
            EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
            EBUS_EVENT(LevelOfDetailInspectorNotifyBus, ReloadLevel, m_widgetName);
        }
    }

    void EffectorInspector::DeleteEventHandler(AZStd::string& moduleName)
    {
        if (m_sourceData == nullptr || m_detail == nullptr)
        {
            AZ_Error("OpenParticleSystem", false, "The pointer is null, delete event handler is failed!\n");
            return;
        }
        
        m_sourceData->DeleteEventHandler(m_detail, moduleName);
        UpdataEventWidgets();
        m_eventHandlerWidgets->SetCheckBoxEnable(true);
        EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void EffectorInspector::AddEventHandler(AZ::u8 index)
    {
        if (m_sourceData == nullptr || m_detail == nullptr)
        {
            AZ_Error("OpenParticleSystem", false, "The pointer is null, add event handler is failed!\n");
            return;
        }

        m_sourceData->AddEventHandler(m_detail, index);
        UpdataEventWidgets();
        EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void EffectorInspector::UpdataEventWidgets()
    {
        m_eventHandlerWidgets->ClearEventHandlerWidgets();
        for (auto& iterModule : m_detail->m_modules[PARTICLE_LINE_NAMES[WIDGET_LINE_EVENT]])
        {
            if (iterModule.second.first &&
                (iterModule.second.second->is<OpenParticle::ParticleEventHandler>() ||
                    iterModule.second.second->is<OpenParticle::InheritanceHandler>()))
            {
                m_eventHandlerWidgets->AddEventHandler(iterModule.first, iterModule.second.second);
            }
        }
        CheckClassStatu(PARTICLE_LINE_NAMES[WIDGET_LINE_EVENT]);
    }

    // Event Widget
    void EffectorInspector::ClickEventWidget(const AZStd::string className)
    {
        m_eventHandlerWidgets = new EventHandlerWidgets(m_serializeContext, this, this);
        m_ui->verticalLayout->addWidget(m_eventHandlerWidgets);

        UpdataEventWidgets();

        ClickPropertyEditorWidget(className);

    }

    // PropertyEditor Widget
    void EffectorInspector::ClickPropertyEditorWidget(const AZStd::string className)
    {
        for (auto& iterTuple : m_sourceData->m_detailConstant.moduleClasses.at(className))
        {
            AZStd::string moduleName = AZStd::get<0>(iterTuple);
            const auto& eventClass = m_detail->m_modules[className];
            auto foundClass = eventClass.find(moduleName);
            if (foundClass != eventClass.end())
            {
                auto module = foundClass->second;
                if (!module.second->is<OpenParticle::ParticleEventHandler>() &&
                    !module.second->is<OpenParticle::InheritanceHandler>())
                {
                    PropertyEditorWidget* widget = new PropertyEditorWidget(
                        moduleName, className, module.first, module.second, m_serializeContext, this, this);
                    m_propertyEditorWidgets[moduleName] = widget;
                    m_ui->verticalLayout->addWidget(widget);
                    widget->Refresh();
                }
            }
        }
    }

    // ComboBox Widget
    void EffectorInspector::ClickComboBoxWidget(const AZStd::string className)
    {
        for (auto& iterModule : m_detail->m_modules[className.data()])
        {
            if (iterModule.second.first)
            {
                m_comboBoxWidget = new ComboBoxWidget(className, m_detail, m_serializeContext, this, this);
                m_ui->verticalLayout->addWidget(m_comboBoxWidget);
                QObject::connect(m_comboBoxWidget, &ComboBoxWidget::OnMaterialChanged, this, &EffectorInspector::OnComboBoxMaterialChanged);
                QObject::connect(m_comboBoxWidget, &ComboBoxWidget::OnModelChanged, this, &EffectorInspector::OnComboBoxModelChanged);
                QObject::connect(m_comboBoxWidget, &ComboBoxWidget::OnSkeletonModelChanged, this, &EffectorInspector::OnComboBoxSkeletonModelChanged);
                break;
            }
        }
    }

    void EffectorInspector::SetClassName(const QString& className, const QIcon& icon)
    {
        m_ui->itemName->setText(className);
        m_ui->itemTitleIco->setIcon(icon);
    }

    void EffectorInspector::UpdataComboBoxWidget(const AZStd::string moduleName, const AZStd::string className, const AZStd::string lastModuleName)
    {
        ParticleItemWidget* itemWidget = static_cast<ParticleItemWidget*>(m_pItemWidget);
        if (itemWidget == nullptr)
        {
            return;
        }
        if (lastModuleName != NOT_AVAILABLE)
        {
            m_sourceData->UnselectModule(m_detail, className, lastModuleName);
        }

        if (moduleName == NOT_AVAILABLE)
        {
            m_comboBoxWidget->Refresh();
        }
        else
        {
            m_sourceData->SelectModule(m_detail, className, moduleName);
            m_comboBoxWidget->Refresh(m_detail->m_modules[className.data()][moduleName.data()].second);
        }
        EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    void EffectorInspector::UpdatePropertyEditorWidget(const AZStd::string moduleName, const AZStd::string className, bool bCheck)
    {
        if (className.compare("PreWarm") == 0)
        {
            m_propertyEditorWidget->SetChecked(bCheck);
            if (bCheck)
            {
                m_sourceData->m_preWarm = m_serializeContext->CreateAny(azrtti_typeid<OpenParticle::PreWarm>());
                m_sourceData->m_preWarm = m_preWarm;
                m_propertyEditorWidget->Refresh(&m_sourceData->m_preWarm);
            }
            else
            {
                m_preWarm = m_sourceData->m_preWarm;
                m_propertyEditorWidget->Refresh(&m_preWarm);
                m_sourceData->m_preWarm.clear();
            }
            m_propertyEditorWidget->SetChecked(bCheck);
            EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
            return;
        }
        ParticleItemWidget* itemWidget = static_cast<ParticleItemWidget*>(m_pItemWidget);
        if (itemWidget == nullptr)
        {
            return;
        }
        if (bCheck)
        {
            m_sourceData->SelectModule(m_detail, className, moduleName);
            m_propertyEditorWidgets[moduleName]->Refresh(m_detail->m_modules[className.data()][moduleName.data()].second);
        }
        else
        {
            m_sourceData->UnselectModule(m_detail, className, moduleName);
            m_propertyEditorWidgets[moduleName]->Refresh(m_detail->m_modules[className.data()][moduleName.data()].second);
        }
        CheckClassStatu(className);
        EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }

    // Detail Widget
    void EffectorInspector::ClearDetailWidget()
    {
        if (m_detail != nullptr)
        {
            m_ui->particleName->setText(QString::fromUtf8(m_detail->m_name.c_str()));
        }
        for (auto& iter : m_propertyEditorWidgets)
        {
            iter.second->ClearInstances();
            m_ui->verticalLayout->removeWidget(iter.second);
        }
        m_propertyEditorWidgets.clear();

        if (m_comboBoxWidget != nullptr)
        {
            m_comboBoxWidget->Clear();
            m_ui->verticalLayout->removeWidget(m_comboBoxWidget);
            m_comboBoxWidget->deleteLater();
            m_comboBoxWidget = nullptr;
        }

        if (m_eventHandlerWidgets != nullptr)
        {
            m_eventHandlerWidgets->Clear();
            m_ui->verticalLayout->removeWidget(m_eventHandlerWidgets);
            m_eventHandlerWidgets->deleteLater();
            m_eventHandlerWidgets = nullptr;
        }

        ResetDefaultDisplay();
    }

    void EffectorInspector::OnClickLineWidget(LineWidgetIndex index, AZStd::string widgetName)
    {
        m_widgetName = widgetName;
        ClearDetailWidget();
        ResetDefaultDisplay();
        switch (index)
        {
        case OpenParticleSystemEditor::WIDGET_LINE_TITLE:
        case OpenParticleSystemEditor::WIDGET_LINE_BLANK:
        case OpenParticleSystemEditor::WIDGET_LINE_EMITTER:
            SetClassName(tr("Emitter"), QIcon(":/ColorPickerDialog/ColorGrid/toggle-normal-on.svg"));
            ClickPropertyEditorWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_EMITTER]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_SPAWN:
            SetClassName(tr("Spawn"), QIcon(":/Gallery/Grid-small.svg"));
            ClickPropertyEditorWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_SPAWN]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_PARTICLES:
            SetClassName(tr("Particles"), QIcon(":/stylesheet/img/UI20/toolbar/particle.svg"));
            ClickPropertyEditorWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_PARTICLES]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_LOCATION:
            SetClassName(tr("Shape"), QIcon(":/stylesheet/img/UI20/toolbar/SketchMode.svg"));
            ClickComboBoxWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_LOCATION]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_VELOCITY:
            SetClassName(tr("Velocity"), QIcon(":/stylesheet/img/UI20/toolbar/joints/SwingLimits.svg"));
            ClickPropertyEditorWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_VELOCITY]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_COLOR:
            SetClassName(tr("Color"), QIcon(":/ColorPickerDialog/ColorGrid/eyedropper-normal.svg"));
            ClickPropertyEditorWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_COLOR]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_SIZE:
            SetClassName(tr("Size"), QIcon(":/stylesheet/img/UI20/toolbar/Measure.svg"));
            ClickPropertyEditorWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_SIZE]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_FORCE:
            SetClassName(tr("Force"), QIcon(":/stylesheet/img/UI20/toolbar/joints/MaxForce.svg"));
            ClickPropertyEditorWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_FORCE]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_LIGHT:
            SetClassName(tr("Light"), QIcon(":/stylesheet/img/UI20/toolbar/particle.svg"));
            ClickPropertyEditorWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_LIGHT]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_SUBUV:
            SetClassName(tr("SubUV"), QIcon(":/stylesheet/img/UI20/toolbar/XY2_copy.svg"));
            ClickPropertyEditorWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_SUBUV]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_EVENT:
            SetClassName(tr("Event"), QIcon(":/stylesheet/img/logging/pending.svg"));
            ClickEventWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_EVENT]);
            break;
        case OpenParticleSystemEditor::WIDGET_LINE_RENDERER:
            SetClassName(tr("Renderer"), QIcon(":/stylesheet/img/UI20/toolbar/AutoFit.svg"));
            ClickComboBoxWidget(PARTICLE_LINE_NAMES[WIDGET_LINE_RENDERER]);
            break;
        default:
            break;
        }
        m_ui->particleName->setEnabled(true);
    }

    // AzToolsFramework::IPropertyEditorNotify overrides...
    void EffectorInspector::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        auto parent = pNode->GetParent();
        if (parent == nullptr)
        {
            return;
        }
        if (parent->GetClassMetadata()->m_typeId == azrtti_typeid<OpenParticle::UpdateRotateAroundPoint>())
        {
            if (pNode->GetElementMetadata()->m_typeId == azrtti_typeid<AZ::Vector3>())
            {
                void* instance = nullptr;
                OpenParticle::UpdateRotateAroundPoint* moduleData = nullptr;
                if (parent->ReadRaw(instance, parent->GetClassMetadata()->m_typeId))
                {
                    moduleData = static_cast<OpenParticle::UpdateRotateAroundPoint*>(instance);
                    m_updateRotateAroundPoint.xAxis = moduleData->xAxis;
                    m_updateRotateAroundPoint.yAxis = moduleData->yAxis;
                }
            }
        }
        else if (parent->GetClassMetadata()->m_typeId == azrtti_typeid<OpenParticle::UpdateVortexForce>()
            || parent->GetClassMetadata()->m_typeId == azrtti_typeid<OpenParticle::SpawnRotation>())
        {
            if (pNode->GetElementMetadata()->m_typeId == azrtti_typeid<AZ::Vector3>())
            {
                void* instance = nullptr;
                pNode->ReadRaw(instance, pNode->GetElementMetadata()->m_typeId);
                m_lastAxis = *(static_cast<AZ::Vector3*>(instance));
            }
        }
    }

    void EffectorInspector::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        auto parent = pNode->GetParent();
        if (parent == nullptr)
        {
            return;
        }
        if (parent->GetClassMetadata()->m_typeId == azrtti_typeid<OpenParticle::PreWarm>())
        {
            OpenParticle::PreWarm preWarm = AZStd::any_cast<OpenParticle::PreWarm>(m_sourceData->m_preWarm);
            AZStd::string name = pNode->GetElementMetadata()->m_name;
            if (name.compare("tickCount") == 0)
            {
                preWarm.warmupTime = preWarm.tickCount * preWarm.tickDelta;
            }
            else if (name.compare("warmupTime") == 0)
            {
                preWarm.tickCount = preWarm.warmupTime / preWarm.tickDelta;
                preWarm.warmupTime = preWarm.tickCount * preWarm.tickDelta;
            }
            else
            {
                preWarm.warmupTime = preWarm.tickDelta * preWarm.tickCount;
            }
            m_sourceData->m_preWarm = preWarm;
            m_propertyEditorWidget->Refresh();
        }
        else if (parent->GetClassMetadata()->m_typeId == azrtti_typeid<OpenParticle::UpdateRotateAroundPoint>())
        {
            if (pNode->GetElementMetadata()->m_typeId == azrtti_typeid<AZ::Vector3>())
            {
                void* instance = nullptr;
                OpenParticle::UpdateRotateAroundPoint* moduleData = nullptr;
                if (parent->ReadRaw(instance, parent->GetClassMetadata()->m_typeId))
                {
                    moduleData = static_cast<OpenParticle::UpdateRotateAroundPoint*>(instance);
                    if (moduleData->xAxis.Cross(moduleData->yAxis).IsZero())
                    {
                        QMessageBox::information(this, tr("Warning"), tr("The two axis' are parallelism!"));
                        moduleData->xAxis = m_updateRotateAroundPoint.xAxis;
                        moduleData->yAxis = m_updateRotateAroundPoint.yAxis;
                        parent->WriteRaw(instance, parent->GetClassMetadata()->m_typeId);
                        auto className =
                            m_sourceData->m_detailConstant.classNames[OpenParticle::ParticleSourceData::DetailConstant::VELOCITY_INDEX];
                        auto [moduleName, typeId, listIndex] = m_sourceData->m_detailConstant.moduleClasses.at(
                            className)[OpenParticle::ParticleSourceData::DetailConstant::VelocityIndex::VELOCITY_ROTATEAROUNDPOINT];
                        m_propertyEditorWidgets[moduleName]->Refresh();
                        return;
                    }
                }
            }
        }
        else if (parent->GetClassMetadata()->m_typeId == azrtti_typeid<OpenParticle::UpdateVortexForce>())
        {
            if (AfterVortexForceModified(pNode))
            {
                return;
            }
        }
        else if (parent->GetClassMetadata()->m_typeId == azrtti_typeid<OpenParticle::SpawnRotation>())
        {
            if (AfterSpawnRotationModified(pNode))
            {
                return;
            }
        }
        EBUS_EVENT_ID(m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, NotifyParticleSourceDataModified);
    }
    
    bool EffectorInspector::AfterVortexForceModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZStd::string name = pNode->GetElementMetadata()->m_name;
        if (name.compare("vortexAxis") == 0)
        {
            AZ::u8 classIndex = OpenParticle::ParticleSourceData::DetailConstant::FORCE_INDEX;
            AZ::u8 moduleIndex = OpenParticle::ParticleSourceData::DetailConstant::ForceIndex::FORCE_VORTEX;
            return AfterVector3Modifed(pNode, classIndex, moduleIndex);
        }
        return false;
    }

    bool EffectorInspector::AfterSpawnRotationModified(AzToolsFramework::InstanceDataNode* pNode)
    {
        AZ::u8 classIndex = OpenParticle::ParticleSourceData::DetailConstant::PARTICLES_INDEX;
        AZ::u8 moduleIndex = OpenParticle::ParticleSourceData::DetailConstant::ParticlesIndex::PARTICLE_START_ROTATION;
        return AfterVector3Modifed(pNode, classIndex, moduleIndex);
    }

    bool EffectorInspector::AfterVector3Modifed(AzToolsFramework::InstanceDataNode* pNode, AZ::u8 classIndex, AZ::u8 moduleIndex)
    {
        if (pNode->GetElementMetadata()->m_typeId == azrtti_typeid<AZ::Vector3>())
        {
            void* instance = nullptr;
            pNode->ReadRaw(instance, pNode->GetElementMetadata()->m_typeId);
            AZ::Vector3* axis = static_cast<AZ::Vector3*>(instance);
            if (axis->IsZero())
            {
                QMessageBox::information(this, tr("Warning"), tr("The Axis cannot be zero!"));
                *axis = m_lastAxis;
                pNode->WriteRaw(instance, pNode->GetElementMetadata()->m_typeId);
                const auto& className = m_sourceData->m_detailConstant.classNames[classIndex];
                const auto& moduleName = AZStd::get<OpenParticle::ParticleSourceData::DetailConstant::MODULE_NAME>(
                    m_sourceData->m_detailConstant.moduleClasses.at(className)[moduleIndex]);
                m_propertyEditorWidgets[moduleName]->Refresh();
                return true;
            }
        }
        return false;
    }

    void EffectorInspector::Init(AZStd::string widgetName)
    {
        if (widgetName != "")
        {
            m_widgetName = widgetName;
        }
        EBUS_EVENT(LevelOfDetailInspectorNotifyBus, ReloadLevel, widgetName);
        ClearDetailWidget();

        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            sourceData->m_currentDetailInfo = nullptr;
        }

        m_detail = nullptr;
        m_ui->particleName->setText("");
        m_ui->itemTitleIco->setIcon(QIcon(""));
        m_ui->itemName->setText("");
    }

    AZStd::vector<AZStd::string> EffectorInspector::GetEmitterNames()
    {
        AZStd::vector<AZStd::string> emitterNames;
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData)
        {
            for (auto iter : sourceData->m_emitters)
            {
                emitterNames.push_back(iter->m_name);
            }
        }
        return emitterNames;
    }

    int EffectorInspector::GetEmitterIndex(size_t index)
    {
        return m_eventHandlerWidgets->GetEventEmitterName(index);
    }

    void EffectorInspector::SetCheckBoxEnabled(bool check) const
    {
        for (auto& it : m_propertyEditorWidgets)
        {
            it.second->SetCheckBoxEnable(check);
        }
        if (m_eventHandlerWidgets)
        {
            m_eventHandlerWidgets->SetCheckBoxEnable(check);
        }
    }

    void EffectorInspector::CheckClassStatu(const AZStd::string& className)
    {
        const auto& classModule = m_detail->m_modules[className];
        auto find = AZStd::find_if(
            classModule.begin(), classModule.end(),
            [](const auto& t)
            {
                return t.second.first;
            });
        ParticleItemWidget* itemWidget = static_cast<ParticleItemWidget*>(m_pItemWidget);
        if (auto check = find != classModule.end(); check != itemWidget->m_classChecks[className]->isChecked())
        {
            itemWidget->UpdataClassCheck(className, check);
        }
    }

    void EffectorInspector::DefaultDisplay()
    {
        if (!m_ui->itemName->text().isEmpty())
        {
            return;
        }
        OpenParticle::ParticleSourceData* sourceData = nullptr;
        EBUS_EVENT_ID_RESULT(sourceData, m_widgetName, OpenParticleSystemEditor::ParticleDocumentRequestBus, GetParticleSourceData);
        if (sourceData && sourceData->m_currentDetailInfo == nullptr)
        {
            m_sourceData = sourceData;
            bool checked = sourceData->m_preWarm.empty() ? false : true;
            if (m_propertyEditorWidget == nullptr)
            {
                m_propertyEditorWidget = new PropertyEditorWidget(
                    "", "PreWarm", checked, checked ? &sourceData->m_preWarm : &m_preWarm, m_serializeContext, this, this);
                m_ui->verticalLayout->addWidget(m_propertyEditorWidget);
                m_propertyEditorWidget->Refresh();
                m_propertyEditorWidget->SetChecked(checked);
            }
            else
            {
                m_propertyEditorWidget->Refresh(checked ? &sourceData->m_preWarm : &m_preWarm);
                m_propertyEditorWidget->SetChecked(checked);
            }
            SetClassName(tr("PreWarm"), QIcon());
            m_ui->particleName->setEnabled(false);
        }
    }

    void EffectorInspector::ResetDefaultDisplay()
    {
        if (m_propertyEditorWidget)
        {
            m_ui->verticalLayout->removeWidget(m_propertyEditorWidget);
            delete m_propertyEditorWidget;
            m_propertyEditorWidget = nullptr;
        }
    }

    void EffectorInspector::OnDocumentOpened([[maybe_unused]] AZ::Data::Asset<OpenParticle::ParticleAsset> particleAsset, [[maybe_unused]] AZStd::string particleAssetPath)
    {
        AzFramework::StringFunc::Path::GetFileName(particleAssetPath.c_str(), m_widgetName);
        DefaultDisplay();
    }

    void EffectorInspector::OnComboBoxMaterialChanged()
    {
        AZStd::string modifiedEffect = m_ui->particleName->text().toUtf8().constData();
        // This will have modified the assets listed in the "Detail" object, but not the actual emitter, so sync them.
        m_sourceData->UpdateEmitterAsset(modifiedEffect, OpenParticle::ParticleSourceData::DetailConstant::ASSET_MATERIAL);
    }

    void EffectorInspector::OnComboBoxModelChanged()
    {
        AZStd::string modifiedEffect = m_ui->particleName->text().toUtf8().constData();
        // This will have modified the assets listed in the "Detail" object, but not the actual emitter, so sync them.
        m_sourceData->UpdateEmitterAsset(modifiedEffect, OpenParticle::ParticleSourceData::DetailConstant::ASSET_MODEL);
    }

    void EffectorInspector::OnComboBoxSkeletonModelChanged()
    {
        AZStd::string modifiedEffect = m_ui->particleName->text().toUtf8().constData();
        // This will have modified the assets listed in the "Detail" object, but not the actual emitter, so sync them.
        m_sourceData->UpdateEmitterAsset(modifiedEffect, OpenParticle::ParticleSourceData::DetailConstant::ASSET_SKELETON_MODEL);
    }

} // namespace OpenParticleSystemEditor

#include <Window/moc_EffectorInspector.cpp>
