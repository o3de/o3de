/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/EntityPresets/EntityPresetMenu.h>
#include <AzToolsFramework/Entity/EntityPresets/EntityPresets.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>

#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/UI/EntityPresets/ActionManagerIdentifiers/EntityPresetsIdentifiers.h>
#include <AzToolsFramework/UI/EntityPresets/PresetEditorDialog.h>

#include <QWidget>

namespace AzToolsFramework
{
    namespace EntityPresetMenu
    {
        namespace
        {
            //! Where uncategorised presets land.
            constexpr const char* OtherCategory = "Other";

            //! Where the presets submenu sits in the context menus. High so it lands below the
            //! stock entries rather than above Create Entity.
            constexpr int RootMenuSortKey = 40000;

            //! Everything registered so far, so a refresh can take the old entries out of the
            //! menus before putting the new ones in. There is no way to unregister an action, so
            //! a removed preset's action lingers - harmlessly, since nothing references it once
            //! it is out of the menu.
            AZStd::vector<AZStd::string>& RegisteredActionIdentifiers()
            {
                static AZStd::vector<AZStd::string> identifiers;
                return identifiers;
            }

            AZStd::vector<AZStd::string>& RegisteredCategoryMenus()
            {
                static AZStd::vector<AZStd::string> identifiers;
                return identifiers;
            }

            //! Turn arbitrary preset text into something usable in an identifier.
            //!
            //! Identifiers are dotted ASCII by convention, and preset names are free text that can
            //! contain spaces, brackets and punctuation ("Point Light (Sphere)", "Plane 3x3").
            AZStd::string Slug(const AZStd::string& text)
            {
                AZStd::string slug;
                slug.reserve(text.size());

                bool lastWasSeparator = false;
                for (const char character : text)
                {
                    const bool alphanumeric = (character >= 'a' && character <= 'z') ||
                        (character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9');

                    if (alphanumeric)
                    {
                        slug.push_back(character);
                        lastWasSeparator = false;
                    }
                    else if (!lastWasSeparator && !slug.empty())
                    {
                        slug.push_back('_');
                        lastWasSeparator = true;
                    }
                }

                while (!slug.empty() && slug.back() == '_')
                {
                    slug.pop_back();
                }

                return slug;
            }

            AZStd::string CategoryMenuIdentifierFor(const AZStd::string& category)
            {
                return AZStd::string(EntityPresetsIdentifiers::EntityPresetsCategoryMenuIdentifierPrefix) +
                    Slug(category);
            }

            //! Categories in alphabetical order, with "Other" last.
            //!
            //! Alphabetical because there is nothing else left to order by. Presets now arrive from
            //! whichever gems happen to be enabled, in whatever order VisitActiveGems returns them,
            //! so first-appearance order stopped carrying meaning the moment the compiled-in table
            //! was emptied - it just made the menu depend on the gem list. Alphabetical at least
            //! makes a category's position predictable from its name.
            //!
            //! Order *within* a category is deliberately left alone: that one is authored. A gem
            //! groups its presets on purpose - the light types read as a progression rather than an
            //! alphabet - and sorting them would throw that away.
            AZStd::vector<AZStd::string> OrderedCategories()
            {
                AZStd::vector<AZStd::string> categories;
                AZStd::unordered_set<AZStd::string> seen;

                for (const EntityPresets::Preset& preset : EntityPresets::All())
                {
                    const AZStd::string category = preset.m_category.empty() ? AZStd::string(OtherCategory) : preset.m_category;
                    if (seen.insert(category).second)
                    {
                        categories.push_back(category);
                    }
                }

                AZStd::sort(
                    categories.begin(), categories.end(),
                    [](const AZStd::string& lhs, const AZStd::string& rhs)
                    {
                        // "Other" is the bucket for presets that named no category, so it belongs
                        // after the real ones rather than sorted in among them.
                        if ((lhs == OtherCategory) != (rhs == OtherCategory))
                        {
                            return rhs == OtherCategory;
                        }

                        return azstricmp(lhs.c_str(), rhs.c_str()) < 0;
                    });

                return categories;
            }
        } // namespace

        AZStd::string ActionIdentifierFor(const EntityPresets::Preset& preset)
        {
            return AZStd::string(EntityPresetsIdentifiers::EntityPresetActionIdentifierPrefix) +
                Slug(preset.m_category) + "." + Slug(preset.m_name);
        }

        void RegisterActions()
        {
            auto* actionManager = AZ::Interface<ActionManagerInterface>::Get();
            if (actionManager == nullptr)
            {
                return;
            }

            RegisteredActionIdentifiers().clear();

            for (const EntityPresets::Preset& preset : EntityPresets::All())
            {
                const AZStd::string identifier = ActionIdentifierFor(preset);

                // A refresh re-walks presets that are mostly unchanged, and registering a second
                // time would fail and log. Skipping is not a leak: the handler below captures the
                // preset by name and looks it up fresh, so an edited preset still creates the
                // edited entity.
                if (actionManager->IsActionRegistered(identifier))
                {
                    RegisteredActionIdentifiers().push_back(identifier);
                    continue;
                }

                ActionProperties properties;
                properties.m_name = preset.m_name;
                properties.m_description = preset.m_description.empty()
                    ? AZStd::string("Create an entity from the '") + preset.m_name + "' preset."
                    : preset.m_description;
                properties.m_category = "Entity Presets";

                // Captured by name rather than by value: presets can be edited in the panel while
                // the editor is running, and a copy taken at registration would go stale.
                const AZStd::string presetName = preset.m_name;
                const AZStd::string presetCategory = preset.m_category;

                actionManager->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier, identifier, properties,
                    [presetName, presetCategory]()
                    {
                        for (const EntityPresets::Preset& current : EntityPresets::All())
                        {
                            if (current.m_name == presetName && current.m_category == presetCategory)
                            {
                                EntityPresets::Create(current);
                                return;
                            }
                        }

                        AZ_Warning(
                            "EntityPresets", false, "Preset '%s' no longer exists.", presetName.c_str());
                    });

                RegisteredActionIdentifiers().push_back(identifier);
            }

            // The way in to the preset manager. Without it the user presets are only reachable by
            // hand-editing the project's JSON file, so this is part of the feature rather than a
            // convenience.
            if (!actionManager->IsActionRegistered(
                    AZStd::string(EntityPresetsIdentifiers::EntityPresetsManageActionIdentifier)))
            {
                ActionProperties properties;
                properties.m_name = "Manage Presets...";
                properties.m_description = "Add, edit and remove the presets stored with this project.";
                properties.m_category = "Entity Presets";

                actionManager->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier,
                    EntityPresetsIdentifiers::EntityPresetsManageActionIdentifier, properties,
                    []()
                    {
                        QWidget* parent = nullptr;
                        EditorWindowRequestBus::BroadcastResult(
                            parent, &EditorWindowRequests::GetAppMainWindow);

                        EntityPresetManagerDialog dialog(parent);
                        dialog.exec();
                    });
            }
        }

        void RegisterMenus()
        {
            auto* menuManager = AZ::Interface<MenuManagerInterface>::Get();
            if (menuManager == nullptr)
            {
                return;
            }

            if (!menuManager->IsMenuRegistered(EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier))
            {
                MenuProperties properties;
                properties.m_name = "Create Preset";
                menuManager->RegisterMenu(EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, properties);
            }

            RegisteredCategoryMenus().clear();

            for (const AZStd::string& category : OrderedCategories())
            {
                const AZStd::string identifier = CategoryMenuIdentifierFor(category);
                RegisteredCategoryMenus().push_back(identifier);

                if (menuManager->IsMenuRegistered(identifier))
                {
                    continue;
                }

                MenuProperties properties;
                properties.m_name = category;
                menuManager->RegisterMenu(identifier, properties);
            }
        }

        void BindMenus()
        {
            auto* menuManager = AZ::Interface<MenuManagerInterface>::Get();
            if (menuManager == nullptr)
            {
                return;
            }

            // Actions are placed into their category submenu in preset order, spaced so a later
            // insertion has room to land between two existing entries.
            AZStd::unordered_map<AZStd::string, int> nextSortKey;

            for (const EntityPresets::Preset& preset : EntityPresets::All())
            {
                const AZStd::string category = preset.m_category.empty() ? AZStd::string(OtherCategory) : preset.m_category;
                const AZStd::string menuIdentifier = CategoryMenuIdentifierFor(category);

                int& sortKey = nextSortKey[menuIdentifier];
                sortKey += 100;

                menuManager->AddActionToMenu(menuIdentifier, ActionIdentifierFor(preset), sortKey);
            }

            int categorySortKey = EntityPresetsIdentifiers::EntityPresetsCategorySortKeyStart;
            for (const AZStd::string& identifier : RegisteredCategoryMenus())
            {
                menuManager->AddSubMenuToMenu(
                    EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, identifier, categorySortKey);
                categorySortKey += EntityPresetsIdentifiers::EntityPresetsCategorySortKeyStep;
            }

            // A separator ahead of the gem band, so whatever a gem hangs here reads as its own
            // group rather than as one more category. Placed by the published constant rather than
            // relative to the last category, because that is exactly what makes the band usable
            // from outside: a gem cannot know how many categories were loaded.
            menuManager->AddSeparatorToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsGemSortKeyStart - 1);

            // Last, under its own separator: it manages the list rather than creating anything, so
            // it should not sit among the things that do.
            menuManager->AddSeparatorToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsManageSortKey - 1);
            menuManager->AddActionToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsManageActionIdentifier,
                EntityPresetsIdentifiers::EntityPresetsManageSortKey);


            // Both places you would right click when you want to make something.
            menuManager->AddSubMenuToMenu(
                EditorIdentifiers::ViewportContextMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, RootMenuSortKey);
            menuManager->AddSubMenuToMenu(
                EditorIdentifiers::EntityOutlinerContextMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, RootMenuSortKey);
        }

        void Refresh()
        {
            auto* menuManager = AZ::Interface<MenuManagerInterface>::Get();
            if (menuManager == nullptr)
            {
                return;
            }

            // Empty the category submenus before rebuilding, or a renamed or deleted preset would
            // stay in the menu alongside its replacement.
            for (const AZStd::string& menuIdentifier : RegisteredCategoryMenus())
            {
                for (const AZStd::string& actionIdentifier : RegisteredActionIdentifiers())
                {
                    menuManager->RemoveActionFromMenu(menuIdentifier, actionIdentifier);
                }
            }

            RegisterActions();
            RegisterMenus();
            BindMenus();
        }
    } // namespace EntityPresetMenu
} // namespace AzToolsFramework
