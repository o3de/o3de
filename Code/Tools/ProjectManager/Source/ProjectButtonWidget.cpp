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

#include <ProjectButtonWidget.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QMenu>
#include <QSpacerItem>

namespace O3DE::ProjectManager
{
    /*ProjectImageLabel::ProjectImageLabel(QPixmap* imagesPixmap, QWidget* parent)
        : QLabel(parent)
        , m_imagePixmap(imagesPixmap)
    {
    }

    ProjectImageLabel::~ProjectImageLabel()
    {
        delete m_imagePixmap;
    }

    void ProjectImageLabel::ResizeImage()
    {
        m_imagePixmap = &m_imagePixmap->scaled(size(), Qt::KeepAspectRatioByExpanding);

        setPixmap(*m_imagePixmap);
    }

    void ProjectImageLabel::resizeEvent(QResizeEvent* event)
    {
        ResizeImage();
    }*/

    ProjectButton::ProjectButton(const QString& projectName, QWidget* parent)
        : QFrame(parent)
        , m_projectName(projectName)
    {
        setFixedSize(210, 300);
        //setStyleSheet("background-color: red;");

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        vLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vLayout);

        QPixmap projectImage;
        projectImage.load(QString::fromUtf8(":/Resources/DefaultProjectImage.png"));

        m_projectImageLabel = new QLabel(this);
        m_projectImageLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        m_projectImageLabel->setFixedSize(210, 280);
        vLayout->addWidget(m_projectImageLabel);

        projectImage = projectImage.scaled(m_projectImageLabel->size(), Qt::KeepAspectRatioByExpanding);
        m_projectImageLabel->setPixmap(projectImage);

        QMenu* newProjectMenu = new QMenu(this);
        m_editProjectAction = newProjectMenu->addAction(tr("Edit Project Settings..."));
        m_editProjectGemsAction = newProjectMenu->addAction(tr("Cutomize Gems..."));
        newProjectMenu->addSeparator();
        m_copyProjectAction = newProjectMenu->addAction(tr("Duplicate"));
        newProjectMenu->addSeparator();
        m_removeProjectAction = newProjectMenu->addAction(tr("Remove from O3DE"));
        m_deleteProjectAction = newProjectMenu->addAction(tr("Delete the Project"));

        m_projectSettingsMenuButton = new QPushButton(this);
        m_projectSettingsMenuButton->setText(projectName);
        m_projectSettingsMenuButton->setMenu(newProjectMenu);
        m_projectSettingsMenuButton->setFocusPolicy(Qt::FocusPolicy::NoFocus);
        m_projectSettingsMenuButton->setFlat(true);
        m_projectSettingsMenuButton->setStyleSheet("font-size: 14px; font-family: 'Open Sans'; text-align:left; background-color: transparent;");
        vLayout->addWidget(m_projectSettingsMenuButton);
    }
} // namespace O3DE::ProjectManager
