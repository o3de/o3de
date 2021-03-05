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
