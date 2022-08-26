/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <ScreenWidget.h>
#include <FormFolderBrowseEditWidget.h>
#include <FormLineEditWidget.h>
#include <FormComboBoxWidget.h>
#include <GemCatalog/GemInfo.h>
#include <PythonBindings.h>

#include <QHash>
#include <QApplication>
#include <QStyleOptionTab>
#include <QStylePainter>
#include <QTabBar>
#include <QTabWidget>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QPushButton>
#include <QScrollArea>
#include <QComboBox>
#include <QVBoxLayout>
#endif

//#define CREATE_A_GAME_ACTIVE

namespace O3DE::ProjectManager
{
    class TabBar : public QTabBar
    {
    public:
        QSize tabSizeHint(int index) const
        {
            QSize s = QTabBar::tabSizeHint(index);
            s.transpose();
            return s;
        }

    protected:
        void paintEvent(QPaintEvent*)
        {
            QStylePainter painter(this);
            QStyleOptionTab opt;

            QPoint currentTopPosition(115, 175);

            QStringList strs = { "1. Gem Setup", "2. Gem Details", "3. Creator Details" };
            for (int i = 0; i < count(); i++)
            {
                initStyleOption(&opt, i);
                painter.drawControl(QStyle::CE_TabBarTabShape, opt);
                painter.save();
                QSize s = opt.rect.size();
                s.transpose();
                s.setWidth(130);
                QRect r(QPoint(), s);
                r.moveCenter(opt.rect.center());
                opt.rect = r;
                QPoint c = tabRect(i).center();
                QPoint leftJustify(currentTopPosition);
                leftJustify.setX(30 + (int)(0.5 * opt.rect.width()));
                painter.translate(leftJustify);
                currentTopPosition.setY(currentTopPosition.y() + 55);
                painter.setFont(QFont("Open Sans", 12));
                painter.translate(-c);
                painter.drawItemText(r, Qt::AlignLeft, QApplication::palette(), true, strs.at(i));
                painter.restore();
            }
        }
    };

    class TabWidget : public QTabWidget
    {
    public:
        TabWidget(QWidget* parent = 0)
            : QTabWidget(parent)
        {
            setTabBar(new TabBar);
            setTabPosition(QTabWidget::West);
        }
    };

    class CreateAGemScreen : public ScreenWidget
    {
        Q_OBJECT
    public:
        explicit CreateAGemScreen(QWidget* parent = nullptr);
        ~CreateAGemScreen() = default;

    signals:
        void CreateButtonPressed();

    private slots:
        void HandleBackButton();
        void HandleNextButton();
        void UpdateNextButtonToCreate();

    private:
        void LoadButtonsFromGemTemplatePaths(QVBoxLayout* firstScreen);
        QScrollArea* CreateFirstScreen();
        QScrollArea* CreateSecondScreen();
        QScrollArea* CreateThirdScreen();
        bool ValidateGemTemplateLocation();
        bool ValidateGemDisplayName();
        bool ValidateGemSystemName();
        bool ValidateLicenseName();
        bool ValidateGlobalGemTag();
        bool ValidateOptionalGemTags();
        bool ValidateCreatorName();
        bool ValidateRepositoryURL();
        void AddDropdownActions(FormComboBoxWidget* dropdown);

        //First Screen
        QVector<ProjectTemplateInfo> m_gemTemplates;
        QButtonGroup* m_radioButtonGroup;
        QRadioButton* m_formFolderRadioButton = nullptr;
        FormFolderBrowseEditWidget* m_gemTemplateLocation = nullptr;

        //Second Screen
        FormLineEditWidget* m_gemDisplayName = nullptr;
        FormLineEditWidget* m_gemSystemName = nullptr;
        FormLineEditWidget* m_gemSummary = nullptr;
        FormLineEditWidget* m_requirements = nullptr;
        FormLineEditWidget* m_license = nullptr;
        FormLineEditWidget* m_licenseURL = nullptr;
        FormLineEditWidget* m_origin = nullptr;
        FormLineEditWidget* m_originURL = nullptr;
        FormLineEditWidget* m_userDefinedGemTags = nullptr;
        FormFolderBrowseEditWidget* m_gemLocation = nullptr;
        FormComboBoxWidget* m_firstDropdown = nullptr;
        FormComboBoxWidget* m_secondDropdown = nullptr;
        FormComboBoxWidget* m_thirdDropdown = nullptr;
        FormFolderBrowseEditWidget* m_gemIconPath = nullptr;
        FormLineEditWidget* m_documentationURL = nullptr;

        //Third Screen
        FormLineEditWidget* m_creatorName = nullptr;
        FormLineEditWidget* m_repositoryURL = nullptr;
        

        TabWidget* m_tabWidget;

        QDialogButtonBox* m_backNextButtons = nullptr;
        QPushButton* m_backButton = nullptr;
        QPushButton* m_nextButton = nullptr;

        GemInfo m_createAGemInfo;
    };


} // namespace O3DE::ProjectManager
