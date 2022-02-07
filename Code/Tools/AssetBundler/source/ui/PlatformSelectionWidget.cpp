/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <source/ui/PlatformSelectionWidget.h>
#include <source/ui/ui_PlatformSelectionWidget.h>

#include <AzCore/std/string/string.h>

namespace AssetBundler
{

    PlatformSelectionWidget::PlatformSelectionWidget(QWidget* parent)
        : QWidget(parent)
    {
        m_ui.reset(new Ui::PlatformSelectionWidget);
        m_ui->setupUi(this);

        m_platformHelper.reset(new AzFramework::PlatformHelper);

        m_selectedPlatforms = AzFramework::PlatformFlags::Platform_NONE;
        m_partiallySelectedPlatforms = AzFramework::PlatformFlags::Platform_NONE;
    }

    void PlatformSelectionWidget::Init(const AzFramework::PlatformFlags& enabledPlatforms, const QString& disabledPatformMessageOverride)
    {
        using namespace AzFramework;

        // Set up Platform Checkboxes
        for (const AZStd::string_view& platformString : m_platformHelper->GetPlatforms(PlatformFlags::AllNamedPlatforms))
        {
            // Create the CheckBox and store what platform it maps to
            QSharedPointer<QCheckBox> platformCheckBox(new QCheckBox(QString::fromUtf8(platformString.data(), static_cast<int>(platformString.size()))));
            m_platformCheckBoxes.push_back(platformCheckBox);
            PlatformFlags currentPlatformFlag = m_platformHelper->GetPlatformFlag(platformString);
            m_platformList.push_back(currentPlatformFlag);

            // Add the CheckBox to the view
            m_ui->platformCheckBoxLayout->addWidget(platformCheckBox.data());

            // If the platform is not enabled for the current project, disable it and tell the user
            if ((enabledPlatforms & currentPlatformFlag) == PlatformFlags::Platform_NONE)
            {
                platformCheckBox->setEnabled(false);
                if (disabledPatformMessageOverride.isEmpty())
                {
                    platformCheckBox->setToolTip(tr("This platform is not enabled for the current project."));
                }
                else
                {
                    platformCheckBox->setToolTip(disabledPatformMessageOverride);
                }
            }
            else
            {
                connect(platformCheckBox.data(), &QCheckBox::stateChanged, this, &PlatformSelectionWidget::OnPlatformSelectionChanged);
            }
        }
    }

    void PlatformSelectionWidget::SetSelectedPlatforms(
        const AzFramework::PlatformFlags& selectedPlatforms,
        const AzFramework::PlatformFlags& partiallySelectedPlatforms)
    {
        m_selectedPlatforms = AzFramework::PlatformFlags::Platform_NONE;
        m_partiallySelectedPlatforms = AzFramework::PlatformFlags::Platform_NONE;
        for (int checkBoxIndex = 0; checkBoxIndex < m_platformCheckBoxes.size(); ++checkBoxIndex)
        {
            AzFramework::PlatformFlags checkBoxPlatform = m_platformList[checkBoxIndex];
            bool isSelected = (checkBoxPlatform & selectedPlatforms) != AzFramework::PlatformFlags::Platform_NONE;
            m_platformCheckBoxes[checkBoxIndex]->setChecked(isSelected);
            if (isSelected)
            {
                m_selectedPlatforms |= checkBoxPlatform;
            }

            // Set the check status to Qt::PartiallyChecked when some of the selected items support the current platform
            bool isPartiallySelected = (checkBoxPlatform & partiallySelectedPlatforms) != AzFramework::PlatformFlags::Platform_NONE;
            if (isPartiallySelected)
            {
                m_platformCheckBoxes[checkBoxIndex]->setCheckState(Qt::PartiallyChecked);
                m_partiallySelectedPlatforms |= checkBoxPlatform;
            }
        }

        emit PlatformsSelected(m_selectedPlatforms, m_partiallySelectedPlatforms);
    }

    AzFramework::PlatformFlags PlatformSelectionWidget::GetSelectedPlatforms()
    {
        return m_selectedPlatforms;
    }

    AzFramework::PlatformFlags PlatformSelectionWidget::GetPartiallySelectedPlatforms()
    {
        return m_partiallySelectedPlatforms;
    }

    void PlatformSelectionWidget::OnPlatformSelectionChanged()
    {
        m_selectedPlatforms = AzFramework::PlatformFlags::Platform_NONE;
        m_partiallySelectedPlatforms = AzFramework::PlatformFlags::Platform_NONE;
        for (int checkBoxIndex = 0; checkBoxIndex < m_platformCheckBoxes.size(); ++checkBoxIndex)
        {
            if (m_platformCheckBoxes[checkBoxIndex]->checkState() == Qt::Checked)
            {
                m_selectedPlatforms |= m_platformList[checkBoxIndex];
            }
            else if (m_platformCheckBoxes[checkBoxIndex]->checkState() == Qt::PartiallyChecked)
            {
                m_partiallySelectedPlatforms |= m_platformList[checkBoxIndex];
            }
        }

        emit PlatformsSelected(m_selectedPlatforms, m_partiallySelectedPlatforms);
    }

} // namespace AssetBundler

#include <source/ui/moc_PlatformSelectionWidget.cpp>
