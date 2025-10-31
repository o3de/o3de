/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ResolutionSettingWidget.h"
#include <Source/Editor/ui_ResolutionSettingWidget.h>
#include <QVBoxLayout>

namespace ImageProcessingAtomEditor
{
    using namespace ImageProcessingAtom;
    ResolutionSettingWidget::ResolutionSettingWidget(ResoultionWidgetType type, EditorTextureSetting& textureSetting, QWidget* parent /*= nullptr*/)
        : QWidget(parent)
        , m_ui(new Ui::ResolutionSettingWidget)
        , m_textureSetting(&textureSetting)
    {
        m_ui->setupUi(this);
        m_type = type;

        //Put default platform in the first row
        ResolutionSettingItemWidget* item = new ResolutionSettingItemWidget(ResoultionWidgetType::TexturePropety, this);
        item->Init(BuilderSettingManager::s_defaultPlatform, m_textureSetting);
        m_ui->listLayout->addWidget(item);

        //Add the other platforms in the list
        for (auto& it : m_textureSetting->m_settingsMap)
        {
            AZStd::string platform = it.first;
            if (platform != BuilderSettingManager::s_defaultPlatform)
            {
                ResolutionSettingItemWidget* item2 = new ResolutionSettingItemWidget(ResoultionWidgetType::TexturePropety, this);
                item2->Init(platform, m_textureSetting);
                m_ui->listLayout->addWidget(item2);
            }
        }

        // Tooltips
        m_ui->platformLabel->setToolTip(QString("Each row displays the resolution and pixel format settings for the relative target platform in this column."));
        m_ui->downResLabel->setToolTip(QString("Adjust the maximum resolution based on the target platform.\n\
                                        Values range from 0 (full resolution) to 5 (lowest resolution) with each step being half the resolution of the preceding step."));
        m_ui->resolutionLabel->setToolTip(QString("The maximum texture resolution for the target platform based on the Resolution Limit setting."));
        m_ui->formatLabel->setToolTip(QString("The pixel format of the processed texture for the target platform."));
    }

    ResolutionSettingWidget::~ResolutionSettingWidget()
    {
    }
}//namespace ImageProcessingAtomEditor
#include <Source/Editor/moc_ResolutionSettingWidget.cpp>
