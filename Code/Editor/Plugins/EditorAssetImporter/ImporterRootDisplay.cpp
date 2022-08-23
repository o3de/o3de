/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImporterRootDisplay.h>
#include <ui_ImporterRootDisplay.h>

#include <IEditor.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzQtComponents/Components/Widgets/Text.h>

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

ImporterRootDisplay::ImporterRootDisplay(AZ::SerializeContext* serializeContext, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ImporterRootDisplay())
    , m_manifestWidget(new AZ::SceneAPI::UI::ManifestWidget(serializeContext))
    , m_hasUnsavedChanges(false)
{
    ui->setupUi(this);
    ui->m_manifestWidgetAreaLayout->addWidget(m_manifestWidget.data());

    ui->m_updateButton->setEnabled(false);
    ui->m_updateButton->setProperty("class", "Primary");
    
    ui->locationLabel->setVisible(false);
    ui->nameLabel->setVisible(false);

    ui->headerFrame->setVisible(false);

    ui->m_fullPathText->SetElideMode(Qt::TextElideMode::ElideMiddle);

    AzQtComponents::Text::addTitleStyle(ui->m_filePathText);
    AzQtComponents::Text::addTitleStyle(ui->m_fullPathText);
    
    AzQtComponents::Text::addSubtitleStyle(ui->locationLabel);
    AzQtComponents::Text::addSubtitleStyle(ui->nameLabel);

    connect(ui->m_updateButton, &QPushButton::clicked, this, &ImporterRootDisplay::UpdateClicked);

    ui->m_showInExplorer->setEnabled(false);

    BusConnect();
}

ImporterRootDisplay::~ImporterRootDisplay()
{
    BusDisconnect();
    delete ui;
}

AZ::SceneAPI::UI::ManifestWidget* ImporterRootDisplay::GetManifestWidget()
{
    return m_manifestWidget.data();
}

void ImporterRootDisplay::SetSceneHeaderText(const QString& headerText)
{
    QFileInfo fileInfo(headerText);
    ui->m_filePathText->setText(fileInfo.fileName());
    QString fullPath = QString("%1%2").arg(QDir::toNativeSeparators(fileInfo.path())).arg(QDir::separator());
    ui->m_fullPathText->setText(fullPath);
    
    ui->m_showInExplorer->setEnabled(true);
    ui->m_showInExplorer->disconnect();
    connect(ui->m_showInExplorer, &QPushButton::clicked, this, [fullPath]()
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath));
    });
}

void ImporterRootDisplay::SetSceneDisplay(const QString& headerText, const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene)
{
    AZ_PROFILE_FUNCTION(Editor);
    if (!scene)
    {
        AZ_Assert(scene, "No scene provided to display.");
        return;
    }

    SetSceneHeaderText(headerText);
    ui->locationLabel->setVisible(true);
    ui->nameLabel->setVisible(true);
    ui->headerFrame->setVisible(true);

    HandleSceneWasReset(scene);

    ui->m_updateButton->setEnabled(false);
    m_hasUnsavedChanges = false;
}

void ImporterRootDisplay::HandleSceneWasReset(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene)
{
    AZ_PROFILE_FUNCTION(Editor);
    // Don't accept updates while the widget is being filled in.
    BusDisconnect();
    m_manifestWidget->BuildFromScene(scene);
    BusConnect();
}

void ImporterRootDisplay::HandleSaveWasSuccessful()
{
    ui->m_updateButton->setEnabled(false);
    m_hasUnsavedChanges = false;
}

bool ImporterRootDisplay::HasUnsavedChanges() const
{
    return m_hasUnsavedChanges;
}

void ImporterRootDisplay::ObjectUpdated(const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IManifestObject* /*target*/, void* /*sender*/)
{
    if (m_manifestWidget)
    {
        if (&scene == m_manifestWidget->GetScene().get())
        {
            m_hasUnsavedChanges = true;
            ui->m_updateButton->setEnabled(true);
        }
    }
}

#include <moc_ImporterRootDisplay.cpp>
