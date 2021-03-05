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

#include "QCustomMessageBox.h"
#include "Qt/ui_QCustomMessageBox.h"

#include <QSignalMapper>
#include <QPushButton>
#include <QStyle>

namespace News
{

    QCustomMessageBox::QCustomMessageBox(
        Icon icon,
        const QString& title,
        const QString& text,
        QWidget* parent)
        : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint)
        , m_ui(new Ui::CustomMessageBoxDialog)
    {
        m_ui->setupUi(this);

        m_signalMapper = new QSignalMapper(this);

        this->setWindowTitle(title);
        m_ui->labelText->setText(text);

        QStyle* style = QApplication::style();
        QIcon tmpIcon;
        switch (icon)
        {
        case Information:
            tmpIcon = style->standardIcon(QStyle::SP_MessageBoxInformation);
            break;
        case Warning:
            tmpIcon = style->standardIcon(QStyle::SP_MessageBoxWarning);
            break;
        case Critical:
            tmpIcon = style->standardIcon(QStyle::SP_MessageBoxCritical);
            break;
        case Question:
            tmpIcon = style->standardIcon(QStyle::SP_MessageBoxQuestion);
        default:
            break;
        }
        if (!tmpIcon.isNull())
        {
            QLabel* iconLabel = new QLabel(this);
            iconLabel->setPixmap(tmpIcon.pixmap(64, 64));
            m_ui->bodyLayout->insertWidget(0, iconLabel);
        }

        connect(m_signalMapper, SIGNAL(mapped(int)),
            this, SLOT(clickedSlot(int)));
    }

    QCustomMessageBox::~QCustomMessageBox()
    {
        delete m_signalMapper;
    }

    void QCustomMessageBox::AddButton(const QString& name, int result)
    {
        auto button = new QPushButton(name, this);
        connect(button, SIGNAL(clicked()), m_signalMapper, SLOT(map()));
        m_signalMapper->setMapping(button, result);
        m_ui->buttonLayout->layout()->addWidget(button);
    }

    void QCustomMessageBox::clickedSlot(int result)
    {
        done(result);
    }

#include "Qt/moc_QCustomMessageBox.cpp"

}
