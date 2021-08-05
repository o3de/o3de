/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/Feature/Material/MaterialAssignment.h>

namespace AZ
{
    namespace Render
    {
        //! Common configuration for MaterialComponent that can be used to create
        //! MaterialComponents dynamically and can be shared with EditorMaterialComponent.
        class MaterialComponentConfig final
            : public ComponentConfig
        {
        public:
            AZ_RTTI(MaterialComponentConfig, "{3366C279-32AE-48F6-839B-7700AE117A54}", ComponentConfig);
            AZ_CLASS_ALLOCATOR(MaterialComponentConfig, SystemAllocator, 0);

            static void Reflect(ReflectContext* context);

            MaterialAssignmentMap m_materials;
        };
    } // namespace Render
} // namespace AZ
