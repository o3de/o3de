/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ResolutionSettingItemWidget.h"
#include <Source/Editor/ui_ResolutionSettingItemWidget.h>
#include <BuilderSettings/PresetSettings.h>

#include <QComboBox>
#include <QSpinBox>

namespace ImageProcessingAtomEditor
{
    using namespace ImageProcessingAtom;

    ResolutionSettingItemWidget::ResolutionSettingItemWidget(ResoultionWidgetType type, QWidget* parent /*= nullptr*/)
        : QWidget(parent)
        , m_ui(new Ui::ResolutionSettingItemWidget)
    {
        m_ui->setupUi(this);
        m_type = type;

        EditorInternalNotificationBus::Handler::BusConnect();
    }

    ResolutionSettingItemWidget::~ResolutionSettingItemWidget()
    {
        EditorInternalNotificationBus::Handler::BusDisconnect();
    }

    void ResolutionSettingItemWidget::Init(AZStd::string platform, EditorTextureSetting* editorTextureSetting)
    {
        m_platform = platform;
        m_editorTextureSetting = editorTextureSetting;
        m_textureSetting = &m_editorTextureSetting->m_settingsMap[m_platform];
        m_preset = BuilderSettingManager::Instance()->GetPreset(m_textureSetting->m_preset, platform);
        SetupResolutionInfo();
        RefreshUI();
        if (m_type == ResoultionWidgetType::TexturePropety)
        {
            m_ui->formatLabel->show();
            m_ui->formatComboBox->hide();
        }
        else
        {
            m_ui->formatLabel->hide();
            m_ui->formatComboBox->show();
            QObject::connect(m_ui->formatComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResolutionSettingItemWidget::OnChangeFormat);
        }
        QObject::connect(m_ui->downResSpinBox, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ResolutionSettingItemWidget::OnChangeDownRes);
    }

    void ResolutionSettingItemWidget::RefreshUI()
    {
        m_ui->platformLabel->setText(EditorHelper::ToReadablePlatformString(m_platform).c_str());

        m_ui->downResSpinBox->setRange(m_minReduce, m_maxReduce);
        int clampedReduce = AZStd::min<int>(AZStd::max<int>(m_textureSetting->m_sizeReduceLevel, s_MinReduceLevel), s_MaxReduceLevel);
        auto it = m_resolutionInfos.begin();
        it = AZStd::next(it, clampedReduce);
        m_ui->downResSpinBox->setValue(it->reduce);

        QString finalResolution;

        if (it->arrayCount > 1)
        {
            finalResolution = QString("%1 x %2 x %3").arg(QString::number(it->width), QString::number(it->height), QString::number(it->arrayCount));
        }
        else
        {
            finalResolution = QString("%1 x %2").arg(QString::number(it->width), QString::number(it->height));
        }

        m_ui->sizeLabel->setText(finalResolution);

        QString finalFormat = GetFinalFormat(m_textureSetting->m_preset);
        if (m_type == ResoultionWidgetType::TexturePropety)
        {
            m_ui->formatLabel->setText(finalFormat);
        }
        else
        {
            SetupFormatComboBox();
            m_ui->formatComboBox->setCurrentText(finalFormat);
        }
    }

    void ResolutionSettingItemWidget::SetupResolutionInfo()
    {
        m_resolutionInfos = m_editorTextureSetting->GetResolutionInfo(m_platform, m_minReduce, m_maxReduce);
    }

    void ResolutionSettingItemWidget::OnChangeDownRes(int downRes)
    {
        if ((unsigned int)downRes >= m_minReduce && (unsigned int)downRes <= m_maxReduce)
        {
            m_textureSetting->m_sizeReduceLevel = downRes;
            RefreshUI();
            EditorInternalNotificationBus::Broadcast(&EditorInternalNotificationBus::Events::OnEditorSettingsChanged, false, m_platform);
        }
    }

    QString ResolutionSettingItemWidget::GetFinalFormat([[maybe_unused]] const ImageProcessingAtom::PresetName& preset)
    {
        if (m_preset && m_preset->m_pixelFormat >= 0 && m_preset->m_pixelFormat < ePixelFormat_Count)
        {
            return EditorHelper::s_PixelFormatString[m_preset->m_pixelFormat];
        }
        return QString();
    }


    void ResolutionSettingItemWidget::SetupFormatComboBox()
    {
        m_ui->formatComboBox->clear();
    }

    void ResolutionSettingItemWidget::OnChangeFormat([[maybe_unused]] int index)
    {
        bool oldState = m_ui->formatComboBox->blockSignals(true);
        m_ui->formatComboBox->blockSignals(oldState);
    }

    void ResolutionSettingItemWidget::OnEditorSettingsChanged(bool needRefresh, const AZStd::string& /*platform*/)
    {
        if (needRefresh)
        {
            m_preset = BuilderSettingManager::Instance()->GetPreset(m_textureSetting->m_preset, m_platform);
            SetupResolutionInfo();
            RefreshUI();
        }
    }
}//namespace ImageProcessingAtomEditor
#include <Source/Editor/moc_ResolutionSettingItemWidget.cpp>
