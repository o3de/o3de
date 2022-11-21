/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <Tests/Serialization/Json/TestCases_Base.h>

namespace JsonSerializationTests
{
    struct SimpleNullPointer
    {
        AZ_RTTI(SimpleNullPointer, "{81087FE6-2C52-4FD4-8706-D1F4CB757937}");

        static const bool SupportsPartialDefaults = true;
        static const bool PartialDefaultReportingIsStrict = true;

        SimpleNullPointer() = default;
        virtual ~SimpleNullPointer();
        SimpleNullPointer(const SimpleNullPointer& rhs);
        SimpleNullPointer(SimpleNullPointer&& rhs);
        SimpleNullPointer& operator=(const SimpleNullPointer& rhs);
        SimpleNullPointer& operator=(SimpleNullPointer&& rhs);

        bool Equals(const SimpleNullPointer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<SimpleNullPointer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<SimpleNullPointer> GetInstanceWithoutDefaults();
        
        int* m_pointer1{ nullptr };
        int* m_pointer2{ nullptr };
    };

    struct SimpleAssignedPointer
    {
        AZ_RTTI(SimpleAssignedPointer, "{33EB4300-88C7-4006-8650-9C3AA25F17E5}");

        static const bool SupportsPartialDefaults = true;
        static const bool PartialDefaultReportingIsStrict = true;

        SimpleAssignedPointer();
        SimpleAssignedPointer(const SimpleAssignedPointer& rhs);
        SimpleAssignedPointer(SimpleAssignedPointer&& rhs);
        virtual ~SimpleAssignedPointer();

        SimpleAssignedPointer& operator=(const SimpleAssignedPointer& rhs);
        SimpleAssignedPointer& operator=(SimpleAssignedPointer&& rhs);
        
        bool Equals(const SimpleAssignedPointer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<SimpleAssignedPointer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<SimpleAssignedPointer> GetInstanceWithoutDefaults();

        int* m_pointer1{ nullptr };
        int* m_pointer2{ nullptr };
    };

    struct ComplexAssignedPointer
    {
        AZ_RTTI(ComplexAssignedPointer, "{CB62638C-5B49-4953-89E5-F65A19E1163C}");

        static const bool SupportsPartialDefaults = true;
        static const bool PartialDefaultReportingIsStrict = true;

        ComplexAssignedPointer();
        ComplexAssignedPointer(const ComplexAssignedPointer& rhs);
        ComplexAssignedPointer(ComplexAssignedPointer&& rhs);
        virtual ~ComplexAssignedPointer();

        ComplexAssignedPointer& operator=(const ComplexAssignedPointer& rhs);
        ComplexAssignedPointer& operator=(ComplexAssignedPointer&& rhs);
        
        bool Equals(const ComplexAssignedPointer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<ComplexAssignedPointer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<ComplexAssignedPointer> GetInstanceWithoutDefaults();

        struct SimpleClass* m_pointer{ nullptr };
    };

    struct ComplexNullInheritedPointer
    {
        AZ_RTTI(ComplexNullInheritedPointer, "{F21CFB49-3F7C-4849-8E1F-EA337CD7D8EF}");

        static const bool SupportsPartialDefaults = true;
        static const bool PartialDefaultReportingIsStrict = true;

        ComplexNullInheritedPointer() = default;
        virtual ~ComplexNullInheritedPointer();
        ComplexNullInheritedPointer(const ComplexNullInheritedPointer& rhs);
        ComplexNullInheritedPointer(ComplexNullInheritedPointer&& rhs);
        ComplexNullInheritedPointer& operator=(const ComplexNullInheritedPointer& rhs);
        ComplexNullInheritedPointer& operator=(ComplexNullInheritedPointer&& rhs);

        bool Equals(const ComplexNullInheritedPointer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<ComplexNullInheritedPointer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<ComplexNullInheritedPointer> GetInstanceWithoutDefaults();
        
        BaseClass* m_pointer{ nullptr }; // only used as SimpleInheritence in tests
    };

    struct ComplexAssignedDifferentInheritedPointer
    {
        AZ_RTTI(ComplexAssignedDifferentInheritedPointer, "{DCFFEDFB-300B-40FE-A787-159D627DD7DA}");

        static const bool SupportsPartialDefaults = true;
        static const bool PartialDefaultReportingIsStrict = true;

        ComplexAssignedDifferentInheritedPointer();
        ComplexAssignedDifferentInheritedPointer(const ComplexAssignedDifferentInheritedPointer& rhs);
        ComplexAssignedDifferentInheritedPointer(ComplexAssignedDifferentInheritedPointer&& rhs);
        virtual ~ComplexAssignedDifferentInheritedPointer();

        ComplexAssignedDifferentInheritedPointer& operator=(const ComplexAssignedDifferentInheritedPointer& rhs);
        ComplexAssignedDifferentInheritedPointer& operator=(ComplexAssignedDifferentInheritedPointer&& rhs);
        
        bool Equals(const ComplexAssignedDifferentInheritedPointer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<ComplexAssignedDifferentInheritedPointer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<ComplexAssignedDifferentInheritedPointer> GetInstanceWithoutDefaults();
        
        BaseClass* m_pointer{ nullptr }; // used as multiple types in tests
    };

    struct ComplexAssignedSameInheritedPointer
    {
        AZ_RTTI(ComplexAssignedSameInheritedPointer, "{6D633C34-584C-42CD-BF6E-4FCB0C398F34}");

        static const bool SupportsPartialDefaults = true;
        static const bool PartialDefaultReportingIsStrict = true;

        ComplexAssignedSameInheritedPointer();
        ComplexAssignedSameInheritedPointer(const ComplexAssignedSameInheritedPointer& rhs);
        ComplexAssignedSameInheritedPointer(ComplexAssignedSameInheritedPointer&& rhs);
        virtual ~ComplexAssignedSameInheritedPointer();

        ComplexAssignedSameInheritedPointer& operator=(const ComplexAssignedSameInheritedPointer& rhs);
        ComplexAssignedSameInheritedPointer& operator=(ComplexAssignedSameInheritedPointer&& rhs);
        
        bool Equals(const ComplexAssignedSameInheritedPointer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<ComplexAssignedSameInheritedPointer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<ComplexAssignedSameInheritedPointer> GetInstanceWithoutDefaults();
        
        BaseClass* m_pointer{ nullptr }; // only used as SimpleInheritence in tests
    };

    struct PrimitivePointerInContainer
    {
        AZ_RTTI(PrimitivePointerInContainer, "{8CB57BE0-371B-4712-88FD-64CA47AE81B4}");

        static const bool SupportsPartialDefaults = true;
        static const bool PartialDefaultReportingIsStrict = false;

        AZStd::vector<int*> m_array;

        PrimitivePointerInContainer() = default;
        PrimitivePointerInContainer(const PrimitivePointerInContainer& rhs);
        PrimitivePointerInContainer(PrimitivePointerInContainer&& rhs) = default;
        virtual ~PrimitivePointerInContainer();

        PrimitivePointerInContainer& operator=(const PrimitivePointerInContainer& rhs);        
        PrimitivePointerInContainer& operator=(PrimitivePointerInContainer&& rhs) = default;

        bool Equals(const PrimitivePointerInContainer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<PrimitivePointerInContainer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<PrimitivePointerInContainer> GetInstanceWithoutDefaults();
    };

    struct SimplePointerInContainer
    {
        AZ_RTTI(SimplePointerInContainer, "{46263461-112C-4E12-A056-63148C085B35}");

        static const bool SupportsPartialDefaults = true;
        static const bool PartialDefaultReportingIsStrict = true;

        SimplePointerInContainer() = default;
        SimplePointerInContainer(const SimplePointerInContainer& rhs);
        SimplePointerInContainer(SimplePointerInContainer&& rhs) = default;
        virtual ~SimplePointerInContainer();

        SimplePointerInContainer& operator=(const SimplePointerInContainer& rhs);        
        SimplePointerInContainer& operator=(SimplePointerInContainer&& rhs) = default;

        bool Equals(const SimplePointerInContainer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<SimplePointerInContainer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<SimplePointerInContainer> GetInstanceWithoutDefaults();

        AZStd::vector<SimpleClass*> m_array;
    };

    struct InheritedPointerInContainer
    {
        AZ_RTTI(InheritedPointerInContainer, "{ADADFF87-B541-4BD6-A3DB-B54A1277F2A7}");

        static const bool SupportsPartialDefaults = true;
        static const bool PartialDefaultReportingIsStrict = true;

        InheritedPointerInContainer() = default;
        InheritedPointerInContainer(const InheritedPointerInContainer& rhs);
        InheritedPointerInContainer(InheritedPointerInContainer&& rhs) = default;
        virtual ~InheritedPointerInContainer();

        InheritedPointerInContainer& operator=(const InheritedPointerInContainer& rhs);
        InheritedPointerInContainer& operator=(InheritedPointerInContainer&& rhs) = default;
        
        bool Equals(const InheritedPointerInContainer& rhs, bool fullReflection) const;
        static void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context, bool fullReflection);
        static InstanceWithSomeDefaults<InheritedPointerInContainer> GetInstanceWithSomeDefaults();
        static InstanceWithoutDefaults<InheritedPointerInContainer> GetInstanceWithoutDefaults();

        AZStd::vector<BaseClass*> m_array; 
    };
} // namespace JsonSerializationTests
