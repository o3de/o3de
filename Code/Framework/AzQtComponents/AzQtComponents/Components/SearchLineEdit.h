/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QLineEdit>
#endif

namespace AzQtComponents
{

class AZ_QT_COMPONENTS_API SearchLineEdit
    : public QLineEdit
{
    Q_OBJECT
public:
    explicit SearchLineEdit(QWidget* parent = nullptr);
    bool errorState() const;
    void setMenu(QMenu* menu);
    void setIconToolTip(const QString& tooltip);

    // Returns the text that the user input in the case of a QCompleter being present.
    // There are some weird edge cases with using QCompleter::completionPrefix that this will handle
    // And give reasonable results for.
    QString userInputText() const;

public slots:
    void setErrorState(bool errorState = true);

signals:
    void errorStateChanged(bool errorState);
    void menuEntryClicked(QAction* action);

private slots:
    void displayMenu();

private:
    bool m_errorState = false;
    QAction* m_searchAction;
    QMenu* m_menu = nullptr;
};

}
