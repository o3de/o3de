/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QMenu>
#include <QString>

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>

struct ICVar;

class CVarMenu
    : public QMenu
{
    Q_OBJECT
public:
    // CVar that can be toggled on and off
    struct CVarToggle
    {
        QString m_cVarName;
        QString m_displayName;
        float m_onValue;
        float m_offValue;
    };

    // List of a CVar's available values and their descriptions
    using CVarDisplayNameValuePairs = AZStd::vector<AZStd::pair<QString, float>>;

    CVarMenu(QWidget* parent = nullptr);

    // Add an action that turns a CVar on/off
    void AddCVarToggleItem(CVarToggle cVarToggle);

    // Add a submenu of actions for a CVar that offers multiple values for exclusive selection
    void AddCVarValuesItem(QString cVarName,
        QString displayName,
        CVarDisplayNameValuePairs availableCVarValues,
        float offValue);

    // Add a submenu of actions for exclusively turning unique CVars on/off
    void AddUniqueCVarsItem(QString displayName,
        AZStd::vector<CVarToggle> availableCVars);

    // Add an action to reset all CVars to their original values before they
    // were modified by this menu
    void AddResetCVarsItem();

    void AddSeparator();

private:
    void SetCVarsToOffValue(const AZStd::vector<CVarToggle>& cVarToggles, const CVarToggle& excludeCVarToggle);
    void SetCVar(ICVar* cVar, float newValue);

    // Original CVar values before they were modified by this menu
    AZStd::unordered_map<AZStd::string, float> m_originalCVarValues;
};
