/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>

namespace AZ
{
    namespace RPI
    {
        // Normally we wouldn't want editor-related code mixed in with runtime code, but
        // since this data can be modified dynamically, keeping it in the runtime makes
        // the overall material functor design simpler and more user-friendly.


        //! Visibility for each material property.
        //! If the data field is empty, use default as editable.
        enum class MaterialPropertyVisibility : uint32_t
        {
            Enabled,  //!< The property is visible and editable
            Disabled, //!< The property is visible but non-editable
            Hidden,   //!< The property is invisible

            Default = Enabled
        };

        struct MaterialPropertyRange
        {
            MaterialPropertyRange() = default;
            MaterialPropertyRange(
                const MaterialPropertyValue& max,
                const MaterialPropertyValue& min,
                const MaterialPropertyValue& softMax,
                const MaterialPropertyValue& softMin
            )
                : m_max(max)
                , m_min(min)
                , m_softMax(softMax)
                , m_softMin(softMin)
            {}

            MaterialPropertyValue m_max;
            MaterialPropertyValue m_min;
            MaterialPropertyValue m_softMax;
            MaterialPropertyValue m_softMin;
        };

        //! Used by material functors to dynamically control property metadata in tools.
        //! For example, show/hide a property based on some other 'enable' flag property.
        struct MaterialPropertyDynamicMetadata
        {
            AZ_TYPE_INFO(MaterialPropertyDynamicMetadata, "{A89F215F-3235-499F-896C-9E63ACC1D657}");

            AZ::RPI::MaterialPropertyVisibility m_visibility;
            AZStd::string m_description;
            AZ::RPI::MaterialPropertyRange m_propertyRange;
        };
        
        //! Visibility for each material property group.
        enum class MaterialPropertyGroupVisibility : uint32_t
        {
            // Note it's helpful to keep these values aligned with MaterialPropertyVisibility in part because in lua it would be easy to accidentally use
            // MaterialPropertyVisibility instead of MaterialPropertyGroupVisibility resulting in sneaky bugs. Also, if the enums end up being the same in
            // the future, we could just merge them into one.

            Enabled,    //!< The property is visible and editable
            //Disabled, //!< The property is visible but non-editable (reserved for possible future use, to match MaterialPropertyVisibility)
            Hidden=2,   //!< The property is invisible

            Default = Enabled
        };

        //! Used by material functors to dynamically control property group metadata in tools.
        //! For example, show/hide an entire property group based on some 'enable' flag property.
        struct MaterialPropertyGroupDynamicMetadata
        {
            AZ_TYPE_INFO(MaterialPropertyGroupDynamicMetadata, "{F94009F7-48A3-4CE0-AF64-D5A86890ACD4}");

            AZ::RPI::MaterialPropertyGroupVisibility m_visibility;
        };

        ATOM_RPI_REFLECT_API void ReflectMaterialDynamicMetadata(ReflectContext* context);

    } // namespace RPI

    AZ_TYPE_INFO_SPECIALIZE(RPI::MaterialPropertyVisibility, "{318B43A2-79E3-4502-8FD0-5815209EA123}");
    AZ_TYPE_INFO_SPECIALIZE(RPI::MaterialPropertyGroupVisibility, "{B803958B-DE64-4FBF-AC00-CF781611BE37}");
} // namespace AZ

