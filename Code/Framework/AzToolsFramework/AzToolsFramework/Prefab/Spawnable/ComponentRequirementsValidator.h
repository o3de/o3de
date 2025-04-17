/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    class ComponentRequirementsValidator
    {
    public:
        AZ_CLASS_ALLOCATOR(ComponentRequirementsValidator, AZ::SystemAllocator);
        AZ_RTTI(AzToolsFramework::Prefab::PrefabConversionUtils::ComponentRequirementsValidator, "{1E9CD55D-FFEA-4E71-A316-731E25E6C981}");

        virtual ~ComponentRequirementsValidator() = default;

        void SetPlatformTags(AZ::PlatformTagSet platformTags);
        void SetEntities(const AZStd::vector<AZ::Entity*>& entities);

        using ValidationResult = AZ::Outcome<void, AZStd::string>;
        ValidationResult Validate(const AZ::Component* component);

    private:
        AZ::ImmutableEntityVector m_immutableEntities;
        AZ::PlatformTagSet m_platformTags;

    };
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
