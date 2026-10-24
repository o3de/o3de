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
    //! Ready-made entities: an entity with a set of components and a few properties set, created in one click.
    //!
    //! Presets are data. Gems ship them as JSON next to the components they use, and user presets
    //! live in a JSON file inside the project.
    namespace EntityPresets
    {
        //! A value to write into a component property.
        //!
        //! The type is stored explicitly because a JSON number cannot say whether it is an integer
        //! or a float, and the property setter needs a type it can convert.
        struct PropertyValue
        {
            AZ_TYPE_INFO(PropertyValue, "{A208839A-3A9E-4E88-8414-135FD6EAE4DA}");

            enum class Type
            {
                Bool,
                Int,
                Double,
                String,
                //! A product path in the asset cache ("objects/_primitives/_box_1x1.fbx.azmodel"),
                //! resolved to an AssetId at creation time.
                AssetPath
            };

            Type m_type = Type::Int;
            bool m_bool = false;
            AZ::s64 m_int = 0;
            double m_double = 0.0;
            AZStd::string m_string;

            //! Convert to the type the property setter expects.
            //! @param resolved Set to false when an asset path is not in the catalog; the caller
            //! should then leave the property at its default.
            AZTF_API AZStd::any ToAny(bool& resolved) const;
        };

        struct PropertyAssignment
        {
            AZ_TYPE_INFO(PropertyAssignment, "{A9CB159D-77B8-433D-8731-BAD4CE74DE1C}");

            //! Edit Context property path, e.g. "Controller|Configuration|Light type".
            AZStd::string m_path;
            PropertyValue m_value;
        };

        struct ComponentSpec
        {
            AZ_TYPE_INFO(ComponentSpec, "{D5A8A950-ED9A-4DF1-89A1-CEAFF4E1B5A2}");

            //! Display name as shown in Add Component, e.g. "Light". Resolved to a type id at creation time.
            AZStd::string m_componentName;
            AZStd::vector<PropertyAssignment> m_properties;
        };

        struct Preset
        {
            AZ_TYPE_INFO(Preset, "{92BD5109-4C04-4E10-844D-2B3F1C387AF9}");

            AZStd::string m_name;

            //! Optional. Used as the menu tooltip.
            AZStd::string m_description;

            //! Groups presets into submenus.
            AZStd::string m_category;
            AZStd::vector<ComponentSpec> m_components;

            //! Components added to the level entity, if not already there, before the new entity is
            //! created (e.g. Terrain World, Vegetation System Settings).
            AZStd::vector<ComponentSpec> m_levelComponents;

            //! True for compiled-in and gem presets. Only user presets are saved to the project file.
            //! Not reflected; set by the loader.
            bool m_readOnly = false;

            //! The gem that supplied the preset, empty for compiled-in and user presets. Not reflected.
            AZStd::string m_sourceGem;
        };

        //! The shape of a presets file. The project file and gem files use the same shape.
        struct PresetFile
        {
            AZ_TYPE_INFO(PresetFile, "{F1ABE77D-85F2-45FB-A45C-FD918E82B256}");

            AZStd::vector<Preset> m_presets;
        };

        //! Reflect the preset types and register their JSON serializer.
        AZTF_API void Reflect(AZ::ReflectContext* context);

        //! The compiled-in presets.
        AZTF_API const AZStd::vector<Preset>& BuiltIn();

        //! Presets from each enabled gem's Presets/*.entitypresets.json. Read-only.
        AZTF_API const AZStd::vector<Preset>& FromGems();

        //! Compiled-in, then gem, then user presets - the order they appear in menus.
        AZTF_API const AZStd::vector<Preset>& All();

        //! User presets, as loaded from the project file.
        AZTF_API const AZStd::vector<Preset>& User();

        //! Re-read gem and user presets from disk.
        AZTF_API void Reload();

        //! Write the user presets to the project file.
        //! @return False if the file could not be written; the reason is logged.
        AZTF_API bool SaveUser(const AZStd::vector<Preset>& presets);

        //! Path of the project's user presets file.
        AZTF_API AZStd::string UserPresetsPath();

        //! Create an entity from @p preset as one undo step.
        //!
        //! The entity is parented to the selected entity when exactly one is selected.
        //! @return The new entity, or an invalid id if it could not be created.
        AZTF_API AZ::EntityId Create(const Preset& preset);
    } // namespace EntityPresets
} // namespace AzToolsFramework
