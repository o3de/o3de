/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QMainWindow>
#include <QScopedPointer>

namespace Ui {
    class ComponentDemoWidget;
}

class QMenu;

class ComponentDemoWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit ComponentDemoWidget(QWidget* parent = nullptr);
    ~ComponentDemoWidget() override;

    //! Rasterizes every gallery page into outputDirPath as numbered PNGs
    //! (whole window, including the custom title bar chrome), plus secondary
    //! interaction states per page: expanded trees/cards, open combo popups,
    //! modal dialogs (grabbed and closed via watchdog), floated dock widgets.
    //! Used by the --screenshot-dir command line mode for parity evaluation.
    void captureAllPages(const QString& outputDirPath);

    //! Selects a page by (case-insensitive, partial) title match or numeric
    //! index. Used by the --page command line option.
    void selectPage(const QString& pageNameOrIndex);

private:
    void capturePageInteractions(const QString& outputDirPath, int pageIndex, const QString& pageName);

public:

Q_SIGNALS:
    void styleChanged(bool enableLegacyUI);
    void refreshStyle();

private:
    void addPage(QWidget* widget, const QString& title);
    void setupMenuBar();
    void createEditMenuPlaceholders();

    QScopedPointer<Ui::ComponentDemoWidget> ui;
    QMenu* m_editMenu = nullptr;
};


