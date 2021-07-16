/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TemplateButtonWidget.h>
#include <ProjectManagerDefs.h>

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
        image->setPixmap(QPixmap(imagePath).scaled(
            QSize(ProjectTemplateImageWidth, ProjectTemplateImageWidth), Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
