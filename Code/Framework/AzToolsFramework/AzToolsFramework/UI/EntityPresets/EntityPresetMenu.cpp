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

            //! Below the stock context menu entries.
            constexpr int RootMenuSortKey = 40000;

            //! What is currently in the menus, so Refresh can take it out first. Actions cannot be
            //! unregistered; a removed preset's action simply stays out of the menus.
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

            //! Reduce free text to ASCII letters and digits joined by '_', for use in identifiers.
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

            //! Categories in alphabetical order, with "Other" last. Presets within a category keep
            //! the order their authors gave them.
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

            // Two presets can reduce to the same identifier ("Plane 3x3", "Plane (3x3)"); only the
            // first gets an entry.
            AZStd::unordered_set<AZStd::string> claimed;

            for (const EntityPresets::Preset& preset : EntityPresets::All())
            {
                const AZStd::string identifier = ActionIdentifierFor(preset);

                if (!claimed.insert(identifier).second)
                {
                    AZ_Warning(
                        "EntityPresets", false,
                        "Preset '%s' in '%s' is left out of the menu: an earlier preset has the same "
                        "name and category once punctuation is ignored. Rename it or move it to another "
                        "category.",
                        preset.m_name.c_str(), preset.m_category.c_str());
                    continue;
                }

                // Already registered by an earlier refresh. The handler looks the preset up by name,
                // so it still builds the current version.
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

                // By name, not by value, so later edits to the preset are picked up.
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

            // In preset order within each category submenu.
            AZStd::unordered_map<AZStd::string, int> nextSortKey;

            // Only the first preset with a given identifier has an action; see RegisterActions.
            AZStd::unordered_set<AZStd::string> bound;

            for (const EntityPresets::Preset& preset : EntityPresets::All())
            {
                const AZStd::string actionIdentifier = ActionIdentifierFor(preset);
                if (!bound.insert(actionIdentifier).second)
                {
                    continue;
                }

                const AZStd::string category = preset.m_category.empty() ? AZStd::string(OtherCategory) : preset.m_category;
                const AZStd::string menuIdentifier = CategoryMenuIdentifierFor(category);

                int& sortKey = nextSortKey[menuIdentifier];
                sortKey += 100;

                menuManager->AddActionToMenu(menuIdentifier, actionIdentifier, sortKey);
            }

            int categorySortKey = EntityPresetsIdentifiers::EntityPresetsCategorySortKeyStart;
            for (const AZStd::string& identifier : RegisteredCategoryMenus())
            {
                menuManager->AddSubMenuToMenu(
                    EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, identifier, categorySortKey);
                categorySortKey += EntityPresetsIdentifiers::EntityPresetsCategorySortKeyStep;
            }

            // Separators before the gem entries and before "Manage Presets...".
            menuManager->AddSeparatorToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsGemSortKeyStart - 1);

            menuManager->AddSeparatorToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsManageSortKey - 1);
            menuManager->AddActionToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsManageActionIdentifier,
                EntityPresetsIdentifiers::EntityPresetsManageSortKey);

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

            // Empty the category submenus first, so renamed or deleted presets drop out.
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
