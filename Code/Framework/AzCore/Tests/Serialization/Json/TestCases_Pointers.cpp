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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>
#include <Tests/Serialization/Json/TestCases_Pointers.h>

namespace JsonSerializationTests
{
    // SimpleNullPointer

    SimpleNullPointer::~SimpleNullPointer()
    {
        azfree(m_pointer2);
        azfree(m_pointer1);
    }

    bool SimpleNullPointer::Equals(const SimpleNullPointer& rhs, bool /*fullReflection*/) const
    {
        if (m_pointer1)
        {
            if (!rhs.m_pointer1)
            {
                return false;
            }
            if (*m_pointer1 != *rhs.m_pointer1)
            {
                return false;
            }
        }
        else
        {
            if (rhs.m_pointer1)
            {
                return false;
            }
        }

        if (m_pointer2)
        {
            if (!rhs.m_pointer2)
            {
                return false;
            }
            if (*m_pointer2 != *rhs.m_pointer2)
            {
                return false;
            }
        }
        else
        {
            if (rhs.m_pointer2)
            {
                return false;
            }
        }

        return true;
    }

    void SimpleNullPointer::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        if (fullReflection)
        {
            context->Class<SimpleNullPointer>()
                ->Field("pointer1", &SimpleNullPointer::m_pointer1)
                ->Field("pointer2", &SimpleNullPointer::m_pointer2);
        }
    }

    InstanceWithSomeDefaults<SimpleNullPointer> SimpleNullPointer::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<SimpleNullPointer>();
        instance->m_pointer2 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        *instance->m_pointer2 = 88;

        const char* strippedDefaults = R"(
            {
                "pointer2": 88
            })";
        const char* keptDefaults = R"(
            {
                "pointer1": null,
                "pointer2": 88
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<SimpleNullPointer> SimpleNullPointer::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<SimpleNullPointer>();
        instance->m_pointer1 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        *instance->m_pointer1 = 42;
        instance->m_pointer2 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        *instance->m_pointer2 = 88;

        const char* json = R"(
            {
                "pointer1": 42,
                "pointer2": 88
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // SimpleAssignedPointer

    SimpleAssignedPointer::SimpleAssignedPointer()
    {
        m_pointer1 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        m_pointer2 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        *m_pointer1 = 13;
        *m_pointer2 = 42;
    }
    
    SimpleAssignedPointer::SimpleAssignedPointer(const SimpleAssignedPointer& rhs)
    {
        m_pointer1 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        m_pointer2 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        *m_pointer1 = *rhs.m_pointer1;
        *m_pointer2 = *rhs.m_pointer2;
    }

    SimpleAssignedPointer::SimpleAssignedPointer(SimpleAssignedPointer&& rhs)
    {
        m_pointer1 = rhs.m_pointer1;
        m_pointer2 = rhs.m_pointer2;
        rhs.m_pointer1 = nullptr;
        rhs.m_pointer2 = nullptr;
    }

    SimpleAssignedPointer::~SimpleAssignedPointer()
    {
        azfree(m_pointer2);
        azfree(m_pointer1);
    }

    SimpleAssignedPointer& SimpleAssignedPointer::operator=(const SimpleAssignedPointer& rhs)
    {
        m_pointer1 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        m_pointer2 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        *m_pointer1 = *rhs.m_pointer1;
        *m_pointer2 = *rhs.m_pointer2;

        return *this;
    }

    SimpleAssignedPointer& SimpleAssignedPointer::operator=(SimpleAssignedPointer&& rhs)
    {
        azfree(m_pointer2);
        azfree(m_pointer1);
        
        m_pointer1 = rhs.m_pointer1;
        m_pointer2 = rhs.m_pointer2;
        rhs.m_pointer1 = nullptr;
        rhs.m_pointer2 = nullptr;
        
        return *this;
    }

    bool SimpleAssignedPointer::Equals(const SimpleAssignedPointer& rhs, bool /*fullReflection*/) const
    {
        AZ_Assert(m_pointer1, "Unexpected nullptr");
        AZ_Assert(m_pointer2, "Unexpected nullptr");
        AZ_Assert(rhs.m_pointer1, "Unexpected nullptr");
        AZ_Assert(rhs.m_pointer2, "Unexpected nullptr");

        return
            *m_pointer1 == *rhs.m_pointer1 &&
            *m_pointer2 == *rhs.m_pointer2;
    }
    
    void SimpleAssignedPointer::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        if (fullReflection)
        {
            context->Class<SimpleAssignedPointer>()
                ->Field("pointer1", &SimpleAssignedPointer::m_pointer1)
                ->Field("pointer2", &SimpleAssignedPointer::m_pointer2);
        }
    }

    InstanceWithSomeDefaults<SimpleAssignedPointer> SimpleAssignedPointer::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<SimpleAssignedPointer>();
        *instance->m_pointer2 = 88;

        const char* strippedDefaults = R"(
            {
                "pointer2": 88
            })";
        const char* keptDefaults = R"(
            {
                "pointer1": 13,
                "pointer2": 88
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<SimpleAssignedPointer> SimpleAssignedPointer::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<SimpleAssignedPointer>();
        *instance->m_pointer1 = 42;
        *instance->m_pointer2 = 88;

        const char* json = R"(
            {
                "pointer1": 42,
                "pointer2": 88
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // ComplexAssignedPointer

    ComplexAssignedPointer::ComplexAssignedPointer()
        : m_pointer(aznew SimpleClass())
    {
    }
    
    ComplexAssignedPointer::ComplexAssignedPointer(const ComplexAssignedPointer& rhs)
        : m_pointer(aznew SimpleClass(*rhs.m_pointer))
    {
    }

    ComplexAssignedPointer::ComplexAssignedPointer(ComplexAssignedPointer&& rhs)
        : m_pointer(rhs.m_pointer)
    {
        rhs.m_pointer = nullptr;
    }

    ComplexAssignedPointer::~ComplexAssignedPointer()
    {
        delete m_pointer;
    }
    
    ComplexAssignedPointer& ComplexAssignedPointer::operator=(const ComplexAssignedPointer& rhs)
    {
        m_pointer = aznew SimpleClass(*rhs.m_pointer);
        return *this;
    }

    ComplexAssignedPointer& ComplexAssignedPointer::operator=(ComplexAssignedPointer&& rhs)
    {
        m_pointer = rhs.m_pointer;
        rhs.m_pointer = nullptr;
        return *this;
    }

    bool ComplexAssignedPointer::Equals(const ComplexAssignedPointer& rhs, bool fullReflection) const
    {
        AZ_Assert(m_pointer, "Pointer wasn't assigned.");
        AZ_Assert(rhs.m_pointer, "Can't compare to pointer that wasn't assigned.");
        return m_pointer->Equals(*rhs.m_pointer, fullReflection);
    }

    void ComplexAssignedPointer::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        SimpleClass::Reflect(context, fullReflection);
        context->Class<ComplexAssignedPointer>()
            ->Field("pointer", &ComplexAssignedPointer::m_pointer);
    }

    InstanceWithSomeDefaults<ComplexAssignedPointer> ComplexAssignedPointer::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<ComplexAssignedPointer>();
        instance->m_pointer->m_var2 = 88.0f;

        const char* strippedDefaults = R"(
            {
                "pointer": 
                {
                    "var2": 88.0
                }
            })";
        const char* keptDefaults = R"(
            {
                "pointer": 
                {
                    "var1": 42,
                    "var2": 88.0
                }
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<ComplexAssignedPointer> ComplexAssignedPointer::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<ComplexAssignedPointer>();
        instance->m_pointer->m_var1 = 88;
        instance->m_pointer->m_var2 = 88.0f;

        const char* json = R"(
            {
                "pointer": 
                {
                    "var1": 88,
                    "var2": 88.0
                }
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // ComplexNullInheritedPointer

    ComplexNullInheritedPointer::~ComplexNullInheritedPointer()
    {
        delete m_pointer;
    }

    bool ComplexNullInheritedPointer::Equals(const ComplexNullInheritedPointer& rhs, bool fullReflection) const
    {
        if (!m_pointer)
        {
            return rhs.m_pointer == nullptr;
        }

        if (!rhs.m_pointer)
        {
            return false;
        }

        if (azrtti_typeid(m_pointer) != azrtti_typeid(rhs.m_pointer))
        {
            return false;
        }

        return static_cast<SimpleInheritence*>(m_pointer)->Equals(
            *static_cast<SimpleInheritence*>(rhs.m_pointer), fullReflection);
    }

    void ComplexNullInheritedPointer::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        SimpleInheritence::Reflect(context, fullReflection);
        context->Class<ComplexNullInheritedPointer>()
            ->Field("pointer", &ComplexNullInheritedPointer::m_pointer);
    }

    InstanceWithSomeDefaults<ComplexNullInheritedPointer> ComplexNullInheritedPointer::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<ComplexNullInheritedPointer>();
        SimpleInheritence* member = aznew SimpleInheritence();
        member->m_var2 = 88.0f;
        instance->m_pointer = member;

        const char* strippedDefaults = R"(
            {
                "pointer": 
                {
                    "$type": "SimpleInheritence",
                    "var2": 88.0
                }
            })";
        const char* keptDefaults = R"(
            {
                "pointer": 
                {
                    "$type": "SimpleInheritence",
                    "base_var": -42.0,
                    "var1": 42,
                    "var2": 88.0
                }
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<ComplexNullInheritedPointer> ComplexNullInheritedPointer::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<ComplexNullInheritedPointer>();
        SimpleInheritence* member = aznew SimpleInheritence();
        member->m_var1 = 88;
        member->m_var2 = 88.0f;
        member->m_baseVar = -88.0f;
        instance->m_pointer = member;

        const char* json = R"(
            {
                "pointer": 
                {
                    "$type": "SimpleInheritence",
                    "base_var": -88.0,
                    "var1": 88,
                    "var2": 88.0
                }
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // ComplexAssignedDifferentInheritedPointer

    ComplexAssignedDifferentInheritedPointer::ComplexAssignedDifferentInheritedPointer()
    {
        m_pointer = aznew BaseClass();
    }

    ComplexAssignedDifferentInheritedPointer::ComplexAssignedDifferentInheritedPointer(
        const ComplexAssignedDifferentInheritedPointer& rhs)
        : m_pointer(aznew BaseClass(*rhs.m_pointer))
    {
    }

    ComplexAssignedDifferentInheritedPointer::ComplexAssignedDifferentInheritedPointer(
        ComplexAssignedDifferentInheritedPointer&& rhs)
        : m_pointer(rhs.m_pointer)
    {
        rhs.m_pointer = nullptr;
    }

    ComplexAssignedDifferentInheritedPointer::~ComplexAssignedDifferentInheritedPointer()
    {
        delete m_pointer;
    }

    ComplexAssignedDifferentInheritedPointer& ComplexAssignedDifferentInheritedPointer::operator=(
        const ComplexAssignedDifferentInheritedPointer& rhs)
    {
        m_pointer = aznew BaseClass(*rhs.m_pointer);
        return *this;
    }

    ComplexAssignedDifferentInheritedPointer& ComplexAssignedDifferentInheritedPointer::operator=(
        ComplexAssignedDifferentInheritedPointer&& rhs)
    {
        m_pointer = rhs.m_pointer;
        rhs.m_pointer = nullptr;
        return *this;
    }

    bool ComplexAssignedDifferentInheritedPointer::Equals(
        const ComplexAssignedDifferentInheritedPointer& rhs, bool fullReflection) const
    {
        if (azrtti_typeid(m_pointer) != azrtti_typeid(rhs.m_pointer))
        {
            return false;
        }

        if (azrtti_typeid(m_pointer) == azrtti_typeid<BaseClass>())
        {
            return m_pointer->Equals(*rhs.m_pointer, fullReflection);
        }
        else if (azrtti_typeid(m_pointer) == azrtti_typeid<SimpleInheritence>())
        {
            return static_cast<SimpleInheritence*>(m_pointer)->Equals(
                *static_cast<SimpleInheritence*>(rhs.m_pointer), fullReflection);
        }
        else
        {
            return false;
        }
    }
    
    void ComplexAssignedDifferentInheritedPointer::Reflect(
        AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        SimpleInheritence::Reflect(context, fullReflection);
        context->Class<ComplexAssignedDifferentInheritedPointer>()
            ->Field("pointer", &ComplexAssignedDifferentInheritedPointer::m_pointer);
    }

    InstanceWithSomeDefaults<ComplexAssignedDifferentInheritedPointer> ComplexAssignedDifferentInheritedPointer::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<ComplexAssignedDifferentInheritedPointer>();
        delete instance->m_pointer;
        SimpleInheritence* member = aznew SimpleInheritence();
        member->m_var2 = 88.0f;
        instance->m_pointer = member;

        const char* strippedDefaults = R"(
            {
                "pointer": 
                {
                    "$type": "SimpleInheritence",
                    "var2": 88.0
                }
            })";
        const char* keptDefaults = R"(
            {
                "pointer": 
                {
                    "$type": "SimpleInheritence",
                    "base_var": -42.0,                
                    "var1": 42,
                    "var2": 88.0
                }
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<ComplexAssignedDifferentInheritedPointer> ComplexAssignedDifferentInheritedPointer::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<ComplexAssignedDifferentInheritedPointer>();
        delete instance->m_pointer;
        SimpleInheritence* member = aznew SimpleInheritence();
        member->m_var1 = 88;
        member->m_var2 = 88.0f;
        member->m_baseVar = -88.0f;
        instance->m_pointer = member;

        const char* json = R"(
            {
                "pointer": 
                {
                    "$type": "SimpleInheritence",
                    "base_var": -88.0,
                    "var1": 88,
                    "var2": 88.0
                }
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // ComplexAssignedSameInheritedPointer

    ComplexAssignedSameInheritedPointer::ComplexAssignedSameInheritedPointer()
    {
        SimpleInheritence* member = aznew SimpleInheritence();
        member->m_var1 = 142;
        m_pointer = member;
    }

    ComplexAssignedSameInheritedPointer::ComplexAssignedSameInheritedPointer(
        const ComplexAssignedSameInheritedPointer& rhs)
        : m_pointer(aznew SimpleInheritence(*static_cast<SimpleInheritence*>(rhs.m_pointer)))
    {
    }

    ComplexAssignedSameInheritedPointer::ComplexAssignedSameInheritedPointer(
        ComplexAssignedSameInheritedPointer&& rhs)
        : m_pointer(rhs.m_pointer)
    {
        rhs.m_pointer = nullptr;
    }

    ComplexAssignedSameInheritedPointer::~ComplexAssignedSameInheritedPointer()
    {
        delete m_pointer;
    }

    ComplexAssignedSameInheritedPointer& ComplexAssignedSameInheritedPointer::operator=(
        const ComplexAssignedSameInheritedPointer& rhs)
    {
        m_pointer = aznew SimpleInheritence(*static_cast<SimpleInheritence*>(rhs.m_pointer));
        return *this;
    }

    ComplexAssignedSameInheritedPointer& ComplexAssignedSameInheritedPointer::operator=(
        ComplexAssignedSameInheritedPointer&& rhs)
    {
        m_pointer = rhs.m_pointer;
        rhs.m_pointer = nullptr;
        return *this;
    }

    bool ComplexAssignedSameInheritedPointer::Equals(
        const ComplexAssignedSameInheritedPointer& rhs, bool fullReflection) const
    {
        if (azrtti_typeid(m_pointer) != azrtti_typeid(rhs.m_pointer))
        {
            return false;
        }

        return static_cast<SimpleInheritence*>(m_pointer)->Equals(
            *static_cast<SimpleInheritence*>(rhs.m_pointer), fullReflection);
    }

    void ComplexAssignedSameInheritedPointer::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        SimpleInheritence::Reflect(context, fullReflection);
        context->Class<ComplexAssignedSameInheritedPointer>()
            ->Field("pointer", &ComplexAssignedSameInheritedPointer::m_pointer);
    }

    InstanceWithSomeDefaults<ComplexAssignedSameInheritedPointer> ComplexAssignedSameInheritedPointer::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<ComplexAssignedSameInheritedPointer>();
        delete instance->m_pointer;
        SimpleInheritence* member = aznew SimpleInheritence();
        member->m_var1 = 142; // Matches the default value of SimpleInheritence set in the constructor of this class.
        member->m_var2 = 88.0f;
        instance->m_pointer = member;

        const char* strippedDefaults = R"(
            {
                "pointer": 
                {
                    "var2": 88.0
                }
            })";
        const char* keptDefaults = R"(
            {
                "pointer": 
                {
                    "$type": "SimpleInheritence",
                    "base_var": -42.0,
                    "var1": 142,
                    "var2": 88.0
                }
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<ComplexAssignedSameInheritedPointer> ComplexAssignedSameInheritedPointer::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<ComplexAssignedSameInheritedPointer>();
        delete instance->m_pointer;
        SimpleInheritence* member = aznew SimpleInheritence();
        member->m_var1 = 88;
        member->m_var2 = 88.0f;
        member->m_baseVar = -88.0f;
        instance->m_pointer = member;

        const char* json = R"(
            {
                "pointer": 
                {
                    "base_var": -88.0,
                    "var1": 88,
                    "var2": 88.0
                }
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // PrimitivePointerInContainer

    PrimitivePointerInContainer::PrimitivePointerInContainer(const PrimitivePointerInContainer& rhs)
    {
        for (int* instance : rhs.m_array)
        {
            int* newInstance = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
            *newInstance = *instance;
            m_array.push_back(newInstance);
        }
    }
    
    PrimitivePointerInContainer::~PrimitivePointerInContainer()
    {
        for (int* instance : m_array)
        {
            azfree(instance);
        }
        m_array.clear();
    }

    PrimitivePointerInContainer& PrimitivePointerInContainer::operator=(const PrimitivePointerInContainer& rhs)
    {
        for (int* instance : rhs.m_array)
        {
            int* newInstance = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
            *newInstance = *instance;
            m_array.push_back(newInstance);
        }
        return *this;
    }

    bool PrimitivePointerInContainer::Equals(const PrimitivePointerInContainer& rhs, bool /*fullReflection*/) const
    {
        if (m_array.size() != rhs.m_array.size())
        {
            return false;
        }

        for (size_t i = 0; i < m_array.size(); ++i)
        {
            if (!m_array[i] || !rhs.m_array[i])
            {
                return m_array[i] == rhs.m_array[i];
            }
            if (*m_array[i] != *rhs.m_array[i])
            {
                return false;
            }
        }
        return true;
    }
    
    void PrimitivePointerInContainer::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        if (fullReflection)
        {
            context->Class<PrimitivePointerInContainer>()
                ->Field("array", &PrimitivePointerInContainer::m_array);
        }
    }

    InstanceWithSomeDefaults<PrimitivePointerInContainer> PrimitivePointerInContainer::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<PrimitivePointerInContainer>();
        int* member = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        *member = 42;
        instance->m_array.push_back(member);
        instance->m_array.push_back(nullptr);

        const char* defaults = R"(
            {
                "array": [ 42, null ] 
            })";
        return MakeInstanceWithSomeDefaults(AZStd::move(instance), defaults, defaults);
    }

    InstanceWithoutDefaults<PrimitivePointerInContainer> PrimitivePointerInContainer::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<PrimitivePointerInContainer>();
        int* member0 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        int* member1 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        int* member2 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
        *member0 = 42;
        *member1 = 88;
        *member2 = 13;
        instance->m_array.push_back(member0);
        instance->m_array.push_back(member1);
        instance->m_array.push_back(member2);

        const char* json = R"(
            {
                "array": [ 42, 88, 13 ]
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // SimplePointerInContainer

    SimplePointerInContainer::SimplePointerInContainer(const SimplePointerInContainer& rhs)
    {
        for (SimpleClass* instance : rhs.m_array)
        {
            m_array.push_back(aznew SimpleClass(*instance));
        }
    }

    SimplePointerInContainer::~SimplePointerInContainer()
    {
        for (SimpleClass* instance : m_array)
        {
            delete instance;
        }
        m_array.clear();
    }

    SimplePointerInContainer& SimplePointerInContainer::operator=(const SimplePointerInContainer& rhs)
    {
        for (SimpleClass* instance : rhs.m_array)
        {
            m_array.push_back(aznew SimpleClass(*instance));
        }
        return *this;
    }

    bool SimplePointerInContainer::Equals(const SimplePointerInContainer& rhs, bool fullReflection) const
    {
        if (m_array.size() != rhs.m_array.size())
        {
            return false;
        }

        for (size_t i = 0; i < m_array.size(); ++i)
        {
            if (!m_array[i] || !rhs.m_array[i])
            {
                if (m_array[i] != rhs.m_array[i])
                {
                    return false;
                }
            }
            else if (!m_array[i]->Equals(*rhs.m_array[i], fullReflection))
            {
                return false;
            }
        }
        return true;
    }

    void SimplePointerInContainer::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        SimpleClass::Reflect(context, fullReflection);
        context->Class<SimplePointerInContainer>()
            ->Field("array", &SimplePointerInContainer::m_array);
    }

    InstanceWithSomeDefaults<SimplePointerInContainer> SimplePointerInContainer::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<SimplePointerInContainer>();
        SimpleClass* member = aznew SimpleClass();
        member->m_var2 = 88.0f;
        instance->m_array.push_back(member);
        instance->m_array.push_back(aznew SimpleClass());
        instance->m_array.push_back(nullptr);

        const char* strippedDefaults = R"(
            {
                "array": [ { "var2": 88.0 }, {}, null ] 
            })";
        const char* keptDefaults = R"(
            {
                "array":
                [
                    {
                        "var1": 42,
                        "var2": 88.0
                    },
                    {
                        "var1": 42,
                        "var2": 42.0
                    },
                    null
                ]
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<SimplePointerInContainer> SimplePointerInContainer::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<SimplePointerInContainer>();
        SimpleClass* member = aznew SimpleClass();
        member->m_var1 = 88;
        member->m_var2 = 88.0f;
        instance->m_array.push_back(member);

        const char* json = R"(
            {
                "array":
                [
                    {
                        "var1": 88,
                        "var2": 88.0
                    }
                ]
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }


    // InheritedPointerInContainer

    InheritedPointerInContainer::InheritedPointerInContainer(const InheritedPointerInContainer& rhs)
    {
        for (BaseClass* instance : rhs.m_array)
        {
            m_array.push_back(aznew SimpleInheritence(*static_cast<SimpleInheritence*>(instance)));
        }
    }

    InheritedPointerInContainer::~InheritedPointerInContainer()
    {
        for (BaseClass* instance : m_array)
        {
            delete instance;
        }
        m_array.clear();
    }

    InheritedPointerInContainer& InheritedPointerInContainer::operator=(const InheritedPointerInContainer& rhs)
    {
        for (BaseClass* instance : rhs.m_array)
        {
            m_array.push_back(aznew SimpleInheritence(*static_cast<SimpleInheritence*>(instance)));
        }
        return *this;
    }

    bool InheritedPointerInContainer::Equals(const InheritedPointerInContainer& rhs, bool fullReflection) const
    {
        if (m_array.size() != rhs.m_array.size())
        {
            return false;
        }

        for (size_t i = 0; i < m_array.size(); ++i)
        {
            if (!m_array[i] || !rhs.m_array[i])
            {
                if (m_array[i] == rhs.m_array[i])
                {
                    continue;
                }
                else
                {
                    return false;
                }
            }

            if (azrtti_typeid(m_array[i]) != azrtti_typeid(rhs.m_array[i]))
            {
                return false;
            }
            if (azrtti_typeid(m_array[i]) != azrtti_typeid<SimpleInheritence>())
            {
                return false;
            }

            if (!static_cast<SimpleInheritence*>(m_array[i])->Equals(
                *static_cast<SimpleInheritence*>(rhs.m_array[i]), fullReflection))
            {
                return false;
            }
        }
        return true;
    }

    void InheritedPointerInContainer::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection)
    {
        SimpleInheritence::Reflect(context, fullReflection);
        context->Class<InheritedPointerInContainer>()
            ->Field("array", &InheritedPointerInContainer::m_array);
    }

    InstanceWithSomeDefaults<InheritedPointerInContainer> InheritedPointerInContainer::GetInstanceWithSomeDefaults()
    {
        auto instance = AZStd::make_unique<InheritedPointerInContainer>();
        SimpleInheritence* member = aznew SimpleInheritence();
        member->m_var2 = 88.0f;
        instance->m_array.push_back(member);
        instance->m_array.push_back(aznew SimpleInheritence());
        instance->m_array.push_back(nullptr);

        const char* strippedDefaults = R"(
            {
                "array":
                [
                    {
                        "$type": "SimpleInheritence",
                        "var2": 88.0
                    },
                    {
                        "$type": "SimpleInheritence"
                    },
                    null
                ] 
            })";
        const char* keptDefaults = R"(
            {
                "array":
                [
                    {
                        "$type": "SimpleInheritence",
                        "base_var": -42.0,                
                        "var1": 42,
                        "var2": 88.0
                    },
                    {
                        "$type": "SimpleInheritence",
                        "base_var": -42.0,                
                        "var1": 42,
                        "var2": 42.0
                    },
                    null
                ]
            })";

        return MakeInstanceWithSomeDefaults(AZStd::move(instance),
            strippedDefaults, keptDefaults);
    }

    InstanceWithoutDefaults<InheritedPointerInContainer> InheritedPointerInContainer::GetInstanceWithoutDefaults()
    {
        auto instance = AZStd::make_unique<InheritedPointerInContainer>();
        SimpleInheritence* member = aznew SimpleInheritence();
        member->m_var1 = 88;
        member->m_var2 = 88.0f;
        member->m_baseVar = -88.0f;
        instance->m_array.push_back(member);

        const char* json = R"(
            {
                "array":
                [
                    {
                        "$type": "SimpleInheritence",
                        "base_var": -88.0,
                        "var1": 88,
                        "var2": 88.0
                    }
                ]                
            })";
        return MakeInstanceWithoutDefaults(AZStd::move(instance), json);
    }
 } // namespace JsonSerializationTests
