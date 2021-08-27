/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "CVarMenu.h"

CVarMenu::CVarMenu(QWidget* parent)
    : QMenu(parent)
{
}

void CVarMenu::AddCVarToggleItem(CVarToggle cVarToggle)
{
    // Add CVar toggle action
    QAction* action = addAction(cVarToggle.m_displayName);
    connect(action, &QAction::triggered, [this, cVarToggle](bool checked)
        {
            // Update the CVar's value based on the action's new checked state
            ICVar* cVar = gEnv->pConsole->GetCVar(cVarToggle.m_cVarName.toUtf8().data());
            if (cVar)
            {
                SetCVar(cVar, checked ? cVarToggle.m_onValue : cVarToggle.m_offValue);
            }
        });
    action->setCheckable(true);

    // Initialize the action's checked state based on the associated CVar's value
    ICVar* cVar = gEnv->pConsole->GetCVar(cVarToggle.m_cVarName.toUtf8().data());
    bool checked = (cVar && cVar->GetFVal() == cVarToggle.m_onValue);
    action->setChecked(checked);
}

void CVarMenu::AddCVarValuesItem(QString cVarName,
    QString displayName,
    CVarDisplayNameValuePairs availableCVarValues,
    float offValue)
{
    // Add a submenu offering multiple values for one CVar
    QMenu* menu = addMenu(displayName);
    QActionGroup* group = new QActionGroup(menu);
    group->setExclusive(true);

    ICVar* cVar = gEnv->pConsole->GetCVar(cVarName.toUtf8().data());
    float cVarValue = cVar ? cVar->GetFVal() : 0.0f;
    for (const auto& availableCVarValue : availableCVarValues)
    {
        QAction* action = menu->addAction(availableCVarValue.first);
        action->setCheckable(true);
        group->addAction(action);

        float availableOnValue = availableCVarValue.second;
        connect(action, &QAction::triggered, [this, action, cVarName, availableOnValue, offValue](bool checked)
            {
                ICVar* cVar = gEnv->pConsole->GetCVar(cVarName.toUtf8().data());
                if (cVar)
                {
                    if (!checked)
                    {
                        SetCVar(cVar, offValue);
                    }
                    else
                    {
                        // Toggle the CVar and update the action's checked state to
                        // allow none of the items to be checked in the exclusive group.
                        // Otherwise we could have just used the action's currently checked
                        // state and updated the CVar's value only
                        bool cVarOn = (cVar->GetFVal() == availableOnValue);
                        checked = !cVarOn;
                        SetCVar(cVar, checked ? availableOnValue : offValue);
                        action->setChecked(checked);
                    }
                }
            });

        // Initialize the action's checked state based on the CVar's current value
        bool checked = (cVarValue == availableOnValue);
        action->setChecked(checked);
    }
}

void CVarMenu::AddUniqueCVarsItem(QString displayName,
    AZStd::vector<CVarToggle> availableCVars)
{
    // Add a submenu of actions offering values for unique CVars
    QMenu* menu = addMenu(displayName);
    QActionGroup* group = new QActionGroup(menu);
    group->setExclusive(true);

    for (const CVarToggle& availableCVar : availableCVars)
    {
        QAction* action = menu->addAction(availableCVar.m_displayName);
        action->setCheckable(true);
        group->addAction(action);

        connect(action, &QAction::triggered, [this, action, availableCVar, availableCVars](bool checked)
            {
                ICVar* cVar = gEnv->pConsole->GetCVar(availableCVar.m_cVarName.toUtf8().data());
                if (cVar)
                {
                    if (!checked)
                    {
                        SetCVar(cVar, availableCVar.m_offValue);
                    }
                    else
                    {
                        // Toggle the CVar and update the action's checked state to
                        // allow none of the items to be checked in the exclusive group.
                        // Otherwise we could have just used the action's currently checked
                        // state and updated the CVar's value only
                        bool cVarOn = (cVar->GetFVal() == availableCVar.m_onValue);
                        bool cVarChecked = !cVarOn;
                        SetCVar(cVar, cVarChecked ? availableCVar.m_onValue : availableCVar.m_offValue);
                        action->setChecked(cVarChecked);
                        if (cVarChecked)
                        {
                            // Set the rest of the CVars in the group to their off values
                            SetCVarsToOffValue(availableCVars, availableCVar);
                        }
                    }
                }
            });

        // Initialize the action's checked state based on its associated CVar's current value
        ICVar* cVar = gEnv->pConsole->GetCVar(availableCVar.m_cVarName.toUtf8().data());
        bool cVarChecked = (cVar && cVar->GetFVal() == availableCVar.m_onValue);
        action->setChecked(cVarChecked);
        if (cVarChecked)
        {
            // Set the rest of the CVars in the group to their off values
            SetCVarsToOffValue(availableCVars, availableCVar);
        }
    }
}

void CVarMenu::AddResetCVarsItem()
{
    QAction* action = addAction(tr("Reset to Default"));
    connect(action, &QAction::triggered, this, [this]()
        {
            for (auto it : m_originalCVarValues)
            {
                ICVar* cVar = gEnv->pConsole->GetCVar(it.first.c_str());
                if (cVar)
                {
                    cVar->Set(it.second);
                }
            }
        });
}

void CVarMenu::SetCVarsToOffValue(const AZStd::vector<CVarToggle>& cVarToggles, const CVarToggle& excludeCVarToggle)
{
    // Set all but the specified CVars to their off values
    for (const CVarToggle& cVarToggle : cVarToggles)
    {
        if (cVarToggle.m_cVarName != excludeCVarToggle.m_cVarName
            || cVarToggle.m_onValue != excludeCVarToggle.m_onValue)
        {
            ICVar* cVar = gEnv->pConsole->GetCVar(cVarToggle.m_cVarName.toUtf8().data());
            if (cVar)
            {
                SetCVar(cVar, cVarToggle.m_offValue);
            }
        }
    }
}

void CVarMenu::SetCVar(ICVar* cVar, float newValue)
{
    float oldValue = cVar->GetFVal();
    cVar->Set(newValue);

    // Store original value for CVar if not already in the list
    m_originalCVarValues.emplace(AZStd::string(cVar->GetName()), oldValue);
}

void CVarMenu::AddSeparator()
{
    addSeparator();
}
