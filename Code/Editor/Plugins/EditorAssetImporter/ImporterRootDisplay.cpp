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
{
    ui->setupUi(this);
    ui->m_manifestWidgetAreaLayout->addWidget(m_manifestWidget.data());

    ui->m_saveButton->setVisible(false);
    ui->m_saveButton->setProperty("class", "Primary");
    ui->m_saveButton->setDefault(true);

    ui->m_timeStamp->setVisible(false);
    ui->m_timeStampTitle->setVisible(false);
    ui->locationLabel->setVisible(false);
    ui->nameLabel->setVisible(false);

    ui->headerFrame->setVisible(false);

    ui->HeaderPythonBuilderLayoutWidget->setVisible(false);

    ui->m_fullPathText->SetElideMode(Qt::TextElideMode::ElideMiddle);

    connect(ui->m_saveButton, &QPushButton::clicked, this, &ImporterRootDisplayWidget::SaveClicked);

    ui->m_showInExplorer->setEnabled(false);

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
    ui->m_filePathText->setText(QString("<b>%1</b>").arg(fileInfo.fileName()));
    QString fullPath(QString("%1%2").arg(QDir::toNativeSeparators(fileInfo.path())).arg(QDir::separator()));
    ui->m_fullPathText->setText(QString("<b>%1</b>").arg(fullPath));
    
    ui->m_showInExplorer->setEnabled(true);
    ui->m_showInExplorer->disconnect();
    connect(ui->m_showInExplorer, &QPushButton::clicked, this, [fullPath]()
    {
        QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath));
    });
}

void ImporterRootDisplayWidget::SetPythonBuilderText(QString pythonBuilderText)
{
    ui->m_pythonBuilderScript->setText(pythonBuilderText);
    ui->HeaderPythonBuilderLayoutWidget->setVisible(!pythonBuilderText.isEmpty());
}

QString ImporterRootDisplayWidget::GetHeaderFileName() const
{
    return ui->m_filePathText->text();
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
    ui->locationLabel->setVisible(true);
    ui->nameLabel->setVisible(true);
    ui->headerFrame->setVisible(true);

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

void ImporterRootDisplayWidget::UpdateTimeStamp(const QString& manifestFilePath)
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
        ui->m_timeStamp->setText(tr("Unsaved file"));
    }
    ui->m_saveButton->setVisible(true);
    ui->m_timeStamp->setVisible(true);
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
        // Include a marker on the save button to help content creators track if there are unsaved changes.
        QWidget* dock = parentWidget();
        while (dock)
        {
            // This is how AssetImporterWindow::SetTitle finds the title to set it.
            QDockWidget* dockWidget = qobject_cast<QDockWidget*>(dock);
            if (dockWidget)
            {
                AppendUnsaveChangesToTitle(*dockWidget);
                break;
            }
            else
            {
                dock = dock->parentWidget();
            }
        }
    }
}

void ImporterRootDisplayWidget::AppendUnsaveChangesToTitle(QDockWidget& dockWidget)
{
    QString title(dockWidget.windowTitle());

    if(m_hasUnsavedChanges && title.front() != "*")
    {
        title.push_front("*");
    }
    else if (!m_hasUnsavedChanges && title.front() == "*")
    {
        title.remove(0,1);
    }
    dockWidget.setWindowTitle(title);
}

#include <moc_ImporterRootDisplay.cpp>
