/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImporterRootDisplay.h>
#include <ui_ImporterRootDisplay.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <IEditor.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzQtComponents/Components/Widgets/Text.h>

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QFileInfo>
#include <QUrl>


SceneSettingsRootDisplayScriptRequestHandler::SceneSettingsRootDisplayScriptRequestHandler()
{
    SceneSettingsRootDisplayScriptRequestBus::Handler::BusConnect();
}

SceneSettingsRootDisplayScriptRequestHandler::~SceneSettingsRootDisplayScriptRequestHandler()
{
    SceneSettingsRootDisplayScriptRequestBus::Handler::BusDisconnect();
}

void SceneSettingsRootDisplayScriptRequestHandler::Reflect(AZ::ReflectContext* context)
{
    if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serialize->Class<SceneSettingsRootDisplayScriptRequestHandler>()->Version(0);
    }

    if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
    {
        behavior->EBus<SceneSettingsRootDisplayScriptRequestBus>("SceneSettingsRootDisplayScriptRequestBus")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
            ->Attribute(AZ::Script::Attributes::Module, "qt")
            ->Event("HasUnsavedChanges", &SceneSettingsRootDisplayScriptRequestBus::Events::HasUnsavedChanges);
    }
}

void SceneSettingsRootDisplayScriptRequestHandler::SetRootDisplay(ImporterRootDisplayWidget* importerRootDisplay)
{
    m_importerRootDisplay = importerRootDisplay;
}

bool SceneSettingsRootDisplayScriptRequestHandler::HasUnsavedChanges() const
{
    if (m_importerRootDisplay)
    {
        return m_importerRootDisplay->HasUnsavedChanges();
    }
    return false;
}

ImporterRootDisplayWidget::ImporterRootDisplayWidget(AZ::SerializeContext* serializeContext, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ImporterRootDisplay())
    , m_manifestWidget(new AZ::SceneAPI::UI::ManifestWidget(serializeContext))
    , m_hasUnsavedChanges(false)
    , m_filePath("")
{
    ui->setupUi(this);
    ui->m_manifestWidgetAreaLayout->addWidget(m_manifestWidget.data());

    ui->m_saveButton->setVisible(false);
    ui->m_saveButton->setProperty("class", "Primary");
    ui->m_saveButton->setDefault(true);

    m_editMenu = new QMenu("Edit Scene Settings Menu", ui->m_editButton);
    ui->m_editButton->setMenu(m_editMenu);
    ui->m_editButton->setVisible(false);

    connect(m_editMenu, &QMenu::aboutToShow, this, &ImporterRootDisplayWidget::AddEditMenu);

    ui->m_inspectButton->setVisible(false);

    ui->m_helpButton->setVisible(false);

    ui->m_timeStamp->setVisible(false);
    ui->m_timeStampTitle->setVisible(false);

    ui->headerFrame->setVisible(false);

    connect(ui->m_saveButton, &QPushButton::clicked, this, &ImporterRootDisplayWidget::SaveClicked);
    connect(ui->m_inspectButton, &QPushButton::clicked, this, &ImporterRootDisplayWidget::InspectClicked);
    connect(ui->m_helpButton, &QPushButton::clicked, this, &ImporterRootDisplayWidget::HelpClicked);

    AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
    m_requestHandler = AZStd::make_shared<SceneSettingsRootDisplayScriptRequestHandler>();
    m_requestHandler->SetRootDisplay(this);
}

ImporterRootDisplayWidget::~ImporterRootDisplayWidget()
{
    AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
    delete ui;
}

AZ::SceneAPI::UI::ManifestWidget* ImporterRootDisplayWidget::GetManifestWidget()
{
    return m_manifestWidget.data();
}

void ImporterRootDisplayWidget::SetSceneHeaderText(const QString& headerText)
{
    QFileInfo fileInfo(headerText);
    m_filePath = fileInfo.fileName();
}

void ImporterRootDisplayWidget::SetPythonBuilderText(QString pythonBuilderText)
{
    ui->m_pythonBuilderScript->setText(pythonBuilderText);
    ui->headerFrame->setVisible(!pythonBuilderText.isEmpty());
}

QString ImporterRootDisplayWidget::GetHeaderFileName() const
{
    return m_filePath;
}

void ImporterRootDisplayWidget::SetSceneDisplay(const QString& headerText, const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene)
{
    AZ_PROFILE_FUNCTION(Editor);
    if (!scene)
    {
        AZ_Assert(scene, "No scene provided to display.");
        return;
    }

    SetSceneHeaderText(headerText);
    HandleSceneWasReset(scene);
    SetUnsavedChanges(false);
}

void ImporterRootDisplayWidget::HandleSceneWasReset(const AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene>& scene)
{
    AZ_PROFILE_FUNCTION(Editor);
    // Don't accept updates while the widget is being filled in.
    BusDisconnect();
    m_manifestWidget->BuildFromScene(scene);
    BusConnect();

    // Resetting the scene doesn't immediately save the changes, so mark this as having unsaved changes.
    SetUnsavedChanges(true);
}

void ImporterRootDisplayWidget::HandleSaveWasSuccessful()
{
    SetUnsavedChanges(false);
}

bool ImporterRootDisplayWidget::HasUnsavedChanges() const
{
    return m_hasUnsavedChanges;
}

void ImporterRootDisplayWidget::ObjectUpdated(
    const AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::DataTypes::IManifestObject* /*target*/, void* /*sender*/)
{
    if (m_manifestWidget)
    {
        if (&scene == m_manifestWidget->GetScene().get())
        {
            SetUnsavedChanges(true);
        }
    }
}

void ImporterRootDisplayWidget::UpdateTimeStamp(const QString& manifestFilePath, bool enableInspector)
{
    const QFileInfo info(manifestFilePath);
    if (info.exists())
    {
        const QDateTime lastModifiedTime(info.lastModified());
        QString lastModifiedDisplay(lastModifiedTime.toString(Qt::TextDate));
        ui->m_timeStampTitle->setVisible(true);
        ui->m_timeStamp->setText(lastModifiedDisplay);
    }
    else
    {
        // If the scene manifest doesn't yet exist, then don't show a timestamp.
        // Don't mark this as dirty, because standard dirty workflows (popup "Would you like to save changes?" on closing, for example)
        // shouldn't be applied to unsaved, unmodified scene settings.
        ui->m_timeStampTitle->setVisible(false);
        ui->m_timeStamp->setText(tr("Unmodified file"));
    }
    ui->m_saveButton->setVisible(true);
    ui->m_editButton->setVisible(true);
    ui->m_helpButton->setVisible(true);
    ui->m_timeStamp->setVisible(true);

    if (enableInspector)
    {
        ui->m_inspectButton->setVisible(true);
    }
}

void ImporterRootDisplayWidget::SetUnsavedChanges(bool hasUnsavedChanges)
{
    bool refreshTitle = false;
    if (hasUnsavedChanges != m_hasUnsavedChanges)
    {
        refreshTitle = true;
    }
    m_hasUnsavedChanges = hasUnsavedChanges;

    if (refreshTitle)
    {
        AppendUnsaveChangesToTitle();
    }
}

void ImporterRootDisplayWidget::AppendUnsaveChangesToTitle()
{
    QString title(ui->m_saveButton->text());

    if (m_hasUnsavedChanges && title.front() != "*")
    {
        title.push_front("*");
    }
    else if (!m_hasUnsavedChanges && title.front() == "*")
    {
        title.remove(0,1);
    }
    ui->m_saveButton->setText(title);
}

void ImporterRootDisplayWidget::AddEditMenu()
{
    m_editMenu->clear();

    m_editMenu->addAction(
        QObject::tr("Reset settings to default..."),
        [this]()
        {
            emit ResetSettings();
        });

    m_editMenu->addAction(
        QObject::tr("Clear unsaved changes..."),
        [this]()
        {
            emit ClearChanges();
        });

    m_editMenu->addAction(
        QObject::tr("Assign build script..."),
        [this]()
        {
            emit AssignScript();
        });
}
#include <moc_ImporterRootDisplay.cpp>
