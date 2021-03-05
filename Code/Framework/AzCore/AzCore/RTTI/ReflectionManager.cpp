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
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Component/Component.h>

namespace AZ
{
    //=========================================================================
    // ~ReflectionManager
    //=========================================================================
    ReflectionManager::~ReflectionManager()
    {
        Clear();
    }

    //=========================================================================
    // Clear
    //=========================================================================
    void ReflectionManager::Clear()
    {
        // Reset current typeid
        while (!m_contexts.empty())
        {
            RemoveReflectContext(azrtti_typeid(m_contexts.back().get()));
        }

        // Clear the entry points
        m_entryPoints.clear();
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void ReflectionManager::Reflect(AZ::TypeId typeId, const ReflectionFunction& reflectEntryPoint)
    {
        // Early out if this type id is not unique
        if (m_typedEntryPoints.find(typeId) != m_typedEntryPoints.end())
        {
            return;
        }

        // Place the reflect function in the collection
        auto entryIt = m_entryPoints.emplace(m_entryPoints.end(), typeId, reflectEntryPoint);
        m_typedEntryPoints.emplace(typeId, AZStd::move(entryIt));

        // Call the new entry point with all known contexts
        for (const auto& context : m_contexts)
        {
            reflectEntryPoint(context.get());
        }
    }

    //=========================================================================
    // Unreflect
    //=========================================================================
    void ReflectionManager::Unreflect(AZ::TypeId typeId)
    {
        auto entryIt = m_typedEntryPoints.find(typeId);
        if (entryIt != m_typedEntryPoints.end())
        {
            // Call unreflect on everything in reverse context order
            for (auto contextIt = m_contexts.rbegin(); contextIt != m_contexts.rend(); ++contextIt)
            {
                (*contextIt)->EnableRemoveReflection();
                (*entryIt->second)(contextIt->get());
                (*contextIt)->DisableRemoveReflection();
            }

            // Remove and destroy entry point
            m_entryPoints.erase(entryIt->second);
            m_typedEntryPoints.erase(entryIt);
        }
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void ReflectionManager::Reflect(StaticReflectionFunctionPtr reflectEntryPoint)
    {
        // Early out if this entry point is not unique
        if (m_nonTypedEntryPoints.find(reflectEntryPoint) != m_nonTypedEntryPoints.end())
        {
            return;
        }

        // Place the reflect function in the collection
        auto entryIt = m_entryPoints.emplace(m_entryPoints.end(), reflectEntryPoint);
        m_nonTypedEntryPoints.emplace(reflectEntryPoint, AZStd::move(entryIt));

        // Call the new entry point with all known contexts
        for (const auto& contextIt : m_contexts)
        {
            reflectEntryPoint(contextIt.get());
        }
    }

    //=========================================================================
    // Unreflect
    //=========================================================================
    void ReflectionManager::Unreflect(StaticReflectionFunctionPtr reflectEntryPoint)
    {
        auto entryIt = m_nonTypedEntryPoints.find(reflectEntryPoint);
        if (entryIt != m_nonTypedEntryPoints.end())
        {
            // Call unreflect on everything (order doesn't matter, the contexts are disparate)
            for (auto contextIt = m_contexts.rbegin(); contextIt != m_contexts.rend(); ++contextIt)
            {
                (*contextIt)->EnableRemoveReflection();
                (*entryIt->second)(contextIt->get());
                (*contextIt)->DisableRemoveReflection();
            }

            // Remove and destroy entry point
            m_entryPoints.erase(entryIt->second);
            m_nonTypedEntryPoints.erase(entryIt);
        }
    }

    ReflectionManager::EntryPoint::EntryPoint(StaticReflectionFunctionPtr staticEntry)
        : m_nonTyped(staticEntry)
    { }

    ReflectionManager::EntryPoint::EntryPoint(AZ::TypeId typeId, const ReflectionFunction& entry)
        : m_typeId(typeId)
        , m_typed(entry)
    { }

    void ReflectionManager::EntryPoint::operator()(ReflectContext* context) const
    {
        if (m_nonTyped)
        {
            m_nonTyped(context);
        }
        else
        {
            m_typed(context);
        }
    }

    bool ReflectionManager::EntryPoint::operator==(const EntryPoint& other) const
    {
        return m_nonTyped == other.m_nonTyped && m_typeId == other.m_typeId;
    }

    //=========================================================================
    // AddReflectContext
    //=========================================================================
    void ReflectionManager::AddReflectContext(AZStd::unique_ptr<ReflectContext>&& context)
    {
        // Early out if the context is already registered
        if (GetReflectContext(azrtti_typeid(context.get())) != nullptr)
        {
            return;
        }

        for (const auto& entry : m_entryPoints)
        {
            entry(context.get());
        }
        m_contexts.emplace_back(AZStd::move(context));
    }

    //=========================================================================
    // GetReflectContext
    //=========================================================================
    ReflectContext* ReflectionManager::GetReflectContext(AZ::TypeId contextTypeId)
    {
        for (const auto& context : m_contexts)
        {
            if (azrtti_typeid(context.get()) == contextTypeId)
            {
                return context.get();
            }
        }

        return nullptr;
    }

    //=========================================================================
    // RemoveReflectContext
    //=========================================================================
    void ReflectionManager::RemoveReflectContext(AZ::TypeId contextTypeId)
    {
        for (auto contextIt = m_contexts.begin(); contextIt != m_contexts.end(); ++contextIt)
        {
            ReflectContext* context = contextIt->get();
            if (azrtti_typeid(context) == contextTypeId)
            {
                // Unreflect everything from the context
                context->EnableRemoveReflection();
                for (auto entryIt = m_entryPoints.rbegin(); entryIt != m_entryPoints.rend(); ++entryIt)
                {
                    (*entryIt)(context);
                }
                context->DisableRemoveReflection();

                m_contexts.erase(contextIt);
                return;
            }
        }
    }
}