/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ::Internal
{
    // Definition of internal ClassBuilderBase non-template member functions
    ClassBuilderBase::ClassBuilderBase(BehaviorContext* context, BehaviorClass* behaviorClass)
        : AZ::Internal::GenericAttributes<ClassBuilderBase>(context)
        , m_class(behaviorClass)
    {
        if (behaviorClass)
        {
            Base::m_currentAttributes = &behaviorClass->m_attributes;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    ClassBuilderBase::~ClassBuilderBase()
    {
        // process all on demand queued reflections
        Base::m_context->ExecuteQueuedOnDemandReflections();

        if (m_class && (!Base::m_context->IsRemovingReflection()))
        {
            for (const auto& method : m_class->m_methods)
            {
                m_class->PostProcessMethod(Base::m_context, *method.second);
                if (MethodReturnsAzEventByReferenceOrPointer(*method.second))
                {
                    ValidateAzEventDescription(*Base::m_context, *method.second);
                }
            }

            // Validate the AzEvent description of the class property getter's
            for (auto&& [propertyName, propertyInst] : m_class->m_properties)
            {
                if (propertyInst->m_getter && MethodReturnsAzEventByReferenceOrPointer(*propertyInst->m_getter))
                {
                    ValidateAzEventDescription(*Base::m_context, *propertyInst->m_getter);
                }
            }

            BehaviorContextBus::Event(Base::m_context, &BehaviorContextBus::Events::OnAddClass, m_class->m_name.c_str(), m_class);
        }
    }

    auto ClassBuilderBase::operator->() -> ClassBuilderBase*
    {
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    auto ClassBuilderBase::RequestBus(const char* name) -> ClassBuilderBase*
    {
        if (m_class)
        {
            m_class->m_requestBuses.insert(name);
        }
        return this;
    }


    //////////////////////////////////////////////////////////////////////////
    auto ClassBuilderBase::NotificationBus(const char* name)-> ClassBuilderBase*
    {
        if (m_class)
        {
            m_class->m_notificationBuses.insert(name);
        }
        return this;
    }

    //////////////////////////////////////////////////////////////////////////
    auto ClassBuilderBase::Allocator(BehaviorClass::AllocateType allocate, BehaviorClass::DeallocateType deallocate) -> ClassBuilderBase*
    {
        AZ_Error("BehaviorContext", m_class, "Allocator can be set on valid classes only!");
        if (m_class)
        {
            m_class->m_allocate = allocate;
            m_class->m_deallocate = deallocate;
        }
        return this;
    }

    auto ClassBuilderBase::UserData(void* userData) -> ClassBuilderBase*
    {
        AZ_Error("BehaviorContext", m_class, "UserData can be set on valid classes only!");
        if (m_class)
        {
            m_class->m_userData = userData;
        }
        return this;
    }
} // namespace AZ
