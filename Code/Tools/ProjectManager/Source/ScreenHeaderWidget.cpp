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

#include <ScreenHeaderWidget.h>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

namespace O3DE::ProjectManager
{
    ScreenHeader::ScreenHeader(QWidget* parent)
        : QFrame(parent)
    {
        setObjectName("header");

        QHBoxLayout* layout = new QHBoxLayout();
        layout->setAlignment(Qt::AlignLeft);
        layout->setContentsMargins(0,0,0,0);

        m_backButton = new QPushButton();
        m_backButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        layout->addWidget(m_backButton);

        QVBoxLayout* titleLayout = new QVBoxLayout();
        m_title = new QLabel();
        m_title->setObjectName("headerTitle");
        titleLayout->addWidget(m_title);

        m_subTitle = new QLabel();
        m_subTitle->setObjectName("headerSubTitle");
        titleLayout->addWidget(m_subTitle);

        layout->addLayout(titleLayout);

        setLayout(layout);
    }

    void ScreenHeader::setTitle(const QString& text)
    {
        m_title->setText(text);
    }

    void ScreenHeader::setSubTitle(const QString& text)
    {
        m_subTitle->setText(text);
    }

    QPushButton* ScreenHeader::backButton()
    {
        return m_backButton;
    }

} // namespace O3DE::ProjectManager
