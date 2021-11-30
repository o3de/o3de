/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectionManager.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/bitset.h>
#include <AzCore/std/containers/fixed_list.h>
#include <AzCore/std/containers/fixed_unordered_map.h>
#include <AzCore/std/containers/fixed_unordered_set.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/utils.h>

// Non intrusive typeinfo for external and intergral types
struct ExternalClass
{
};

// These 2 types must only EVER be used by the MultiThreadedTypeInfo test, or else
// that test is invalidated because the statics will have been initialized already
struct MTTI {};
struct MTTI2
{
    AZ_TYPE_INFO(MTTI2, "{CBC94693-5ECD-4CBF-A8DB-9B122E697E8D}");
};

namespace AZ
{
    enum class PlatformID;
    AZ_TYPE_INFO_SPECIALIZE(ExternalClass, "{38380915-084B-4886-8D3D-B8439E9E987C}");
    AZ_TYPE_INFO_SPECIALIZE(MTTI, "{4876C017-0C26-4D0D-9A1F-2A738BAE6449}");
}

using namespace AZ;

namespace UnitTest
{
    class Rtti
        : public AllocatorsFixture
    {
    };

    // Intrusive TypeInfo
    struct MyClass
    {
        AZ_TYPE_INFO(MyClass, "{CADA6BA7-D479-4C20-B7F0-121A1DF4E9CC}");
    };

    template<class T1, class T2>
    struct MyClassTemplate
    {
    };

    template<class... Args>
    struct MyClassVariadicTemplate
    {
    };
}

namespace AZ
{
    AZ_TYPE_INFO_TEMPLATE(UnitTest::MyClassTemplate, "{EBFE7ADF-1FCE-47F0-B417-14FE06BAF02D}", AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS);
    AZ_TYPE_INFO_TEMPLATE(UnitTest::MyClassVariadicTemplate, "{60C1D809-09FA-48EB-A9B7-0BD8DBFF21C8}", AZ_TYPE_INFO_CLASS_VARARGS);
}

namespace UnitTest
{
    // Tests if known types maintain their assigned/constructed uuids properly. Changes to this can have significant impact
    // various systems such as serialization.
    TEST_F(Rtti, KnownTypes)
    {
        EXPECT_EQ(AZ::Uuid("{3AB0037F-AF8D-48ce-BCA0-A170D18B2C03}"), azrtti_typeid<char>());
        EXPECT_EQ(AZ::Uuid("{58422C0E-1E47-4854-98E6-34098F6FE12D}"), azrtti_typeid<AZ::s8>());
        EXPECT_EQ(AZ::Uuid("{B8A56D56-A10D-4dce-9F63-405EE243DD3C}"), azrtti_typeid<short>());
        EXPECT_EQ(AZ::Uuid("{72039442-EB38-4d42-A1AD-CB68F7E0EEF6}"), azrtti_typeid<int>());
        EXPECT_EQ(AZ::Uuid("{8F24B9AD-7C51-46cf-B2F8-277356957325}"), azrtti_typeid<long>());
        EXPECT_EQ(AZ::Uuid("{70D8A282-A1EA-462d-9D04-51EDE81FAC2F}"), azrtti_typeid<AZ::s64>());
        EXPECT_EQ(AZ::Uuid("{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"), azrtti_typeid<unsigned char>());
        EXPECT_EQ(AZ::Uuid("{ECA0B403-C4F8-4b86-95FC-81688D046E40}"), azrtti_typeid<unsigned short>());
        EXPECT_EQ(AZ::Uuid("{43DA906B-7DEF-4ca8-9790-854106D3F983}"), azrtti_typeid<unsigned int>());
        EXPECT_EQ(AZ::Uuid("{5EC2D6F7-6859-400f-9215-C106F5B10E53}"), azrtti_typeid<unsigned long>());
        EXPECT_EQ(AZ::Uuid("{D6597933-47CD-4fc8-B911-63F3E2B0993A}"), azrtti_typeid<AZ::u64>());
        EXPECT_EQ(AZ::Uuid("{EA2C3E90-AFBE-44d4-A90D-FAAF79BAF93D}"), azrtti_typeid<float>());
        EXPECT_EQ(AZ::Uuid("{110C4B14-11A8-4e9d-8638-5051013A56AC}"), azrtti_typeid<double>());
        EXPECT_EQ(AZ::Uuid("{A0CA880C-AFE4-43cb-926C-59AC48496112}"), azrtti_typeid<bool>());
        EXPECT_EQ(AZ::Uuid("{E152C105-A133-4d03-BBF8-3D4B2FBA3E2A}"), azrtti_typeid<AZ::Uuid>());
        EXPECT_EQ(AZ::Uuid("{C0F1AFAD-5CB3-450E-B0F5-ADB5D46B0E22}"), azrtti_typeid<void>());
        EXPECT_EQ(AZ::Uuid("{9F4E062E-06A0-46D4-85DF-E0DA96467D3A}"), azrtti_typeid<Crc32>());
        EXPECT_EQ(AZ::Uuid("{0635D08E-DDD2-48DE-A7AE-73CC563C57C3}"), azrtti_typeid<PlatformID>());

        EXPECT_EQ(AZ::Uuid("{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"), azrtti_typeid<int*>());
        EXPECT_EQ(AZ::Uuid("{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"), azrtti_typeid<int&>());
        EXPECT_EQ(AZ::Uuid("{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"), azrtti_typeid<int&&>());
        EXPECT_EQ(AZ::Uuid("{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"), azrtti_typeid<const int*>());
        EXPECT_EQ(AZ::Uuid("{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"), azrtti_typeid<const int&>());
        EXPECT_EQ(AZ::Uuid("{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"), azrtti_typeid<const int&&>());
        EXPECT_EQ(AZ::Uuid("{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"), azrtti_typeid<const int>());

        EXPECT_EQ(AZ::Uuid("{B2F5707A-08FA-566A-BE44-226C634405BE}"), (azrtti_typeid<AZStd::less<int>>()));
        EXPECT_EQ(AZ::Uuid("{6D2500BA-EE64-5288-9766-4C7CD8A10476}"), (azrtti_typeid<AZStd::less_equal<int>>()));
        EXPECT_EQ(AZ::Uuid("{5959973B-2113-5789-BC8C-2F1E4A917953}"), (azrtti_typeid<AZStd::greater<int>>()));
        EXPECT_EQ(AZ::Uuid("{7769141C-BF97-5E9B-B77F-F075FA915905}"), (azrtti_typeid<AZStd::greater_equal<int>>()));
        EXPECT_EQ(AZ::Uuid("{39487937-0E1C-5F78-8A7E-B24EFE32F48F}"), (azrtti_typeid<AZStd::equal_to<int>>()));
        EXPECT_EQ(AZ::Uuid("{AE785799-21A1-5D89-A083-E4441E1F81A8}"), (azrtti_typeid<AZStd::hash<int>>()));
        EXPECT_EQ(AZ::Uuid("{64503325-ECF4-5F02-95F9-E37D00810E59}"), (azrtti_typeid<AZStd::pair<int, int>>()));
        EXPECT_EQ(AZ::Uuid("{853CDD8D-12FF-5619-9A42-10178785620A}"), (azrtti_typeid<AZStd::tuple<int, char, float, double>>()));
        EXPECT_EQ(AZ::Uuid("{85AFA5E8-AA5C-50A3-9CAB-B8C483DA88C5}"), (azrtti_typeid<AZStd::vector<int>>()));
        EXPECT_EQ(AZ::Uuid("{09C2272F-2353-5337-BDCB-B1D0D6A2A778}"), (azrtti_typeid<AZStd::list<int>>()));
        EXPECT_EQ(AZ::Uuid("{2D875DAD-A157-5792-AE25-96D909E1BE4C}"), (azrtti_typeid<AZStd::forward_list<int>>()));
        EXPECT_EQ(AZ::Uuid("{9DF03CD1-931A-544D-A93B-0546907B70CA}"), (azrtti_typeid<AZStd::set<int>>()));
        EXPECT_EQ(AZ::Uuid("{243A34FA-C6F6-51D1-8166-06DED5141370}"), (azrtti_typeid<AZStd::unordered_set<int>>()));
        EXPECT_EQ(AZ::Uuid("{79F4B21A-02CD-58C1-9669-FA2E5E7A142A}"), (azrtti_typeid<AZStd::unordered_multiset<int>>()));
        EXPECT_EQ(AZ::Uuid("{BB54671F-18E6-5F96-B659-FA236D1B7D31}"), (azrtti_typeid<AZStd::map<int, int>>()));
        EXPECT_EQ(AZ::Uuid("{C543E26A-7772-5511-8CE1-A8FA6441CAD3}"), (azrtti_typeid<AZStd::unordered_map<int, int>>()));
        EXPECT_EQ(AZ::Uuid("{FD30FBC0-B826-51CF-A75B-E00466FEB0F0}"), (azrtti_typeid<AZStd::unordered_map<AZStd::string, MyClass>>()));
        EXPECT_EQ(AZ::Uuid("{64E53B04-DD49-55DB-8299-5B4ED53A5F1C}"), (azrtti_typeid<AZStd::unordered_multimap<int, int>>()));
        EXPECT_EQ(AZ::Uuid("{1C213FE1-ED58-5889-8FC9-48D0E11D2E7E}"), (azrtti_typeid<AZStd::unordered_multimap<AZStd::string, MyClass>>()));
        EXPECT_EQ(AZ::Uuid("{0BF83553-00B0-5B7C-9BF3-A87C811F0752}"), (azrtti_typeid<AZStd::shared_ptr<int>>()));
        EXPECT_EQ(AZ::Uuid("{E91D2018-767D-57D4-AF21-5CBEA51A15EC}"), (azrtti_typeid<AZStd::optional<int>>()));
        EXPECT_EQ(AZ::Uuid("{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"), (azrtti_typeid<AZStd::basic_string<char>>()));
        EXPECT_EQ(AZ::Uuid("{406E9B16-A89C-5289-B10E-17F338588559}"), (azrtti_typeid<AZStd::char_traits<char>>()));
        EXPECT_EQ(AZ::Uuid("{7114E998-A8B4-519B-9342-A86D1587B4F7}"), (azrtti_typeid<AZStd::basic_string_view<char>>()));

        EXPECT_EQ(AZ::Uuid("{A3C35B6E-E2DE-58F7-A897-06C64C5BC1E3}"), (azrtti_typeid<AZStd::fixed_vector<int, 4>>()));
        EXPECT_EQ(AZ::Uuid("{F670463F-FB3F-5CF3-A1FE-A7CC6DB312E8}"), (azrtti_typeid<AZStd::fixed_list<int, 4>>()));
        EXPECT_EQ(AZ::Uuid("{71C90433-74CE-5018-BEFD-FC98F4451AEF}"), (azrtti_typeid<AZStd::fixed_forward_list<int, 4>>()));
        EXPECT_EQ(AZ::Uuid("{DD9565F2-A80F-5DD3-B33F-0B0BF1C24A4F}"), (azrtti_typeid<AZStd::array<int, 4>>()));
        EXPECT_EQ(AZ::Uuid("{E5848517-FBDC-5D0F-9012-B16951027D9E}"), (azrtti_typeid<AZStd::bitset<8>>()));
        EXPECT_EQ(AZ::Uuid("{537AD6E8-7443-5C1F-97FD-9284C41C13A4}"), (azrtti_typeid<AZStd::function<bool(int)>>()));

        EXPECT_EQ(AZ::Uuid("{B1E9136B-D77A-4643-BE8E-2ABDA246AE0E}"), (azrtti_typeid<AZStd::monostate>()));
        EXPECT_EQ(AZ::Uuid("{7570E0E7-0BA8-5382-BB14-CEB7B1C0DBEB}"), (azrtti_typeid<AZStd::variant<int, char>>()));
    }

    TEST_F(Rtti, TypeInfoTest)
    {
        AZ_TEST_ASSERT(AzTypeInfo<MyClass>::Uuid() == Uuid("{CADA6BA7-D479-4C20-B7F0-121A1DF4E9CC}"));
        AZ_TEST_ASSERT(strcmp(AzTypeInfo<MyClass>::Name(), "MyClass") == 0);

        AZ_TEST_ASSERT(AzTypeInfo<ExternalClass>::Uuid() == Uuid("{38380915-084B-4886-8D3D-B8439E9E987C}"));
        AZ_TEST_ASSERT(strcmp(AzTypeInfo<ExternalClass>::Name(), "ExternalClass") == 0);

        // template templates
        {
            // Check if the correct type id is returned.
            Uuid templateUuid = Uuid("{EBFE7ADF-1FCE-47F0-B417-14FE06BAF02D}");
            AZ_TEST_ASSERT(AzGenericTypeInfo::Uuid<MyClassTemplate>() == templateUuid);

            // Check that the uuid of the template is returned if AzGenericTypeInfo is used to return the uuid.
            AZ_TEST_ASSERT((AzGenericTypeInfo::Uuid<MyClassTemplate<MyClass, int>>() == templateUuid));
            typedef MyClassTemplate<MyClass, int> MyClassTemplateType;
            AZ_TEST_ASSERT(AzGenericTypeInfo::Uuid<MyClassTemplateType>() == templateUuid);

            // Check all combinations return a valid id.
            AZ_TEST_ASSERT(AzGenericTypeInfo::Uuid<AZStd::array>() == AZ::Uuid("{911B2EA8-CCB1-4F0C-A535-540AD00173AE}"));
            AZ_TEST_ASSERT(AzGenericTypeInfo::Uuid<AZStd::bitset>() == AZ::Uuid("{6BAE9836-EC49-466A-85F2-F4B1B70839FB}"));
            AZ_TEST_ASSERT(AzGenericTypeInfo::Uuid<AZStd::function>() == AZ::Uuid("{C9F9C644-CCC3-4F77-A792-F5B5DBCA746E}"));
            AZ_TEST_ASSERT(AzGenericTypeInfo::Uuid<AZStd::vector>() == AZ::Uuid("{A60E3E61-1FF6-4982-B6B8-9E4350C4C679}"));
        }
        
        // templates
        {
            Uuid templateUuid = Uuid("{EBFE7ADF-1FCE-47F0-B417-14FE06BAF02D}") + AZ::Internal::AggregateTypes<MyClass, int>::Uuid();

            typedef MyClassTemplate<MyClass, int> MyClassTemplateType;

            AZ_TEST_ASSERT(AzTypeInfo<MyClassTemplateType>::Uuid() == templateUuid);
            const char* myClassTemplatename = AzTypeInfo<MyClassTemplateType>::Name();

            AZ_TEST_ASSERT(strstr(myClassTemplatename, "MyClassTemplate"));
            AZ_TEST_ASSERT(strstr(myClassTemplatename, "MyClass"));
            AZ_TEST_ASSERT(strstr(myClassTemplatename, "int"));
        }


        // variadic templates
        {
            Uuid templateUuid = Uuid("{60C1D809-09FA-48EB-A9B7-0BD8DBFF21C8}") + AZ::Internal::AggregateTypes<MyClass, int>::Uuid();

            typedef MyClassVariadicTemplate<MyClass, int> MyClassVariadicTemplateType;

            AZ_TEST_ASSERT(AzTypeInfo<MyClassVariadicTemplateType>::Uuid() == templateUuid);
            const char* myClassTemplatename = AzTypeInfo<MyClassVariadicTemplateType>::Name();

            AZ_TEST_ASSERT(strstr(myClassTemplatename, "MyClassVariadicTemplate"));
            AZ_TEST_ASSERT(strstr(myClassTemplatename, "MyClass"));
            AZ_TEST_ASSERT(strstr(myClassTemplatename, "int"));
        }
    }

    class MyBase
    {
    public:
        AZ_TYPE_INFO(MyBase, "{6A0855E5-6899-482B-B470-C3E5C13D13F5}");
        virtual ~MyBase() {}
        int dataMyBase;
    };

    class MyBase1
        : public MyBase
    {
    public:
        ~MyBase1() override {}
        // Event though MyBase doesn't have RTTI we do allow to be noted as a base class, of course it will NOT be
        // part of the RTTI chain. The goal is to allow AZ_RTTI to declare any base classes without worry if they have RTTI or not
        AZ_RTTI(MyBase1, "{F3F97A32-15D2-48FF-B741-B89EA2DD2280}", MyBase);
        int data1MyBase1;
        int data2MyBase1;
    };

    class MyDerived
        : public MyBase1
    {
    public:
        ~MyDerived() override {}
        AZ_RTTI(MyDerived, "{3BE0590A-F20F-4056-96AF-C2F0565C2EA5}", MyBase1);
        int dataMyDerived;
    };

    class MyDerived1
    {
    public:
        virtual ~MyDerived1() {}
        AZ_RTTI(MyDerived1, "{527B6166-1A4F-4782-8D06-F228860B1102}");
        int datatypename;
    };

    class MyDerived2
        : public MyDerived
    {
    public:
        ~MyDerived2() override {}
        AZ_RTTI(MyDerived2, "{8902C46B-61C5-4294-82A2-06CB61ACA314}", MyDerived);
        int dataMyDerived2;
    };

    class MyClassMix
        : public MyDerived2
        , public MyDerived1
    {
    public:
        ~MyClassMix() override {}
        AZ_RTTI(MyClassMix, "{F6CDCF25-3161-46AE-A46C-0F9B8A1027AF}", MyDerived2, MyDerived1);
        int dataMix;
    };

    class MyClassA
    {
    public:
        virtual ~MyClassA() {}
        AZ_RTTI(MyClassA, "{F2D44607-1BB6-4A6D-8D8B-4FDE27B488CF}");
        int dataClassA;
    };

    class MyClassB
    {
    public:
        virtual ~MyClassB() {}
        AZ_RTTI(MyClassB, "{E46477C8-4833-4F8C-A57A-02EAFA0C33D8}");
        int dataClassB;
    };

    class MyClassC
    {
    public:
        virtual ~MyClassC() {}
        AZ_RTTI(MyClassC, "{614F230F-1AD0-419D-8376-18891112F55D}");
        int dataClassC;
    };

    class MyClassD
        : public MyClassA
    {
    public:
        ~MyClassD() override {}
        AZ_RTTI(MyClassD, "{8E047831-1445-4D13-8F6F-DD36C871FD05}", MyClassA);
        int dataClassD;
    };

    class MyClassMaxMix
        : public MyDerived2
        , public MyDerived1
        , public MyClassB
        , public MyClassC
        , public MyClassD
    {
    public:
        ~MyClassMaxMix() override {}
        AZ_RTTI(MyClassMaxMix, "{49A7F45B-D039-44ED-A6BF-E500CB84E867}", MyDerived2, MyDerived1, MyClassB, MyClassC, MyClassD);
        int dataMaxMix;
    };

    TEST_F(Rtti, IsTypeOfTest)
    {
        typedef AZStd::vector<Uuid> TypeIdArray;
    
        auto EnumTypes = [](const Uuid& id, void* userData)
        {
            TypeIdArray* idArray = reinterpret_cast<TypeIdArray*>(userData);
            idArray->push_back(id);
        };

        MyBase1 mb1;
        MyDerived md;
        MyDerived2 md2;
        MyClassMix mcm;
        MyClassMaxMix mcmm;

        AZ_TEST_ASSERT(azrtti_istypeof<MyBase>(mb1) == false);// MyBase has not RTTI enabled, even though it's a base class
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived>(mb1) == false);
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase>(md) == false);
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived2>(md) == false);

        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived>(md2));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1>(md2));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase>(md2) == false);

        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived1>(mcm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived2>(mcm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived>(mcm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1>(mcm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase>(mcm) == false);

        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived1>(&mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived2>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyDerived>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyClassA>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyClassB>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyClassC>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyClassD>(mcmm));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase>(mcmm) == false);

        // type checks
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1&>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1&>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1&>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1&>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1*>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof<MyBase1*>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1*>(md));
        AZ_TEST_ASSERT(azrtti_istypeof<const MyBase1*>(&md));
        AZ_TEST_ASSERT(azrtti_istypeof(AzTypeInfo<const MyBase1>::Uuid(), &md));
        AZ_TEST_ASSERT(azrtti_istypeof(AzTypeInfo<MyBase1>::Uuid(), md));

        // template templates
        AZStd::vector<int> vector;
        AZStd::array<int, 1> array;
        AZStd::bitset<8> bitset;
        AZStd::function<void()> function;
        AZ_TEST_ASSERT(azrtti_istypeof<AZStd::vector>(vector));
        AZ_TEST_ASSERT(azrtti_istypeof<AZStd::array>(array));
        AZ_TEST_ASSERT(azrtti_istypeof<AZStd::bitset>(bitset));
        AZ_TEST_ASSERT(!azrtti_istypeof<AZStd::vector>(mb1)); // MyBase has not RTTI enabled, even though it's a base class
        AZ_TEST_ASSERT(!azrtti_istypeof<AZStd::vector>(md));

        // check type enumeration
        TypeIdArray typeIds;
        // check a single type (no base types)
        MyDerived1::RTTI_EnumHierarchy(EnumTypes, &typeIds);
        AZ_TEST_ASSERT(typeIds.size() == 1);
        AZ_TEST_ASSERT(typeIds[0] == AzTypeInfo<MyDerived1>::Uuid());
        // check a simple inheritance
        typeIds.clear();
        MyDerived::RTTI_EnumHierarchy(EnumTypes, &typeIds);
        AZ_TEST_ASSERT(typeIds.size() == 2);
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyBase1>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived>::Uuid()) != typeIds.end());
        // check a little more complicated one
        typeIds.clear();
        MyClassMix::RTTI_EnumHierarchy(EnumTypes, &typeIds);
        AZ_TEST_ASSERT(typeIds.size() == 5);
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyBase1>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived1>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived2>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyClassMix>::Uuid()) != typeIds.end());

        // now check the virtual full time selection
        MyBase1* mb1Ptr = &mcm;
        typeIds.clear();
        mb1Ptr->RTTI_EnumTypes(EnumTypes, &typeIds);
        AZ_TEST_ASSERT(typeIds.size() == 5);
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyBase1>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived1>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyDerived2>::Uuid()) != typeIds.end());
        AZ_TEST_ASSERT(AZStd::find(typeIds.begin(), typeIds.end(), AzTypeInfo<MyClassMix>::Uuid()) != typeIds.end());
    }

    TEST_F(Rtti, GetGenericTypeIdTest)
    {
        using IntVector = AZStd::vector<int>;
        IRttiHelper* helper = GetRttiHelper<IntVector>();
        EXPECT_EQ(azrtti_typeid<IntVector>(), helper->GetTypeId());
        EXPECT_EQ(azrtti_typeid<AZStd::vector>(), helper->GetGenericTypeId());
        EXPECT_EQ((azrtti_typeid<IntVector, GenericTypeIdTag>()), helper->GetGenericTypeId());

        helper = GetRttiHelper<MyClassMix>();
        EXPECT_EQ(azrtti_typeid<MyClassMix>(), helper->GetTypeId());
        EXPECT_EQ(azrtti_typeid<MyClassMix>(), helper->GetGenericTypeId());
    }

    class ExampleAbstractClass
    {
    public: 
        AZ_RTTI(ExampleAbstractClass, "{F99EC269-3077-4984-A1B6-FA5656A65AC9}")
        virtual void AbstractFunction1() = 0;
        virtual void AbstractFunction2() = 0;
    };

    class ExampleFullImplementationClass : public ExampleAbstractClass
    {
    public:
        AZ_RTTI(ExampleFullImplementationClass, "{81B043ED-3770-414E-8B54-0F623C035926}", ExampleAbstractClass)
        void AbstractFunction1() override {} 
        void AbstractFunction2() override {}
    };

    class ExamplePartialImplementationClass1 
        : public ExampleAbstractClass
    {
    public:
        AZ_RTTI(ExamplePartialImplementationClass1, "{049B29D7-0414-4C5F-8FB2-589D0833121B}", ExampleAbstractClass)
        void AbstractFunction1() override {}
    };

    class ExampleCombined 
        : public ExamplePartialImplementationClass1
    {
    public:
        AZ_RTTI(ExampleCombined, "{0D03E811-F8F1-4AA5-8DA2-4CD6B7FB7080}", ExamplePartialImplementationClass1)
        void AbstractFunction2() override {}
    };

    TEST_F(Rtti, IsAbstract)
    {
        // compile time proof that the two non-abstract classes are not abstract at compile time:
        [[maybe_unused]] ExampleFullImplementationClass one;
        [[maybe_unused]] ExampleCombined two;

        ASSERT_NE(GetRttiHelper<ExampleAbstractClass>(), nullptr);
        ASSERT_NE(GetRttiHelper<ExampleFullImplementationClass>(), nullptr);
        ASSERT_NE(GetRttiHelper<ExamplePartialImplementationClass1>(), nullptr);
        ASSERT_NE(GetRttiHelper<ExampleCombined>(), nullptr);

        EXPECT_TRUE(GetRttiHelper<ExampleAbstractClass>()->IsAbstract());
        EXPECT_FALSE(GetRttiHelper<ExampleFullImplementationClass>()->IsAbstract());
        EXPECT_TRUE(GetRttiHelper<ExamplePartialImplementationClass1>()->IsAbstract());
        EXPECT_FALSE(GetRttiHelper<ExampleCombined>()->IsAbstract());
    }

    TEST_F(Rtti, DynamicCastTest)
    {
        MyBase1 i_mb1;
        MyDerived i_md;
        MyDerived2 i_md2;
        MyClassMix i_mcm;
        MyClassMaxMix i_mcmm;

        MyBase1* mb1 = &i_mb1;
        MyDerived* md = &i_md;
        MyDerived2* md2 = &i_md2;
        MyClassMix* mcm = &i_mcm;
        MyClassMaxMix* mcmm = &i_mcmm;

        // downcast
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(mb1) == nullptr);// MyBase has not RTTI enabled, even though it's a base class
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived*>(mb1) == nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyBase1*>(md));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(md) == nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived2*>(md) == nullptr);

        AZ_TEST_ASSERT(azdynamic_cast<MyDerived*>(md2));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase1*>(md2));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(md2) == nullptr);

        AZ_TEST_ASSERT(azdynamic_cast<MyDerived1*>(mcm));
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived2*>(mcm));
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived*>(mcm));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase1*>(mcm));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(mcm) == nullptr);

        AZ_TEST_ASSERT(azdynamic_cast<MyDerived1*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived2*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase1*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyClassA*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyClassB*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyClassC*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyClassD*>(mcmm));
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(mcmm) == nullptr);

        // up cast
        mb1 = mcmm;
        MyClassA* mca = mcmm;
        int i_i;
        int* pi = &i_i;

        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(nullptr) == nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyBase*>(pi) == nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<int*>(pi) == pi);

        AZ_TEST_ASSERT(azdynamic_cast<MyDerived*>(mb1) != nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyDerived2*>(mb1) != nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyClassMaxMix*>(mb1) != nullptr);

        AZ_TEST_ASSERT(azdynamic_cast<MyClassD*>(mca) != nullptr);
        AZ_TEST_ASSERT(azdynamic_cast<MyClassMaxMix*>(mca) != nullptr);

        // type checks
        const MyDerived* cmd = md;
        AZ_TEST_ASSERT(azdynamic_cast<const MyBase1*>(md));
        AZ_TEST_ASSERT(azdynamic_cast<const volatile MyBase1*>(md));
        AZ_TEST_ASSERT(azdynamic_cast<const MyBase1*>(cmd));
        AZ_TEST_ASSERT(azdynamic_cast<const volatile MyBase1*>(cmd));
        // unrelated cast not supported (we can, but why)
        //AZ_TEST_ASSERT(azdynamic_cast<MyBase1*>(mca));

        md = mcmm;

        // serialization helpers
        AZ_TEST_ASSERT(mca->RTTI_AddressOf(AzTypeInfo<MyClassMaxMix>::Uuid()) == mcmm);
        AZ_TEST_ASSERT(mb1->RTTI_AddressOf(AzTypeInfo<MyClassMaxMix>::Uuid()) == mcmm);
        AZ_TEST_ASSERT(mb1->RTTI_AddressOf(AzTypeInfo<MyClassA>::Uuid()) == mca);
        AZ_TEST_ASSERT(mb1->RTTI_AddressOf(AzTypeInfo<MyDerived>::Uuid()) == md);
        AZ_TEST_ASSERT(md2->RTTI_AddressOf(AzTypeInfo<MyClassA>::Uuid()) == nullptr);
        AZ_TEST_ASSERT(mcmm->RTTI_AddressOf(AzTypeInfo<MyClassA>::Uuid()) == mca);
        AZ_TEST_ASSERT(mcmm->RTTI_AddressOf(AzTypeInfo<MyBase1>::Uuid()) == mb1);

        // typeid
        AZ_TEST_ASSERT(azrtti_typeid<MyBase>() == AzTypeInfo<MyBase>::Uuid());
        AZ_TEST_ASSERT(azrtti_typeid(i_mb1) == AzTypeInfo<MyBase1>::Uuid());
        AZ_TEST_ASSERT(azrtti_typeid(md2) == AzTypeInfo<MyDerived2>::Uuid());
        AZ_TEST_ASSERT(azrtti_typeid(mca) == AzTypeInfo<MyClassMaxMix>::Uuid());
        MyClassA& mcar = i_mcmm;
        AZ_TEST_ASSERT(azrtti_typeid(mcar) == AzTypeInfo<MyClassMaxMix>::Uuid());
        AZ_TEST_ASSERT(azrtti_typeid<int>() == AzTypeInfo<int>::Uuid());
    }

    TEST_F(Rtti, MultiThreadedTypeInfo)
    {
        // These must be Uuids so that they don't engage the UuidHolder code
        const AZ::Uuid expectedMtti("{4876C017-0C26-4D0D-9A1F-2A738BAE6449}");
        const AZ::Uuid expectedMtti2("{CBC94693-5ECD-4CBF-A8DB-9B122E697E8D}");

        // Create 2x of each of these threads which are doing RTTI ops and
        // let the scheduler run them at random. This is attempting to crash
        // them into each other as best we can
        auto threadFunc1 = [&expectedMtti, &expectedMtti2]()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            const AZ::TypeId& mtti = azrtti_typeid<MTTI>();
            const AZ::TypeId& mtti2 = azrtti_typeid<MTTI2>();
            EXPECT_FALSE(mtti.IsNull());
            EXPECT_EQ(expectedMtti, mtti);
            EXPECT_FALSE(mtti2.IsNull());
            EXPECT_EQ(expectedMtti2, mtti2);
        };

        auto threadFunc2 = []()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            MTTI* mtti = new MTTI();
            bool castSucceeded = (azrtti_cast<MTTI2*>(mtti) != nullptr);
            EXPECT_FALSE(castSucceeded);
            delete mtti;
        };

        auto threadFunc3 = []()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            MTTI2* mtti2 = new MTTI2();
            bool castSucceeded = (azrtti_cast<MTTI*>(mtti2) != nullptr);
            EXPECT_FALSE(castSucceeded);
            delete mtti2;
        };

        auto threadFunc4 = []()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            MTTI* mtti = new MTTI();
            bool castSucceeded = (azrtti_cast<MTTI*>(mtti) != nullptr);
            EXPECT_TRUE(castSucceeded);
            delete mtti;
        };

        auto threadFunc5 = []()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            MTTI2* mtti2 = new MTTI2();
            bool castSucceeded = (azrtti_cast<MTTI2*>(mtti2) != nullptr);
            EXPECT_TRUE(castSucceeded);
            delete mtti2;
        };

        AZStd::fixed_vector<AZStd::function<void()>, 5> threadFuncs({ threadFunc1, threadFunc2, threadFunc3, threadFunc4, threadFunc5 });
        AZStd::thread threads[10];
        for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
        {
            auto threadFunc = threadFuncs[threadIdx % threadFuncs.size()];
            threads[threadIdx] = AZStd::thread(threadFunc);
        }
        for (auto& thread : threads)
        {
            thread.join();
        }
    }

    static void ExternalRttiEnumHeirarchyHelper(const AZ::TypeId&, void* userData)
    {
        auto totalClassesEnumerated = reinterpret_cast<size_t*>(userData);
        ++*totalClassesEnumerated;
    }

    class MyBaseExternal
    {
    public:
        AZ_TYPE_INFO(MyBaseExternal, "{F0F36BB2-14E6-4C44-B3D5-E0CBFD783C99}");

        int32_t m_intValue;
    };

    class MyDerivedExternal
        : public MyBaseExternal
    {
    public:
        AZ_TYPE_INFO(MyDerivedExternal, "{FFD1C3B7-7957-4270-BF10-700CE8BE2B53}");

        float m_floatValue;
    };

    class MyConvertibleExternal
    {
    public:
        AZ_TYPE_INFO(MyConvertibleExternal, "{3962F510-309B-4E32-8CE5-6DEE85F351A9}");

        MyConvertibleExternal() = default;
        MyConvertibleExternal(const MyBaseExternal& baseExternal)
            : m_baseExternal(baseExternal)
        {
        }

        operator MyBaseExternal() const
        {
            return m_baseExternal;
        }

        MyBaseExternal m_baseExternal;
    };

    class MyBaseIntrusive
    {
    public:
        AZ_RTTI(MyBaseIntrusive, "{06D41B30-CEDB-46C9-BD98-B8672A04F71F}");
        virtual ~MyBaseIntrusive() = default;
        uint64_t m_uintValue;
    };

    class MyDerivedIntrusive
        : public MyBaseIntrusive
    {
    public:
        AZ_RTTI(MyDerivedIntrusive, "{6F3FA2A5-CD05-424F-8E37-1DEDA7CE8816}", MyBaseIntrusive);
        ~MyDerivedIntrusive() override = default;
        double m_doubleValue;
    };

    class MyExternalDerivedFromExternalAndIntrusive
        : public MyDerivedExternal
        , public MyDerivedIntrusive
    {
    public:
        AZ_TYPE_INFO(MyExternalDerivedFromExternalAndIntrusive, "{79DC295D-98C5-4FEB-9DC0-0AC3D5A91855}");
    };
}

namespace AZ
{
    AZ_EXTERNAL_RTTI_SPECIALIZE(UnitTest::MyBaseExternal);
    AZ_EXTERNAL_RTTI_SPECIALIZE(UnitTest::MyDerivedExternal, UnitTest::MyBaseExternal);
    AZ_EXTERNAL_RTTI_SPECIALIZE(UnitTest::MyExternalDerivedFromExternalAndIntrusive, UnitTest::MyDerivedExternal, UnitTest::MyDerivedIntrusive);
    AZ_EXTERNAL_RTTI_SPECIALIZE(UnitTest::MyConvertibleExternal, UnitTest::MyBaseExternal);
}
namespace UnitTest
{
    class MyIntrusiveDerivedFromExternalAndIntrusive
        : public MyDerivedExternal
        , public MyDerivedIntrusive
    {
    public:
        AZ_RTTI(MyIntrusiveDerivedFromExternalAndIntrusive, "{3822CF8D-6AC7-4B71-B755-5C69B9DF5A3C}", MyDerivedExternal, MyDerivedIntrusive);
        ~MyIntrusiveDerivedFromExternalAndIntrusive() override = default;
    };


    TEST_F(Rtti, ExternalRtti)
    {
        MyBaseExternal baseInstance{ 7 };
        MyDerivedExternal derivedInstance;
        derivedInstance.m_intValue = 15;
        derivedInstance.m_floatValue = 0.0f;
        MyConvertibleExternal convertibleInstance(MyBaseExternal{ 24 });

        MyExternalDerivedFromExternalAndIntrusive externalDerivedFromExternalAndIntrusiveInstance;
        externalDerivedFromExternalAndIntrusiveInstance.m_intValue = -1;
        externalDerivedFromExternalAndIntrusiveInstance.m_uintValue = 2;
        externalDerivedFromExternalAndIntrusiveInstance.m_floatValue = 2.0f;
        externalDerivedFromExternalAndIntrusiveInstance.m_doubleValue = -32.0;

        MyIntrusiveDerivedFromExternalAndIntrusive intrusiveDerivedFromExternalAndIntrusiveInstance;
        intrusiveDerivedFromExternalAndIntrusiveInstance.m_intValue = -55;
        intrusiveDerivedFromExternalAndIntrusiveInstance.m_uintValue = 256;
        intrusiveDerivedFromExternalAndIntrusiveInstance.m_floatValue = -1023.0f;
        intrusiveDerivedFromExternalAndIntrusiveInstance.m_doubleValue = .0223;

        AZ::IRttiHelper* baseExternal = AZ::GetRttiHelper<MyBaseExternal>();
        AZ::IRttiHelper* derivedExternal = AZ::GetRttiHelper<MyDerivedExternal>();
        AZ::IRttiHelper* convertibleExternal = AZ::GetRttiHelper<MyConvertibleExternal>();
        AZ::IRttiHelper* externalDerivedFromExternalAndIntrusive = AZ::GetRttiHelper<MyExternalDerivedFromExternalAndIntrusive>();
        AZ::IRttiHelper* intrusiveDerivedFromExternalAndIntrusive = AZ::GetRttiHelper<MyIntrusiveDerivedFromExternalAndIntrusive>();

        ASSERT_NE(nullptr, baseExternal);
        ASSERT_NE(nullptr, derivedExternal);
        ASSERT_NE(nullptr, convertibleExternal);
        ASSERT_NE(nullptr, externalDerivedFromExternalAndIntrusive);
        ASSERT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive);

        // Base Class External RTTI
        {
            EXPECT_EQ(AZ::AzTypeInfo<MyBaseExternal>::Uuid(), baseExternal->GetTypeId());
            EXPECT_TRUE(baseExternal->IsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid()));
            size_t enumHierarchyTotalClasses{};

            baseExternal->EnumHierarchy(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            // MyBaseExternal has no other base classes so this count should be 1
            EXPECT_EQ(1, enumHierarchyTotalClasses);
        }

        // Derived Class External RTTI
        {
            EXPECT_EQ(AZ::AzTypeInfo<MyDerivedExternal>::Uuid(), derivedExternal->GetTypeId());
            EXPECT_TRUE(derivedExternal->IsTypeOf(AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));
            EXPECT_TRUE(derivedExternal->IsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid()));
            size_t enumHierarchyTotalClasses{};

            derivedExternal->EnumHierarchy(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            // MyDerivedExternal has MyBaseExternal as a base classes so this count should be 2
            EXPECT_EQ(2, enumHierarchyTotalClasses);

            // MyDerivedExternal -> MyDerivedExternal - succeeds
            EXPECT_NE(nullptr, derivedExternal->Cast(&derivedInstance, AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));
            // MyDerivedExternal -> MyBaseExternal - succeeds
            EXPECT_NE(nullptr, derivedExternal->Cast(&derivedInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyBaseExternal -> MyDerivedExternal - fails
            EXPECT_EQ(nullptr, baseExternal->Cast<MyDerivedExternal>(&baseInstance));

            // MyBaseExternal -> MyBaseExternal(using derived class RttiHelper)- succeeds
            EXPECT_NE(nullptr, derivedExternal->Cast(&baseInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyBaseExternal -> MyBaseExternal(using RttiCast function which must lookup RTTI information from the derived instance)- fails
            // The reason why this fails is because the instance data does not have RTTI on it so it must lookup using
            // using the supplied template type id
            EXPECT_NE(nullptr, AZ::RttiCast<MyBaseExternal*>(&derivedInstance));
        }

        // Convertible Class External RTTI
        {
            EXPECT_EQ(AZ::AzTypeInfo<MyConvertibleExternal>::Uuid(), convertibleExternal->GetTypeId());
            EXPECT_TRUE(convertibleExternal->IsTypeOf(AZ::AzTypeInfo<MyConvertibleExternal>::Uuid()));
            EXPECT_TRUE(convertibleExternal->IsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid()));
            size_t enumHierarchyTotalClasses{};

            convertibleExternal->EnumHierarchy(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            // MyConvertibleExternal specifies "MyBaseExternal" as a base classes even though it really is not,
            // but EnumHierarchy should still enumerate for the "MyBaseExternal" typeid. Therefore the count should be 2
            EXPECT_EQ(2, enumHierarchyTotalClasses);

            // MyConvertibleExternal -> MyConvertibleExternal - succeeds
            EXPECT_NE(nullptr, convertibleExternal->Cast(&convertibleInstance, AZ::AzTypeInfo<MyConvertibleExternal>::Uuid()));
            // MyConvertibleExternal -> MyBaseExternal - succeeds
            EXPECT_NE(nullptr, convertibleExternal->Cast(&convertibleInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyBaseExternal -> MyConvertibleExternal - fails
            EXPECT_EQ(nullptr, baseExternal->Cast<MyConvertibleExternal>(&baseInstance));

            // MyBaseExternal -> MyBaseExternal(using convertible class RttiHelper)- succeeds
            EXPECT_NE(nullptr, convertibleExternal->Cast(&baseInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyBaseExternal -> MyBaseExternal(using RttiCast function which must lookup RTTI information from the derived instance)- succeeds
            EXPECT_NE(nullptr, AZ::RttiCast<MyBaseExternal*>(&derivedInstance));
        }

        // Derived class with External RTTI which inherits from a class with external RTTI and intrusive RTTI
        {
            EXPECT_EQ(AZ::AzTypeInfo<MyExternalDerivedFromExternalAndIntrusive>::Uuid(), externalDerivedFromExternalAndIntrusive->GetTypeId());
            EXPECT_TRUE(externalDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyExternalDerivedFromExternalAndIntrusive>::Uuid()));
            EXPECT_TRUE(externalDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));
            EXPECT_TRUE(externalDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyDerivedIntrusive>::Uuid()));
            EXPECT_TRUE(externalDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid()));
            EXPECT_TRUE(externalDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));
            size_t enumHierarchyTotalClasses{};

            externalDerivedFromExternalAndIntrusive->EnumHierarchy(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            // MyDerivedFromExternalAndIntrusive inherits from MyDerivedExternal which has one base class with external RTTI.
            // This adds 2 to the enumeration count.
            // MyDerivedFromExternalAndIntrusive also inherits from MyDerivedIntrusive which has one base with intrusive RTTI
            // This adds 2 more the enumeration count. Combining these counts with the one for this class the count value should be 5
            EXPECT_EQ(5, enumHierarchyTotalClasses);

            // MyDerivedFromExternalAndIntrusive -> MyDerivedFromExternalAndIntrusive - succeeds
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&externalDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyExternalDerivedFromExternalAndIntrusive>::Uuid()));
            // MyDerivedFromExternalAndIntrusive -> MyDerivedExternal - succeeds
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&externalDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));

            // MyDerivedFromExternalAndIntrusive -> MyBaseExternal - succeeds
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&externalDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyDerivedFromExternalAndIntrusive -> MyDerivedIntrusive- succeeds
            MyDerivedIntrusive* castedDerivedIntrusiveInstance = externalDerivedFromExternalAndIntrusive->Cast<MyDerivedIntrusive>(&externalDerivedFromExternalAndIntrusiveInstance);
            ASSERT_NE(nullptr, castedDerivedIntrusiveInstance);
            EXPECT_DOUBLE_EQ(-32.0, castedDerivedIntrusiveInstance->m_doubleValue);
            castedDerivedIntrusiveInstance->m_doubleValue = -64.0; // Verify that access doesn't crash due to invalid memory address
            // MyDerivedFromExternalAndIntrusive -> MyBaseIntrusive- succeeds
            MyBaseIntrusive* castedBaseIntrusiveInstance = externalDerivedFromExternalAndIntrusive->Cast<MyBaseIntrusive>(&externalDerivedFromExternalAndIntrusiveInstance);
            ASSERT_NE(nullptr, castedBaseIntrusiveInstance);
            EXPECT_EQ(2U, castedBaseIntrusiveInstance->m_uintValue);
            castedDerivedIntrusiveInstance->m_uintValue = 4U;

            // MyDerivedExternal -> MyDerivedFromExternalAndIntrusive - fails
            EXPECT_EQ(nullptr, derivedExternal->Cast<MyExternalDerivedFromExternalAndIntrusive>(&derivedInstance));

            // MyBaseExternal -> MyDerivedFromExternalAndIntrusive - fails
            EXPECT_EQ(nullptr, baseExternal->Cast<MyExternalDerivedFromExternalAndIntrusive>(&baseInstance));

            // MyBaseExternal -> MyBaseExternal(using externalDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&baseInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyDerivedExternal -> MyBaseExternal(using externalDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&derivedInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyBaseIntrusive -> MyBaseIntrusive(using externalDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            MyBaseIntrusive baseIntrusiveInstance;
            baseIntrusiveInstance.m_uintValue = 3456893U;
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&baseIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));

            // MyDerivedIntrusive-> MyBaseIntrusive(using externalDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            MyDerivedIntrusive derivedIntrusiveInstance;
            derivedIntrusiveInstance.m_uintValue = 1700U;
            derivedIntrusiveInstance.m_doubleValue = 24.0f;
            EXPECT_NE(nullptr, externalDerivedFromExternalAndIntrusive->Cast(&derivedIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));

            // Test Rtti Free functions for External class with external Rtti
            enumHierarchyTotalClasses = 0;
            AZ::RttiEnumHierarchy<MyExternalDerivedFromExternalAndIntrusive>(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            EXPECT_EQ(5, enumHierarchyTotalClasses);

            // This should fail
            EXPECT_EQ(nullptr, AZ::RttiCast<MyExternalDerivedFromExternalAndIntrusive*>(&derivedIntrusiveInstance));


            EXPECT_NE(nullptr, AZ::RttiCast<MyExternalDerivedFromExternalAndIntrusive*>(&externalDerivedFromExternalAndIntrusiveInstance));
            void* baseIntrusiveAddress = AZ::RttiAddressOf(&externalDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid());
            ASSERT_NE(nullptr, baseIntrusiveAddress);
            EXPECT_EQ(4U, static_cast<MyBaseIntrusive*>(baseIntrusiveAddress)->m_uintValue);

            EXPECT_FALSE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyConvertibleExternal>::Uuid(), externalDerivedFromExternalAndIntrusiveInstance));
            EXPECT_FALSE(AZ::RttiIsTypeOf<MyConvertibleExternal>(externalDerivedFromExternalAndIntrusiveInstance));

            EXPECT_TRUE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyDerivedIntrusive>::Uuid(), externalDerivedFromExternalAndIntrusiveInstance));
            EXPECT_TRUE(AZ::RttiIsTypeOf<MyDerivedExternal>(externalDerivedFromExternalAndIntrusiveInstance));
            // Check pointer case template specializations for RttiIsTypeOf
            EXPECT_TRUE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid(), &externalDerivedFromExternalAndIntrusiveInstance));
            EXPECT_TRUE(AZ::RttiIsTypeOf<MyBaseExternal>(&externalDerivedFromExternalAndIntrusiveInstance));

            EXPECT_EQ(AZ::AzTypeInfo<MyExternalDerivedFromExternalAndIntrusive>::Uuid(), AZ::RttiTypeId(externalDerivedFromExternalAndIntrusiveInstance));
            // Check pointer case template specializations for RttiTypeId
            EXPECT_EQ(AZ::AzTypeInfo<MyExternalDerivedFromExternalAndIntrusive>::Uuid(), AZ::RttiTypeId(&externalDerivedFromExternalAndIntrusiveInstance));
        }

        // Derived class with Intrusive RTTI which inherits from a class with external RTTI and intrusive RTTI
        {
            EXPECT_EQ(AZ::AzTypeInfo<MyIntrusiveDerivedFromExternalAndIntrusive>::Uuid(), intrusiveDerivedFromExternalAndIntrusive->GetTypeId());
            EXPECT_TRUE(intrusiveDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyIntrusiveDerivedFromExternalAndIntrusive>::Uuid()));
            EXPECT_TRUE(intrusiveDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));
            EXPECT_TRUE(intrusiveDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyDerivedIntrusive>::Uuid()));
            EXPECT_TRUE(intrusiveDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid()));
            EXPECT_TRUE(intrusiveDerivedFromExternalAndIntrusive->IsTypeOf(AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));
            size_t enumHierarchyTotalClasses{};

            intrusiveDerivedFromExternalAndIntrusive->EnumHierarchy(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            // MyIntrusiveDerivedFromExternalAndIntrusive inherits from MyDerivedExternal which has one base class with intrusive RTTI.
            // This adds 2 to the enumeration count.
            // MyIntrusiveDerivedFromExternalAndIntrusive also inherits from MyDerivedIntrusive which has one base with intrusive RTTI
            // This adds 2 more the enumeration count. Combining these counts with the one for this class the count value should be 5
            EXPECT_EQ(5, enumHierarchyTotalClasses);

            // MyIntrusiveDerivedFromExternalAndIntrusive -> MyIntrusiveDerivedFromExternalAndIntrusive - succeeds
            EXPECT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive->Cast(&intrusiveDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyIntrusiveDerivedFromExternalAndIntrusive>::Uuid()));
            // MyIntrusiveDerivedFromExternalAndIntrusive -> MyDerivedExternal - succeeds
            EXPECT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive->Cast(&intrusiveDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyDerivedExternal>::Uuid()));

            // MyIntrusiveDerivedFromExternalAndIntrusive -> MyBaseExternal - succeeds
            EXPECT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive->Cast(&intrusiveDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyBaseExternal>::Uuid()));

            // MyIntrusiveDerivedFromExternalAndIntrusive -> MyDerivedIntrusive- succeeds
            MyDerivedIntrusive* castedDerivedIntrusiveInstance = intrusiveDerivedFromExternalAndIntrusive->Cast<MyDerivedIntrusive>(&intrusiveDerivedFromExternalAndIntrusiveInstance);
            ASSERT_NE(nullptr, castedDerivedIntrusiveInstance);
            EXPECT_DOUBLE_EQ(.0223, castedDerivedIntrusiveInstance->m_doubleValue);
            castedDerivedIntrusiveInstance->m_doubleValue = -64.0; // Verify that access doesn't crash due to invalid memory address
            // MyIntrusiveDerivedFromExternalAndIntrusive -> MyBaseIntrusive- succeeds
            MyBaseIntrusive* castedBaseIntrusiveInstance = intrusiveDerivedFromExternalAndIntrusive->Cast<MyBaseIntrusive>(&intrusiveDerivedFromExternalAndIntrusiveInstance);
            ASSERT_NE(nullptr, castedBaseIntrusiveInstance);
            EXPECT_EQ(256U, castedBaseIntrusiveInstance->m_uintValue);
            castedDerivedIntrusiveInstance->m_uintValue = 71U;

            // MyDerivedExternal -> MyIntrusiveDerivedFromExternalAndIntrusive - fails
            EXPECT_EQ(nullptr, derivedExternal->Cast<MyIntrusiveDerivedFromExternalAndIntrusive>(&derivedInstance));

            // MyBaseExternal -> MyIntrusiveDerivedFromExternalAndIntrusive - fails
            EXPECT_EQ(nullptr, baseExternal->Cast<MyIntrusiveDerivedFromExternalAndIntrusive>(&baseInstance));

            // MyBaseIntrusive -> MyBaseIntrusive(using intrusiveDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            MyBaseIntrusive baseIntrusiveInstance;
            baseIntrusiveInstance.m_uintValue = 3456893U;
            EXPECT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive->Cast(&baseIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));

            // MyDerivedIntrusive-> MyBaseIntrusive(using intrusiveDerivedFromExternalAndIntrusive class RttiHelper)- succeeds
            MyDerivedIntrusive derivedIntrusiveInstance;
            derivedIntrusiveInstance.m_uintValue = 1700U;
            derivedIntrusiveInstance.m_doubleValue = 24.0f;
            EXPECT_NE(nullptr, intrusiveDerivedFromExternalAndIntrusive->Cast(&derivedIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid()));

            // Test Rtti Free functions for class with intrusive Rtti
            enumHierarchyTotalClasses = 0;
            AZ::RttiEnumHierarchy<MyIntrusiveDerivedFromExternalAndIntrusive>(&ExternalRttiEnumHeirarchyHelper, &enumHierarchyTotalClasses);
            EXPECT_EQ(5, enumHierarchyTotalClasses);

            // This should fail
            EXPECT_EQ(nullptr, AZ::RttiCast<MyIntrusiveDerivedFromExternalAndIntrusive*>(&derivedIntrusiveInstance));


            EXPECT_NE(nullptr, AZ::RttiCast<MyIntrusiveDerivedFromExternalAndIntrusive*>(&intrusiveDerivedFromExternalAndIntrusiveInstance));
            void* baseIntrusiveAddress = AZ::RttiAddressOf(&intrusiveDerivedFromExternalAndIntrusiveInstance, AZ::AzTypeInfo<MyBaseIntrusive>::Uuid());
            ASSERT_NE(nullptr, baseIntrusiveAddress);
            EXPECT_EQ(71U, static_cast<MyBaseIntrusive*>(baseIntrusiveAddress)->m_uintValue);

            EXPECT_FALSE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyConvertibleExternal>::Uuid(), intrusiveDerivedFromExternalAndIntrusiveInstance));
            EXPECT_FALSE(AZ::RttiIsTypeOf<MyConvertibleExternal>(intrusiveDerivedFromExternalAndIntrusiveInstance));

            EXPECT_TRUE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyDerivedIntrusive>::Uuid(), intrusiveDerivedFromExternalAndIntrusiveInstance));
            EXPECT_TRUE(AZ::RttiIsTypeOf<MyDerivedExternal>(intrusiveDerivedFromExternalAndIntrusiveInstance));
            // Check pointer case template specializations for RttiIsTypeOf
            EXPECT_TRUE(AZ::RttiIsTypeOf(AZ::AzTypeInfo<MyBaseExternal>::Uuid(), &intrusiveDerivedFromExternalAndIntrusiveInstance));
            EXPECT_TRUE(AZ::RttiIsTypeOf<MyBaseExternal>(&intrusiveDerivedFromExternalAndIntrusiveInstance));

            EXPECT_EQ(AZ::AzTypeInfo<MyIntrusiveDerivedFromExternalAndIntrusive>::Uuid(), AZ::RttiTypeId(intrusiveDerivedFromExternalAndIntrusiveInstance));
            // Check pointer case template specializations for RttiTypeId
            EXPECT_EQ(AZ::AzTypeInfo<MyIntrusiveDerivedFromExternalAndIntrusive>::Uuid(), AZ::RttiTypeId(&intrusiveDerivedFromExternalAndIntrusiveInstance));
        }
    }

    TEST_F(Rtti, ExternalRttiStoresTypeTraits)
    {
        AZ::IRttiHelper* externalRtti = AZ::GetRttiHelper<UnitTest::MyExternalDerivedFromExternalAndIntrusive>();
        ASSERT_NE(nullptr, externalRtti);
        EXPECT_NE(AZ::TypeTraits::is_signed, externalRtti->GetTypeTraits() & AZ::TypeTraits::is_signed);
        EXPECT_NE(AZ::TypeTraits::is_unsigned, externalRtti->GetTypeTraits() & AZ::TypeTraits::is_unsigned);
    }

    TEST_F(Rtti, InternalRttiStoresTypeTraits)
    {
        AZ::IRttiHelper* internalRtti = AZ::GetRttiHelper<UnitTest::ExampleCombined>();
        ASSERT_NE(nullptr, internalRtti);
        EXPECT_NE(AZ::TypeTraits::is_signed, internalRtti->GetTypeTraits() & AZ::TypeTraits::is_signed);
        EXPECT_NE(AZ::TypeTraits::is_unsigned, internalRtti->GetTypeTraits() & AZ::TypeTraits::is_unsigned);
    }

    enum TestEnumWithTypeInfo : uint16_t
    {};
}
namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(UnitTest::TestEnumWithTypeInfo, "{6C2F6697-4E32-4E54-8A9E-AF2FB3F77C69}");
}
namespace UnitTest
{
    TEST_F(Rtti, TypeInfoStoresTypeTraits)
    {
        AZ::IRttiHelper* internalRtti = AZ::GetRttiHelper<int>();
        ASSERT_NE(nullptr, internalRtti);
        EXPECT_EQ(AZ::TypeTraits::is_signed, internalRtti->GetTypeTraits() & AZ::TypeTraits::is_signed);
        EXPECT_NE(AZ::TypeTraits::is_unsigned, internalRtti->GetTypeTraits() & AZ::TypeTraits::is_unsigned);
    }

    class ReflectionManagerTest
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_reflection = AZStd::make_unique<ReflectionManager>();
        }

        void TearDown() override
        {
            m_reflection.reset();

            AllocatorsFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<ReflectionManager> m_reflection;
    };

    class TestReflectedClass
    {
    public:
        static bool s_isReflected;
        static void Reflect(ReflectContext* context)
        {
            s_isReflected = !context->IsRemovingReflection();
        }
    };
    bool TestReflectedClass::s_isReflected = false;

    TEST_F(ReflectionManagerTest, AddContext_AddClass)
    {
        m_reflection->AddReflectContext<SerializeContext>();

        m_reflection->Reflect(&TestReflectedClass::Reflect);
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection->RemoveReflectContext<SerializeContext>();

        EXPECT_FALSE(TestReflectedClass::s_isReflected);
    }

    TEST_F(ReflectionManagerTest, AddClass_AddContext)
    {
        m_reflection->Reflect(&TestReflectedClass::Reflect);

        m_reflection->AddReflectContext<SerializeContext>();
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection->Unreflect(&TestReflectedClass::Reflect);

        EXPECT_FALSE(TestReflectedClass::s_isReflected);
    }

    TEST_F(ReflectionManagerTest, UnreflectOnDestruct)
    {
        m_reflection->Reflect(&TestReflectedClass::Reflect);

        m_reflection->AddReflectContext<SerializeContext>();
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection.reset();

        EXPECT_FALSE(TestReflectedClass::s_isReflected);
    }

    TEST_F(ReflectionManagerTest, UnreflectReReflect)
    {
        m_reflection->AddReflectContext<SerializeContext>();

        m_reflection->Reflect(&TestReflectedClass::Reflect);
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection->Unreflect(&TestReflectedClass::Reflect);
        EXPECT_FALSE(TestReflectedClass::s_isReflected);

        m_reflection->Reflect(&TestReflectedClass::Reflect);
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection->RemoveReflectContext<SerializeContext>();
        EXPECT_FALSE(TestReflectedClass::s_isReflected);

        m_reflection->AddReflectContext<SerializeContext>();
        EXPECT_TRUE(TestReflectedClass::s_isReflected);

        m_reflection.reset();
        EXPECT_FALSE(TestReflectedClass::s_isReflected);
    }
}
