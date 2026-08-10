/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/EntityPresets/EntityPresetMenu.h>
#include <AzToolsFramework/Entity/EntityPresets/EntityPresets.h>
#include <AzToolsFramework/UI/EntityPresets/TerrainPreset.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Interface/Interface.h>
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
            //! Terrain is not an ordinary preset - it is hand-written because it touches the level
            //! entity and builds a multi-entity hierarchy - so it is registered separately and sits
            //! in its own submenu alongside the data-driven ones.
            struct TerrainVariantAction
            {
                const char* m_identifier;
                const char* m_name;
                const char* m_description;
                TerrainPreset::Variant m_variant;
            };

            //! Ordered as a progression - each builds on the one above - so the list reads as
            //! "how much do I want" rather than as three unrelated options.
            constexpr TerrainVariantAction TerrainVariants[] = {
                { "o3de.action.editor.entityPreset.terrain.simple", "Simple Terrain",
                  "A terrain region with noise driving its height. The quickest way to have ground.",
                  TerrainPreset::Variant::Simple },
                { "o3de.action.editor.entityPreset.terrain.landscape", "Landscape Terrain",
                  "The same terrain under a Landscape Canvas entity, so the whole setup opens as a "
                  "node graph.",
                  TerrainPreset::Variant::Landscape },
                { "o3de.action.editor.entityPreset.terrain.vegetation", "Landscape Terrain + Vegetation",
                  "Landscape terrain plus a vegetation area and the level's vegetation settings. Add a "
                  "mesh to its Vegetation Asset List to see anything planted.",
                  TerrainPreset::Variant::LandscapeWithVegetation },
            };

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

            //! Categories in the order they first appear in the preset list, so built-ins keep the
            //! order they are declared in and user categories follow.
            AZStd::vector<AZStd::string> OrderedCategories()
            {
                AZStd::vector<AZStd::string> categories;
                AZStd::unordered_set<AZStd::string> seen;

                for (const EntityPresets::Preset& preset : EntityPresets::All())
                {
                    const AZStd::string category = preset.m_category.empty() ? AZStd::string("Other") : preset.m_category;
                    if (seen.insert(category).second)
                    {
                        categories.push_back(category);
                    }
                }

                return categories;
            }
        } // namespace

        AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> TerrainActions()
        {
            AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> actions;
            for (const TerrainVariantAction& terrain : TerrainVariants)
            {
                actions.emplace_back(terrain.m_name, terrain.m_identifier);
            }
            return actions;
        }

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
                properties.m_description =
                    AZStd::string("Create an entity from the '") + preset.m_name + "' preset.";
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

            for (const TerrainVariantAction& terrain : TerrainVariants)
            {
                if (actionManager->IsActionRegistered(terrain.m_identifier))
                {
                    continue;
                }

                ActionProperties properties;
                properties.m_name = terrain.m_name;
                properties.m_description = terrain.m_description;
                properties.m_category = "Entity Presets";

                const TerrainPreset::Variant variant = terrain.m_variant;
                actionManager->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier, terrain.m_identifier, properties,
                    [variant]() { TerrainPreset::Create(variant); });
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

            if (!menuManager->IsMenuRegistered(EntityPresetsIdentifiers::EntityPresetsTerrainMenuIdentifier))
            {
                MenuProperties terrainProperties;
                terrainProperties.m_name = "Terrain";
                menuManager->RegisterMenu(
                    EntityPresetsIdentifiers::EntityPresetsTerrainMenuIdentifier, terrainProperties);
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
                const AZStd::string category = preset.m_category.empty() ? AZStd::string("Other") : preset.m_category;
                const AZStd::string menuIdentifier = CategoryMenuIdentifierFor(category);

                int& sortKey = nextSortKey[menuIdentifier];
                sortKey += 100;

                menuManager->AddActionToMenu(menuIdentifier, ActionIdentifierFor(preset), sortKey);
            }

            int categorySortKey = 0;
            for (const AZStd::string& identifier : RegisteredCategoryMenus())
            {
                categorySortKey += 100;
                menuManager->AddSubMenuToMenu(
                    EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, identifier, categorySortKey);
            }

            // Terrain sits at the bottom of the root menu, below the category submenus, separated
            // because it builds something level-scoped rather than an ordinary entity.
            int terrainSortKey = 0;
            for (const TerrainVariantAction& terrain : TerrainVariants)
            {
                terrainSortKey += 100;
                menuManager->AddActionToMenu(
                    EntityPresetsIdentifiers::EntityPresetsTerrainMenuIdentifier, terrain.m_identifier,
                    terrainSortKey);
            }

            menuManager->AddSeparatorToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, categorySortKey + 50);
            menuManager->AddSubMenuToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsTerrainMenuIdentifier, categorySortKey + 100);

            // Last, under its own separator: it manages the list rather than creating anything, so
            // it should not sit among the things that do.
            menuManager->AddSeparatorToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier, categorySortKey + 150);
            menuManager->AddActionToMenu(
                EntityPresetsIdentifiers::EntityPresetsRootMenuIdentifier,
                EntityPresetsIdentifiers::EntityPresetsManageActionIdentifier, categorySortKey + 200);

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
