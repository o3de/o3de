/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <ImageProcessing_precompiled.h>
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
        m_ui->downResLabel->setToolTip(QString("Adjust the resolution based on the target platform. \
                                        Use this setting to preserve the resolution of a source file even though it appears smaller in the game. \
                                        Select 0 to preserve the original size or 5 for the maximum reduction."));
    }

    ResolutionSettingWidget::~ResolutionSettingWidget()
    {
    }
}//namespace ImageProcessingAtomEditor
#include <Source/Editor/moc_ResolutionSettingWidget.cpp>
