/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Compiled-in presets: only those built from components AzToolsFramework itself provides.
// Everything else ships as Presets/*.entitypresets.json in the gem that owns its components.

#include <AzToolsFramework/Entity/EntityPresets/EntityPresets.h>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        namespace
        {
            //! A preset of components left at their defaults.
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
