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

#include <TemplateButtonWidget.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QAbstractButton>
#include <QStyle>
#include <QVariant>

namespace O3DE::ProjectManager
{

    TemplateButton::TemplateButton(const QString& imagePath, const QString& labelText, QWidget* parent)
        : QPushButton(parent)
    {
        setAutoExclusive(true);

        setObjectName("templateButton");

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setSpacing(0);
        vLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vLayout);

        QLabel* image = new QLabel(this);
        image->setObjectName("templateImage");
        image->setPixmap(
            QPixmap(imagePath).scaled(QSize(s_templateImageWidth,s_templateImageHeight) , Qt::KeepAspectRatio, Qt::SmoothTransformation));
        vLayout->addWidget(image);

        QLabel* label = new QLabel(labelText, this);
        label->setObjectName("templateLabel");
        vLayout->addWidget(label);

        connect(this, &QAbstractButton::toggled, this, &TemplateButton::onToggled);
    }

    void TemplateButton::onToggled()
    {
        setProperty("Checked", isChecked());

        // we must unpolish/polish every child after changing a property
        // or else they won't use the correct stylesheet selector
        for (auto child : findChildren<QWidget*>())
        {
            child->style()->unpolish(child);
            child->style()->polish(child);
        }

        style()->unpolish(this);
        style()->polish(this);
    }
} // namespace O3DE::ProjectManager
