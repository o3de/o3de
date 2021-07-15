/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAssetImporter_precompiled.h"
#include <ImporterRootDisplay.h>
#include <ui_ImporterRootDisplay.h>

#include <IEditor.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <AzCore/Serialization/SerializeContext.h>

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

    connect(ui->m_updateButton, &QPushButton::clicked, this, &ImporterRootDisplay::UpdateClicked);

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

void ImporterRootDisplay::SetSceneDisplay(const QString& headerText, const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
    if (!scene)
    {
        AZ_Assert(scene, "No scene provided to display.");
        return;
    }

    ui->m_filePathText->setText(headerText);

    HandleSceneWasReset(scene);

    ui->m_updateButton->setEnabled(false);
    m_hasUnsavedChanges = false;
}

void ImporterRootDisplay::HandleSceneWasReset(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);
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
