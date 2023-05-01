/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/ReflectContext.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/typetraits/is_same.h>

namespace AZ
{
    /**
     * Class that manages all ReflectContexts and all reflection entry point functions
     */
    class ReflectionManager
    {
    private:
        // Reusable check for whether or not a type is a reflect context
        template <typename ReflectContextT>
        using IsReflectContextT = AZStd::enable_if_t<AZStd::is_base_of<ReflectContext, ReflectContextT>::value>;

    public:
        AZ_CLASS_ALLOCATOR(ReflectionManager, SystemAllocator);

        ReflectionManager() = default;
        ~ReflectionManager();

        /// Unreflect all entry points and contexts
        void Clear();

        /// Add a reflect function with an associated typeid
        void Reflect(AZ::TypeId typeId, const ReflectionFunction& reflectEntryPoint);
        /// Call unreflect on the entry point associated with the typeId
        void Unreflect(AZ::TypeId typeId);

        /// Add a static reflect function
        void Reflect(StaticReflectionFunctionPtr reflectEntryPoint);
        /// Unreflect a static reflect function
        void Unreflect(StaticReflectionFunctionPtr reflectEntryPoint);

        /// Creates a reflect context, and reflects all registered entry points
        template <typename ReflectContextT, typename = IsReflectContextT<ReflectContextT>>
        void AddReflectContext() { AddReflectContext(AZStd::make_unique<ReflectContextT>()); }

        /// Gets a reflect context of the requested type
        template <typename ReflectContextT, typename = IsReflectContextT<ReflectContextT>>
        ReflectContextT* GetReflectContext() { return azrtti_cast<ReflectContextT*>(GetReflectContext(azrtti_typeid<ReflectContextT>())); }

        /// Destroy a reflect context (unreflects all entry points)
        template <typename ReflectContextT, typename = IsReflectContextT<ReflectContextT>>
        void RemoveReflectContext() { RemoveReflectContext(azrtti_typeid<ReflectContextT>()); }

    protected:
        struct EntryPoint
        {
            // Only m_nonTyped OR m_typeId and m_entry will be set
            StaticReflectionFunctionPtr m_nonTyped = nullptr;
            AZ::TypeId m_typeId = AZ::TypeId::CreateNull();
            ReflectionFunction m_typed;

            EntryPoint(StaticReflectionFunctionPtr staticEntry);
            EntryPoint(AZ::TypeId typeId, const ReflectionFunction& entry);

            void operator()(ReflectContext* context) const;
            bool operator==(const EntryPoint& other) const;
        };

        AZStd::vector<AZStd::unique_ptr<ReflectContext>> m_contexts;

        using EntryPointList = AZStd::list<EntryPoint>;
        EntryPointList m_entryPoints;
        // The following maps are used to track reflected entry points in a collection that may be accessed in constant time
        AZStd::unordered_map<TypeId, EntryPointList::iterator> m_typedEntryPoints;
        AZStd::unordered_map<StaticReflectionFunctionPtr, EntryPointList::iterator> m_nonTypedEntryPoints;

        void AddReflectContext(AZStd::unique_ptr<ReflectContext>&& context);
        ReflectContext* GetReflectContext(AZ::TypeId contextTypeId);
        void RemoveReflectContext(AZ::TypeId contextTypeId);
    };
}
