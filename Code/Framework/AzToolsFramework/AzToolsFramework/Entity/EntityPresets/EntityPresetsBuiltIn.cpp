/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/*
 * The compiled-in preset library.
 *
 * Kept in its own file because it is data, not logic - the code that consumes it lives in
 * EntityPresets.cpp and should stay readable without scrolling past seventy table entries.
 *
 * What is left is the one preset built from a component AzToolsFramework provides itself:
 * ScriptEditorComponent registers "Lua Script". Everything else moved out to
 * Presets/<name>.entitypresets.json inside the gem that owns its components, which
 * EntityPresets::FromGems() reads - so a preset now appears exactly when the thing it builds
 * is actually available, and disappears with its gem.
 *
 * Kept rather than deleted because this is the right home for a preset built on a framework
 * component, and there is no gem that could honestly claim one.
 */

#include <AzToolsFramework/Entity/EntityPresets/EntityPresets.h>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        namespace
        {
            //! A preset that is just "an entity with these components on it, all left at default".
            Preset Simple(const char* name, const char* category, const AZStd::vector<const char*>& componentNames)
            {
                Preset preset;
                preset.m_name = name;
                preset.m_category = category;
                preset.m_readOnly = true;

                for (const char* componentName : componentNames)
                {
                    ComponentSpec component;
                    component.m_componentName = componentName;
                    preset.m_components.push_back(AZStd::move(component));
                }

                return preset;
            }

            AZStd::vector<Preset> BuildBuiltInPresets()
            {
                AZStd::vector<Preset> presets;

                // -- Scripting ---------------------------------------------------------
                presets.push_back(Simple("Lua Script", "Scripting", { "Lua Script" }));

                return presets;
            }
        } // namespace

        const AZStd::vector<Preset>& BuiltIn()
        {
            static const AZStd::vector<Preset> presets = BuildBuiltInPresets();
            return presets;
        }
    } // namespace EntityPresets
} // namespace AzToolsFramework
