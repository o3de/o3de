/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MipmapSettingWidget.h"
#include <Source/Editor/ui_MipmapSettingWidget.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <QCheckBox>
#include <QSizePolicy>

namespace ImageProcessingAtomEditor
{
    using namespace ImageProcessingAtom;
    MipmapSettingWidget::MipmapSettingWidget(EditorTextureSetting& textureSetting, QWidget* parent /*= nullptr*/)
        : QWidget(parent)
        , m_ui(new Ui::MipmapSettingWidget)
        , m_textureSetting(&textureSetting)
    {
        m_ui->setupUi(this);


        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Serialization context not available");

        m_ui->propertyEditor->SetAutoResizeLabels(true);
        m_ui->propertyEditor->Setup(serializeContext, this, true, 250);

        m_ui->propertyEditor->ClearInstances();
        TextureSettings* instance = &m_textureSetting->GetMultiplatformTextureSetting();
        const AZ::Uuid& classId = AZ::SerializeTypeInfo<TextureSettings>::GetUuid(instance);
        m_ui->propertyEditor->AddInstance(instance, classId);
        m_ui->propertyEditor->InvalidateAll();
        m_ui->propertyEditor->ExpandAll();

        // Style the Checkbox
        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_ui->enableCheckBox);

        RefreshUI();

        EditorInternalNotificationBus::Handler::BusConnect();
    }

    MipmapSettingWidget::~MipmapSettingWidget()
    {
        EditorInternalNotificationBus::Handler::BusDisconnect();
    }

    void MipmapSettingWidget::RefreshUI()
    {
        TextureSettings& texSetting = m_textureSetting->GetMultiplatformTextureSetting();
        const PresetSettings* preset = BuilderSettingManager::Instance()->GetPreset(texSetting.m_preset);
        if (preset == nullptr || preset->m_mipmapSetting == nullptr)
        {
            m_ui->enableCheckBox->setCheckState(Qt::CheckState::Unchecked);
            m_ui->enableCheckBox->setEnabled(false);
            m_ui->propertyEditor->hide();
        }
        else
        {
            bool showMipMap = texSetting.m_enableMipmap;
            m_ui->enableCheckBox->setEnabled(true);
            m_ui->enableCheckBox->setCheckState(showMipMap ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
            QObject::connect(m_ui->enableCheckBox, &QCheckBox::clicked, this, &MipmapSettingWidget::OnCheckBoxStateChanged);
            if (showMipMap)
            {
                m_ui->propertyEditor->show();
                this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            }
            else
            {
                m_ui->propertyEditor->hide();
                this->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            }
        }
        m_ui->propertyEditor->InvalidateValues();
    }

    void MipmapSettingWidget::OnCheckBoxStateChanged(bool checked)
    {
        bool finalChecked = m_textureSetting->RefreshMipSetting(checked);

        if (finalChecked)
        {
            m_ui->propertyEditor->show();
        }
        else
        {
            m_ui->propertyEditor->hide();
        }
        EditorInternalNotificationBus::Broadcast(&EditorInternalNotificationBus::Events::OnEditorSettingsChanged, false, BuilderSettingManager::s_defaultPlatform);
    }


    void MipmapSettingWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*pNode*/)
    {
        //Only the first texture setting reflected is changed, we need to propagate the change to every texture settings.
        m_textureSetting->PropagateCommonSettings();
        m_ui->propertyEditor->InvalidateValues();
        EditorInternalNotificationBus::Broadcast(&EditorInternalNotificationBus::Events::OnEditorSettingsChanged, false, BuilderSettingManager::s_defaultPlatform);
    }

    void MipmapSettingWidget::OnEditorSettingsChanged(bool needRefresh, const AZStd::string& /*platform*/)
    {
        if (needRefresh)
        {
            RefreshUI();
        }
    }
}//namespace ImageProcessingAtomEditor
#include <Source/Editor/moc_MipmapSettingWidget.cpp>
