/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/EntityPresets/PresetRequirements.h>

#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>

#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/UI/EntityPresets/EntityPresetsStringUtils.h>

#include <QMessageBox>
#include <QStringList>
#include <QPushButton>
#include <QWidget>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        namespace
        {
            QWidget* MainWindow()
            {
                QWidget* parent = nullptr;
                EditorWindowRequestBus::BroadcastResult(parent, &EditorWindowRequests::GetAppMainWindow);
                return parent;
            }

            //! "PhysX Heightfield Collider (PhysX5 gem)", one per line.
            QString AsList(const AZStd::vector<RequiredComponent>& missing)
            {
                QStringList lines;
                for (const RequiredComponent& component : missing)
                {
                    lines.append(
                        component.m_gemName.empty()
                            ? ToQString(component.m_componentName)
                            : QStringLiteral("%1  (%2 gem)")
                                  .arg(ToQString(component.m_componentName), ToQString(component.m_gemName)));
                }

                return lines.join(QStringLiteral("\n"));
            }

            //! The gems behind the missing components, each named once and in the order met.
            QString AsGemSentence(const AZStd::vector<RequiredComponent>& missing)
            {
                QStringList gems;
                for (const RequiredComponent& component : missing)
                {
                    const QString gem = ToQString(component.m_gemName);
                    if (!gem.isEmpty() && !gems.contains(gem))
                    {
                        gems.append(gem);
                    }
                }

                if (gems.isEmpty())
                {
                    return {};
                }

                return gems.size() == 1
                    ? QStringLiteral("Enable the %1 gem in Project Manager and restart the Editor.").arg(gems.front())
                    : QStringLiteral("Enable these gems in Project Manager and restart the Editor: %1.")
                          .arg(gems.join(QStringLiteral(", ")));
            }
        } // namespace

        AZStd::vector<RequiredComponent> MissingComponents(const AZStd::vector<RequiredComponent>& required)
        {
            AZStd::vector<RequiredComponent> missing;
            if (required.empty())
            {
                return missing;
            }

            // Match against the registered names instead of looking each component up, which would log a
            // warning for every name that does not resolve. Both entity types, since level components
            // (Terrain World, Vegetation System Settings) are not in the Game list.
            AZStd::unordered_set<AZStd::string> registered;
            for (const EditorComponentAPIRequests::EntityType entityType :
                 { EditorComponentAPIRequests::EntityType::Game,
                   EditorComponentAPIRequests::EntityType::Level })
            {
                AZStd::vector<AZStd::string> names;
                EditorComponentAPIBus::BroadcastResult(
                    names, &EditorComponentAPIRequests::BuildComponentTypeNameListByEntityType, entityType);

                registered.insert(names.begin(), names.end());
            }

            for (const RequiredComponent& component : required)
            {
                if (registered.find(component.m_componentName) == registered.end())
                {
                    missing.push_back(component);
                }
            }

            return missing;
        }

        void ReportMissingRequirements(
            const AZStd::string& presetName, const AZStd::vector<RequiredComponent>& missing)
        {
            if (missing.empty())
            {
                return;
            }

            QMessageBox box(MainWindow());
            box.setIcon(QMessageBox::Warning);
            box.setWindowTitle(QStringLiteral("Cannot create %1").arg(ToQString(presetName)));
            box.setText(
                QStringLiteral("'%1' needs components that are not available in this project, so "
                               "nothing has been created.")
                    .arg(ToQString(presetName)));
            box.setInformativeText(AsGemSentence(missing));
            box.setDetailedText(AsList(missing));
            box.setStandardButtons(QMessageBox::Ok);
            box.exec();
        }

        bool ConfirmReducedSetup(
            const AZStd::string& presetName,
            const AZStd::string& reduction,
            const AZStd::vector<RequiredComponent>& missing)
        {
            if (missing.empty())
            {
                return true;
            }

            QMessageBox box(MainWindow());
            box.setIcon(QMessageBox::Question);
            box.setWindowTitle(QStringLiteral("Create %1?").arg(ToQString(presetName)));
            box.setText(
                QStringLiteral("Some of what '%1' builds is not available in this project.")
                    .arg(ToQString(presetName)));
            box.setInformativeText(
                QStringLiteral("%1\n\n%2")
                    .arg(QStringLiteral("It can still be created %1.").arg(ToQString(reduction)))
                    .arg(AsGemSentence(missing)));
            box.setDetailedText(AsList(missing));

            // The reduced build is the default; Cancel builds nothing.
            QPushButton* create = box.addButton(
                QStringLiteral("Create %1").arg(ToQString(reduction)), QMessageBox::AcceptRole);
            box.addButton(QMessageBox::Cancel);
            box.setDefaultButton(create);

            box.exec();
            return box.clickedButton() == create;
        }
    } // namespace EntityPresets
} // namespace AzToolsFramework
