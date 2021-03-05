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

#ifndef __EMSTUDIO_RENDERPLUGINLAYOUTS_H
#define __EMSTUDIO_RENDERPLUGINLAYOUTS_H

// include the required headers
#include "RenderPlugin.h"
#include <QWidget>
#include <QSplitter>

namespace EMStudio
{
    class EMSTUDIO_API SingleRenderWidget
        : public RenderPlugin::Layout
    {
        MCORE_MEMORYOBJECTCATEGORY(SingleRenderWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        QWidget* Create(RenderPlugin* renderPlugin, QWidget* parent) override
        {
            QWidget* result = renderPlugin->CreateViewWidget(parent);

            // switch the camera mode
            renderPlugin->GetViewWidget(0)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_ORBIT);

            return result;
        }

        const char* GetName() override                      { return "Single"; }
        const char* GetImageFileName() override             { return "Images/Rendering/LayoutSingle.png"; }
    };


    class EMSTUDIO_API HorizontalDoubleRenderWidget
        : public RenderPlugin::Layout
    {
        MCORE_MEMORYOBJECTCATEGORY(HorizontalDoubleRenderWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        QWidget* Create(RenderPlugin* renderPlugin, QWidget* parent) override
        {
            // create the widget for the render view
            QSplitter* splitterWidget = new QSplitter(parent);

            splitterWidget->addWidget(renderPlugin->CreateViewWidget(splitterWidget));
            splitterWidget->setCollapsible(splitterWidget->count() - 1, false);

            splitterWidget->addWidget(renderPlugin->CreateViewWidget(splitterWidget));
            splitterWidget->setCollapsible(splitterWidget->count() - 1, false);

            // switch the camera modes
            renderPlugin->GetViewWidget(0)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_ORBIT);
            renderPlugin->GetViewWidget(1)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_ORBIT);

            return splitterWidget;
        }

        const char* GetName() override                      { return "Horizontal Split"; }
        const char* GetImageFileName() override             { return "Images/Rendering/LayoutHDouble.png"; }
    };


    class EMSTUDIO_API VerticalDoubleRenderWidget
        : public RenderPlugin::Layout
    {
        MCORE_MEMORYOBJECTCATEGORY(VerticalDoubleRenderWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        QWidget* Create(RenderPlugin* renderPlugin, QWidget* parent) override
        {
            // create the widget for the render view
            QSplitter* splitterWidget = new QSplitter(parent);

            splitterWidget->addWidget(renderPlugin->CreateViewWidget(splitterWidget));
            splitterWidget->setCollapsible(splitterWidget->count() - 1, false);

            splitterWidget->addWidget(renderPlugin->CreateViewWidget(splitterWidget));
            splitterWidget->setCollapsible(splitterWidget->count() - 1, false);

            splitterWidget->setOrientation(Qt::Vertical);

            // switch the camera modes
            renderPlugin->GetViewWidget(0)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_ORBIT);
            renderPlugin->GetViewWidget(1)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_ORBIT);

            return splitterWidget;
        }

        const char* GetName() override                      { return "Vertical Split"; }
        const char* GetImageFileName() override             { return "Images/Rendering/LayoutVDouble.png"; }
    };


    class EMSTUDIO_API TripleBigTopRenderWidget
        : public RenderPlugin::Layout
    {
        MCORE_MEMORYOBJECTCATEGORY(TripleBigTopRenderWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        QWidget* Create(RenderPlugin* renderPlugin, QWidget* parent) override
        {
            // create the widgets
            QSplitter* splitterWidget = new QSplitter(parent);
            splitterWidget->setOrientation(Qt::Vertical);

            splitterWidget->addWidget(renderPlugin->CreateViewWidget(splitterWidget));
            splitterWidget->setCollapsible(splitterWidget->count() - 1, false);

            QSplitter* sideSplitter = new QSplitter(splitterWidget);
            sideSplitter->addWidget(renderPlugin->CreateViewWidget(sideSplitter));
            sideSplitter->setCollapsible(sideSplitter->count() - 1, false);
            sideSplitter->addWidget(renderPlugin->CreateViewWidget(sideSplitter));
            sideSplitter->setCollapsible(sideSplitter->count() - 1, false);

            // switch the camera modes
            renderPlugin->GetViewWidget(0)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_ORBIT);
            renderPlugin->GetViewWidget(1)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_TOP);
            renderPlugin->GetViewWidget(2)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_LEFT);

            return splitterWidget;
        }

        const char* GetName() override                      { return "Triple"; }
        const char* GetImageFileName() override             { return "Images/Rendering/LayoutTripleBigTop.png"; }
    };


    class EMSTUDIO_API QuadrupleRenderWidget
        : public RenderPlugin::Layout
    {
        MCORE_MEMORYOBJECTCATEGORY(QuadrupleRenderWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

    public:
        QWidget* Create(RenderPlugin* renderPlugin, QWidget* parent) override
        {
            // create the widgets
            QSplitter* splitterWidget   = new QSplitter(parent);
            QSplitter* leftSplitter     = new QSplitter(splitterWidget);
            QSplitter* rightSplitter    = new QSplitter(splitterWidget);

            splitterWidget->setCollapsible(0, false);
            splitterWidget->setCollapsible(1, false);
            leftSplitter->setOrientation(Qt::Vertical);
            rightSplitter->setOrientation(Qt::Vertical);

            leftSplitter->addWidget(renderPlugin->CreateViewWidget(leftSplitter));
            leftSplitter->setCollapsible(leftSplitter->count() - 1, false);
            leftSplitter->addWidget(renderPlugin->CreateViewWidget(leftSplitter));
            leftSplitter->setCollapsible(leftSplitter->count() - 1, false);

            rightSplitter->addWidget(renderPlugin->CreateViewWidget(rightSplitter));
            rightSplitter->setCollapsible(rightSplitter->count() - 1, false);
            rightSplitter->addWidget(renderPlugin->CreateViewWidget(rightSplitter));
            rightSplitter->setCollapsible(rightSplitter->count() - 1, false);

            // switch the camera modes
            renderPlugin->GetViewWidget(0)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_TOP);
            renderPlugin->GetViewWidget(1)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_FRONT);
            renderPlugin->GetViewWidget(2)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_ORBIT);
            renderPlugin->GetViewWidget(3)->GetRenderWidget()->SwitchCamera(RenderWidget::CAMMODE_LEFT);

            return splitterWidget;
        }

        const char* GetName() override                      { return "Quad"; }
        const char* GetImageFileName() override             { return "Images/Rendering/LayoutQuad.png"; }
    };


    // register all available layouts (this will be automatically called inside the RenderPlugin's constructor)
    void EMSTUDIO_API RegisterRenderPluginLayouts(RenderPlugin* renderPlugin);
} // namespace EMStudio


#endif
