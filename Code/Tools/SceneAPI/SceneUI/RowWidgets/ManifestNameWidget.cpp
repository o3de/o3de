/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QStyle>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <SceneAPI/SceneUI/SceneWidgets/ManifestWidget.h>
#include <SceneAPI/SceneUI/RowWidgets/ManifestNameWidget.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            AZ_CLASS_ALLOCATOR_IMPL(ManifestNameWidget, SystemAllocator);

            ManifestNameWidget::ManifestNameWidget(QWidget* parent)
                : QLineEdit(parent)
                , m_filterType(GetO3deTypeId(AZ::Adl{}, AZStd::type_identity<DataTypes::IManifestObject>()))
                , m_inFailureState(false)
            {
                connect(this, &QLineEdit::textChanged, this, &ManifestNameWidget::OnTextChanged);
            }

            void ManifestNameWidget::SetName(const char* name)
            {
                setText(name);
                UpdateStatus(name, false);
            }

            void ManifestNameWidget::SetName(const AZStd::string& name)
            {
                setText(name.c_str());
                UpdateStatus(name, false);
            }

            const AZStd::string& ManifestNameWidget::GetName() const
            {
                return m_name;
            }

            void ManifestNameWidget::SetFilterType(const Uuid& type)
            {
                m_filterType = type;
            }

            void ManifestNameWidget::OnTextChanged(const QString& textValue)
            {
                m_name = textValue.toStdString().c_str();
                UpdateStatus(m_name, true);
                emit valueChanged(m_name);
            }

            void ManifestNameWidget::UpdateStatus(const AZStd::string& newName, bool checkAvailability)
            {
                AZStd::string error;
                bool isValid = AzFramework::StringFunc::Path::IsValid(newName.c_str(), false, false, &error);
                if (isValid && checkAvailability)
                {
                    isValid = IsAvailableName(error, newName);
                }

                if (!isValid && !m_inFailureState)
                {
                    m_originalToolTip = toolTip();
                    setToolTip(error.c_str());
                    setProperty("inputValid", "false");
                    m_inFailureState = true;
                    style()->unpolish(this);
                    style()->polish(this);
                }
                else if (isValid && m_inFailureState)
                {
                    setToolTip(m_originalToolTip);
                    setProperty("inputValid", "true");
                    m_inFailureState = false;
                    style()->unpolish(this);
                    style()->polish(this);
                }
            }

            bool ManifestNameWidget::IsAvailableName(AZStd::string& error, const AZStd::string& name) const
            {
                const UI::ManifestWidget* manifestWidget = UI::ManifestWidget::FindRoot(this);
                if (!manifestWidget)
                {
                    error = "ManifestNameWidget is not a child of a ManifestWidget. For correct name checking this is required.";
                    return false;
                }

                const SceneAPI::Containers::SceneManifest& manifest = manifestWidget->GetScene()->GetManifest();
                if (!DataTypes::Utilities::IsNameAvailable(name, manifest, m_filterType))
                {
                    error = "Name is already in use.";
                    return false;
                }

                return true;
            }

        } // SceneUI
    } // SceneAPI
} // AZ

#include <RowWidgets/moc_ManifestNameWidget.cpp>
