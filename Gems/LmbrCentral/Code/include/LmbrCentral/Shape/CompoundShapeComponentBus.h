/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace LmbrCentral
{
    /**
    * Type ID for the EditorCompoundShapeComponent
    */
    inline constexpr AZ::TypeId EditorCompoundShapeComponentTypeId{ "{837AA0DF-9C14-4311-8410-E7983E1F4B8D}" };

    /// Configuration data for CompoundShapeConfiguration
    class CompoundShapeConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CompoundShapeConfiguration, AZ::SystemAllocator)
        AZ_RTTI(CompoundShapeConfiguration, "{4CEB4E5C-4CBD-4A84-88BA-87B23C103F3F}")

        static void Reflect(AZ::ReflectContext* context);

        virtual ~CompoundShapeConfiguration() = default;

        const AZStd::list<AZ::EntityId>& GetChildEntities() const
        {
            return m_childEntities;
        }

    private:
        AZStd::list<AZ::EntityId> m_childEntities;
    };

    /// Services provided by the Compound Shape Component
    class CompoundShapeComponentRequests : public AZ::ComponentBus
    {
    public:
        virtual const CompoundShapeConfiguration& GetCompoundShapeConfiguration() const = 0;
    };

    // Bus to service the Compound Shape component event group
    using CompoundShapeComponentRequestsBus = AZ::EBus<CompoundShapeComponentRequests>;

    /// Services provided by the Compound Shape Component hierarchy tests
    class CompoundShapeComponentHierarchyRequests : public AZ::ComponentBus
    {
    public:
        // This method returns whether or not any entity referenced in the shape component (traversing the enitre reference tree through compound shape components) has a reference to the passed
        // in entity id.  This is needed to detect circular references
        virtual bool HasChildId(const AZ::EntityId& /*entityId*/) const { return false; }

        virtual bool ValidateChildIds() { return true; }
    };

    // Bus to service the Compound Shape component hierarchy tests
    using CompoundShapeComponentHierarchyRequestsBus = AZ::EBus<CompoundShapeComponentHierarchyRequests>;

} // namespace LmbrCentral
