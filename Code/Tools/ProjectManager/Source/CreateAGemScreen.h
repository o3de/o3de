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

#include <QApplication>
#include <QStyleOptionTab>
#include <QStylePainter>
#include <QTabBar>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QScrollArea>


#endif

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
    void paintEvent(QPaintEvent* /*event*/)
    {
        QStylePainter painter(this);
        QStyleOptionTab opt;

        for (int i = 0; i < count(); i++)
        {
            initStyleOption(&opt, i);
            painter.drawControl(QStyle::CE_TabBarTabShape, opt);
            painter.save();

            QSize s = opt.rect.size();
            s.transpose();
            QRect r(QPoint(), s);
            r.moveCenter(opt.rect.center());
            opt.rect = r;

            QPoint c = tabRect(i).center();
            painter.translate(c);
            painter.rotate(90);
            painter.translate(-c);
            painter.drawControl(QStyle::CE_TabBarTabLabel, opt);
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


namespace O3DE::ProjectManager
{

    class CreateAGemScreen : public ScreenWidget
    {
    public:
        explicit CreateAGemScreen(QWidget* parent = nullptr);
        ~CreateAGemScreen() = default;

    public slots:

    protected:

    private slots:
        void OnGemDisplayNameUpdated();
        void OnGemSystemNameUpdated();
        void OnLicenseNameUpdated();
        void OnCreatorNameUpdated();
        void OnRepositoryURLUpdated();
        void HandleBackButton();
        void HandleNextButton();
        void ConvertNextButtonToCreate();

    private:
        QScrollArea* CreateFirstScreen();
        QScrollArea* CreateSecondScreen();
        QScrollArea* CreateThirdScreen();
        bool ValidateGemDisplayName();
        bool ValidateGemSystemName();
        bool ValidateLicenseName();
        bool ValidateCreatorName();
        bool ValidateRepositoryURL();

        FormLineEditWidget* m_gemDisplayName;
        FormLineEditWidget* m_gemSystemName;
        FormLineEditWidget* m_license;
        FormLineEditWidget* m_creatorName;
        FormLineEditWidget* m_repositoryURL;

        TabWidget* m_tabWidget;

        QDialogButtonBox* m_backNextButtons;
        QPushButton* m_backButton = nullptr;
        QPushButton* m_nextButton = nullptr;
    };


} // namespace O3DE::ProjectManager
