/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "CollapsiblePanel.hxx"
#include <Source/Driller/ui_CollapsiblePanel.h>
#include <Source/Driller/moc_CollapsiblePanel.cpp>

namespace Driller
{
    CollapsiblePanel::CollapsiblePanel(QWidget* parent)
        : QWidget(parent)
        , m_isCollapsed(false)
        , m_content(nullptr)
        , m_gui(nullptr)
    {
        m_gui = azcreate(Ui::CollapsiblePanel, ());
        m_gui->setupUi(this);

        // Default to collapsed, need to initialize the variable to true to trigger the delta.
        SetCollapsed(true);

        QObject::connect(m_gui->stateIcon, SIGNAL(clicked()), this, SLOT(OnClicked()));
    }

    CollapsiblePanel::~CollapsiblePanel()
    {
        azdestroy(m_gui);
        m_gui = nullptr;
    }

    void CollapsiblePanel::SetTitle(const QString& title)
    {
        m_gui->description->setText(title);
    }

    void CollapsiblePanel::SetContent(QWidget* content)
    {
        if (m_content)
        {
            m_gui->contentLayout->removeWidget(content);
            m_content = nullptr;
        }

        m_content = content;

        if (m_content)
        {
            m_gui->contentLayout->addWidget(m_content);
        }
    }

    void CollapsiblePanel::SetCollapsed(bool collapsed)
    {
        if (m_isCollapsed != collapsed)
        {
            m_isCollapsed = collapsed;

            m_gui->groupBox->setVisible(!m_isCollapsed);

            if (m_isCollapsed)
            {
                m_gui->stateIcon->setArrowType(Qt::RightArrow);
                emit Collapsed();
            }
            else
            {
                m_gui->stateIcon->setArrowType(Qt::DownArrow);
                emit Expanded();
            }
        }
    }

    bool CollapsiblePanel::IsCollapsed() const
    {
        return m_isCollapsed;
    }

    void CollapsiblePanel::OnClicked()
    {
        SetCollapsed(!m_isCollapsed);
    }
}
