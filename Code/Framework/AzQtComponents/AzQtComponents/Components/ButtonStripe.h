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

#include <QWidget>
#include <QList>
#endif

class QGridLayout;
class QPushButton;
class QButtonGroup;
class QStringList;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ButtonStripe
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit ButtonStripe(QWidget* parent = nullptr);
        void addButtons(const QStringList& buttonNames, int current = 0);
        void setCurrent(int index);

    signals:
        void buttonClicked(int);

    private:
        QGridLayout* const m_gridLayout;
        QButtonGroup* const m_buttonGroup;
        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QList<QPushButton*> m_buttons;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    };
} // namespace AzQtComponents

