/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Name/NameDictionary.h>

#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>

namespace AZ::Internal
{
    AttributeDeleter::AttributeDeleter() = default;
    AttributeDeleter::AttributeDeleter(bool deletePtr)
        : m_deletePtr(deletePtr)
    {}

    void AttributeDeleter::operator()(AZ::Attribute* attribute)
    {
        if (m_deletePtr)
        {
            delete attribute;
        }
    }
}

namespace AZ
{
    // Add implementations of TypeInfo and rtti functions for ReflectContext
    // Attribute
    AZ_TYPE_INFO_WITH_NAME_IMPL(ReflectContext, "ReflectContext", "{B18D903B-7FAD-4A53-918A-3967B3198224}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(ReflectContext);

    AZ_TYPE_INFO_WITH_NAME_IMPL(Attribute, "Attribute", "{2C656E00-12B0-476E-9225-5835B92209CC}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(Attribute);

    const AZ::Name Attribute::s_typeField = AZ::Name::FromStringLiteral("$type", AZ::Interface<AZ::NameDictionary>::Get());
    const AZ::Name Attribute::s_instanceField = AZ::Name::FromStringLiteral("instance", AZ::Interface<AZ::NameDictionary>::Get());
    const AZ::Name Attribute::s_attributeField = AZ::Name::FromStringLiteral("attribute", AZ::Interface<AZ::NameDictionary>::Get());

    //=========================================================================
    // OnDemandReflectionOwner
    //=========================================================================
    OnDemandReflectionOwner::OnDemandReflectionOwner(ReflectContext& context)
        : m_reflectContext(context)
    { }

    //=========================================================================
    // ~OnDemandReflectionOwner
    //=========================================================================
    OnDemandReflectionOwner::~OnDemandReflectionOwner()
    {
        m_reflectFunctions.clear();
        m_reflectFunctions.shrink_to_fit();
    }

    //=========================================================================
    // AddReflectFunction
    //=========================================================================
    void OnDemandReflectionOwner::AddReflectFunction(AZ::Uuid typeId, Internal::ReflectionFunctionRef reflectFunction)
    {
        auto& currentTypes = m_reflectContext.m_currentlyProcessingTypeIds;
        // If we're in process of reflecting this type already, don't store references to it
        if (AZStd::find(currentTypes.begin(), currentTypes.end(), typeId) != currentTypes.end())
        {
            return;
        }

        auto reflectionIt = m_reflectContext.m_onDemandReflection.find(typeId);
        if (reflectionIt == m_reflectContext.m_onDemandReflection.end())
        {
            // Capture for lambda (in case this is gone when unreflecting)
            AZ::ReflectContext* reflectContext = &m_reflectContext;
            // If it's not already reflected, add it to the list, and capture a reference to it
            AZStd::shared_ptr<Internal::ReflectionFunctionRef> reflectionPtr(reflectFunction, [reflectContext, typeId](Internal::ReflectionFunctionRef unreflectFunction)
            {
                bool isRemovingReflection = reflectContext->IsRemovingReflection();

                // Call the function in RemoveReflection mode
                reflectContext->EnableRemoveReflection();
                unreflectFunction(reflectContext);
                // Reset RemoveReflection mode
                reflectContext->m_isRemoveReflection = isRemovingReflection;

                // Remove the function from the central store (otherwise it stores an empty weak_ptr)
                reflectContext->m_onDemandReflection.erase(typeId);
            });

            // Capture reference to the reflection function
            m_reflectFunctions.emplace_back(reflectionPtr);

            // Tell the manager about the function
            m_reflectContext.m_onDemandReflection.emplace(typeId, reflectionPtr);
            m_reflectContext.m_toProcessOnDemandReflection.emplace_back(typeId, AZStd::move(reflectFunction));
        }
        else
        {
            // If it is already reflected, just capture a reference to it
            auto reflectionPtr = reflectionIt->second.lock();
            AZ_Assert(reflectionPtr, "OnDemandReflection for typed %s is missing, despite being present in the reflect context", typeId.ToString<AZStd::string>().c_str());
            if (reflectionPtr)
            {
                m_reflectFunctions.emplace_back(AZStd::move(reflectionPtr));
            }
        }
    }

    //=========================================================================
    // ReflectContext
    //=========================================================================
    ReflectContext::ReflectContext()
        : m_isRemoveReflection(false)
    { }

    //=========================================================================
    // EnableRemoveReflection
    //=========================================================================
    void ReflectContext::EnableRemoveReflection()
    {
        m_isRemoveReflection = true;
    }

    //=========================================================================
    // DisableRemoveReflection
    //=========================================================================
    void ReflectContext::DisableRemoveReflection()
    {
        m_isRemoveReflection = false;
    }

    //=========================================================================
    // IsRemovingReflection
    //=========================================================================
    bool ReflectContext::IsRemovingReflection() const
    {
        return m_isRemoveReflection;
    }

    //=========================================================================
    // IsTypeReflected
    //=========================================================================
    bool ReflectContext::IsOnDemandTypeReflected(AZ::Uuid typeId)
    {
        return m_onDemandReflection.find(typeId) != m_onDemandReflection.end();
    }

    //=========================================================================
    // ExecuteQueuedReflections
    //=========================================================================
    void ReflectContext::ExecuteQueuedOnDemandReflections()
    {
        // Need to do move so recursive definitions don't result in reprocessing the same type
        auto toProcess = AZStd::move(m_toProcessOnDemandReflection);
        m_toProcessOnDemandReflection.clear();
        for (const auto& reflectPair : toProcess)
        {
            m_currentlyProcessingTypeIds.emplace_back(reflectPair.first);
            reflectPair.second(this);
            m_currentlyProcessingTypeIds.pop_back();
        }
    }

    template class AttributeData<Crc32>;

}
