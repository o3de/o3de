/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StandaloneTools_precompiled.h"

#include <Source/Driller/Replica/ReplicaDataAggregatorConfigurationPanel.hxx>
#include <Source/Driller/Replica/moc_ReplicaDataAggregatorConfigurationPanel.cpp>

namespace Driller
{
    ////////////////////////////////////////////
    // ReplicaDataAggregatorConfigurationPanel
    ////////////////////////////////////////////

    ReplicaDataAggregatorConfigurationPanel::ReplicaDataAggregatorConfigurationPanel(ReplicaDataConfigurationSettings& configurationSettings)
        : m_configurationSettings(configurationSettings)
    {
        setupUi(this);

        InitUI();

        connect(fpsSpinBox, SIGNAL(valueChanged(int)), SLOT(OnFPSChanged(int)));
        connect(unitSelector, SIGNAL(currentIndexChanged(int)), SLOT(OnTypeChanged(int)));
        connect(budgetSpinBox, SIGNAL(valueChanged(int)), SLOT(OnBudgetChanged(int)));
        
        m_changeTimer.setInterval(500);
        m_changeTimer.setSingleShot(true);

        connect(&m_changeTimer, SIGNAL(timeout()), SLOT(OnTimeout()));
    }

    ReplicaDataAggregatorConfigurationPanel::~ReplicaDataAggregatorConfigurationPanel()
    {
    }

    void ReplicaDataAggregatorConfigurationPanel::InitUI()
    {
        fpsSpinBox->setValue(m_configurationSettings.m_frameRate);

        for (unsigned int i = 0; i < static_cast<int>(ReplicaDataConfigurationSettings::CDT_Max); ++i)
        {
            switch (static_cast<ReplicaDataConfigurationSettings::ConfigurationDisplayType>(i))
            {
            case ReplicaDataConfigurationSettings::CDT_Frame:
                unitSelector->addItem("Bytes per Frame");
                break;
            case ReplicaDataConfigurationSettings::CDT_Second:
                unitSelector->addItem("Bytes per Second");
                break;
            case ReplicaDataConfigurationSettings::CDT_Minute:
                unitSelector->addItem("Bytes per Minute");
                break;
            default:
                unitSelector->addItem("???");
                AZ_Error("ReplicaDataAggregatorConfigurationPanel", false, "Unhandled unit given to ReplicaDataConfigurationSettings.");
                break;
            }
        }

        int displayConfiguration = static_cast<int>(m_configurationSettings.m_configurationDisplay);

        if (displayConfiguration >= 0 && displayConfiguration < static_cast<int>(ReplicaDataConfigurationSettings::CDT_Max))
        {
            unitSelector->setCurrentIndex(displayConfiguration);
        }

        DisplayTypeDescriptor();
        UpdateBudgetDisplay();
    }

    void ReplicaDataAggregatorConfigurationPanel::OnBudgetChanged(int value)
    {
        float budget = static_cast<float>(value);
        switch (m_configurationSettings.m_configurationDisplay)
        {
        case ReplicaDataConfigurationSettings::CDT_Frame:
            // Don't need to do anything on Seconds
            break;
        case ReplicaDataConfigurationSettings::CDT_Minute:
            budget = budget / 60.0f;
            // Fall through to the seconds.
        case ReplicaDataConfigurationSettings::CDT_Second:
            budget = budget / static_cast<float>(m_configurationSettings.m_frameRate);
            break;
        default:
            AZ_Error("ReplicaDataAggregatorConfigurationPanel", false, "Unknown configuraiton display given.");
            break;
        }

        m_configurationSettings.m_averageFrameBudget = static_cast<float>(budget);

        m_changeTimer.start();
    }

    void ReplicaDataAggregatorConfigurationPanel::OnTypeChanged(int type)
    {
        if (type >= 0 && type < static_cast<int>(ReplicaDataConfigurationSettings::CDT_Max))
        {
            m_configurationSettings.m_configurationDisplay = static_cast<ReplicaDataConfigurationSettings::ConfigurationDisplayType>(type);

            DisplayTypeDescriptor();
            UpdateBudgetDisplay();
        }
    }

    void ReplicaDataAggregatorConfigurationPanel::OnFPSChanged(int fps)
    {
        // We don't want the budget to change when we switch FPS
        // but we may need to update the value we actually store.
        // so we'll store the original value
        int budget = budgetSpinBox->value();

        m_configurationSettings.m_frameRate = fps;

        // Then fake setting that value to recalculate using the
        // new frame rate correctly.
        OnBudgetChanged(budget);
    }

    void ReplicaDataAggregatorConfigurationPanel::OnTimeout()
    {
        emit ConfigurationChanged();
    }

    void ReplicaDataAggregatorConfigurationPanel::DisplayTypeDescriptor()
    {
        switch (m_configurationSettings.m_configurationDisplay)
        {
        case ReplicaDataConfigurationSettings::CDT_Frame:
            unitLabel->setText("Frame");
            break;
        case ReplicaDataConfigurationSettings::CDT_Second:
            unitLabel->setText("Second");
            break;
        case ReplicaDataConfigurationSettings::CDT_Minute:
            unitLabel->setText("Minute");
            break;
        default:
            unitLabel->setText("???");
            AZ_Error("ReplicaDataAggregatorCOnfigurationPanel", false, "Unknown unit configuration.");
        }
    }

    void ReplicaDataAggregatorConfigurationPanel::UpdateBudgetDisplay()
    {
        float displayValue = m_configurationSettings.m_averageFrameBudget;

        switch (m_configurationSettings.m_configurationDisplay)
        {
        case ReplicaDataConfigurationSettings::CDT_Frame:
            // Frame is good as is.
            break;
        case ReplicaDataConfigurationSettings::CDT_Minute:
            // Multiply by 60 then let it fall through into the conversion to seconds.
            displayValue *= 60;
        case ReplicaDataConfigurationSettings::CDT_Second:
            // For seconds, multiply the average frame budget, by the frame rate.
            displayValue *= m_configurationSettings.m_frameRate;
            break;
        default:
            AZ_Error("ReplicaDataAggregationConfigurationPanel", false, "Unknown configuration type given.");
        }

        budgetSpinBox->setValue(static_cast<int>(displayValue));
    }


#undef TOSTRING
}
