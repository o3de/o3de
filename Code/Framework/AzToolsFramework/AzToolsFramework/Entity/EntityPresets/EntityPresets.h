/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZStd
{
    class any;
}

namespace AzToolsFramework
{
    //! Ready-made entities: "give me a spot light", "give me a shaderball", "give me a PostFX
    //! volume with exposure control on it" - one click instead of create-entity then hunt for the
    //! components then set the two properties that make it useful.
    //!
    //! Presets are **data**, not code. The built-in set is compiled in and read-only; anything the
    //! user adds lives in a JSON file inside the project, so presets travel with the project and
    //! can be committed and shared rather than being stuck on one machine.
    namespace EntityPresets
    {
        //! A value to write into a component property.
        //!
        //! The type has to be carried explicitly rather than inferred from the JSON: the property
        //! setter takes an AZStd::any, and what is inside it has to be something
        //! PropertyTreeEditor can convert to the property's real type. A bare JSON number does not
        //! say whether it meant an integer or a float.
        struct PropertyValue
        {
            AZ_TYPE_INFO(PropertyValue, "{A208839A-3A9E-4E88-8414-135FD6EAE4DA}");

            enum class Type
            {
                Bool,
                Int,
                Double,
                String,
                //! A product path in the asset cache ("objects/_primitives/_box_1x1.fbx.azmodel").
                //! Resolved to an AssetId at creation time rather than stored as one, so a preset
                //! stays valid across machines, re-processed caches and project moves.
                AssetPath
            };

            Type m_type = Type::Int;
            bool m_bool = false;
            AZ::s64 m_int = 0;
            double m_double = 0.0;
            AZStd::string m_string;

            //! Convert to the type the property setter expects.
            //! @param resolved Set to false when an asset path is not in the catalogue, in which
            //! case the caller should leave the property at its default rather than clearing it.
            AZTF_API AZStd::any ToAny(bool& resolved) const;
        };

        struct PropertyAssignment
        {
            AZ_TYPE_INFO(PropertyAssignment, "{A9CB159D-77B8-433D-8731-BAD4CE74DE1C}");

            //! Property path as the Edit Context spells it, e.g.
            //! "Controller|Configuration|Light type".
            AZStd::string m_path;
            PropertyValue m_value;
        };

        struct ComponentSpec
        {
            AZ_TYPE_INFO(ComponentSpec, "{D5A8A950-ED9A-4DF1-89A1-CEAFF4E1B5A2}");

            //! Display name as it appears in Add Component, e.g. "Light", "PostFX Layer".
            //! Resolved to a type id at creation time - type ids are not stable to write down.
            AZStd::string m_componentName;
            AZStd::vector<PropertyAssignment> m_properties;
        };

        struct Preset
        {
            AZ_TYPE_INFO(Preset, "{92BD5109-4C04-4E10-844D-2B3F1C387AF9}");

            AZStd::string m_name;
            //! Groups presets into submenus. Free text; anything sharing a category groups together.
            AZStd::string m_category;
            AZStd::vector<ComponentSpec> m_components;

            //! Not editable here - either compiled in, or owned by a gem. Only user presets are
            //! written back to the project's JSON file.
            //!
            //! Deliberately not reflected: it describes where this preset came from at runtime, not
            //! anything that belongs in a file. A preset read from the project file is a user preset
            //! by definition, and one read from a gem is not, so both are set by the loader.
            bool m_builtIn = false;

            //! Name of the gem that supplied it, empty for built-ins and user presets. Shown in the
            //! manager so it is obvious where a preset came from and why it cannot be edited.
            //! Not reflected, for the same reason as m_builtIn.
            AZStd::string m_sourceGem;
        };

        //! The shape of a presets file: one "presets" array and nothing else.
        //!
        //! The project's file and a gem's files use the same shape on purpose, so a preset can be
        //! moved between them by copying it, and a gem can ship exactly what the editor writes.
        struct PresetFile
        {
            AZ_TYPE_INFO(PresetFile, "{F1ABE77D-85F2-45FB-A45C-FD918E82B256}");

            AZStd::vector<Preset> m_presets;
        };

        //! Reflect the preset types and register the JSON serializer that keeps the file format
        //! compact. Call once at application startup.
        AZTF_API void Reflect(AZ::ReflectContext* context);

        //! The compiled-in presets. Always available, never written to disk.
        AZTF_API const AZStd::vector<Preset>& BuiltIn();

        //! Presets shipped by whichever gems are enabled, from each gem's Presets/*.json.
        //!
        //! A gem that introduces components is the thing best placed to say how they are usually
        //! assembled - the terrain gem knows what a terrain needs, the PhysX gem knows what a rigid
        //! body setup looks like. Reading them from the gem means enabling it brings its presets
        //! along, with no separate install step and nothing to keep in sync.
        //!
        //! Read-only: they belong to the gem, and editing them here would be overwritten the next
        //! time it updated. Duplicate one to get an editable copy.
        AZTF_API const AZStd::vector<Preset>& FromGems();

        //! Built-ins, then gem-provided, then the user's own - the order they appear in menus.
        AZTF_API const AZStd::vector<Preset>& All();

        //! User presets only, as loaded from the project file.
        AZTF_API const AZStd::vector<Preset>& User();

        //! Re-read the user presets from disk and rebuild the combined list.
        AZTF_API void Reload();

        //! Write the user presets to the project file and rebuild the combined list.
        //! @return False if the file could not be written, with the reason logged.
        AZTF_API bool SaveUser(const AZStd::vector<Preset>& presets);

        //! Where the user presets are stored, for the UI to show and for error messages.
        AZTF_API AZStd::string UserPresetsPath();

        //! Build an entity from @p preset: create it, add each component, apply each property.
        //!
        //! Parented to the selected entity when exactly one is selected - so building a rig by
        //! clicking presets in sequence nests naturally - and left at the root otherwise.
        //!
        //! The whole thing is one undo batch: Ctrl+Z removes the entity, its components and its
        //! property values in a single step rather than a dozen.
        //!
        //! @return The new entity, or an invalid id if it could not be created.
        AZTF_API AZ::EntityId Create(const Preset& preset);
    } // namespace EntityPresets
} // namespace AzToolsFramework
