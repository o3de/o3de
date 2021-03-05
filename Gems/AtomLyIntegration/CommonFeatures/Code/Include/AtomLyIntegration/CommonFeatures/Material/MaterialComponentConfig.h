/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
