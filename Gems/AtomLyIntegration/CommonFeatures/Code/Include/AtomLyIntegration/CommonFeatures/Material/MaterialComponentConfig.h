/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Material/Material.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialAssignment.h>
#include <AzCore/Component/Component.h>

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
            AZ_CLASS_ALLOCATOR(MaterialComponentConfig, SystemAllocator);

            static void Reflect(ReflectContext* context);

            MaterialAssignmentMap m_materials;

            //! Pending material overrides keyed by slot display label. Resolved to entries in m_materials
            //! once the associated model is ready and labels are queryable. Survives mesh swaps so the
            //! same authored override re-binds against a new model that exposes a slot with the same label.
            //! Use case: prefab authoring pipelines that do not have access to SceneAPI MaterialUid
            //! stable IDs at generation time can write by-label entries and let the runtime bind them.
            AZStd::unordered_map<AZStd::string, MaterialAssignment> m_materialsByLabel;
        };
    } // namespace Render
} // namespace AZ
