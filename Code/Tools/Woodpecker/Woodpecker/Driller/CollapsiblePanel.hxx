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

#ifndef DRILLER_COLLAPSIBLEPANEL_H
#define DRILLER_COLLAPSIBLEPANEL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#pragma once

#include <QtWidgets/QWidget>
#endif

namespace Ui
{
    class CollapsiblePanel;
}

namespace Driller
{
    class CollapsiblePanel
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(CollapsiblePanel, AZ::SystemAllocator, 0);
        
        CollapsiblePanel(QWidget* parent = nullptr);
        ~CollapsiblePanel();
        
        void SetTitle(const QString& title);
        void SetContent(QWidget* content);
        
        void SetCollapsed(bool collapsed);
        bool IsCollapsed() const;
        
    public slots:
        void OnClicked();
        
    signals:
        void Collapsed();
        void Expanded();
        
    private:
    
        bool m_isCollapsed;
      
        QWidget* m_content;
        Ui::CollapsiblePanel* m_gui;
    };
}

#endif