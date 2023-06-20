/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <ProjectSettingsScreen.h>
#include <ProjectTemplateInfo.h>
#include <DownloadController.h>
#include <TemplateButtonWidget.h>
#include <QPointer>
#include <QVector>
#endif

QT_FORWARD_DECLARE_CLASS(QButtonGroup)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(FlowLayout)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(TagContainerWidget)

    class NewProjectSettingsScreen
        : public ProjectSettingsScreen
    {
        Q_OBJECT

    public:
        explicit NewProjectSettingsScreen(DownloadController* downloadController, QWidget* parent = nullptr);
        ~NewProjectSettingsScreen() = default;
        ProjectManagerScreen GetScreenEnum() override;

        //! returns the project template path or "" if the template is remote and has not been downloaded
        QString GetProjectTemplatePath();

        bool IsDownloadingTemplate() const;

        void NotifyCurrentScreen() override;

        void SelectProjectTemplate(int index, bool blockSignals = false);

        AZ::Outcome<void, QString> Validate() const override;

        void ShowDownloadTemplateDialog(const ProjectTemplateInfo& templateInfo = {});

        const ProjectTemplateInfo GetSelectedProjectTemplateInfo() const;

    signals:
        void OnTemplateSelectionChanged(int oldIndex, int newIndex);

    public slots:
        void StartTemplateDownload(const QString& templateName, const QString& destinationPath);
        void HandleDownloadResult(const QString& projectName, bool succeeded);
        void HandleDownloadProgress(const QString& projectName, DownloadController::DownloadObjectType objectType, int bytesDownloaded, int totalBytes);

    protected:
        void OnProjectNameUpdated() override;
        void OnProjectPathUpdated() override;

    private:
        QString GetDefaultProjectName();
        QString GetProjectAutoPath();
        QFrame* CreateTemplateDetails(int margin);
        void AddTemplateButtons();
        void UpdateTemplateDetails(const ProjectTemplateInfo& templateInfo);

        QButtonGroup* m_projectTemplateButtonGroup;
        QLabel* m_templateDisplayName;
        QLabel* m_templateSummary;
        QPushButton* m_downloadTemplateButton;
        TemplateButton* m_remoteTemplateButton = nullptr;
        TagContainerWidget* m_templateIncludedGems;
        QVector<ProjectTemplateInfo> m_templates;
        QVector<TemplateButton*> m_templateButtons;
        FlowLayout* m_templateFlowLayout = nullptr;
        int m_selectedTemplateIndex = -1;
        bool m_userChangedProjectPath = false;

        QPointer<DownloadController> m_downloadController = nullptr;

        inline constexpr static int s_spacerSize = 20;
        inline constexpr static int s_templateDetailsContentMargin = 20;
    };

} // namespace O3DE::ProjectManager
