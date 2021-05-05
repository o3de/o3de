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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_GRIDSETTINGSDIALOG_H
#define CRYINCLUDE_EDITOR_GRIDSETTINGSDIALOG_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <AzCore/EBus/EBus.h>
#endif

// CGridSettingsDialog dialog

namespace Ui {
    class CGridSettingsDialog;
}

class CGridSettingsDialog
    : public QDialog
{
    Q_OBJECT
public:

    class Notifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void OnGridValuesUpdated() {}
    };

    using NotificationBus = AZ::EBus<Notifications>;

    CGridSettingsDialog(QWidget* pParent = nullptr);   // standard constructor
    virtual ~CGridSettingsDialog();

private slots:
    void accept() override;
    void OnBnUserDefined();
    void OnBnGetFromObject();
    void OnValueUpdate();

private:
    void EnableGridPropertyControls(const bool isUserDefined, const bool isGetFromObject);

    void OnInitDialog();
    void UpdateValues();

    QScopedPointer<Ui::CGridSettingsDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_GRIDSETTINGSDIALOG_H
