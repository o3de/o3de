/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FileIOBaseTestTypes.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/DataOverlayProviderMsgs.h>
#include <AzCore/Serialization/DataOverlayInstanceMsgs.h>
#include <AzCore/Serialization/DynamicSerializableField.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/DataPatch.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/forward_list.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/bitset.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/tuple.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Quaternion.h>

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Plane.h>

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>

#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Utils/Utils.h>

namespace SerializeTestClasses {
    class MyClassBase1
    {
    public:
        AZ_RTTI(MyClassBase1, "{AA882C72-C7FB-4D19-A167-44BAF96C7D79}");

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<MyClassBase1>()->
                Version(1)->
                Field("data", &MyClassBase1::m_data);
        }

        virtual ~MyClassBase1() {}
        virtual void Set(float v) = 0;

        float m_data{ 0.0f };
    };

    class MyClassBase2
    {
    public:
        AZ_RTTI(MyClassBase2, "{E2DE87D8-15FD-417B-B7E4-5BDF05EA7088}");

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<MyClassBase2>()->
                Version(1)->
                Field("data", &MyClassBase2::m_data);
        }

        virtual ~MyClassBase2() {}
        virtual void Set(float v) = 0;

        float m_data{ 0.0f };
    };

    class MyClassBase3
    {
    public:
        AZ_RTTI(MyClassBase3, "{E9308B39-14B9-4760-A141-EBECFE8891D5}");

        // enum class EnumField : char // Test C++11
        enum EnumField
        {
            Option1,
            Option2,
            Option3,
        };

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<MyClassBase3>()->
                Version(1)->
                Field("data", &MyClassBase3::m_data)->
                Field("enum", &MyClassBase3::m_enum);
        }

        virtual ~MyClassBase3() {}
        virtual void Set(float v) = 0;

        float       m_data{ 0.f };
        EnumField   m_enum{ Option1 };
    };

    class MyClassMix
        : public MyClassBase1
        , public MyClassBase2
        , public MyClassBase3
    {
    public:
        AZ_RTTI(MyClassMix, "{A15003C6-797A-41BB-9D21-716DF0678D02}", MyClassBase1, MyClassBase2, MyClassBase3);
        AZ_CLASS_ALLOCATOR(MyClassMix, AZ::SystemAllocator, 0);

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<MyClassMix, MyClassBase1, MyClassBase2, MyClassBase3>()->
                Field("dataMix", &MyClassMix::m_dataMix);
        }

        void Set(float v) override
        {
            m_dataMix = v;
            MyClassBase1::m_data = v * 2;
            MyClassBase2::m_data = v * 3;
            MyClassBase3::m_data = v * 4;
        }

        bool operator==(const MyClassMix& rhs) const
        {
            return m_dataMix == rhs.m_dataMix
                   && MyClassBase1::m_data == rhs.MyClassBase1::m_data
                   && MyClassBase2::m_data == rhs.MyClassBase2::m_data
                   && MyClassBase3::m_data == rhs.MyClassBase3::m_data;
        }

        double m_dataMix{ 0. };
    };

    class MyClassMixNew
        : public MyClassBase1
        , public MyClassBase2
        , public MyClassBase3
    {
    public:
        AZ_RTTI(MyClassMixNew, "{A15003C6-797A-41BB-9D21-716DF0678D02}", MyClassBase1, MyClassBase2, MyClassBase3); // Use the same UUID as MyClassMix for conversion test
        AZ_CLASS_ALLOCATOR(MyClassMixNew, AZ::SystemAllocator, 0);

        static bool ConvertOldVersions(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() == 0)
            {
                // convert from version 0
                float sum = 0.f;
                for (int i = 0; i < classElement.GetNumSubElements(); )
                {
                    AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(i);
                    if (elementNode.GetName() == AZ_CRC("dataMix", 0x041bcc8d))
                    {
                        classElement.RemoveElement(i);
                        continue;
                    }
                    else
                    {
                        // go through our base classes adding their data members
                        for (int j = 0; j < elementNode.GetNumSubElements(); ++j)
                        {
                            AZ::SerializeContext::DataElementNode& dataNode = elementNode.GetSubElement(j);
                            if (dataNode.GetName() == AZ_CRC("data", 0xadf3f363))
                            {
                                float data;
                                bool result = dataNode.GetData(data);
                                EXPECT_TRUE(result);
                                sum += data;
                                break;
                            }
                        }
                    }
                    ++i;
                }

                // add a new element
                int newElement = classElement.AddElement(context, "baseSum", AZ::SerializeTypeInfo<float>::GetUuid());
                if (newElement != -1)
                {
                    classElement.GetSubElement(newElement).SetData(context, sum);
                }

                return true;
            }

            return false; // just discard unknown versions
        }

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<MyClassMixNew, MyClassBase1, MyClassBase2, MyClassBase3>()->
                Version(1, &MyClassMixNew::ConvertOldVersions)->
                Field("baseSum", &MyClassMixNew::m_baseSum);
        }

        void Set(float v) override
        {
            MyClassBase1::m_data = v * 2;
            MyClassBase2::m_data = v * 3;
            MyClassBase3::m_data = v * 4;
            m_baseSum = v * 2 + v * 3 + v * 4;
        }

        bool operator==(const MyClassMixNew& rhs)
        {
            return m_baseSum == rhs.m_baseSum
                   && MyClassBase1::m_data == rhs.MyClassBase1::m_data
                   && MyClassBase2::m_data == rhs.MyClassBase2::m_data
                   && MyClassBase3::m_data == rhs.MyClassBase3::m_data;
        }

        float m_baseSum;
    };

    class MyClassMix2
        : public MyClassBase2
        , public MyClassBase3
        , public MyClassBase1
    {
    public:
        AZ_RTTI(MyClassMix2, "{D402F58C-812C-4c20-ABE5-E4AF43D66A71}", MyClassBase2, MyClassBase3, MyClassBase1);
        AZ_CLASS_ALLOCATOR(MyClassMix2, AZ::SystemAllocator, 0);

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<MyClassMix2, MyClassBase2, MyClassBase3, MyClassBase1>()->
                Field("dataMix", &MyClassMix2::m_dataMix);
        }

        void Set(float v) override
        {
            m_dataMix = v;
            MyClassBase1::m_data = v * 2;
            MyClassBase2::m_data = v * 3;
            MyClassBase3::m_data = v * 4;
        }

        bool operator==(const MyClassMix2& rhs)
        {
            return m_dataMix == rhs.m_dataMix
                   && MyClassBase1::m_data == rhs.MyClassBase1::m_data
                   && MyClassBase2::m_data == rhs.MyClassBase2::m_data
                   && MyClassBase3::m_data == rhs.MyClassBase3::m_data;
        }

        double m_dataMix;
    };

    class MyClassMix3
        : public MyClassBase3
        , public MyClassBase1
        , public MyClassBase2
    {
    public:
        AZ_RTTI(MyClassMix3, "{4179331A-F4AB-49D2-A14B-06B80CE5952C}", MyClassBase3, MyClassBase1, MyClassBase2);
        AZ_CLASS_ALLOCATOR(MyClassMix3, AZ::SystemAllocator, 0);

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<MyClassMix3, MyClassBase3, MyClassBase1, MyClassBase2>()->
                Field("dataMix", &MyClassMix3::m_dataMix);
        }

        void Set(float v) override
        {
            m_dataMix = v;
            MyClassBase1::m_data = v * 2;
            MyClassBase2::m_data = v * 3;
            MyClassBase3::m_data = v * 4;
        }

        bool operator==(const MyClassMix3& rhs)
        {
            return m_dataMix == rhs.m_dataMix
                   && MyClassBase1::m_data == rhs.MyClassBase1::m_data
                   && MyClassBase2::m_data == rhs.MyClassBase2::m_data
                   && MyClassBase3::m_data == rhs.MyClassBase3::m_data;
        }

        double m_dataMix;
    };

    struct UnregisteredBaseClass
    {
        AZ_RTTI(UnregisteredBaseClass, "{19C26D43-4512-40D8-B5F5-1A69872252D4}");
        virtual ~UnregisteredBaseClass() {}

        virtual void Func() = 0;
    };

    struct ChildOfUndeclaredBase
        : public UnregisteredBaseClass
    {
        AZ_CLASS_ALLOCATOR(ChildOfUndeclaredBase, AZ::SystemAllocator, 0);
        AZ_RTTI(ChildOfUndeclaredBase, "{85268A9C-1CC1-49C6-9E65-9B5089EBC4CD}", UnregisteredBaseClass);
        ChildOfUndeclaredBase()
            : m_data(0) {}

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<ChildOfUndeclaredBase>()->
                Field("data", &ChildOfUndeclaredBase::m_data);
        }

        void Func() override {}

        int m_data;
    };

    struct PolymorphicMemberPointers
    {
        AZ_CLASS_ALLOCATOR(PolymorphicMemberPointers, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(PolymorphicMemberPointers, "{06864A72-A2E2-40E1-A8F9-CC6C59BFBF2D}")

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<PolymorphicMemberPointers>()->
                Field("base1Mix", &PolymorphicMemberPointers::m_pBase1MyClassMix)->
                Field("base1Mix2", &PolymorphicMemberPointers::m_pBase1MyClassMix2)->
                Field("base1Mix3", &PolymorphicMemberPointers::m_pBase1MyClassMix3)->
                Field("base2Mix", &PolymorphicMemberPointers::m_pBase2MyClassMix)->
                Field("base2Mix2", &PolymorphicMemberPointers::m_pBase2MyClassMix2)->
                Field("base2Mix3", &PolymorphicMemberPointers::m_pBase2MyClassMix3)->
                Field("base3Mix", &PolymorphicMemberPointers::m_pBase3MyClassMix)->
                Field("base3Mix2", &PolymorphicMemberPointers::m_pBase3MyClassMix2)->
                Field("base3Mix3", &PolymorphicMemberPointers::m_pBase3MyClassMix3)->
                Field("memberWithUndeclaredBase", &PolymorphicMemberPointers::m_pMemberWithUndeclaredBase);
        }

        PolymorphicMemberPointers()
        {
            m_pBase1MyClassMix = nullptr;
            m_pBase1MyClassMix2 = nullptr;
            m_pBase1MyClassMix3 = nullptr;
            m_pBase2MyClassMix = nullptr;
            m_pBase2MyClassMix2 = nullptr;
            m_pBase2MyClassMix3 = nullptr;
            m_pBase3MyClassMix = nullptr;
            m_pBase3MyClassMix2 = nullptr;
            m_pBase3MyClassMix3 = nullptr;
            m_pMemberWithUndeclaredBase = nullptr;
        }

        virtual ~PolymorphicMemberPointers()
        {
            if (m_pBase1MyClassMix)
            {
                Unset();
            }
        }

        void Set()
        {
            (m_pBase1MyClassMix = aznew MyClassMix)->Set(10.f);
            (m_pBase1MyClassMix2 = aznew MyClassMix2)->Set(20.f);
            (m_pBase1MyClassMix3 = aznew MyClassMix3)->Set(30.f);
            (m_pBase2MyClassMix = aznew MyClassMix)->Set(100.f);
            (m_pBase2MyClassMix2 = aznew MyClassMix2)->Set(200.f);
            (m_pBase2MyClassMix3 = aznew MyClassMix3)->Set(300.f);
            (m_pBase3MyClassMix = aznew MyClassMix)->Set(1000.f);
            (m_pBase3MyClassMix2 = aznew MyClassMix2)->Set(2000.f);
            (m_pBase3MyClassMix3 = aznew MyClassMix3)->Set(3000.f);
            (m_pMemberWithUndeclaredBase = aznew ChildOfUndeclaredBase)->m_data = 1234;
        }

        void Unset()
        {
            delete m_pBase1MyClassMix; m_pBase1MyClassMix = nullptr;
            delete m_pBase1MyClassMix2;
            delete m_pBase1MyClassMix3;
            delete m_pBase2MyClassMix;
            delete m_pBase2MyClassMix2;
            delete m_pBase2MyClassMix3;
            delete m_pBase3MyClassMix;
            delete m_pBase3MyClassMix2;
            delete m_pBase3MyClassMix3;
            delete m_pMemberWithUndeclaredBase;
        }

        MyClassBase1*               m_pBase1MyClassMix;
        MyClassBase1*               m_pBase1MyClassMix2;
        MyClassBase1*               m_pBase1MyClassMix3;
        MyClassBase2*               m_pBase2MyClassMix;
        MyClassBase2*               m_pBase2MyClassMix2;
        MyClassBase2*               m_pBase2MyClassMix3;
        MyClassBase2*               m_pBase3MyClassMix;
        MyClassBase2*               m_pBase3MyClassMix2;
        MyClassBase2*               m_pBase3MyClassMix3;
        ChildOfUndeclaredBase*  m_pMemberWithUndeclaredBase;
    };

    struct BaseNoRtti
    {
        AZ_CLASS_ALLOCATOR(BaseNoRtti, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(BaseNoRtti, "{E57A19BA-EF68-4AFF-A534-2C90B9583781}")

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<BaseNoRtti>()->
                Field("data", &BaseNoRtti::m_data);
        }

        void Set() { m_data = false; }
        bool operator==(const BaseNoRtti& rhs) const { return m_data == rhs.m_data; }
        bool m_data;
    };

    struct BaseRtti
    {
        AZ_RTTI(BaseRtti, "{2581047D-26EC-4969-8354-BA0A4510C51A}");
        AZ_CLASS_ALLOCATOR(BaseRtti, AZ::SystemAllocator, 0);

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<BaseRtti>()->
                Field("data", &BaseRtti::m_data);
        }

        virtual ~BaseRtti() {}

        void Set() { m_data = true; }
        bool operator==(const BaseRtti& rhs) const { return m_data == rhs.m_data; }
        bool m_data;
    };

    struct DerivedNoRtti
        : public BaseNoRtti
    {
        AZ_CLASS_ALLOCATOR(DerivedNoRtti, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(DerivedNoRtti, "{B5E77A22-9C6F-4755-A074-FEFD8AC2C971}")

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<DerivedNoRtti, BaseNoRtti>()->
                Field("basesRtti", &DerivedNoRtti::m_basesRtti)->
                Field("basesNoRtti", &DerivedNoRtti::m_basesNoRtti);
        }

        void Set() { m_basesRtti = 0; m_basesNoRtti = 1; BaseNoRtti::Set(); }
        bool operator==(const DerivedNoRtti& rhs) const { return m_basesRtti == rhs.m_basesRtti && m_basesNoRtti == rhs.m_basesNoRtti && BaseNoRtti::operator==(static_cast<const BaseNoRtti&>(rhs)); }
        int m_basesRtti;
        int m_basesNoRtti;
    };

    struct DerivedRtti
        : public BaseRtti
    {
        AZ_RTTI(DerivedRtti, "{A14C419C-6F25-46A6-8D17-7777893073EF}", BaseRtti);
        AZ_CLASS_ALLOCATOR(DerivedRtti, AZ::SystemAllocator, 0);

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<DerivedRtti, BaseRtti>()->
                Field("basesRtti", &DerivedRtti::m_basesRtti)->
                Field("basesNoRtti", &DerivedRtti::m_basesNoRtti);
        }

        void Set() { m_basesRtti = 1; m_basesNoRtti = 0; BaseRtti::Set(); }
        bool operator==(const DerivedRtti& rhs) const { return m_basesRtti == rhs.m_basesRtti && m_basesNoRtti == rhs.m_basesNoRtti && BaseRtti::operator==(static_cast<const BaseRtti&>(rhs)); }
        int m_basesRtti;
        int m_basesNoRtti;
    };

    struct DerivedMix
        : public BaseNoRtti
        , public BaseRtti
    {
        AZ_RTTI(DerivedMix, "{BED5293B-3B80-4CEC-BB0F-2E56F921F550}", BaseRtti);
        AZ_CLASS_ALLOCATOR(DerivedMix, AZ::SystemAllocator, 0);

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<DerivedMix, BaseNoRtti, BaseRtti>()->
                Field("basesRtti", &DerivedMix::m_basesRtti)->
                Field("basesNoRtti", &DerivedMix::m_basesNoRtti);
        }

        void Set() { m_basesRtti = 1; m_basesNoRtti = 1; BaseNoRtti::Set(); BaseRtti::Set(); }
        bool operator==(const DerivedMix& rhs) const { return m_basesRtti == rhs.m_basesRtti && m_basesNoRtti == rhs.m_basesNoRtti && BaseNoRtti::operator==(static_cast<const BaseNoRtti&>(rhs)) && BaseRtti::operator==(static_cast<const BaseRtti&>(rhs)); }
        int m_basesRtti;
        int m_basesNoRtti;
    };

    struct BaseProtected
    {
        AZ_TYPE_INFO(BaseProtected, "{c6e244d8-ffd8-4710-900b-1d3dc4043ffe}");

        int m_pad; // Make sure there is no offset assumptions, for base members and we offset properly with in the base class.
        int m_data;

    protected:
        BaseProtected(int data = 0)
            : m_data(data) {}
    };

    struct DerivedWithProtectedBase
        : public BaseProtected
    {
        AZ_TYPE_INFO(DerivedWithProtectedBase, "{ad736023-a491-440a-84e3-5c507c969673}");
        AZ_CLASS_ALLOCATOR(DerivedWithProtectedBase, AZ::SystemAllocator, 0);

        DerivedWithProtectedBase(int data = 0)
            : BaseProtected(data)
        {}

        static void Reflect(AZ::SerializeContext& context)
        {
            // Expose base class field without reflecting the class
            context.Class<DerivedWithProtectedBase>()
                ->FieldFromBase<DerivedWithProtectedBase>("m_data", &DerivedWithProtectedBase::m_data);
        }
    };

    struct SmartPtrClass
    {
        AZ_CLASS_ALLOCATOR(SmartPtrClass, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(SmartPtrClass, "{A0A2D0A8-8D5D-454D-BE92-684C92C05B06}")

        SmartPtrClass(int data = 0)
            : m_counter(0)
            , m_data(data) {}

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<SmartPtrClass>()->
                Field("data", &SmartPtrClass::m_data);
        }

        //////////////////////////////////////////////////////////////////////////
        // For intrusive pointers
        void add_ref()  { ++m_counter; }
        void release()
        {
            --m_counter;
            if (m_counter == 0)
            {
                delete this;
            }
        }
        int m_counter;
        //////////////////////////////////////////////////////////////////////////

        int m_data;
    };

    struct Generics
    {
        AZ_CLASS_ALLOCATOR(Generics, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(Generics, "{ACA50B82-D04B-4ACF-9FF6-F780040C9EB9}")

        enum class GenericEnum
        {
            Value1 = 0x01,
            Value2 = 0x02,
            Value3 = 0x04,
        };

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<Generics>()->
                Field("emptyTextData", &Generics::m_emptyTextData)->
                Field("textData", &Generics::m_textData)->
                Field("vectorInt", &Generics::m_vectorInt)->
                Field("vectorIntVector", &Generics::m_vectorIntVector)->
                Field("vectorEnum", &Generics::m_vectorEnum)->
                Field("fixedVectorInt", &Generics::m_fixedVectorInt)->
                Field("listInt", &Generics::m_listInt)->
                Field("forwardListInt", &Generics::m_forwardListInt)->
                Field("setInt", &Generics::m_setInt)->
                Field("usetInt", &Generics::m_usetInt)->
                Field("umultisetInt", &Generics::m_umultisetInt)->
                Field("mapIntFloat", &Generics::m_mapIntFloat)->
                Field("umapIntFloat", &Generics::m_umapIntFloat)->
                Field("umultimapIntFloat", &Generics::m_umultimapIntFloat)->
                Field("umapPolymorphic", &Generics::m_umapPolymorphic)->
                Field("byteStream", &Generics::m_byteStream)->
                Field("bitSet", &Generics::m_bitSet)->
                Field("sharedPtr", &Generics::m_sharedPtr)->
                Field("intrusivePtr", &Generics::m_intrusivePtr)->
                Field("uniquePtr", &Generics::m_uniquePtr)->
                Field("emptyInitTextData", &Generics::m_emptyInitTextData);
        }

        Generics()
        {
            m_emptyInitTextData = "Some init text";
        }

        ~Generics()
        {
            if (m_umapPolymorphic.size() > 0)
            {
                Unset();
            }
        }

    public:

        void Set()
        {
            m_emptyInitTextData = ""; // data was initialized, we set it to nothing (make sure we write empty strings)
            m_textData = "Random Text";
            m_vectorInt.push_back(1);
            m_vectorInt.push_back(2);
            m_vectorIntVector.push_back();
            m_vectorIntVector.back().push_back(5);
            m_vectorEnum.push_back(GenericEnum::Value3);
            m_vectorEnum.push_back(GenericEnum::Value1);
            m_vectorEnum.push_back(GenericEnum::Value3);
            m_vectorEnum.push_back(GenericEnum::Value2);
            m_fixedVectorInt.push_back(1000);
            m_fixedVectorInt.push_back(2000);
            m_fixedVectorInt.push_back(3000);
            m_fixedVectorInt.push_back(4000);
            m_fixedVectorInt.push_back(5000);
            m_listInt.push_back(10);
            m_forwardListInt.push_back(15);
            m_setInt.insert(20);
            m_usetInt.insert(20);
            m_umultisetInt.insert(20);
            m_umultisetInt.insert(20);
            m_mapIntFloat.insert(AZStd::make_pair(1, 5.f));
            m_mapIntFloat.insert(AZStd::make_pair(2, 10.f));
            m_umapIntFloat.insert(AZStd::make_pair(1, 5.f));
            m_umapIntFloat.insert(AZStd::make_pair(2, 10.f));
            m_umultimapIntFloat.insert(AZStd::make_pair(1, 5.f));
            m_umultimapIntFloat.insert(AZStd::make_pair(2, 10.f));
            m_umultimapIntFloat.insert(AZStd::make_pair(2, 20.f));
            m_umapPolymorphic.insert(AZStd::make_pair(1, aznew MyClassMix)).first->second->Set(100.f);
            m_umapPolymorphic.insert(AZStd::make_pair(2, aznew MyClassMix2)).first->second->Set(200.f);
            m_umapPolymorphic.insert(AZStd::make_pair(3, aznew MyClassMix3)).first->second->Set(300.f);

            AZ::u32 binaryData = 0xbad0f00d;
            m_byteStream.assign((AZ::u8*)&binaryData, (AZ::u8*)(&binaryData + 1));
            m_bitSet = AZStd::bitset<32>(AZStd::string("01011"));

            m_sharedPtr = AZStd::shared_ptr<SmartPtrClass>(aznew SmartPtrClass(122));
            m_intrusivePtr = AZStd::intrusive_ptr<SmartPtrClass>(aznew SmartPtrClass(233));
            m_uniquePtr = AZStd::unique_ptr<SmartPtrClass>(aznew SmartPtrClass(4242));
        }

        void Unset()
        {
            m_emptyTextData.set_capacity(0);
            m_emptyInitTextData.set_capacity(0);
            m_textData.set_capacity(0);
            m_vectorInt.set_capacity(0);
            m_vectorIntVector.set_capacity(0);
            m_vectorEnum.set_capacity(0);
            m_listInt.clear();
            m_forwardListInt.clear();
            m_setInt.clear();
            m_mapIntFloat.clear();
            for (AZStd::unordered_map<int, MyClassBase1*>::iterator it = m_umapPolymorphic.begin(); it != m_umapPolymorphic.end(); ++it)
            {
                delete it->second;
            }
            m_umapPolymorphic.clear();
            m_byteStream.set_capacity(0);
            m_bitSet.reset();
            m_sharedPtr.reset();
            m_intrusivePtr.reset();
            m_uniquePtr.reset();
        }

        AZStd::string                               m_emptyTextData;
        AZStd::string                               m_emptyInitTextData;
        AZStd::string                               m_textData;
        AZStd::vector<int>                          m_vectorInt;
        AZStd::vector<AZStd::vector<int> >          m_vectorIntVector;
        AZStd::vector<GenericEnum>                  m_vectorEnum;
        AZStd::fixed_vector<int, 5>                 m_fixedVectorInt;
        AZStd::list<int>                            m_listInt;
        AZStd::forward_list<int>                    m_forwardListInt;
        AZStd::set<int>                             m_setInt;
        AZStd::map<int, float>                      m_mapIntFloat;
        AZStd::unordered_set<int>                   m_usetInt;
        AZStd::unordered_multiset<int>              m_umultisetInt;
        AZStd::unordered_map<int, float>            m_umapIntFloat;
        AZStd::unordered_map<int, MyClassBase1*>    m_umapPolymorphic;
        AZStd::unordered_multimap<int, float>       m_umultimapIntFloat;
        AZStd::vector<AZ::u8>                       m_byteStream;
        AZStd::bitset<32>                           m_bitSet;
        AZStd::shared_ptr<SmartPtrClass>            m_sharedPtr;
        AZStd::intrusive_ptr<SmartPtrClass>         m_intrusivePtr;
        AZStd::unique_ptr<SmartPtrClass>            m_uniquePtr;
    };
} //SerializeTestClasses

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(SerializeTestClasses::Generics::GenericEnum, "{1D382230-EF25-4583-812B-7576334AB1A9}");
}

namespace SerializeTestClasses
{
    struct GenericsNew
    {
        AZ_CLASS_ALLOCATOR(GenericsNew, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(GenericsNew, "{ACA50B82-D04B-4ACF-9FF6-F780040C9EB9}") // Match Generics ID for conversion test

        static bool ConvertOldVersions(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() == 0)
            {
                // convert from version 0
                for (int i = 0; i < classElement.GetNumSubElements(); )
                {
                    AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(i);
                    if (elementNode.GetName() == AZ_CRC("textData", 0xf322c69d))
                    {
                        AZStd::string text;
                        bool result = elementNode.GetData(text);
                        EXPECT_TRUE(result);

                        int memberIdx = classElement.AddElement<AZStd::string>(context, "string");
                        if (memberIdx != -1)
                        {
                            AZ::SerializeContext::DataElementNode& memberNode = classElement.GetSubElement(memberIdx);
                            memberNode.SetData(context, text);
                        }
                        classElement.RemoveElement(i);
                    }
                    else if (elementNode.GetName() == AZ_CRC("emptyTextData", 0x61d55942))
                    {
                        AZStd::string text;
                        bool result = elementNode.GetData(text);
                        EXPECT_TRUE(result);
                        EXPECT_TRUE(text.empty()); // this should be empty

                        classElement.RemoveElement(i);
                    }
                    else if (elementNode.GetName() == AZ_CRC("vectorInt", 0xe61292a9))
                    {
                        int memberIdx = classElement.AddElement<AZStd::vector<int> >(context, "vectorInt2");
                        if (memberIdx != -1)
                        {
                            AZ::SerializeContext::DataElementNode& memberNode = classElement.GetSubElement(memberIdx);
                            for (int j = 0; j < elementNode.GetNumSubElements(); ++j)
                            {
                                AZ::SerializeContext::DataElementNode& vecElemNode = elementNode.GetSubElement(j);
                                int val;
                                bool result = vecElemNode.GetData(val);
                                EXPECT_TRUE(result);
                                int elemIdx = memberNode.AddElement<int>(context, AZ::SerializeContext::IDataContainer::GetDefaultElementName());
                                if (elemIdx != -1)
                                {
                                    memberNode.GetSubElement(elemIdx).SetData(context, val * 2);
                                }
                            }
                        }
                        classElement.RemoveElement(i);
                    }
                    else if (elementNode.GetName() == AZ_CRC("vectorIntVector", 0xd9c44f0b))
                    {
                        // add a new element
                        int newListIntList = classElement.AddElement<AZStd::list<AZStd::list<int> > >(context, "listIntList");
                        if (newListIntList != -1)
                        {
                            AZ::SerializeContext::DataElementNode& listIntListNode = classElement.GetSubElement(newListIntList);
                            for (int j = 0; j < elementNode.GetNumSubElements(); ++j)
                            {
                                AZ::SerializeContext::DataElementNode& subVecNode = elementNode.GetSubElement(j);
                                int newListInt = listIntListNode.AddElement<AZStd::list<int> >(context, AZ::SerializeContext::IDataContainer::GetDefaultElementName());
                                if (newListInt != -1)
                                {
                                    AZ::SerializeContext::DataElementNode& listIntNode = listIntListNode.GetSubElement(newListInt);
                                    for (int k = 0; k < subVecNode.GetNumSubElements(); ++k)
                                    {
                                        AZ::SerializeContext::DataElementNode& intNode = subVecNode.GetSubElement(k);
                                        int val;
                                        bool result = intNode.GetData(val);
                                        EXPECT_TRUE(result);
                                        int newInt = listIntNode.AddElement<int>(context, AZ::SerializeContext::IDataContainer::GetDefaultElementName());
                                        if (newInt != -1)
                                        {
                                            listIntNode.GetSubElement(newInt).SetData(context, val);
                                        }
                                    }
                                }
                            }
                        }
                        classElement.RemoveElement(i);
                    }
                    else if (elementNode.GetName() == AZ_CRC("emptyInitTextData", 0x17b55a4f)
                            || elementNode.GetName() == AZ_CRC("listInt", 0x4fbe090a)
                            || elementNode.GetName() == AZ_CRC("setInt", 0x62eb1299)
                            || elementNode.GetName() == AZ_CRC("usetInt")
                            || elementNode.GetName() == AZ_CRC("umultisetInt")
                            || elementNode.GetName() == AZ_CRC("mapIntFloat", 0xb558ac3f)
                            || elementNode.GetName() == AZ_CRC("umapIntFloat")
                            || elementNode.GetName() == AZ_CRC("umultimapIntFloat")
                            || elementNode.GetName() == AZ_CRC("byteStream", 0xda272a22)
                            || elementNode.GetName() == AZ_CRC("bitSet", 0x9dd4d1cb)
                            || elementNode.GetName() == AZ_CRC("sharedPtr", 0x033de7f0)
                            || elementNode.GetName() == AZ_CRC("intrusivePtr", 0x20733e45)
                            || elementNode.GetName() == AZ_CRC("uniquePtr", 0xdb6f5bd3)
                            || elementNode.GetName() == AZ_CRC("forwardListInt", 0xf54c1600)
                            || elementNode.GetName() == AZ_CRC("fixedVectorInt", 0xf7108293)
                            || elementNode.GetName() == AZ_CRC("vectorEnum"))
                    {
                        classElement.RemoveElement(i);
                    }
                    else
                    {
                        ++i;
                    }
                }

                // add a new element
                int newElement = classElement.AddElement(context, "newInt", AZ::SerializeTypeInfo<int>::GetUuid());
                if (newElement != -1)
                {
                    classElement.GetSubElement(newElement).SetData(context, 50);
                }

                return true;
            }

            return false; // just discard unknown versions
        }

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<GenericsNew>()->
                Version(1, &GenericsNew::ConvertOldVersions)->
                Field("string", &GenericsNew::m_string)->
                Field("vectorInt2", &GenericsNew::m_vectorInt2)->
                Field("listIntList", &GenericsNew::m_listIntList)->
                Field("umapPolymorphic", &GenericsNew::m_umapPolymorphic)->
                Field("newInt", &GenericsNew::m_newInt);
        }

        ~GenericsNew()
        {
            if (m_umapPolymorphic.size() > 0)
            {
                Unset();
            }
        }

        void Set()
        {
            m_string = "Random Text";
            m_vectorInt2.push_back(1 * 2);
            m_vectorInt2.push_back(2 * 2);
            m_listIntList.push_back();
            m_listIntList.back().push_back(5);
            m_umapPolymorphic.insert(AZStd::make_pair(1, aznew MyClassMixNew)).first->second->Set(100.f);
            m_umapPolymorphic.insert(AZStd::make_pair(2, aznew MyClassMix2)).first->second->Set(200.f);
            m_umapPolymorphic.insert(AZStd::make_pair(3, aznew MyClassMix3)).first->second->Set(300.f);
            m_newInt = 50;
        }

        void Unset()
        {
            m_string.set_capacity(0);
            m_vectorInt2.set_capacity(0);
            m_listIntList.clear();
            for (AZStd::unordered_map<int, MyClassBase1*>::iterator it = m_umapPolymorphic.begin(); it != m_umapPolymorphic.end(); ++it)
            {
                delete it->second;
            }
            m_umapPolymorphic.clear();
        }

        AZStd::string                               m_string;           // rename m_textData to m_string
        AZStd::vector<int>                          m_vectorInt2;       // rename m_vectorInt to m_vectorInt2 and multiply all values by 2
        AZStd::list<AZStd::list<int> >              m_listIntList;      // convert vector<vector<int>> to list<list<int>>
        AZStd::unordered_map<int, MyClassBase1*>    m_umapPolymorphic;   // using new version of MyClassMix
        int                                         m_newInt;           // added new member
    };

    class ClassThatAllocatesMemoryInDefaultCtor final
    {
    public:
        AZ_RTTI("ClassThatAllocatesMemoryInDefaultCtor", "{CF9B593D-A19E-467B-8370-28AF68D2F345}")
        AZ_CLASS_ALLOCATOR(ClassThatAllocatesMemoryInDefaultCtor, AZ::SystemAllocator, 0)

        ClassThatAllocatesMemoryInDefaultCtor()
            : m_data(aznew InstanceTracker)
        {
        }

        ~ClassThatAllocatesMemoryInDefaultCtor()
        {
            delete m_data;
        }

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<InstanceTracker>();

            sc.Class<ClassThatAllocatesMemoryInDefaultCtor>()->
                Field("data", &ClassThatAllocatesMemoryInDefaultCtor::m_data)
                ;
        }

        class InstanceTracker final
        {
        public:
            AZ_RTTI("InstanceTracker", "{DED6003B-11E0-454C-B170-4889697815A0}");
            AZ_CLASS_ALLOCATOR(InstanceTracker, AZ::SystemAllocator, 0);

            InstanceTracker()
            {
                ++s_instanceCount;
            }

            ~InstanceTracker()
            {
                --s_instanceCount;
            }

            InstanceTracker(const InstanceTracker&) = delete;
            InstanceTracker(InstanceTracker&&) = delete;

            static AZStd::atomic_int s_instanceCount;
        };

    private:
        const InstanceTracker* m_data;
    };

    AZStd::atomic_int ClassThatAllocatesMemoryInDefaultCtor::InstanceTracker::s_instanceCount(0);
}   // namespace SerializeTestClasses

namespace ContainerElementDeprecationTestData
{
    using namespace AZ;
    // utility classes for testing what happens to container elements, when they are deprecated.
    class BaseClass
    {
    public:
        AZ_RTTI(BaseClass, "{B736AD73-E627-467D-A779-7B942D2B5359}");
        AZ_CLASS_ALLOCATOR(BaseClass, SystemAllocator, 0);
        virtual ~BaseClass() {}

        static void Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<BaseClass>();
            }
        }
    };

    class DerivedClass1 : public BaseClass
    {
    public:
        AZ_RTTI(DerivedClass1, "{E55D26B8-96B9-4918-94F0-5ABCA29F2508}", BaseClass);
        AZ_CLASS_ALLOCATOR(DerivedClass1, SystemAllocator, 0);
        static void Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DerivedClass1, BaseClass>();
            }
        }
    };

    class DerivedClass2 : public BaseClass
    {
    public:
        AZ_RTTI(DerivedClass2, "{91F6C9A1-1EB1-477E-99FC-41A35FE9CF0B}", BaseClass);
        AZ_CLASS_ALLOCATOR(DerivedClass2, SystemAllocator, 0);
        static void Reflect(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DerivedClass2, BaseClass>();
            }
        }
    };

    class DerivedClass3 : public BaseClass
    {
    public:
        AZ_RTTI(DerivedClass3, "{1399CC2D-D525-4061-B190-5FCD82FCC161}", BaseClass);
        AZ_CLASS_ALLOCATOR(DerivedClass3, AZ::SystemAllocator, 0);
        static void Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<DerivedClass3, BaseClass>();
            }
        }
    };

    static bool ConvertDerivedClass2ToDerivedClass3(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        classElement.Convert(context, AZ::AzTypeInfo<DerivedClass3>::Uuid());
        return true;
    }

    class ClassWithAVectorOfBaseClasses final
    {
    public:
        AZ_RTTI(ClassWithAVectorOfBaseClasses, "{B62A3327-8BEE-43BD-BA2C-32BAE9EE5455}");
        AZ_CLASS_ALLOCATOR(ClassWithAVectorOfBaseClasses, AZ::SystemAllocator, 0);
        AZStd::vector<BaseClass*> m_vectorOfBaseClasses;

        ~ClassWithAVectorOfBaseClasses()
        {
            for (auto base : m_vectorOfBaseClasses)
            {
                delete base;
            }
            m_vectorOfBaseClasses.swap(AZStd::vector<BaseClass*>());
        }

        static void Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                BaseClass::Reflect(context);
                DerivedClass1::Reflect(context);
                DerivedClass2::Reflect(context);
                DerivedClass3::Reflect(context);

                serializeContext->Class<ClassWithAVectorOfBaseClasses>()
                    ->Field("m_vectorOfBaseClasses", &ClassWithAVectorOfBaseClasses::m_vectorOfBaseClasses);
            }
        }
    };

} // End of namespace ContainerElementDeprecationTestData


namespace AZ {
    struct GenericClass
    {
        AZ_RTTI(GenericClass, "{F2DAA5D8-CA20-4DD4-8942-356458AF23A1}");
        virtual ~GenericClass() {}
    };

    class NullFactory
        : public SerializeContext::IObjectFactory
    {
    public:
        void* Create(const char* name) override
        {
            (void)name;
            AZ_Assert(false, "We cannot 'new' %s class, it should be used by value in a parent class!", name);
            return nullptr;
        }

        void Destroy(void*) override
        {
            // do nothing...
        }
    };

    template<>
    struct SerializeGenericTypeInfo<GenericClass>
    {
        class GenericClassGenericInfo
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassGenericInfo, "{7A26F864-DADC-4bdf-8C4C-A162349031C6}");
            GenericClassGenericInfo()
                : m_classData{ SerializeContext::ClassData::Create<GenericClass>("GenericClass", GetSpecializedTypeId(), &m_factory) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<GenericClass>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<GenericClass>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext*) override {}

            NullFactory m_factory;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassGenericInfo;

        static ClassInfoType* GetGenericInfo()
        {
            return static_cast<ClassInfoType*>(GetCurrentSerializeContextModule().CreateGenericClassInfo<GenericClass>());
        }

        static const Uuid& GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };

    struct GenericChild
        : public GenericClass
    {
        AZ_RTTI(GenericChild, "{086E933D-F3F9-41EA-9AA9-BA80D3DCF90A}", GenericClass);
        ~GenericChild() override {}
    };
    template<>
    struct SerializeGenericTypeInfo<GenericChild>
    {
        class GenericClassGenericInfo
            : public GenericClassInfo
        {
        public:
            AZ_TYPE_INFO(GenericClassGenericInfo, "{D1E1ACC0-7B90-48e9-999B-5825D4D4E397}");
            GenericClassGenericInfo()
                : m_classData{ SerializeContext::ClassData::Create<GenericChild>("GenericChild", GetSpecializedTypeId(), &m_factory) }
            {
            }

            SerializeContext::ClassData* GetClassData() override
            {
                return &m_classData;
            }

            size_t GetNumTemplatedArguments() override
            {
                return 1;
            }

            const Uuid& GetTemplatedTypeId(size_t element) override
            {
                (void)element;
                return SerializeGenericTypeInfo<GenericClass>::GetClassTypeId();
            }

            const Uuid& GetSpecializedTypeId() const override
            {
                return azrtti_typeid<GenericChild>();
            }

            const Uuid& GetGenericTypeId() const override
            {
                return TYPEINFO_Uuid();
            }

            void Reflect(SerializeContext*) override;

            NullFactory m_factory;
            SerializeContext::ClassData m_classData;
        };

        using ClassInfoType = GenericClassGenericInfo;

        static ClassInfoType* GetGenericInfo()
        {
            return static_cast<ClassInfoType*>(GetCurrentSerializeContextModule().CreateGenericClassInfo<GenericChild>());
        }

        static const Uuid& GetClassTypeId()
        {
            return GetGenericInfo()->GetClassData()->m_typeId;
        }
    };
}

using namespace SerializeTestClasses;
using namespace AZ;

namespace UnitTest
{
    /*
    * Base class for all serialization unit tests
    */
    class Serialization
        : public ScopedAllocatorSetupFixture
        , public ComponentApplicationBus::Handler
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationMessages
        ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const ComponentDescriptor*) override { }
        void UnregisterComponentDescriptor(const ComponentDescriptor*) override { }
        void RegisterEntityAddedEventHandler(EntityAddedEvent::Handler&) override { }
        void RegisterEntityRemovedEventHandler(EntityRemovedEvent::Handler&) override { }
        void RegisterEntityActivatedEventHandler(EntityActivatedEvent::Handler&) override { }
        void RegisterEntityDeactivatedEventHandler(EntityDeactivatedEvent::Handler&) override { }
        void SignalEntityActivated(Entity*) override { }
        void SignalEntityDeactivated(Entity*) override { }
        bool AddEntity(Entity*) override { return false; }
        bool RemoveEntity(Entity*) override { return false; }
        bool DeleteEntity(const EntityId&) override { return false; }
        Entity* FindEntity(const EntityId&) override { return nullptr; }
        SerializeContext* GetSerializeContext() override { return m_serializeContext.get(); }
        BehaviorContext*  GetBehaviorContext() override { return nullptr; }
        JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
        const char* GetAppRoot() const override { return nullptr; }
        const char* GetEngineRoot() const override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        void EnumerateEntities(const EntityCallback& /*callback*/) override {}
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        //////////////////////////////////////////////////////////////////////////

        void SetUp() override
        {
            m_serializeContext.reset(aznew AZ::SerializeContext());

            ComponentApplicationBus::Handler::BusConnect();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_streamer = AZStd::make_unique<IO::Streamer>(AZStd::thread_desc{}, AZ::StreamerComponent::CreateStreamerStack());
            Interface<IO::IStreamer>::Register(m_streamer.get());
        }

        void TearDown() override
        {
            m_serializeContext.reset();

            Interface<IO::IStreamer>::Unregister(m_streamer.get());
            m_streamer.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
            ComponentApplicationBus::Handler::BusDisconnect();
        }

        template<typename Container>
        void ReserveAndFreeWithoutMemLeaks()
        {
            Container instance;

            GenericClassInfo* containerInfo = SerializeGenericTypeInfo<decltype(instance)>::GetGenericInfo();
            EXPECT_NE(nullptr, containerInfo);
            EXPECT_NE(nullptr, containerInfo->GetClassData());
            SerializeContext::IDataContainer* container = containerInfo->GetClassData()->m_container;
            EXPECT_NE(nullptr, container);

            SerializeContext::IEventHandler* eventHandler = containerInfo->GetClassData()->m_eventHandler;
            if (eventHandler)
            {
                eventHandler->OnWriteBegin(&container);
            }

            void* element = container->ReserveElement(&instance, nullptr);
            EXPECT_NE(nullptr, element);
            *reinterpret_cast<float*>(element) = 42.0f;
            container->FreeReservedElement(&instance, element, nullptr);

            if (eventHandler)
            {
                eventHandler->OnWriteEnd(&container);
            }
        }

    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::IO::Streamer> m_streamer;
    };

    /*
    * Test serialization of built-in types
    */
    class SerializeBasicTest
        : public Serialization
    {
    protected:
        enum ClassicEnum
        {
            CE_A = 0,
            CR_B = 1,
        };
        enum class ClassEnum : char
        {
            A = 0,
            B = 1,
        };

        AZStd::unique_ptr<SerializeContext> m_context;

        char            m_char;
        short           m_short;
        int             m_int;
        long            m_long;
        AZ::s64         m_s64;
        unsigned char   m_uchar;
        unsigned short  m_ushort;
        unsigned int    m_uint;
        unsigned long   m_ulong;
        AZ::u64         m_u64;
        float           m_float;
        double          m_double;
        bool            m_true;
        bool            m_false;

        // Math
        AZ::Uuid        m_uuid;
        Vector2         m_vector2;
        Vector3         m_vector3;
        Vector4         m_vector4;

        Transform       m_transform;
        Matrix3x3       m_matrix3x3;
        Matrix4x4       m_matrix4x4;

        Quaternion      m_quaternion;

        Aabb            m_aabb;
        Plane           m_plane;

        ClassicEnum     m_classicEnum;
        ClassEnum       m_classEnum;

    public:
        void SetUp() override
        {
            Serialization::SetUp();

            m_context.reset(aznew SerializeContext());
        }

        void TearDown() override
        {
            m_context.reset();

            Serialization::TearDown();
        }

        void OnLoadedClassReady(void* classPtr, const Uuid& classId, int* callCount)
        {
            switch ((*callCount)++)
            {
            case 0:
                EXPECT_EQ( SerializeTypeInfo<char>::GetUuid(), classId );
                EXPECT_EQ( m_char, *reinterpret_cast<char*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, char);
                break;
            case 1:
                EXPECT_EQ( SerializeTypeInfo<short>::GetUuid(), classId );
                EXPECT_EQ( m_short, *reinterpret_cast<short*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, short);
                break;
            case 2:
                EXPECT_EQ( SerializeTypeInfo<int>::GetUuid(), classId );
                EXPECT_EQ( m_int, *reinterpret_cast<int*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, int);
                break;
            case 3:
                EXPECT_EQ( SerializeTypeInfo<long>::GetUuid(), classId );
                EXPECT_EQ( m_long, *reinterpret_cast<long*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, long);
                break;
            case 4:
                EXPECT_EQ( SerializeTypeInfo<AZ::s64>::GetUuid(), classId );
                EXPECT_EQ( m_s64, *reinterpret_cast<AZ::s64*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, AZ::s64);
                break;
            case 5:
                EXPECT_EQ( SerializeTypeInfo<unsigned char>::GetUuid(), classId );
                EXPECT_EQ( m_uchar, *reinterpret_cast<unsigned char*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, unsigned char);
                break;
            case 6:
                EXPECT_EQ( SerializeTypeInfo<unsigned short>::GetUuid(), classId );
                EXPECT_EQ( m_ushort, *reinterpret_cast<unsigned short*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, unsigned short);
                break;
            case 7:
                EXPECT_EQ( SerializeTypeInfo<unsigned int>::GetUuid(), classId );
                EXPECT_EQ( m_uint, *reinterpret_cast<unsigned int*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, unsigned int);
                break;
            case 8:
                EXPECT_EQ( SerializeTypeInfo<unsigned long>::GetUuid(), classId );
                EXPECT_EQ( m_ulong, *reinterpret_cast<unsigned long*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, unsigned long);
                break;
            case 9:
                EXPECT_EQ( SerializeTypeInfo<AZ::u64>::GetUuid(), classId );
                EXPECT_EQ( m_u64, *reinterpret_cast<AZ::u64*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, AZ::u64);
                break;
            case 10:
                EXPECT_EQ( SerializeTypeInfo<float>::GetUuid(), classId );
                EXPECT_TRUE(fabsf(*reinterpret_cast<float*>(classPtr) - m_float) < 0.001f);
                azdestroy(classPtr, AZ::SystemAllocator, float);
                break;
            case 11:
                EXPECT_EQ( SerializeTypeInfo<double>::GetUuid(), classId );
                EXPECT_TRUE(fabs(*reinterpret_cast<double*>(classPtr) - m_double) < 0.00000001);
                azdestroy(classPtr, AZ::SystemAllocator, double);
                break;
            case 12:
                EXPECT_EQ( SerializeTypeInfo<bool>::GetUuid(), classId );
                EXPECT_EQ( m_true, *reinterpret_cast<bool*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, bool);
                break;
            case 13:
                EXPECT_EQ( SerializeTypeInfo<bool>::GetUuid(), classId );
                EXPECT_EQ( m_false, *reinterpret_cast<bool*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, bool);
                break;
            case 14:
                EXPECT_EQ( SerializeTypeInfo<AZ::Uuid>::GetUuid(), classId );
                EXPECT_EQ( m_uuid, *reinterpret_cast<AZ::Uuid*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, AZ::Uuid);
                break;
            case 15:
                EXPECT_EQ( SerializeTypeInfo<AZ::Vector2>::GetUuid(), classId );
                EXPECT_TRUE(reinterpret_cast<AZ::Vector2*>(classPtr)->IsClose(m_vector2, Constants::FloatEpsilon));
                azdestroy(classPtr, AZ::SystemAllocator, AZ::Vector2);
                break;
            case 16:
                EXPECT_EQ( SerializeTypeInfo<AZ::Vector3>::GetUuid(), classId );
                EXPECT_TRUE(reinterpret_cast<AZ::Vector3*>(classPtr)->IsClose(m_vector3, Constants::FloatEpsilon));
                azdestroy(classPtr, AZ::SystemAllocator, AZ::Vector3);
                break;
            case 17:
                EXPECT_EQ( SerializeTypeInfo<AZ::Vector4>::GetUuid(), classId );
                EXPECT_TRUE(reinterpret_cast<AZ::Vector4*>(classPtr)->IsClose(m_vector4, Constants::FloatEpsilon));
                azdestroy(classPtr, AZ::SystemAllocator, AZ::Vector4);
                break;
            case 18:
                EXPECT_EQ( SerializeTypeInfo<AZ::Transform>::GetUuid(), classId );
                EXPECT_TRUE(reinterpret_cast<AZ::Transform*>(classPtr)->IsClose(m_transform, Constants::FloatEpsilon));
                azdestroy(classPtr, AZ::SystemAllocator, AZ::Transform);
                break;
            case 19:
                EXPECT_EQ( SerializeTypeInfo<AZ::Matrix3x3>::GetUuid(), classId );
                EXPECT_TRUE(reinterpret_cast<AZ::Matrix3x3*>(classPtr)->IsClose(m_matrix3x3, Constants::FloatEpsilon));
                azdestroy(classPtr, AZ::SystemAllocator, AZ::Matrix3x3);
                break;
            case 20:
                EXPECT_EQ( SerializeTypeInfo<AZ::Matrix4x4>::GetUuid(), classId );
                EXPECT_TRUE(reinterpret_cast<AZ::Matrix4x4*>(classPtr)->IsClose(m_matrix4x4, Constants::FloatEpsilon));
                azdestroy(classPtr, AZ::SystemAllocator, AZ::Matrix4x4);
                break;
            case 21:
                EXPECT_EQ( SerializeTypeInfo<AZ::Quaternion>::GetUuid(), classId );
                EXPECT_TRUE(reinterpret_cast<AZ::Quaternion*>(classPtr)->IsClose(m_quaternion, Constants::FloatEpsilon));
                azdestroy(classPtr, AZ::SystemAllocator, AZ::Quaternion);
                break;
            case 22:
                EXPECT_EQ( SerializeTypeInfo<AZ::Aabb>::GetUuid(), classId );
                EXPECT_TRUE(reinterpret_cast<AZ::Aabb*>(classPtr)->GetMin().IsClose(m_aabb.GetMin(), Constants::FloatEpsilon));
                EXPECT_TRUE(reinterpret_cast<AZ::Aabb*>(classPtr)->GetMax().IsClose(m_aabb.GetMax(), Constants::FloatEpsilon));
                azdestroy(classPtr, AZ::SystemAllocator, AZ::Aabb);
                break;
            case 23:
                EXPECT_EQ( SerializeTypeInfo<AZ::Plane>::GetUuid(), classId );
                EXPECT_TRUE(reinterpret_cast<AZ::Plane*>(classPtr)->GetPlaneEquationCoefficients().IsClose(m_plane.GetPlaneEquationCoefficients(), Constants::FloatEpsilon));
                azdestroy(classPtr, AZ::SystemAllocator, AZ::Plane);
                break;
            case 24:
                EXPECT_EQ( SerializeTypeInfo<ClassicEnum>::GetUuid(), classId );
                EXPECT_EQ( CE_A, *reinterpret_cast<ClassicEnum*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, ClassicEnum);
                break;
            case 25:
                EXPECT_EQ( SerializeTypeInfo<ClassEnum>::GetUuid(), classId );
                EXPECT_EQ( ClassEnum::B, *reinterpret_cast<ClassEnum*>(classPtr) );
                azdestroy(classPtr, AZ::SystemAllocator, ClassEnum);
                break;
            }
        }

        void SaveObjects(ObjectStream* writer)
        {
            bool success = true;

            success = writer->WriteClass(&m_char);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_short);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_int);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_long);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_s64);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_uchar);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_ushort);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_uint);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_ulong);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_u64);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_float);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_double);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_true);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_false);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_uuid);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_vector2);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_vector3);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_vector4);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_transform);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_matrix3x3);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_matrix4x4);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_quaternion);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_aabb);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_plane);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_classicEnum);
            EXPECT_TRUE(success);
            success = writer->WriteClass(&m_classEnum);
            EXPECT_TRUE(success);
        }

        void OnDone(ObjectStream::Handle handle, bool success, bool* done)
        {
            (void)handle;
            EXPECT_TRUE(success);
            *done = true;
        }

        void TestSave(IO::GenericStream* stream, ObjectStream::StreamType format)
        {
            ObjectStream* objStream = ObjectStream::Create(stream, *m_context, format);
            SaveObjects(objStream);
            bool done = objStream->Finalize();
            EXPECT_TRUE(done);
        }

        void TestLoad(IO::GenericStream* stream)
        {
            int cbCount = 0;
            bool done = false;
            ObjectStream::ClassReadyCB readyCB(AZStd::bind(&SerializeBasicTest::OnLoadedClassReady, this, AZStd::placeholders::_1, AZStd::placeholders::_2, &cbCount));
            ObjectStream::CompletionCB doneCB(AZStd::bind(&SerializeBasicTest::OnDone, this, AZStd::placeholders::_1, AZStd::placeholders::_2, &done));
            ObjectStream::LoadBlocking(stream, *m_context, readyCB);
            EXPECT_EQ( 26, cbCount );
        }
    };

    namespace AdvancedTest
    {
        class EmptyClass
        {
        public:
            AZ_CLASS_ALLOCATOR(EmptyClass, SystemAllocator, 0);
            AZ_TYPE_INFO(EmptyClass, "{7B2AA956-80A9-4996-B750-7CE8F7F79A29}")

                EmptyClass()
                : m_data(101)
            {
            }

            static void Reflect(SerializeContext& context)
            {
                context.Class<EmptyClass>()
                    ->Version(1)
                    ->SerializeWithNoData();
            }

            int m_data;
        };

        // We don't recommend using this pattern as it can be tricky to track why some objects are stored, we
        // wecommend that you have fully symetrical save/load.
        class ConditionalSave
        {
        public:
            AZ_CLASS_ALLOCATOR(ConditionalSave, SystemAllocator, 0);
            AZ_TYPE_INFO(ConditionalSave, "{E1E6910F-C029-492A-8163-026F6F69FC53}");

            ConditionalSave()
                : m_doSave(true)
                , m_data(201)
            {
            }

            static void Reflect(SerializeContext& context)
            {
                context.Class<ConditionalSave>()->
                    Version(1)->
                    SerializerDoSave([](const void* instance) { return reinterpret_cast<const ConditionalSave*>(instance)->m_doSave; })->
                    Field("m_data", &ConditionalSave::m_data);
            }

            bool m_doSave;
            int m_data;
        };
    }

    namespace ContainersTest
    {
        struct ContainersStruct
        {
            AZ_TYPE_INFO(ContainersStruct, "{E88A592D-5221-49DE-9DFD-6E25B39C65C7}");
            AZ_CLASS_ALLOCATOR(ContainersStruct, AZ::SystemAllocator, 0);
            AZStd::vector<int>                  m_vector;
            AZStd::fixed_vector<int, 5>         m_fixedVector;
            AZStd::array<int, 5>                m_array;
            AZStd::list<int>                    m_list;
            AZStd::forward_list<int>            m_forwardList;
            AZStd::unordered_set<int>           m_unorderedSet;
            AZStd::unordered_map<int, float>    m_unorderedMap;
            AZStd::bitset<10>                   m_bitset;
        };

        struct AssociativePtrContainer
        {
            AZ_TYPE_INFO(AssociativePtrContainer, "{02223E23-9B9C-4196-84C2-77D3A57BFF87}");
            AZ_CLASS_ALLOCATOR(AssociativePtrContainer, AZ::SystemAllocator, 0);

            static void Reflect(SerializeContext& serializeContext)
            {
                serializeContext.Class<AssociativePtrContainer>()
                    ->Field("m_setOfPointers", &AssociativePtrContainer::m_setOfPointers)
                    ->Field("m_mapOfFloatPointers", &AssociativePtrContainer::m_mapOfFloatPointers)
                    ->Field("m_sharedEntityPointer", &AssociativePtrContainer::m_sharedEntityPointer)
                    ;
            }

            AZStd::unordered_set<AZ::Entity*> m_setOfPointers;
            AZStd::unordered_map<int, float*> m_mapOfFloatPointers;
            AZStd::shared_ptr<AZ::Entity> m_sharedEntityPointer;
        };

        void ReflectVectorOfInts(AZ::SerializeContext* serializeContext)
        {
            AZ::GenericClassInfo* genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<int>>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(serializeContext);
            }
            genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<int*>>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(serializeContext);
            }
        }
    }

    TEST_F(Serialization, ContainerTypeContainedTypeDiffersByPointer)
    {
        ContainersTest::ReflectVectorOfInts(m_serializeContext.get());
        AZStd::vector<int> vectorOfInts;
        AZStd::vector<int*> vectorOfIntPointers;

        vectorOfInts.push_back(5);
        vectorOfIntPointers.push_back(azcreate(int, (5), AZ::SystemAllocator, "Container Int Pointer"));

        // Write Vector of Int to object stream
        AZStd::vector<char> vectorIntBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > vectorIntStream(&vectorIntBuffer);
        {
            ObjectStream* objStream = ObjectStream::Create(&vectorIntStream, *m_serializeContext, ObjectStream::ST_XML);
            objStream->WriteClass(&vectorOfInts);
            objStream->Finalize();
        }

        AZStd::vector<char> vectorIntPointerBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > vectorIntPointerStream(&vectorIntPointerBuffer);
        {
            /*
             * The vectorIntPointerBuffer.data() function call should be examined in the debugger after this block
             * This will write out an the address of the integer 5 stored in the vectorOfIntPointers instead of 5 to the xml data
             */
            ObjectStream* objStream = ObjectStream::Create(&vectorIntPointerStream, *m_serializeContext, ObjectStream::ST_XML);
            objStream->WriteClass(&vectorOfIntPointers);
            objStream->Finalize();
        }

        vectorIntStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        vectorIntPointerStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AZStd::vector<int> loadIntVector;
        AZStd::vector<int*> loadIntPtrVector;
        bool loadResult = AZ::Utils::LoadObjectFromStreamInPlace(vectorIntStream, loadIntVector, m_serializeContext.get());
        EXPECT_TRUE(loadResult);
        loadResult = AZ::Utils::LoadObjectFromStreamInPlace(vectorIntPointerStream, loadIntPtrVector, m_serializeContext.get());
        EXPECT_TRUE(loadResult);

        /*
        * As the vector to int pointer class was reflected second, it would not get placed into the SerializeContext GenericClassInfoMap
        * Therefore the write of the AZStd::vector<int*> to vectorIntPointerStream will output bad data as it reinterpret_cast
        * the supplied AZStd::vector<int*> to an AZStd::vector<int>
        */
        ASSERT_EQ(1, loadIntVector.size());
        EXPECT_EQ(vectorOfInts[0], loadIntVector[0]);
        ASSERT_EQ(1, loadIntPtrVector.size());
        ASSERT_NE(nullptr, loadIntPtrVector[0]);
        EXPECT_NE(vectorOfIntPointers[0], loadIntPtrVector[0]);
        EXPECT_EQ(*vectorOfIntPointers[0], *loadIntPtrVector[0]);

        for (int* intPtr : vectorOfIntPointers)
        {
            azdestroy(intPtr);
        }

        for (int* intPtr : loadIntPtrVector)
        {
            // NOTE: This will crash if loadIntPtrVector uses the incorrect GenericClassInfo to serialize its data
            azdestroy(intPtr);
        }

        m_serializeContext->EnableRemoveReflection();
        ContainersTest::ReflectVectorOfInts(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
    }

#if AZ_TRAIT_DISABLE_FAILED_SERIALIZE_BASIC_TEST
TEST_F(SerializeBasicTest, DISABLED_BasicTypeTest_Succeed)
#else
TEST_F(SerializeBasicTest, BasicTypeTest_Succeed)
#endif
    {
        m_char = -1;
        m_short = -2;
        m_int = -3;
        m_long = -4;
        m_s64 = -5;
        m_uchar = 1;
        m_ushort = 2;
        m_uint = 3;
        m_ulong = 4;
        m_u64 = 5;
        m_float = 2.f;
        m_double = 20.0000005;
        m_true = true;
        m_false = false;

        // Math
        m_uuid = AZ::Uuid::CreateString("{16490FB4-A7CE-4a8a-A882-F98DDA6A788F}");
        m_vector2 = Vector2(1.0f, 2.0f);
        m_vector3 = Vector3(3.0f, 4.0f, 5.0f);
        m_vector4 = Vector4(6.0f, 7.0f, 8.0f, 9.0f);

        m_quaternion = Quaternion::CreateRotationZ(0.7f);
        m_transform = Transform::CreateRotationX(1.1f);
        m_matrix3x3 = Matrix3x3::CreateRotationY(0.5f);
        m_matrix4x4 = Matrix4x4::CreateFromQuaternionAndTranslation(m_quaternion, m_vector3);

        m_aabb.Set(-m_vector3, m_vector3);
        m_plane.Set(m_vector4);

        m_classicEnum = CE_A;
        m_classEnum = ClassEnum::B;

        TestFileIOBase fileIO;
        SetRestoreFileIOBaseRAII restoreFileIOScope(fileIO);

#if AZ_TRAIT_TEST_APPEND_ROOT_FOLDER_TO_PATH
        AZ::IO::Path serializeTestFilePath(AZ_TRAIT_TEST_ROOT_FOLDER);
#else
        AZ::IO::Path serializeTestFilePath;
#endif
        // XML version
        AZ::IO::Path testXmlFilePath = serializeTestFilePath / "serializebasictest.xml";
        {
            AZ_TracePrintf("SerializeBasicTest", "\nWriting as XML...\n");
            IO::FileIOStream stream(testXmlFilePath.c_str(), IO::OpenMode::ModeWrite);
            TestSave(&stream, ObjectStream::ST_XML);
        }
        {
            AZ_TracePrintf("SerializeBasicTest", "Loading as XML...\n");
            IO::FileIOStream stream(testXmlFilePath.c_str(), IO::OpenMode::ModeRead);
            TestLoad(&stream);
        }

        // JSON version
        AZ::IO::Path testJsonFilePath = serializeTestFilePath / "serializebasictest.json";
        {
            AZ_TracePrintf("SerializeBasicTest", "\nWriting as JSON...\n");
            IO::FileIOStream stream(testJsonFilePath.c_str(), IO::OpenMode::ModeWrite);
            TestSave(&stream, ObjectStream::ST_JSON);
        }
        {
            AZ_TracePrintf("SerializeBasicTest", "Loading as JSON...\n");
            IO::FileIOStream stream(testJsonFilePath.c_str(), IO::OpenMode::ModeRead);
            TestLoad(&stream);
        }

        // Binary version
        AZ::IO::Path testBinFilePath = serializeTestFilePath / "serializebasictest.bin";
        {
            AZ_TracePrintf("SerializeBasicTest", "Writing as Binary...\n");
            IO::FileIOStream stream(testBinFilePath.c_str(), IO::OpenMode::ModeWrite);
            TestSave(&stream, ObjectStream::ST_BINARY);
        }
        {
            AZ_TracePrintf("SerializeBasicTest", "Loading as Binary...\n");
            IO::FileIOStream stream(testBinFilePath.c_str(), IO::OpenMode::ModeRead);
            TestLoad(&stream);
        }
    }
    /*
    * Test serialization of built-in container types
    */
    TEST_F(Serialization, ContainersTest)
    {
        using namespace ContainersTest;
        class ContainersTest
        {
        public:
            void VerifyLoad(void* classPtr, const Uuid& classId, ContainersStruct* controlData)
            {
                EXPECT_EQ( SerializeTypeInfo<ContainersStruct>::GetUuid(), classId );
                ContainersStruct* data = reinterpret_cast<ContainersStruct*>(classPtr);
                EXPECT_EQ( controlData->m_vector, data->m_vector );
                EXPECT_EQ( controlData->m_fixedVector, data->m_fixedVector );
                EXPECT_EQ( controlData->m_array[0], data->m_array[0] );
                EXPECT_EQ( controlData->m_array[1], data->m_array[1] );
                EXPECT_EQ( controlData->m_list, data->m_list );
                EXPECT_EQ( controlData->m_forwardList, data->m_forwardList );
                EXPECT_EQ( controlData->m_unorderedSet.size(), data->m_unorderedSet.size() );
                for (AZStd::unordered_set<int>::const_iterator it = data->m_unorderedSet.begin(), ctrlIt = controlData->m_unorderedSet.begin(); it != data->m_unorderedSet.end(); ++it, ++ctrlIt)
                {
                    EXPECT_EQ( *ctrlIt, *it );
                }
                EXPECT_EQ( controlData->m_unorderedMap.size(), data->m_unorderedMap.size() );
                for (AZStd::unordered_map<int, float>::const_iterator it = data->m_unorderedMap.begin(), ctrlIt = controlData->m_unorderedMap.begin(); it != data->m_unorderedMap.end(); ++it, ++ctrlIt)
                {
                    EXPECT_EQ( *ctrlIt, *it );
                }
                EXPECT_EQ( controlData->m_bitset, data->m_bitset );
                delete data;
            }

            void run()
            {
                SerializeContext serializeContext;
                serializeContext.Class<ContainersStruct>()
                    ->Field("m_vector", &ContainersStruct::m_vector)
                    ->Field("m_fixedVector", &ContainersStruct::m_fixedVector)
                    ->Field("m_array", &ContainersStruct::m_array)
                    ->Field("m_list", &ContainersStruct::m_list)
                    ->Field("m_forwardList", &ContainersStruct::m_forwardList)
                    ->Field("m_unorderedSet", &ContainersStruct::m_unorderedSet)
                    ->Field("m_unorderedMap", &ContainersStruct::m_unorderedMap)
                    ->Field("m_bitset", &ContainersStruct::m_bitset);

                ContainersStruct testData;
                testData.m_vector.push_back(1);
                testData.m_vector.push_back(2);
                testData.m_fixedVector.push_back(3);
                testData.m_fixedVector.push_back(4);
                testData.m_array[0] = 5;
                testData.m_array[1] = 6;
                testData.m_list.push_back(7);
                testData.m_list.push_back(8);
                testData.m_forwardList.push_back(9);
                testData.m_forwardList.push_back(10);
                testData.m_unorderedSet.insert(11);
                testData.m_unorderedSet.insert(12);
                testData.m_unorderedMap.insert(AZStd::make_pair(13, 13.f));
                testData.m_unorderedMap.insert(AZStd::make_pair(14, 14.f));
                testData.m_bitset.set(0);
                testData.m_bitset.set(9);

                // XML
                AZStd::vector<char> xmlBuffer;
                IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBuffer);
                ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, serializeContext, ObjectStream::ST_XML);
                xmlObjStream->WriteClass(&testData);
                xmlObjStream->Finalize();

                AZ::IO::SystemFile tmpOut;
                tmpOut.Open("SerializeContainersTest.xml", AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
                tmpOut.Write(xmlStream.GetData()->data(), xmlStream.GetLength());
                tmpOut.Close();

                xmlStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
                ObjectStream::ClassReadyCB readyCB(AZStd::bind(&ContainersTest::VerifyLoad, this, AZStd::placeholders::_1, AZStd::placeholders::_2, &testData));
                ObjectStream::LoadBlocking(&xmlStream, serializeContext, readyCB);
            }
        };

        ContainersTest test;
        test.run();
    }

    TEST_F(Serialization, AssociativeContainerPtrTest)
    {
        using namespace ContainersTest;

        // We must expose the class for serialization first.
        AZ::Entity::Reflect(m_serializeContext.get());
        AssociativePtrContainer::Reflect(*m_serializeContext);

        AssociativePtrContainer testObj;
        testObj.m_setOfPointers.insert(aznew AZ::Entity("Entity1"));
        testObj.m_setOfPointers.insert(aznew AZ::Entity("Entity2"));
        testObj.m_mapOfFloatPointers.emplace(5, azcreate(float, (3.14f), AZ::SystemAllocator, "Bob the Float"));
        testObj.m_sharedEntityPointer.reset(aznew AZ::Entity("Entity3"));

        // XML
        AZStd::vector<char> xmlBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBuffer);
        ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, *m_serializeContext, ObjectStream::ST_XML);
        xmlObjStream->WriteClass(&testObj);
        xmlObjStream->Finalize();

        xmlStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        bool result = ObjectStream::LoadBlocking(&xmlStream, *m_serializeContext, [&testObj](void* classPtr, const AZ::Uuid& classId, SerializeContext*)
        {
            EXPECT_EQ(SerializeTypeInfo<AssociativePtrContainer>::GetUuid(), classId);
            auto loadObj = reinterpret_cast<AssociativePtrContainer*>(classPtr);

            EXPECT_EQ(testObj.m_setOfPointers.size(), loadObj->m_setOfPointers.size());
            auto testObjSetBeginIt = testObj.m_setOfPointers.begin();
            auto loadObjSetBeginIt = loadObj->m_setOfPointers.begin();
            for (; testObjSetBeginIt != testObj.m_setOfPointers.end(); ++testObjSetBeginIt, ++loadObjSetBeginIt)
            {
                EXPECT_EQ((*testObjSetBeginIt)->GetId(), (*loadObjSetBeginIt)->GetId());
            }

            EXPECT_EQ(testObj.m_mapOfFloatPointers.size(), loadObj->m_mapOfFloatPointers.size());
            auto testObjMapBeginIt = testObj.m_mapOfFloatPointers.begin();
            auto loadObjMapBeginIt = loadObj->m_mapOfFloatPointers.begin();
            for (; testObjMapBeginIt != testObj.m_mapOfFloatPointers.end(); ++testObjMapBeginIt, ++loadObjMapBeginIt)
            {
                EXPECT_EQ(*(testObjMapBeginIt->second), *(loadObjMapBeginIt->second));
            }

            EXPECT_NE(nullptr, loadObj->m_sharedEntityPointer.get());
            EXPECT_EQ(testObj.m_sharedEntityPointer->GetId(), loadObj->m_sharedEntityPointer->GetId());

            //Free the allocated memory
            for (auto& entitySet : { testObj.m_setOfPointers, loadObj->m_setOfPointers })
            {
                for (AZ::Entity* entityPtr : entitySet)
                {
                    delete entityPtr;
                }
            }

            for (auto& intFloatPtrMap : { testObj.m_mapOfFloatPointers, loadObj->m_mapOfFloatPointers })
            {
                for (auto& intFloatPtrPair : intFloatPtrMap)
                {
                    azdestroy(intFloatPtrPair.second);
                }
            }
            delete loadObj;
        });

        EXPECT_TRUE(result);

    }


    /*
        This test will dynamic cast (azrtti_cast) between incompatible types, which should always result in nullptr.
        If this test fails, the RTTI declaration for the relevant type is incorrect.
    */
    TEST_F(Serialization, AttributeRTTI)
    {
        {
            AttributeInvocable<AZStd::function<AZStd::string(AZStd::string)>> fn([](AZStd::string x) { return x + x; });
            Attribute* fnDownCast = &fn;
            auto fnUpCast = azrtti_cast<AttributeInvocable<AZStd::function<int(int)>>*>(fnDownCast);
            EXPECT_EQ(fnUpCast, nullptr);
        }

        {
            AttributeFunction<AZStd::string(AZStd::string)> fn([](AZStd::string x) { return x + x; });
            Attribute* fnDownCast = &fn;
            auto fnUpCast = azrtti_cast<AttributeFunction<int(int)>*>(fnDownCast);
            EXPECT_EQ(fnUpCast, nullptr);
        }
    }

    /*
    * Deprecation
    */

    namespace Deprecation
    {
        struct DeprecatedClass
        {
            AZ_CLASS_ALLOCATOR(DeprecatedClass, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(DeprecatedClass, "{893CA46E-6D1A-4D27-94F7-09E26DE5AE4B}")

                DeprecatedClass()
                : m_data(0) {}

            int m_data;
        };

        struct DeprecationTestClass
        {
            AZ_CLASS_ALLOCATOR(DeprecationTestClass, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(DeprecationTestClass, "{54E27F53-EF3F-4436-9378-E9AF56A9FA4C}")

                DeprecationTestClass()
                : m_deprecatedPtr(nullptr)
                , m_oldClassData(0)
                , m_newClassData(0.f)
                , m_missingMember(0)
                , m_data(0)
            {}

            ~DeprecationTestClass() { Clear(); }

            void Clear()
            {
                if (m_deprecatedPtr)
                {
                    delete m_deprecatedPtr;
                    m_deprecatedPtr = nullptr;
                }
            }

            DeprecatedClass     m_deprecated;
            DeprecatedClass*    m_deprecatedPtr;
            int                 m_oldClassData;
            float               m_newClassData;
            int                 m_missingMember;
            int                 m_data;
        };

        struct SimpleBaseClass
        {
            AZ_CLASS_ALLOCATOR(SimpleBaseClass, AZ::SystemAllocator, 0);
            AZ_RTTI(SimpleBaseClass, "{829F6E24-AAEF-4C97-9003-0BC22CB64786}")

                SimpleBaseClass()
                : m_data(0.f) {}
            virtual ~SimpleBaseClass() {}

            float m_data;
        };

        struct SimpleDerivedClass1 : public SimpleBaseClass
        {
            AZ_CLASS_ALLOCATOR(SimpleDerivedClass1, AZ::SystemAllocator, 0);
            AZ_RTTI(SimpleDerivedClass1, "{78632262-C303-49BC-ABAD-88B088098311}", SimpleBaseClass)

                SimpleDerivedClass1() {}
        };

        struct SimpleDerivedClass2 : public SimpleBaseClass
        {
            AZ_CLASS_ALLOCATOR(SimpleDerivedClass2, AZ::SystemAllocator, 0);
            AZ_RTTI(SimpleDerivedClass2, "{4932DF7C-0482-4846-AAE5-BED7D03F9E02}", SimpleBaseClass)

                SimpleDerivedClass2() {}
        };

        struct OwnerClass
        {
            AZ_CLASS_ALLOCATOR(OwnerClass, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(OwnerClass, "{3F305C77-4BE1-49E6-9C51-9F1284F18CCE}");

            OwnerClass() {}
            SimpleBaseClass* m_pointer = nullptr;
        };
    }

    TEST_F(Serialization, TestDeprecatedClassAtRootLevel_Succeeds)
    {
        using namespace Deprecation;
        // Test a deprecated class at the root level.
        SerializeContext sc;

        SimpleDerivedClass1 simpleDerivedClass1;
        sc.Class<SimpleBaseClass>()
            ->Version(1)
            ->Field("m_data", &SimpleBaseClass::m_data);
        sc.Class<SimpleDerivedClass1, SimpleBaseClass>()
            ->Version(1);
        sc.Class<SimpleDerivedClass2, SimpleBaseClass>()
            ->Version(1);

        AZStd::vector<char> xmlBufferRootTest;
        AZStd::vector<char> jsonBufferRootTest;
        AZStd::vector<char> binaryBufferRootTest;

        {
            // XML
            IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBufferRootTest);
            AZ_TracePrintf("SerializeDeprecationTest", "Writing XML\n");
            ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, sc, ObjectStream::ST_XML);
            xmlObjStream->WriteClass(&simpleDerivedClass1);
            bool success = xmlObjStream->Finalize();
            EXPECT_TRUE(success);

            // JSON
            IO::ByteContainerStream<AZStd::vector<char> > jsonStream(&jsonBufferRootTest);
            AZ_TracePrintf("SerializeDeprecationTest", "Writing JSON\n");
            ObjectStream* jsonObjStream = ObjectStream::Create(&jsonStream, sc, ObjectStream::ST_JSON);
            jsonObjStream->WriteClass(&simpleDerivedClass1);
            success = jsonObjStream->Finalize();
            EXPECT_TRUE(success);

            // Binary
            IO::ByteContainerStream<AZStd::vector<char> > binaryStream(&binaryBufferRootTest);
            AZ_TracePrintf("SerializeDeprecationTest", "Writing Binary\n");
            ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
            binaryObjStream->WriteClass(&simpleDerivedClass1);
            success = binaryObjStream->Finalize();
            EXPECT_TRUE(success);
        }

        sc.EnableRemoveReflection();
        sc.Class<SimpleDerivedClass1>();
        sc.DisableRemoveReflection();

        AZ::SerializeContext::VersionConverter converter = [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement) -> bool
        {
            return classElement.Convert<SimpleDerivedClass2>(context);
        };

        sc.ClassDeprecate("SimpleDerivedClass1", "{78632262-C303-49BC-ABAD-88B088098311}", converter);

        auto cb = [](void* classPtr, const Uuid& classId, SerializeContext* /*context*/) -> void
        {
            EXPECT_EQ(AzTypeInfo<SimpleDerivedClass2>::Uuid(), classId);
            delete static_cast<SimpleDerivedClass2*>(classPtr);
        };

        ObjectStream::ClassReadyCB readyCBTest(cb);

        // XML
        AZ_TracePrintf("SerializeDeprecationTest", "Loading XML with deprecated class\n");
        IO::ByteContainerStream<const AZStd::vector<char> > xmlStreamUuidTest(&xmlBufferRootTest);
        xmlStreamUuidTest.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        EXPECT_TRUE(ObjectStream::LoadBlocking(&xmlStreamUuidTest, sc, readyCBTest));

        // JSON
        AZ_TracePrintf("SerializeDeprecationTest", "Loading JSON with deprecated class\n");
        IO::ByteContainerStream<const AZStd::vector<char> > jsonStream(&jsonBufferRootTest);
        jsonStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        ObjectStream::LoadBlocking(&jsonStream, sc, readyCBTest);

        // Binary
        AZ_TracePrintf("SerializeDeprecationTest", "Loading Binary with deprecated class\n");
        IO::ByteContainerStream<const AZStd::vector<char> > binaryStream(&binaryBufferRootTest);
        binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        ObjectStream::LoadBlocking(&binaryStream, sc, readyCBTest);
    }

    TEST_F(Serialization, DeprecationRulesTest)
    {
        using namespace Deprecation;
        class DeprecationTest
        {
        public:
            DeprecatedClass m_deprecated;
            DeprecationTestClass m_deprecationTestClass;

            void WriteDeprecated(ObjectStream* writer)
            {
                bool success = writer->WriteClass(&m_deprecated);
                EXPECT_TRUE(success);
            }

            void WriteDeprecationTestClass(ObjectStream* writer)
            {
                bool success = writer->WriteClass(&m_deprecationTestClass);
                EXPECT_TRUE(success);
            }

            void CheckDeprecated(void* classPtr, const Uuid& classId)
            {
                (void)classPtr;
                (void)classId;
                // We should never hit here since our class was deprecated
                EXPECT_TRUE(false);
            }

            void CheckMemberDeprecation(void* classPtr, const Uuid& classId)
            {
                (void)classId;
                DeprecationTestClass* obj = reinterpret_cast<DeprecationTestClass*>(classPtr);
                EXPECT_EQ( 0, obj->m_deprecated.m_data );
                EXPECT_EQ( nullptr, obj->m_deprecatedPtr );
                EXPECT_EQ( 0, obj->m_oldClassData );
                EXPECT_EQ( 0.f, obj->m_newClassData );
                EXPECT_EQ( 0, obj->m_missingMember );
                EXPECT_EQ( m_deprecationTestClass.m_data, obj->m_data );
                delete obj;
            }

            void run()
            {
                m_deprecated.m_data = 10;
                m_deprecationTestClass.m_deprecated.m_data = 10;
                m_deprecationTestClass.m_deprecatedPtr = aznew DeprecatedClass;
                m_deprecationTestClass.m_oldClassData = 10;
                m_deprecationTestClass.m_missingMember = 10;
                m_deprecationTestClass.m_data = 10;

                // Test new version without conversion.
                //  -Member types without reflection should be silently dropped.
                //  -Members whose reflection data don't match should be silently dropped.
                //  -Members whose names don't match should be silently dropped.
                //  -The converted class itself should still be accepted.
                AZ_TracePrintf("SerializeDeprecationTest", "\nTesting dropped/deprecated members:\n");
                {
                    // Write original data
                    AZStd::vector<char> xmlBuffer;
                    AZStd::vector<char> jsonBuffer;
                    AZStd::vector<char> binaryBuffer;
                    {
                        SerializeContext sc;
                        sc.Class<DeprecatedClass>()
                            ->Field("m_data", &DeprecatedClass::m_data);
                        sc.Class<DeprecationTestClass>()
                            ->Field("m_deprecated", &DeprecationTestClass::m_deprecated)
                            ->Field("m_deprecatedPtr", &DeprecationTestClass::m_deprecatedPtr)
                            ->Field("m_oldClassData", &DeprecationTestClass::m_oldClassData)
                            ->Field("m_missingMember", &DeprecationTestClass::m_missingMember)
                            ->Field("m_data", &DeprecationTestClass::m_data);

                        // XML
                        IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBuffer);
                        AZ_TracePrintf("SerializeDeprecationTest", "Writing XML\n");
                        ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, sc, ObjectStream::ST_XML);
                        WriteDeprecationTestClass(xmlObjStream);
                        bool success = xmlObjStream->Finalize();
                        EXPECT_TRUE(success);

                        // JSON
                        IO::ByteContainerStream<AZStd::vector<char> > jsonStream(&jsonBuffer);
                        AZ_TracePrintf("SerializeDeprecationTest", "Writing JSON\n");
                        ObjectStream* jsonObjStream = ObjectStream::Create(&jsonStream, sc, ObjectStream::ST_JSON);
                        WriteDeprecationTestClass(jsonObjStream);
                        success = jsonObjStream->Finalize();
                        EXPECT_TRUE(success);

                        // Binary
                        IO::ByteContainerStream<AZStd::vector<char> > binaryStream(&binaryBuffer);
                        AZ_TracePrintf("SerializeDeprecationTest", "Writing Binary\n");
                        ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
                        WriteDeprecationTestClass(binaryObjStream);
                        success = binaryObjStream->Finalize();
                        EXPECT_TRUE(success);
                    }

                    ObjectStream::ClassReadyCB readyCB(AZStd::bind(&DeprecationTest::CheckMemberDeprecation, this, AZStd::placeholders::_1, AZStd::placeholders::_2));
                    // Test deprecation with one member class not reflected at all
                    {
                        SerializeContext sc;
                        sc.Class<DeprecationTestClass>()
                            ->Version(2)
                            ->Field("m_deprecated", &DeprecationTestClass::m_deprecated)
                            ->Field("m_deprecatedPtr", &DeprecationTestClass::m_deprecatedPtr)
                            ->Field("m_oldClassData", &DeprecationTestClass::m_newClassData)
                            ->Field("m_data", &DeprecationTestClass::m_data);

                        // XML
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading XML with dropped class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > xmlStream(&xmlBuffer);
                        xmlStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&xmlStream, sc, readyCB);

                        // JSON
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading JSON with dropped class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > jsonStream(&jsonBuffer);
                        jsonStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&jsonStream, sc, readyCB);

                        // Binary
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading Binary with dropped class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > binaryStream(&binaryBuffer);
                        binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&binaryStream, sc, readyCB);
                    }
                    // Test deprecation with one member class marked as deprecated
                    {
                        SerializeContext sc;
                        sc.ClassDeprecate("DeprecatedClass", "{893CA46E-6D1A-4D27-94F7-09E26DE5AE4B}");
                        sc.Class<DeprecationTestClass>()
                            ->Version(2)
                            ->Field("m_deprecated", &DeprecationTestClass::m_deprecated)
                            ->Field("m_deprecatedPtr", &DeprecationTestClass::m_deprecatedPtr)
                            ->Field("m_oldClassData", &DeprecationTestClass::m_newClassData)
                            ->Field("m_data", &DeprecationTestClass::m_data);

                        // XML
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading XML with deprecated class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > xmlStream(&xmlBuffer);
                        xmlStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&xmlStream, sc, readyCB);

                        // JSON
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading JSON with deprecated class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > jsonStream(&jsonBuffer);
                        jsonStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&jsonStream, sc, readyCB);

                        // Binary
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading Binary with deprecated class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > binaryStream(&binaryBuffer);
                        binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&binaryStream, sc, readyCB);
                    }

                    // Test deprecation with a converter to an entirely new type.
                    {
                        SerializeContext sc;

                        sc.Class<DeprecationTestClass>()
                            ->Version(2)
                            ->Field("m_deprecated", &DeprecationTestClass::m_deprecated)
                            ->Field("m_deprecatedPtr", &DeprecationTestClass::m_deprecatedPtr)
                            ->Field("m_oldClassData", &DeprecationTestClass::m_newClassData)
                            ->Field("m_data", &DeprecationTestClass::m_data);

                        AZ::SerializeContext::VersionConverter converter = [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement) -> bool
                        {
                            return classElement.Convert<DeprecationTestClass>(context);
                        };

                        sc.ClassDeprecate("DeprecatedClass", "{893CA46E-6D1A-4D27-94F7-09E26DE5AE4B}", converter);

                        // XML
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading XML with deprecated class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > xmlStream(&xmlBuffer);
                        xmlStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&xmlStream, sc, readyCB);

                        // JSON
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading JSON with deprecated class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > jsonStream(&jsonBuffer);
                        jsonStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&jsonStream, sc, readyCB);

                        // Binary
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading Binary with deprecated class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > binaryStream(&binaryBuffer);
                        binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&binaryStream, sc, readyCB);
                    }

                    // Test a converter that completely swaps uuid.
                    // This test should FAIL, because the uuid cannot be swapped in non-deprecation cases.
                    {
                        SerializeContext sc;

                        sc.Class<SimpleBaseClass>()
                            ->Version(1)
                            ->Field("m_data", &SimpleBaseClass::m_data);

                        AZ::SerializeContext::VersionConverter converter = [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement) -> bool
                        {
                            return classElement.Convert<SimpleBaseClass>(context);
                        };

                        sc.Class<DeprecationTestClass>()
                            ->Version(3, converter)
                            ->Field("m_oldClassData", &DeprecationTestClass::m_newClassData)
                            ->Field("m_data", &DeprecationTestClass::m_data);

                        // XML
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading XML with deprecated class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > xmlStream(&xmlBuffer);
                        xmlStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

                        // This should fail!
                        AZ_TEST_START_TRACE_SUPPRESSION;
                        ObjectStream::LoadBlocking(&xmlStream, sc, readyCB);
                        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                    }

                    // Test a converter that swaps uuid to a castable/compatible type in a deprecation converter.
                    {
                        SimpleDerivedClass1 simpleDerivedClass1;
                        OwnerClass ownerClass;
                        ownerClass.m_pointer = &simpleDerivedClass1;

                        SerializeContext sc;

                        sc.Class<SimpleBaseClass>()
                            ->Version(1)
                            ->Field("m_data", &SimpleBaseClass::m_data);
                        sc.Class<SimpleDerivedClass1, SimpleBaseClass>()
                            ->Version(1);
                        sc.Class<SimpleDerivedClass2, SimpleBaseClass>()
                            ->Version(1);
                        sc.Class<OwnerClass>()
                            ->Version(1)
                            ->Field("Pointer", &OwnerClass::m_pointer);

                        AZStd::vector<char> xmlBufferUuidTest;
                        AZStd::vector<char> jsonBufferUuidTest;
                        AZStd::vector<char> binaryBufferUuidTest;

                        {
                            // XML
                            IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBufferUuidTest);
                            AZ_TracePrintf("SerializeDeprecationTest", "Writing XML\n");
                            ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, sc, ObjectStream::ST_XML);
                            xmlObjStream->WriteClass(&ownerClass);
                            bool success = xmlObjStream->Finalize();
                            EXPECT_TRUE(success);

                            // JSON
                            IO::ByteContainerStream<AZStd::vector<char> > jsonStream(&jsonBufferUuidTest);
                            AZ_TracePrintf("SerializeDeprecationTest", "Writing JSON\n");
                            ObjectStream* jsonObjStream = ObjectStream::Create(&jsonStream, sc, ObjectStream::ST_JSON);
                            jsonObjStream->WriteClass(&ownerClass);
                            success = jsonObjStream->Finalize();
                            EXPECT_TRUE(success);

                            // Binary
                            IO::ByteContainerStream<AZStd::vector<char> > binaryStream(&binaryBufferUuidTest);
                            AZ_TracePrintf("SerializeDeprecationTest", "Writing Binary\n");
                            ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
                            binaryObjStream->WriteClass(&ownerClass);
                            success = binaryObjStream->Finalize();
                            EXPECT_TRUE(success);
                        }

                        sc.EnableRemoveReflection();
                        sc.Class<OwnerClass>();
                        sc.DisableRemoveReflection();

                        AZ::SerializeContext::VersionConverter converter = [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement) -> bool
                        {
                            const int idx = classElement.FindElement(AZ_CRC("Pointer", 0x320468a8));
                            classElement.GetSubElement(idx).Convert<SimpleDerivedClass2>(context);
                            return true;
                        };

                        sc.Class<OwnerClass>()
                            ->Version(2, converter)
                            ->Field("Pointer", &OwnerClass::m_pointer);

                        auto cb = [](void* classPtr, const Uuid& classId, SerializeContext* /*context*/) -> void
                        {
                            EXPECT_EQ( AzTypeInfo<OwnerClass>::Uuid(), classId );
                            EXPECT_TRUE(static_cast<OwnerClass*>(classPtr)->m_pointer);
                            EXPECT_EQ( AzTypeInfo<SimpleDerivedClass2>::Uuid(), static_cast<OwnerClass*>(classPtr)->m_pointer->RTTI_GetType() );
                            delete static_cast<OwnerClass*>(classPtr)->m_pointer;
                            delete static_cast<OwnerClass*>(classPtr);
                        };

                        ObjectStream::ClassReadyCB readyCBTest(cb);

                        // XML
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading XML with deprecated class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > xmlStreamUuidTest(&xmlBufferUuidTest);
                        xmlStreamUuidTest.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        EXPECT_TRUE(ObjectStream::LoadBlocking(&xmlStreamUuidTest, sc, readyCBTest));

                        // JSON
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading JSON with deprecated class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > jsonStream(&jsonBufferUuidTest);
                        jsonStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&jsonStream, sc, readyCBTest);

                        // Binary
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading Binary with deprecated class\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > binaryStream(&binaryBufferUuidTest);
                        binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&binaryStream, sc, readyCBTest);
                    }
                }

                // Test root objects of deprecated classes.
                //  -Classes reflected as deprecated should be silently dropped.
                AZ_TracePrintf("SerializeDeprecationTest", "Testing deprecated root objects:\n");
                {
                    AZStd::vector<char> xmlBuffer;
                    AZStd::vector<char> jsonBuffer;
                    AZStd::vector<char> binaryBuffer;
                    // Write original data
                    {
                        SerializeContext sc;
                        sc.Class<DeprecatedClass>()
                            ->Field("m_data", &DeprecatedClass::m_data);

                        // XML
                        AZ_TracePrintf("SerializeDeprecationTest", "Writing XML\n");
                        IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBuffer);
                        ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, sc, ObjectStream::ST_XML);
                        WriteDeprecated(xmlObjStream);
                        bool success = xmlObjStream->Finalize();
                        EXPECT_TRUE(success);

                        // JSON
                        AZ_TracePrintf("SerializeDeprecationTest", "Writing JSON\n");
                        IO::ByteContainerStream<AZStd::vector<char> > jsonStream(&jsonBuffer);
                        ObjectStream* jsonObjStream = ObjectStream::Create(&jsonStream, sc, ObjectStream::ST_JSON);
                        WriteDeprecated(jsonObjStream);
                        success = jsonObjStream->Finalize();
                        EXPECT_TRUE(success);

                        // Binary
                        AZ_TracePrintf("SerializeDeprecationTest", "Writing Binary\n");
                        IO::ByteContainerStream<AZStd::vector<char> > binaryStream(&binaryBuffer);
                        ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
                        WriteDeprecated(binaryObjStream);
                        success = binaryObjStream->Finalize();
                        EXPECT_TRUE(success);
                    }
                    // Test deprecation
                    {
                        SerializeContext sc;
                        sc.ClassDeprecate("DeprecatedClass", "{893CA46E-6D1A-4D27-94F7-09E26DE5AE4B}");

                        ObjectStream::ClassReadyCB readyCB(AZStd::bind(&DeprecationTest::CheckDeprecated, this, AZStd::placeholders::_1, AZStd::placeholders::_2));

                        // XML
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading XML with deprecated root object\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > xmlStream(&xmlBuffer);
                        xmlStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&xmlStream, sc, readyCB);

                        // JSON
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading JSON with deprecated root object\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > jsonStream(&jsonBuffer);
                        jsonStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&jsonStream, sc, readyCB);

                        // Binary
                        AZ_TracePrintf("SerializeDeprecationTest", "Loading Binary with deprecated root object\n");
                        IO::ByteContainerStream<const AZStd::vector<char> > binaryStream(&binaryBuffer);
                        binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
                        ObjectStream::LoadBlocking(&binaryStream, sc, readyCB);
                    }
                }

                m_deprecationTestClass.Clear();
            }
        };

        DeprecationTest test;
        test.run();
    }

    /*
    * Test complicated conversion
    */
    namespace Conversion
    {
        struct TestObj
        {
            AZ_TYPE_INFO(TestObj, "{6AE2EE4A-1DB8-41B7-B909-296A10CEF4EA}");
            AZ_CLASS_ALLOCATOR(TestObj, AZ::SystemAllocator, 0);
            TestObj() = default;
            Generics        m_dataOld;
            GenericsNew     m_dataNew;
        };
    }

    /*
    * Data Overlay Test
    */
    namespace DataOverlay
    {
        struct DataOverlayTestStruct
        {
            AZ_TYPE_INFO(DataOverlayTestStruct, "{AD843B4D-0D08-4CE0-99F9-7E4E1EAD5984}");
            AZ_CLASS_ALLOCATOR(DataOverlayTestStruct, AZ::SystemAllocator, 0);
            DataOverlayTestStruct()
                : m_int(0)
                , m_ptr(nullptr) {}

            int                     m_int;
            AZStd::vector<int>      m_intVector;
            DataOverlayTestStruct*  m_ptr;
        };
    }

    TEST_F(Serialization, DataOverlayTest)
    {
        using namespace DataOverlay;
        class DataOverlayTest
        {
            class DataOverlayProviderExample
                : public DataOverlayProviderBus::Handler
            {
            public:
                static DataOverlayProviderId    GetProviderId() { return AZ_CRC("DataOverlayProviderExample", 0x60dafdbd); }
                static u32                      GetIntToken() { return AZ_CRC("int_data", 0xd74868f3); }
                static u32                      GetVectorToken() { return AZ_CRC("vector_data", 0x0aca20c0); }
                static u32                      GetPointerToken() { return AZ_CRC("pointer_data", 0xa46a746e); }

                DataOverlayProviderExample()
                {
                    m_ptrData.m_int = 5;
                    m_ptrData.m_intVector.push_back(1);
                    m_ptrData.m_ptr = nullptr;

                    m_data.m_int = 3;
                    m_data.m_intVector.push_back(10);
                    m_data.m_intVector.push_back(20);
                    m_data.m_intVector.push_back(30);
                    m_data.m_ptr = &m_ptrData;
                }

                void FillOverlayData(DataOverlayTarget* dest, const DataOverlayToken& dataToken) override
                {
                    if (*reinterpret_cast<const u32*>(dataToken.m_dataUri.data()) == GetIntToken())
                    {
                        dest->SetData(m_data.m_int);
                    }
                    else if (*reinterpret_cast<const u32*>(dataToken.m_dataUri.data()) == GetVectorToken())
                    {
                        dest->SetData(m_data.m_intVector);
                    }
                    else if (*reinterpret_cast<const u32*>(dataToken.m_dataUri.data()) == GetPointerToken())
                    {
                        dest->SetData(*m_data.m_ptr);
                    }
                }

                DataOverlayTestStruct   m_data;
                DataOverlayTestStruct   m_ptrData;
            };

            class DataOverlayInstanceEnumeratorExample
                : public DataOverlayInstanceBus::Handler
            {
            public:
                enum InstanceType
                {
                    Type_Int,
                    Type_Vector,
                    Type_Pointer,
                };

                DataOverlayInstanceEnumeratorExample(InstanceType type)
                    : m_type(type) {}

                ~DataOverlayInstanceEnumeratorExample() override
                {
                    BusDisconnect();
                }

                DataOverlayInfo GetOverlayInfo() override
                {
                    DataOverlayInfo info;
                    info.m_providerId = DataOverlayProviderExample::GetProviderId();
                    u32 token = m_type == Type_Int ? DataOverlayProviderExample::GetIntToken() : m_type == Type_Vector ? DataOverlayProviderExample::GetVectorToken() : DataOverlayProviderExample::GetPointerToken();
                    info.m_dataToken.m_dataUri.insert(info.m_dataToken.m_dataUri.end(), reinterpret_cast<u8*>(&token), reinterpret_cast<u8*>(&token) + sizeof(u32));
                    return info;
                }

                InstanceType m_type;
            };

            void CheckOverlay(const DataOverlayTestStruct* controlData, void* classPtr, const Uuid& uuid)
            {
                EXPECT_EQ( SerializeTypeInfo<DataOverlayTestStruct>::GetUuid(), uuid );
                DataOverlayTestStruct* newData = reinterpret_cast<DataOverlayTestStruct*>(classPtr);
                EXPECT_EQ( controlData->m_int, newData->m_int );
                EXPECT_EQ( controlData->m_intVector, newData->m_intVector );
                EXPECT_TRUE(newData->m_ptr != nullptr);
                EXPECT_TRUE(newData->m_ptr != controlData->m_ptr);
                EXPECT_EQ( controlData->m_ptr->m_int, newData->m_ptr->m_int );
                EXPECT_EQ( controlData->m_ptr->m_intVector, newData->m_ptr->m_intVector );
                EXPECT_EQ( controlData->m_ptr->m_ptr, newData->m_ptr->m_ptr );
                delete newData->m_ptr;
                delete newData;
            }

        public:
            void run()
            {
                SerializeContext serializeContext;

                // We must expose the class for serialization first.
                serializeContext.Class<DataOverlayTestStruct>()
                    ->Field("int", &DataOverlayTestStruct::m_int)
                    ->Field("intVector", &DataOverlayTestStruct::m_intVector)
                    ->Field("pointer", &DataOverlayTestStruct::m_ptr);

                DataOverlayTestStruct testData;
                testData.m_ptr = &testData;

                DataOverlayInstanceEnumeratorExample intOverlayEnumerator(DataOverlayInstanceEnumeratorExample::Type_Int);
                intOverlayEnumerator.BusConnect(DataOverlayInstanceId(&testData.m_int, SerializeTypeInfo<int>::GetUuid()));
                DataOverlayInstanceEnumeratorExample vectorOverlayEnumerator(DataOverlayInstanceEnumeratorExample::Type_Vector);
                vectorOverlayEnumerator.BusConnect(DataOverlayInstanceId(&testData.m_intVector, SerializeGenericTypeInfo<AZStd::vector<int> >::GetClassTypeId()));
                DataOverlayInstanceEnumeratorExample pointerOverlayEnumerator(DataOverlayInstanceEnumeratorExample::Type_Pointer);
                pointerOverlayEnumerator.BusConnect(DataOverlayInstanceId(&testData.m_ptr, SerializeTypeInfo<DataOverlayTestStruct>::GetUuid()));

                // XML
                AZStd::vector<char> xmlBuffer;
                IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBuffer);
                ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, serializeContext, ObjectStream::ST_XML);
                xmlObjStream->WriteClass(&testData);
                xmlObjStream->Finalize();

                AZ::IO::SystemFile tmpOut;
                tmpOut.Open("DataOverlayTest.xml", AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
                tmpOut.Write(xmlStream.GetData()->data(), xmlStream.GetLength());
                tmpOut.Close();

                DataOverlayProviderExample overlayProvider;
                overlayProvider.BusConnect(DataOverlayProviderExample::GetProviderId());
                xmlStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
                ObjectStream::ClassReadyCB readyCB(AZStd::bind(&DataOverlayTest::CheckOverlay, this, &overlayProvider.m_data, AZStd::placeholders::_1, AZStd::placeholders::_2));
                ObjectStream::LoadBlocking(&xmlStream, serializeContext, readyCB);
            }
        };

        DataOverlayTest test;
        test.run();
    }

    /*
    * DynamicSerializableFieldTest
    */
    TEST_F(Serialization, DynamicSerializableFieldTest)
    {
        SerializeContext serializeContext;

        // We must expose the class for serialization first.
        MyClassBase1::Reflect(serializeContext);
        MyClassBase2::Reflect(serializeContext);
        MyClassBase3::Reflect(serializeContext);
        SerializeTestClasses::MyClassMix::Reflect(serializeContext);

        SerializeTestClasses::MyClassMix obj;
        obj.Set(5); // Initialize with some value

        DynamicSerializableField testData;
        testData.m_data = &obj;
        testData.m_typeId = SerializeTypeInfo<SerializeTestClasses::MyClassMix>::GetUuid();

        // XML
        AZStd::vector<char> xmlBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBuffer);
        ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, serializeContext, ObjectStream::ST_XML);
        xmlObjStream->WriteClass(&testData);
        xmlObjStream->Finalize();

        AZ::IO::SystemFile tmpOut;
        tmpOut.Open("DynamicSerializableFieldTest.xml", AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
        tmpOut.Write(xmlStream.GetData()->data(), xmlStream.GetLength());
        tmpOut.Close();

        xmlStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        auto verifyLoad = [&testData](void* classPtr, const Uuid& uuid, SerializeContext* sc) -> void
        {
            EXPECT_EQ( SerializeTypeInfo<DynamicSerializableField>::GetUuid(), uuid );
            DynamicSerializableField* newData = reinterpret_cast<DynamicSerializableField*>(classPtr);
            EXPECT_EQ( SerializeTypeInfo<SerializeTestClasses::MyClassMix>::GetUuid(), newData->m_typeId );
            EXPECT_TRUE(newData->m_data != nullptr);
            EXPECT_TRUE( *reinterpret_cast<SerializeTestClasses::MyClassMix*>(testData.m_data) == *reinterpret_cast<SerializeTestClasses::MyClassMix*>(newData->m_data) );
            newData->DestroyData(sc);
            azdestroy(newData, AZ::SystemAllocator, DynamicSerializableField);
        };

        ObjectStream::ClassReadyCB readyCB(verifyLoad);
        ObjectStream::LoadBlocking(&xmlStream, serializeContext, readyCB);
    }

    /*
    * DynamicSerializableFieldTest
    */
    class SerializeDynamicSerializableFieldTest
        : public AllocatorsFixture
    {
    public:

        // Structure for reflecting Generic Template types to the Serialize context
        // so that they get added to the SerializeContext m_uuidGenericMap
        struct GenericTemplateTypes
        {
            AZ_TYPE_INFO(GenericTemplateTypes, "{24D83563-2AAA-40FE-8C77-0DC8298EDDEA}");

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<GenericTemplateTypes>()->
                    Field("stringToStringMap", &GenericTemplateTypes::m_stringStringMap)
                    ;
            }
            AZStd::unordered_map<AZStd::string, AZStd::string> m_stringStringMap;
        };
    };

    TEST_F(SerializeDynamicSerializableFieldTest, NonSerializableTypeTest)
    {
        SerializeContext serializeContext;
        DynamicSerializableField testData;
        EXPECT_EQ(nullptr, testData.m_data);
        EXPECT_EQ(AZ::Uuid::CreateNull(), testData.m_typeId);

        // Write DynamicSerializableField to stream
        AZStd::vector<AZ::u8> buffer;
        AZ::IO::ByteContainerStream<decltype(buffer)> stream(&buffer);
        {
            ObjectStream* binObjectStream = ObjectStream::Create(&stream, serializeContext, ObjectStream::ST_BINARY);
            binObjectStream->WriteClass(&testData);
            binObjectStream->Finalize();
        }

        // Load DynamicSerializableField from stream
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        {
            DynamicSerializableField loadData;
            loadData.m_typeId = AZ::Uuid::CreateRandom();

            // TypeId should be serialized in as a null Uuid and the m_data field should remain unchanged
            AZ::Utils::LoadObjectFromStreamInPlace(stream, loadData, &serializeContext);
            EXPECT_EQ(AZ::Uuid::CreateNull(), loadData.m_typeId);
        }
    }

    TEST_F(SerializeDynamicSerializableFieldTest, TemplateTypeSerializeTest)
    {
        SerializeContext serializeContext;
        GenericTemplateTypes::Reflect(serializeContext);
        DynamicSerializableField testData;
        EXPECT_EQ(nullptr, testData.m_data);
        EXPECT_EQ(AZ::Uuid::CreateNull(), testData.m_typeId);

        AZStd::unordered_map<AZStd::string, AZStd::string> stringMap;
        stringMap.emplace("Key", "Value");
        stringMap.emplace("Lumber", "Yard");

        testData.Set(&stringMap);

        // Write DynamicSerializableField to stream
        AZStd::vector<AZ::u8> buffer;
        AZ::IO::ByteContainerStream<decltype(buffer)> stream(&buffer);
        {
            ObjectStream* binObjectStream = ObjectStream::Create(&stream, serializeContext, ObjectStream::ST_BINARY);
            binObjectStream->WriteClass(&testData);
            binObjectStream->Finalize();
        }

        // Load DynamicSerializableField from stream
        stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        /* Loaded AZStd::Containers for serialization cannot be pointers, as they use to a NullFactory for heap creation
        {
            DynamicSerializableField loadData;
            loadData.m_typeId = AZ::Uuid::CreateRandom();

            AZ::Utils::LoadObjectFromStreamInPlace(stream, loadData, &serializeContext);
            EXPECT_NE(nullptr, loadData.Get<decltype(stringMap)>());
            auto& loadedStringMap = *loadData.Get<decltype(stringMap)>();
            auto loadedStringIt = loadedStringMap.find("Lumber");
            EXPECT_NE(loadedStringMap.end(), loadedStringIt);
            EXPECT_EQ("Yard", loadedStringIt->second);
            loadData.DestroyData(&serializeContext);
        }
        */
    }

    /*
    * CloneTest
    */
    namespace Clone
    {
        struct RefCounted
        {
            AZ_CLASS_ALLOCATOR(RefCounted, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(RefCounted, "{ca52979d-b926-461a-b1f5-66bbfdb80639}");

            RefCounted()
                : m_refCount(0)
                , m_data(0)
            {}

            RefCounted(int data)
                : m_refCount(0)
                , m_data(data)
            {}

            virtual ~RefCounted() = default;

            static void Reflect(SerializeContext& serializeContext)
            {
                serializeContext.Class<RefCounted>()
                    ->Field("Data", &RefCounted::m_data);
            }

            //////////////////////////////////////////////////////////////////////////
            // For intrusive pointers
            void add_ref() { m_refCount++; }
            void release()
            {
                --m_refCount;
                if (m_refCount == 0)
                {
                    delete this;
                }
            }
            int m_refCount;
            //////////////////////////////////////////////////////////////////////////

            int m_data;
        };

        struct Clonable
        {
            Clonable()
                : m_emptyInitText("Some init text!")
            {
            }

            virtual ~Clonable() = default;

            AZ_RTTI(Clonable, "{3E463CC3-CC78-4F21-9BE8-0B0AA10E8E26}");
            AZ_CLASS_ALLOCATOR(Clonable, AZ::SystemAllocator, 0);

            static void Reflect(SerializeContext& serializeContext)
            {
                serializeContext.Class<Clonable>()
                    ->Field("m_int", &Clonable::m_int)
                    ->Field("m_emptyInitText", &Clonable::m_emptyInitText)
                    ->Field("m_map", &Clonable::m_map)
                    ->Field("m_fieldValues", &Clonable::m_fieldValues)
                    ->Field("m_smartArray", &Clonable::m_smartArray);
            }

            int m_int;
            AZStd::string m_emptyInitText;
            AZStd::unordered_map<int, int> m_map;
            AZStd::vector<DynamicSerializableField> m_fieldValues;
            AZStd::array<AZStd::intrusive_ptr<RefCounted>, 10> m_smartArray;
        };

        struct ClonableMutlipleInheritanceOrderingA
            : public AZ::TickBus::Handler
            , public RefCounted
            , public Clonable
        {
            AZ_RTTI(ClonableMutlipleInheritanceOrderingA, "{4A1FA4E5-48FB-413D-876F-E6633240773A}", Clonable);
            AZ_CLASS_ALLOCATOR(ClonableMutlipleInheritanceOrderingA, AZ::SystemAllocator, 0);

            ClonableMutlipleInheritanceOrderingA() = default;
            ~ClonableMutlipleInheritanceOrderingA() override = default;

            MOCK_METHOD2(OnTick, void (float, AZ::ScriptTimePoint));
            virtual void MyNewVirtualFunction() {}

            static void Reflect(SerializeContext& serializeContext)
            {
                serializeContext.Class<ClonableMutlipleInheritanceOrderingA, Clonable>()
                    ->Field("myInt0", &ClonableMutlipleInheritanceOrderingA::m_myInt0)
                    ;
            }

            int m_myInt0 = 0;
        };

        struct ClonableMutlipleInheritanceOrderingB
            : public Clonable
            , public RefCounted
            , public AZ::TickBus::Handler
        {
            AZ_RTTI(ClonableMutlipleInheritanceOrderingB, "{169D8A4F-6C8A-4F50-8B7B-3EE81A9948BB}", Clonable);
            AZ_CLASS_ALLOCATOR(ClonableMutlipleInheritanceOrderingB, AZ::SystemAllocator, 0);

            ClonableMutlipleInheritanceOrderingB() = default;
            ~ClonableMutlipleInheritanceOrderingB() override = default;
            
            MOCK_METHOD2(OnTick, void (float, AZ::ScriptTimePoint));
            MOCK_METHOD0(SomeVirtualFunction, void ());

            virtual char MyCharSumFunction() { return m_myChar0 + m_myChar1 + m_myChar2; }
            virtual void MyCharResetFunction() { m_myChar0 = m_myChar1 = m_myChar2 = 0; }

            static void Reflect(SerializeContext& serializeContext)
            {
                serializeContext.Class<ClonableMutlipleInheritanceOrderingB, Clonable>()
                    ->Field("myChar0", &ClonableMutlipleInheritanceOrderingB::m_myChar0)
                    ->Field("myChar1", &ClonableMutlipleInheritanceOrderingB::m_myChar1)
                    ->Field("myChar2", &ClonableMutlipleInheritanceOrderingB::m_myChar2)
                    ;
            }

            char m_myChar0 = 0;
            char m_myChar1 = 1;
            char m_myChar2 = 2;
        };

        struct ClonableAssociativePointerContainer
        {
            AZ_TYPE_INFO(ClonableAssociativePointerContainer, "{F558DC57-7850-42E1-9D16-5538C0D839E2}");
            AZ_CLASS_ALLOCATOR(ClonableAssociativePointerContainer, AZ::SystemAllocator, 0);

            static void Reflect(SerializeContext& serializeContext)
            {
                serializeContext.Class<ClonableAssociativePointerContainer>()
                    ->Field("m_setOfPointers", &ClonableAssociativePointerContainer::m_setOfPointers)
                    ->Field("m_mapOfFloatPointers", &ClonableAssociativePointerContainer::m_mapOfFloatPointers)
                    ->Field("m_sharedEntityPointer", &ClonableAssociativePointerContainer::m_sharedEntityPointer)
                    ;
            }

            AZStd::unordered_set<AZ::Entity*> m_setOfPointers;
            AZStd::unordered_map<int, float*> m_mapOfFloatPointers;
            AZStd::shared_ptr<AZ::Entity> m_sharedEntityPointer;
        };
    }
    TEST_F(Serialization, CloneTest)
    {
        using namespace Clone;

        // We must expose the class for serialization first.
        MyClassBase1::Reflect((*m_serializeContext));
        MyClassBase2::Reflect((*m_serializeContext));
        MyClassBase3::Reflect((*m_serializeContext));
        SerializeTestClasses::MyClassMix::Reflect((*m_serializeContext));
        RefCounted::Reflect((*m_serializeContext));
        Clonable::Reflect((*m_serializeContext));

        Clonable testObj;
        testObj.m_int = 100;
        testObj.m_emptyInitText = ""; // set to empty to make sure we write zero values
        testObj.m_map.insert(AZStd::make_pair(1, 2));
        testObj.m_smartArray[0] = aznew RefCounted(101);
        testObj.m_smartArray[1] = aznew RefCounted(201);
        testObj.m_smartArray[2] = aznew RefCounted(301);

        SerializeTestClasses::MyClassMix val1;
        val1.Set(5);    // Initialize with some value
        DynamicSerializableField valField1;
        valField1.m_data = &val1;
        valField1.m_typeId = SerializeTypeInfo<SerializeTestClasses::MyClassMix>::GetUuid();
        testObj.m_fieldValues.push_back(valField1);

        Clonable* cloneObj = m_serializeContext->CloneObject(&testObj);
        EXPECT_TRUE(cloneObj);
        EXPECT_EQ( testObj.m_int, cloneObj->m_int );
        EXPECT_EQ( testObj.m_emptyInitText, cloneObj->m_emptyInitText );
        EXPECT_EQ( testObj.m_map, cloneObj->m_map );
        EXPECT_EQ( testObj.m_fieldValues.size(), cloneObj->m_fieldValues.size() );
        EXPECT_TRUE(cloneObj->m_fieldValues[0].m_data);
        EXPECT_TRUE(cloneObj->m_fieldValues[0].m_data != testObj.m_fieldValues[0].m_data);
        EXPECT_TRUE( *reinterpret_cast<SerializeTestClasses::MyClassMix*>(testObj.m_fieldValues[0].m_data) == *reinterpret_cast<SerializeTestClasses::MyClassMix*>(cloneObj->m_fieldValues[0].m_data) );
        delete reinterpret_cast<SerializeTestClasses::MyClassMix*>(cloneObj->m_fieldValues[0].m_data);
        AZ_TEST_ASSERT(cloneObj->m_smartArray[0] && cloneObj->m_smartArray[0]->m_data == 101);
        AZ_TEST_ASSERT(cloneObj->m_smartArray[1] && cloneObj->m_smartArray[1]->m_data == 201);
        AZ_TEST_ASSERT(cloneObj->m_smartArray[2] && cloneObj->m_smartArray[2]->m_data == 301);
        delete cloneObj;
        delete reinterpret_cast<SerializeTestClasses::MyClassMix*>(testObj.m_fieldValues[0].m_data);
    }

    TEST_F(Serialization, CloneInplaceTest)
    {
        using namespace Clone;

        // We must expose the class for serialization first.
        MyClassBase1::Reflect(*m_serializeContext);
        MyClassBase2::Reflect(*m_serializeContext);
        MyClassBase3::Reflect(*m_serializeContext);
        SerializeTestClasses::MyClassMix::Reflect(*m_serializeContext);
        RefCounted::Reflect(*m_serializeContext);
        Clonable::Reflect(*m_serializeContext);

        Clonable testObj;
        testObj.m_int = 100;
        testObj.m_emptyInitText = ""; // set to empty to make sure we write zero values
        testObj.m_map.insert(AZStd::make_pair(1, 2));
        testObj.m_smartArray[0] = aznew RefCounted(101);
        testObj.m_smartArray[1] = aznew RefCounted(201);
        testObj.m_smartArray[2] = aznew RefCounted(301);

        SerializeTestClasses::MyClassMix val1;
        val1.Set(5);    // Initialize with some value
        DynamicSerializableField valField1;
        valField1.m_data = &val1;
        valField1.m_typeId = SerializeTypeInfo<SerializeTestClasses::MyClassMix>::GetUuid();
        testObj.m_fieldValues.push_back(valField1);

        Clonable cloneObj;
        m_serializeContext->CloneObjectInplace(cloneObj, &testObj);
        EXPECT_EQ(testObj.m_int, cloneObj.m_int);
        EXPECT_EQ(testObj.m_emptyInitText, cloneObj.m_emptyInitText);
        EXPECT_EQ(testObj.m_map, cloneObj.m_map);
        EXPECT_EQ(testObj.m_fieldValues.size(), cloneObj.m_fieldValues.size());
        EXPECT_TRUE(cloneObj.m_fieldValues[0].m_data);
        EXPECT_TRUE(cloneObj.m_fieldValues[0].m_data != testObj.m_fieldValues[0].m_data);
        EXPECT_TRUE(*reinterpret_cast<SerializeTestClasses::MyClassMix*>(testObj.m_fieldValues[0].m_data) == *reinterpret_cast<SerializeTestClasses::MyClassMix*>(cloneObj.m_fieldValues[0].m_data));
        delete reinterpret_cast<SerializeTestClasses::MyClassMix*>(cloneObj.m_fieldValues[0].m_data);
        AZ_TEST_ASSERT(cloneObj.m_smartArray[0] && cloneObj.m_smartArray[0]->m_data == 101);
        AZ_TEST_ASSERT(cloneObj.m_smartArray[1] && cloneObj.m_smartArray[1]->m_data == 201);
        AZ_TEST_ASSERT(cloneObj.m_smartArray[2] && cloneObj.m_smartArray[2]->m_data == 301);
        delete reinterpret_cast<SerializeTestClasses::MyClassMix*>(testObj.m_fieldValues[0].m_data);
    }

    TEST_F(Serialization, CloneAssociativeContainerOfPointersTest)
    {
        using namespace Clone;

        // We must expose the class for serialization first.
        AZ::Entity::Reflect(m_serializeContext.get());
        ClonableAssociativePointerContainer::Reflect(*m_serializeContext);

        ClonableAssociativePointerContainer testObj;
        testObj.m_setOfPointers.insert(aznew AZ::Entity("Entity1"));
        testObj.m_setOfPointers.insert(aznew AZ::Entity("Entity2"));
        testObj.m_mapOfFloatPointers.emplace(5, azcreate(float, (3.14f), AZ::SystemAllocator, "Frank the Float"));
        testObj.m_sharedEntityPointer.reset(aznew AZ::Entity("Entity3"));

        ClonableAssociativePointerContainer* cloneObj = m_serializeContext->CloneObject(&testObj);

        EXPECT_EQ(testObj.m_setOfPointers.size(), cloneObj->m_setOfPointers.size());
        auto testObjSetBeginIt = testObj.m_setOfPointers.begin();
        auto cloneObjSetBeginIt = cloneObj->m_setOfPointers.begin();
        for (; testObjSetBeginIt != testObj.m_setOfPointers.end(); ++testObjSetBeginIt, ++cloneObjSetBeginIt)
        {
            EXPECT_EQ((*testObjSetBeginIt)->GetId(), (*cloneObjSetBeginIt)->GetId());
        }

        EXPECT_EQ(testObj.m_mapOfFloatPointers.size(), cloneObj->m_mapOfFloatPointers.size());
        auto testObjMapBeginIt = testObj.m_mapOfFloatPointers.begin();
        auto cloneObjMapBeginIt = cloneObj->m_mapOfFloatPointers.begin();
        for (; testObjMapBeginIt != testObj.m_mapOfFloatPointers.end(); ++testObjMapBeginIt, ++cloneObjMapBeginIt)
        {
            EXPECT_EQ(*(testObjMapBeginIt->second), *(cloneObjMapBeginIt->second));
        }

        EXPECT_NE(nullptr, cloneObj->m_sharedEntityPointer.get());
        EXPECT_EQ(testObj.m_sharedEntityPointer->GetId(), cloneObj->m_sharedEntityPointer->GetId());

        //Free the allocated memory
        for (auto& entitySet : { testObj.m_setOfPointers, cloneObj->m_setOfPointers })
        {
            for (AZ::Entity* entityPtr : entitySet)
            {
                delete entityPtr;
            }
        }

        for (auto& intFloatPtrMap : { testObj.m_mapOfFloatPointers, cloneObj->m_mapOfFloatPointers })
        {
            for (auto& intFloatPtrPair : intFloatPtrMap)
            {
                azdestroy(intFloatPtrPair.second);
            }
        }
        delete cloneObj;
    }

    struct TestCloneAssetData
        : public AZ::Data::AssetData
    {
        AZ_CLASS_ALLOCATOR(TestCloneAssetData, AZ::SystemAllocator, 0);
        AZ_RTTI(TestCloneAssetData, "{0BAECA70-262F-4BDC-9D42-B7F7A10077DA}", AZ::Data::AssetData);

        TestCloneAssetData() = default;
        TestCloneAssetData(const AZ::Data::AssetId& assetId, AZ::Data::AssetData::AssetStatus status, uint32_t valueInt = 0)
            : AZ::Data::AssetData(assetId, status)
            , m_valueInt(valueInt)
        {}

        uint32_t m_valueInt{};
    };

    class TestCloneAssetHandler
        : public AZ::Data::AssetHandler
        , public AZ::Data::AssetCatalog
    {
    public:
        AZ_CLASS_ALLOCATOR(TestCloneAssetHandler, AZ::SystemAllocator, 0);

        //////////////////////////////////////////////////////////////////////////
        // AssetHandler
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            EXPECT_EQ(AzTypeInfo<TestCloneAssetData>::Uuid(), type);
            return aznew TestCloneAssetData(id, AZ::Data::AssetData::AssetStatus::NotLoaded);
        }
        Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            EXPECT_EQ(AzTypeInfo<TestCloneAssetData>::Uuid(), asset.GetType());
            const size_t assetDataSize = static_cast<size_t>(stream->GetLength());
            EXPECT_EQ(sizeof(TestCloneAssetData::m_valueInt), assetDataSize);
            TestCloneAssetData* cloneAssetData = asset.GetAs<TestCloneAssetData>();
            stream->Read(assetDataSize, &cloneAssetData->m_valueInt);

            return Data::AssetHandler::LoadResult::LoadComplete;
        }
        bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, IO::GenericStream* stream) override
        {
            EXPECT_EQ(AzTypeInfo<TestCloneAssetData>::Uuid(), asset.GetType());
            TestCloneAssetData* cloneAssetData = asset.GetAs<TestCloneAssetData>();

            EXPECT_NE(nullptr, cloneAssetData);
            return Save(*cloneAssetData, stream);
        }

        bool Save(const TestCloneAssetData& testCloneAssetData, IO::GenericStream* stream) const
        {
            stream->Write(sizeof(TestCloneAssetData::m_valueInt), &testCloneAssetData.m_valueInt);
            return true;
        }
        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            EXPECT_EQ(AzTypeInfo<TestCloneAssetData>::Uuid(), ptr->GetType());
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.push_back(AzTypeInfo<TestCloneAssetData>::Uuid());
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalog
        // Redirects stream info request for assets to always return stream info needed to load a TestCloneAssetData
        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId&, const AZ::Data::AssetType& type) override
        {
            EXPECT_EQ(AzTypeInfo<TestCloneAssetData>::Uuid(), type);
            AZ::Data::AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;
            info.m_streamName = GetAssetFilename();

            AZStd::string fullName = AZStd::string::format("%s%s", GetAssetFolderPath(), info.m_streamName.data());
            info.m_dataLen = static_cast<size_t>(IO::SystemFile::Length(fullName.c_str()));
            return info;
        }

        AZ::Data::AssetStreamInfo GetStreamInfoForSave(const AZ::Data::AssetId&, const AZ::Data::AssetType& type) override
        {
            EXPECT_EQ(AzTypeInfo<TestCloneAssetData>::Uuid(), type);
            AZ::Data::AssetStreamInfo info;
            info.m_dataOffset = 0;
            info.m_streamFlags = AZ::IO::OpenMode::ModeWrite;
            info.m_streamName = GetAssetFilename();

            AZStd::string fullName = AZStd::string::format("%s%s", GetAssetFolderPath(), info.m_streamName.data());
            info.m_dataLen = static_cast<size_t>(AZ::IO::SystemFile::Length(fullName.c_str()));
            return info;
        }

        static const char* GetAssetFilename()
        {
            return "TestCloneAsset.bin";
        }

        static const char* GetAssetFolderPath()
        {
            return "";
        }
    };

    struct TestCloneWrapperObject
    {
        AZ_TYPE_INFO(TestCloneWrapperObject, "{4BAE1D45-EFFD-4157-9F80-E20239265304}");
        AZ_CLASS_ALLOCATOR(TestCloneWrapperObject, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<TestCloneWrapperObject>()
                    ->Field("TestCloneAsset", &TestCloneWrapperObject::m_cloneAsset);
            }
        }

        AZ::Data::Asset<TestCloneAssetData> m_cloneAsset;
    };

    class SerializeAssetFixture
        : public AllocatorsFixture
    {
    public:

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            m_prevFileIO = IO::FileIOBase::GetInstance();
            IO::FileIOBase::SetInstance(&m_fileIO);
            m_streamer = aznew IO::Streamer(AZStd::thread_desc{}, StreamerComponent::CreateStreamerStack());
            Interface<IO::IStreamer>::Register(m_streamer);

            m_serializeContext.reset(aznew AZ::SerializeContext());
            TestCloneWrapperObject::Reflect(m_serializeContext.get());

            // create the database
            AZ::Data::AssetManager::Descriptor desc;
            AZ::Data::AssetManager::Create(desc);

            // create and register an asset handler
            AZ::Data::AssetManager::Instance().RegisterHandler(&m_testAssetHandlerAndCatalog, AzTypeInfo<TestCloneAssetData>::Uuid());
            AZ::Data::AssetManager::Instance().RegisterCatalog(&m_testAssetHandlerAndCatalog, AzTypeInfo<TestCloneAssetData>::Uuid());
            CreateTestCloneAsset();
        }

        void TearDown() override
        {
            m_serializeContext->EnableRemoveReflection(); 
            TestCloneWrapperObject::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();
            m_serializeContext.reset();
            // destroy the database
            AZ::Data::AssetManager::Instance().UnregisterHandler(&m_testAssetHandlerAndCatalog);
            AZ::Data::AssetManager::Instance().UnregisterCatalog(&m_testAssetHandlerAndCatalog);
            AZ::Data::AssetManager::Destroy();

            Interface<IO::IStreamer>::Unregister(m_streamer);
            delete m_streamer;

            DestroyTestCloneAsset();
            IO::FileIOBase::SetInstance(m_prevFileIO);
            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorInstance<PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }


        void CreateTestCloneAsset()
        {
#if AZ_TRAIT_TEST_APPEND_ROOT_FOLDER_TO_PATH
            AZ::IO::Path assetFullPath(AZ_TRAIT_TEST_ROOT_FOLDER);
#else
            AZ::IO::Path assetFullPath;
#endif
            assetFullPath /= TestCloneAssetHandler::GetAssetFolderPath();
            assetFullPath /= TestCloneAssetHandler::GetAssetFilename();
            AZ::IO::FileIOStream cloneTestFileStream(assetFullPath.c_str(), AZ::IO::OpenMode::ModeWrite);
            TestCloneAssetData testCloneAssetData;
            testCloneAssetData.m_valueInt = 5;
            m_testAssetHandlerAndCatalog.Save(testCloneAssetData, &cloneTestFileStream);
        }

        void DestroyTestCloneAsset()
        {
#if AZ_TRAIT_TEST_APPEND_ROOT_FOLDER_TO_PATH
            AZ::IO::Path assetFullPath(AZ_TRAIT_TEST_ROOT_FOLDER);
#else
            AZ::IO::Path assetFullPath;
#endif
            assetFullPath /= TestCloneAssetHandler::GetAssetFolderPath();
            assetFullPath /= TestCloneAssetHandler::GetAssetFilename();
            m_fileIO.Remove(assetFullPath.c_str());
        }

    protected:
        AZ::IO::FileIOBase* m_prevFileIO{};
        AZ::IO::Streamer* m_streamer{};
        TestFileIOBase m_fileIO;
        TestCloneAssetHandler m_testAssetHandlerAndCatalog;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };

    TEST_F(SerializeAssetFixture, CloneObjectWithAssetReferenceTest)
    {
        static const AZ::Data::AssetId cloneObjectAssetId(AZ::Uuid("{AF338D46-C607-4F2B-8F0B-8828F88EA5F2}"));

        {
            // Create a TestCloneAssetData asset and keep a reference to in the local testCloneAsset variable so that the AssetManager manages the asset
            AZ::Data::Asset<TestCloneAssetData> testCloneAsset = AZ::Data::AssetManager::Instance().CreateAsset(cloneObjectAssetId, AZ::AzTypeInfo<TestCloneAssetData>::Uuid(), AZ::Data::AssetLoadBehavior::Default);
            testCloneAsset.Get()->m_valueInt = 15;

            /* Create a testCloneWrapper object that has its Asset<T> object set to an AssetId, but not to a loaded asset.
             The PreLoad flag is set on the Asset<T> to validate if the SerializeContext::CloneObject function is attempting to load the asset.
             If the SerializeContext::CloneObject is not attempting to load the asset, then the cloned TestCloneWrapperObject m_cloneAsset member
             should have its asset id set to cloneObjectAssetId without the asset being loaded
             */
            TestCloneWrapperObject testObj;
            testObj.m_cloneAsset = AZ::Data::Asset<TestCloneAssetData>(cloneObjectAssetId, AZ::AzTypeInfo<TestCloneAssetData>::Uuid());
            testObj.m_cloneAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);

            // the testCloneAsset should have one reference.
            EXPECT_EQ(1, testCloneAsset.Get()->GetUseCount());

            TestCloneWrapperObject clonedTestObj;
            m_serializeContext->CloneObjectInplace(clonedTestObj, &testObj);
            // the testCloneAsset should still be the only reference after the clone
            EXPECT_EQ(1, testCloneAsset.Get()->GetUseCount());
            // The cloned test object should not have AssetData associated with it,
            // but should have the cloneObjectAssetId asset id set on it
            EXPECT_EQ(cloneObjectAssetId, clonedTestObj.m_cloneAsset.GetId());
            ASSERT_EQ(nullptr, clonedTestObj.m_cloneAsset.Get());
        }

        AZ::Data::AssetManager::Instance().DispatchEvents();
    }

    TEST_F(Serialization, CloneMultipleInheritance_RTTIBaseClassDiffererentOrder_KeepsCorrectOffsets)
    {
        using namespace Clone;

        EXPECT_NE(sizeof(ClonableMutlipleInheritanceOrderingA), sizeof(ClonableMutlipleInheritanceOrderingB));

        Clonable::Reflect(*m_serializeContext.get());
        ClonableMutlipleInheritanceOrderingA::Reflect(*m_serializeContext.get());
        ClonableMutlipleInheritanceOrderingB::Reflect(*m_serializeContext.get());

        AZStd::unique_ptr<Clonable> objA(aznew ClonableMutlipleInheritanceOrderingA);
        AZStd::unique_ptr<Clonable> objB(aznew ClonableMutlipleInheritanceOrderingB);

        // sanity check that the pointer offset for the classes being used is different
        const void* aAsBasePtr = AZ::SerializeTypeInfo<Clonable>::RttiCast(objA.get(), AZ::SerializeTypeInfo<Clonable>::GetRttiTypeId(objA.get()));
        const void* bAsBasePtr = AZ::SerializeTypeInfo<Clonable>::RttiCast(objB.get(), AZ::SerializeTypeInfo<Clonable>::GetRttiTypeId(objB.get()));

        AZStd::ptrdiff_t aOffset = (char*)objA.get() - (char*)aAsBasePtr;
        AZStd::ptrdiff_t bOffset = (char*)objB.get() - (char*)bAsBasePtr;
        EXPECT_NE(aOffset, 0);
        EXPECT_EQ(bOffset, 0);

        // Now clone the original objects, and store in the RTTI base type
        AZStd::unique_ptr<Clonable> cloneObjA(m_serializeContext->CloneObject(objA.get()));
        AZStd::unique_ptr<Clonable> cloneObjB(m_serializeContext->CloneObject(objB.get()));

        // Check our pointer offsets are still different in the cloned objects
        const void* aCloneAsBasePtr = AZ::SerializeTypeInfo<Clonable>::RttiCast(cloneObjA.get(), AZ::SerializeTypeInfo<Clonable>::GetRttiTypeId(cloneObjA.get()));
        const void* bCloneAsBasePtr = AZ::SerializeTypeInfo<Clonable>::RttiCast(cloneObjB.get(), AZ::SerializeTypeInfo<Clonable>::GetRttiTypeId(cloneObjB.get()));

        AZStd::ptrdiff_t aCloneOffset = (char*)cloneObjA.get() - (char*)aCloneAsBasePtr;
        AZStd::ptrdiff_t bCloneOffset = (char*)cloneObjB.get() - (char*)bCloneAsBasePtr;
        EXPECT_NE(aCloneOffset, 0);
        EXPECT_EQ(bCloneOffset, 0);

        // Check that offsets are equivalent between the clones and the original objects
        EXPECT_EQ(aCloneOffset, aOffset);
        EXPECT_EQ(bCloneOffset, bOffset);

        m_serializeContext->EnableRemoveReflection();
        ClonableMutlipleInheritanceOrderingB::Reflect(*m_serializeContext.get());
        ClonableMutlipleInheritanceOrderingA::Reflect(*m_serializeContext.get());
        Clonable::Reflect(*m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
    }


    // Prove that if a member of a vector of baseclass pointers is unreadable, the container
    // removes the element instead of leaving a null.  This is an arbitrary choice (to remove or leave
    // the null) and this test exists just to prove that the chosen way functions as expected.
    TEST_F(Serialization, Clone_UnreadableVectorElements_LeaveNoGaps_Errors)
    {
        using namespace ContainerElementDeprecationTestData;
        // make sure that when a component is deprecated, it is removed during deserialization
        // and does not leave a hole that is a nullptr.
        ClassWithAVectorOfBaseClasses::Reflect(m_serializeContext.get());

        ClassWithAVectorOfBaseClasses vectorContainer;
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());

        // (remove it, but without deprecating)
        m_serializeContext->EnableRemoveReflection();
        DerivedClass2::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();

        // clone it, we expect errors:
        AZ_TEST_START_TRACE_SUPPRESSION;
        ClassWithAVectorOfBaseClasses loadedContainer;
        m_serializeContext->CloneObjectInplace(loadedContainer, &vectorContainer);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2); // 2 classes should have failed and generated warnings/errors

        EXPECT_EQ(loadedContainer.m_vectorOfBaseClasses.size(), 2); // we still preserve the ones we CAN read.
        for (auto baseclass : loadedContainer.m_vectorOfBaseClasses)
        {
            // we should only have baseclass1's in there.
            EXPECT_EQ(baseclass->RTTI_GetType(), azrtti_typeid<DerivedClass1>());
        }
    }

    // Prove that if you properly deprecate a member of a vector of baseclass pointers, the container
    // removes the element instead of leaving a null and does not emit an error
    TEST_F(Serialization, Clone_DeprecatedVectorElements_LeaveNoGaps_DoesNotError)
    {
        using namespace ContainerElementDeprecationTestData;
        // make sure that when a component is deprecated, it is removed during deserialization
        // and does not leave a hole that is a nullptr.
        ClassWithAVectorOfBaseClasses::Reflect(m_serializeContext.get());

        ClassWithAVectorOfBaseClasses vectorContainer;
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());

        // remove it and properly deprecate it
        m_serializeContext->EnableRemoveReflection();
        DerivedClass2::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        m_serializeContext->ClassDeprecate("Dummy UUID", azrtti_typeid<DerivedClass2>());

        // clone it, we expect no errors:
        ClassWithAVectorOfBaseClasses loadedContainer;
        m_serializeContext->CloneObjectInplace(loadedContainer, &vectorContainer);

        EXPECT_EQ(loadedContainer.m_vectorOfBaseClasses.size(), 2); // we still preserve the ones we CAN read.
        for (auto baseclass : loadedContainer.m_vectorOfBaseClasses)
        {
            // we should only have baseclass1's in there.
            EXPECT_EQ(baseclass->RTTI_GetType(), azrtti_typeid<DerivedClass1>());
        }
    }

    // Prove that if you deprecate but upgrade a member of a vector of baseclass pointers, the container
    // Clone actually errors.  This behavior differs from serialize and datapatch because you're not
    // expected to even have a deprecated class being cloned in the first place (it should have
    // converted on deserialize or datapatch!)
    TEST_F(Serialization, Clone_DeprecatedVectorElements_ConvertedClass_LeavesGaps_Errors)
    {
        using namespace ContainerElementDeprecationTestData;
        // make sure that when a component is deprecated, it is removed during deserialization
        // and does not leave a hole that is a nullptr.
        ClassWithAVectorOfBaseClasses::Reflect(m_serializeContext.get());

        ClassWithAVectorOfBaseClasses vectorContainer;
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());

        // remove it and properly deprecate it with a converter that will upgrade it.
        m_serializeContext->EnableRemoveReflection();
        DerivedClass2::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        m_serializeContext->ClassDeprecate("Dummy UUID", azrtti_typeid<DerivedClass2>(), ConvertDerivedClass2ToDerivedClass3);

        // clone it, we expect no errors:
        ClassWithAVectorOfBaseClasses loadedContainer;
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serializeContext->CloneObjectInplace(loadedContainer, &vectorContainer);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2); // one for each converter

        ASSERT_EQ(loadedContainer.m_vectorOfBaseClasses.size(), 2); // we still preserve the ones we CAN read.

                                                                    // this also proves it does not shuffle elements around.
        EXPECT_EQ(loadedContainer.m_vectorOfBaseClasses[0]->RTTI_GetType(), azrtti_typeid<DerivedClass1>());
        EXPECT_EQ(loadedContainer.m_vectorOfBaseClasses[1]->RTTI_GetType(), azrtti_typeid<DerivedClass1>());
    }

    struct TestContainerType
    {
        AZ_TYPE_INFO(TestContainerType, "{81F20E9F-3F35-4063-BE29-A22EAF10AF59}");
        int32_t m_value{};
    };

    struct ContainerWrapper
    {
        AZ_TYPE_INFO(ContainerWrapper, "{F4EE9211-CABE-4D28-8356-2C2ADE6E5315}");
        TestContainerType m_testContainer;
    };

    TEST_F(Serialization, Clone_Container_WhereReserveElement_ReturnsNullptr_DoesNotCrash)
    {

        struct EmptyDataContainer
            : AZ::SerializeContext::IDataContainer
        {
            EmptyDataContainer()
            {
                // Create SerializeContext ClassElement for a int32_t type that is not a pointer
                m_classElement.m_name = "Test";
                m_classElement.m_nameCrc = AZ_CRC("Test");
                m_classElement.m_typeId = azrtti_typeid<int32_t>();
                m_classElement.m_dataSize = sizeof(int32_t);
                m_classElement.m_offset = 0;
                m_classElement.m_azRtti = {};
                m_classElement.m_editData = {};
                m_classElement.m_flags = 0;
            }
            const AZ::SerializeContext::ClassElement* GetElement(uint32_t) const override
            {
                return {};
            }
            bool GetElement(AZ::SerializeContext::ClassElement&, const AZ::SerializeContext::DataElement&) const override
            {
                return {};
            }
            void EnumElements(void* instance, const ElementCB& cb) override
            {
                auto dataContainer = reinterpret_cast<TestContainerType*>(instance);
                cb(&dataContainer->m_value, m_classElement.m_typeId, m_classElement.m_genericClassInfo ? m_classElement.m_genericClassInfo->GetClassData() : nullptr, &m_classElement);
            }
            void EnumTypes(const ElementTypeCB& cb) override
            {
                cb(m_classElement.m_typeId, &m_classElement);
            }
            size_t  Size(void*) const override
            {
                return {};
            }
            size_t Capacity(void*) const override
            {
                return {};
            }
            bool IsStableElements() const override
            {
                return {};
            }
            bool IsFixedSize() const override
            {
                return {};
            }
            bool IsFixedCapacity() const override
            {
                return {};
            }
            bool IsSmartPointer() const override
            {
                return {};
            }
            bool CanAccessElementsByIndex() const override
            {
                return {};
            }
            void* ReserveElement(void*, const AZ::SerializeContext::ClassElement*) override
            {
                return {};
            }
            void* GetElementByIndex(void*, const AZ::SerializeContext::ClassElement*, size_t) override
            {
                return {};
            }
            void StoreElement([[maybe_unused]] void* instance, [[maybe_unused]] void* element) override
            {}
            bool RemoveElement(void*, const void*, [[maybe_unused]] AZ::SerializeContext*  serializeContext) override
            {
                return {};
            }
            size_t RemoveElements(void*, const void**, size_t, [[maybe_unused]] AZ::SerializeContext* serializeContext) override
            {
                return {};
            }
            void ClearElements(void*, AZ::SerializeContext*) override
            {}

            AZ::SerializeContext::ClassElement m_classElement;
        };

        m_serializeContext->Class<TestContainerType>()
            ->DataContainer<EmptyDataContainer>();
        m_serializeContext->Class<ContainerWrapper>()
            ->Field("m_testContainer", &ContainerWrapper::m_testContainer);

        ContainerWrapper expectObject{ {42} };
        ContainerWrapper resultObject;
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serializeContext->CloneObjectInplace(resultObject, &expectObject);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(0, resultObject.m_testContainer.m_value);

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<TestContainerType>();
        m_serializeContext->Class<ContainerWrapper>();
        m_serializeContext->DisableRemoveReflection();
    }

    /*
    * Error Testing
    */
    namespace Error
    {
        struct UnregisteredClass
        {
            AZ_TYPE_INFO(UnregisteredClass, "{6558CEBC-D764-4E50-BAA0-025BF55FAD15}")
        };

        struct UnregisteredRttiClass
        {
            AZ_RTTI(UnregisteredRttiClass, "{F948E16B-975D-4F23-911E-2AA5758D8B21}");
            virtual ~UnregisteredRttiClass() {}
        };

        struct ChildOfUnregisteredClass
            : public UnregisteredClass
        {
            AZ_TYPE_INFO(ChildOfUnregisteredClass, "{C72CB2C9-7E9A-41EB-8219-5D13B6445AFC}")

                ChildOfUnregisteredClass() {}
            ChildOfUnregisteredClass(SerializeContext& sc)
            {
                sc.Class<ChildOfUnregisteredClass, UnregisteredClass>();
            }
        };
        struct ChildOfUnregisteredRttiClass
            : public UnregisteredRttiClass
        {
            AZ_RTTI(ChildOfUnregisteredRttiClass, "{E58F6984-4C0A-4D1B-B034-FDEF711AB711}", UnregisteredRttiClass);
            ChildOfUnregisteredRttiClass() {}
            ChildOfUnregisteredRttiClass(SerializeContext& sc)
            {
                sc.Class<ChildOfUnregisteredRttiClass, UnregisteredRttiClass>();
            }
        };
        struct UnserializableMembers
        {
            AZ_TYPE_INFO(UnserializableMembers, "{36F0C52A-5CAC-4060-982C-FC9A86D1393A}");

            UnserializableMembers() {}
            UnserializableMembers(SerializeContext& sc)
                : m_childOfUnregisteredRttiBase(sc)
                , m_childOfUnregisteredBase(&m_childOfUnregisteredRttiBase)
                , m_basePtrToGenericChild(&m_unserializableGeneric)
            {
                m_vectorUnregisteredClass.push_back();
                m_vectorUnregisteredRttiClass.push_back();
                m_vectorUnregisteredRttiBase.push_back(&m_unregisteredRttiMember);
                m_vectorGenericChildPtr.push_back(&m_unserializableGeneric);
                sc.Class<UnserializableMembers>()->
                    Field("unregisteredMember", &UnserializableMembers::m_unregisteredMember)->
                    Field("unregisteredRttiMember", &UnserializableMembers::m_unregisteredRttiMember)->
                    Field("childOfUnregisteredBase", &UnserializableMembers::m_childOfUnregisteredBase)->
                    Field("basePtrToGenericChild", &UnserializableMembers::m_basePtrToGenericChild)->
                    Field("vectorUnregisteredClass", &UnserializableMembers::m_vectorUnregisteredClass)->
                    Field("vectorUnregisteredRttiClass", &UnserializableMembers::m_vectorUnregisteredRttiClass)->
                    Field("vectorUnregisteredRttiBase", &UnserializableMembers::m_vectorUnregisteredRttiBase)->
                    Field("vectorGenericChildPtr", &UnserializableMembers::m_vectorGenericChildPtr);
            }

            ChildOfUnregisteredRttiClass            m_childOfUnregisteredRttiBase;
            GenericChild                            m_unserializableGeneric;

            UnregisteredClass                       m_unregisteredMember;
            UnregisteredRttiClass                   m_unregisteredRttiMember;
            UnregisteredRttiClass*                  m_childOfUnregisteredBase;
            GenericClass*                           m_basePtrToGenericChild;
            AZStd::vector<UnregisteredClass>        m_vectorUnregisteredClass;
            AZStd::vector<UnregisteredRttiClass>    m_vectorUnregisteredRttiClass;
            AZStd::vector<UnregisteredRttiClass*>   m_vectorUnregisteredRttiBase;
            AZStd::vector<GenericClass*>            m_vectorGenericChildPtr;
        };
    }

    // Tests that reflection of classes with no base types and those with base types will reflect and unreflect
    // as expected using the templated function, Class()
    TEST_F(Serialization, ClassReflectAndUnreflect)
    {
        using namespace SerializeTestClasses;

        m_serializeContext->Class<SerializeTestClasses::MyClassMix>();
        m_serializeContext->Class<BaseRtti>();

        {
            AZStd::vector<AZ::Uuid> foundUuids = m_serializeContext->FindClassId(AZ::Crc32(AzTypeInfo<BaseRtti>::Name()));
            ASSERT_FALSE(foundUuids.empty());
            EXPECT_EQ(foundUuids.size(), 1);
            EXPECT_EQ(foundUuids[0], AZ::Uuid::CreateString("{2581047D-26EC-4969-8354-BA0A4510C51A}"));
            EXPECT_NE(m_serializeContext->FindClassData(azrtti_typeid<BaseRtti>()), nullptr);
            AZStd::any testAnyCreate = m_serializeContext->CreateAny(azrtti_typeid<BaseRtti>());
            EXPECT_FALSE(testAnyCreate.empty());
            EXPECT_TRUE(testAnyCreate.is<BaseRtti>());
        }

        {
            AZStd::vector<AZ::Uuid> foundUuids = m_serializeContext->FindClassId(AZ::Crc32(AzTypeInfo<SerializeTestClasses::MyClassMix>::Name()));
            ASSERT_FALSE(foundUuids.empty());
            EXPECT_EQ(foundUuids.size(), 1);
            EXPECT_EQ(foundUuids[0], AZ::Uuid::CreateString("{A15003C6-797A-41BB-9D21-716DF0678D02}"));
            EXPECT_NE(m_serializeContext->FindClassData(azrtti_typeid<SerializeTestClasses::MyClassMix>()), nullptr);
            AZStd::any testAnyCreate = m_serializeContext->CreateAny(azrtti_typeid<SerializeTestClasses::MyClassMix>());
            EXPECT_FALSE(testAnyCreate.empty());
            EXPECT_TRUE(testAnyCreate.is<SerializeTestClasses::MyClassMix>());
        }

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<SerializeTestClasses::MyClassMix>();
        m_serializeContext->Class<BaseRtti>();
        m_serializeContext->DisableRemoveReflection();

        {
            AZStd::vector<AZ::Uuid> foundUuids = m_serializeContext->FindClassId(AZ::Crc32(AzTypeInfo<BaseRtti>::Name()));
            EXPECT_TRUE(foundUuids.empty());
            EXPECT_EQ(m_serializeContext->FindClassData(azrtti_typeid<BaseRtti>()), nullptr);
            AZStd::any testAnyCreate = m_serializeContext->CreateAny(azrtti_typeid<BaseRtti>());
            EXPECT_TRUE(testAnyCreate.empty());
            EXPECT_FALSE(testAnyCreate.is<BaseRtti>());
        }

        {
            AZStd::vector<AZ::Uuid> foundUuids = m_serializeContext->FindClassId(AZ::Crc32(AzTypeInfo<SerializeTestClasses::MyClassMix>::Name()));
            EXPECT_TRUE(foundUuids.empty());
            EXPECT_EQ(m_serializeContext->FindClassData(azrtti_typeid<SerializeTestClasses::MyClassMix>()), nullptr);
            AZStd::any testAnyCreate = m_serializeContext->CreateAny(azrtti_typeid<SerializeTestClasses::MyClassMix>());
            EXPECT_TRUE(testAnyCreate.empty());
            EXPECT_FALSE(testAnyCreate.is<SerializeTestClasses::MyClassMix>());
        }
    }

    TEST_F(Serialization, ErrorTest)
    {
        using namespace Error;
        class ErrorTest
        {
        public:
            void SaveObjects(ObjectStream* writer, SerializeContext* sc)
            {
                static int i = 0;
                bool success;

                // test saving root unregistered class
                if (i == 0)
                {
                    UnregisteredClass unregisteredClass;
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    success = writer->WriteClass(&unregisteredClass);
                    EXPECT_TRUE(!success);
                    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                }

                // test saving root unregistered Rtti class
                else if (i == 1)
                {
                    UnregisteredRttiClass unregisteredRttiClass;
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    success = writer->WriteClass(&unregisteredRttiClass);
                    EXPECT_TRUE(!success);
                    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                }

                // test saving root generic class
                else if (i == 2)
                {
                    GenericClass genericClass;
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    success = writer->WriteClass(&genericClass);
                    EXPECT_TRUE(!success);
                    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                }

                // test saving as pointer to unregistered base class with no rtti
                else if (i == 3)
                {
                    ChildOfUnregisteredClass childOfUnregisteredClass(*sc);
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    success = writer->WriteClass(static_cast<UnregisteredClass*>(&childOfUnregisteredClass));
                    EXPECT_TRUE(!success);
                    AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                }

                // test saving unserializable members
                else if (i == 4)
                {
                    UnserializableMembers badMembers(*sc);
                    AZ_TEST_START_TRACE_SUPPRESSION;
                    success = writer->WriteClass(&badMembers);
                    EXPECT_TRUE(!success);
                    AZ_TEST_STOP_TRACE_SUPPRESSION(8); // 1 failure for each member
                }
                i++;
            }

            void run()
            {
                AZStd::vector<char> buffer;
                IO::ByteContainerStream<AZStd::vector<char> > stream(&buffer);

                // test saving root unregistered class
                {
                    SerializeContext sc;
                    ObjectStream* objStream = ObjectStream::Create(&stream, sc, ObjectStream::ST_XML);
                    SaveObjects(objStream, &sc);
                    objStream->Finalize();
                }
                // test saving root unregistered Rtti class
                {
                    SerializeContext sc;
                    ObjectStream* objStream = ObjectStream::Create(&stream, sc, ObjectStream::ST_XML);
                    SaveObjects(objStream, &sc);
                    objStream->Finalize();
                }
                // test saving root generic class
                {
                    SerializeContext sc;
                    ObjectStream* objStream = ObjectStream::Create(&stream, sc, ObjectStream::ST_XML);
                    SaveObjects(objStream, &sc);
                    objStream->Finalize();
                }
                // test saving as pointer to unregistered base class with no rtti
                {
                    SerializeContext sc;
                    ObjectStream* objStream = ObjectStream::Create(&stream, sc, ObjectStream::ST_XML);
                    SaveObjects(objStream, &sc);
                    objStream->Finalize();
                }
                // test saving unserializable members
                // errors covered:
                //  - unregistered type with no rtti
                //  - unregistered type with rtti
                //  - pointer to unregistered base with rtti
                //  - base pointer pointing to a generic child
                //  - vector of unregistered types
                //  - vector of unregistered types with rtti
                //  - vector of pointers to unregistered base with rtti
                //  - vector of base pointers pointing to generic child
                {
                    SerializeContext sc;
                    ObjectStream* objStream = ObjectStream::Create(&stream, sc, ObjectStream::ST_XML);
                    SaveObjects(objStream, &sc);
                    objStream->Finalize();
                }
            }
        };

        ErrorTest test;
        test.run();
    }

    namespace EditTest
    {
        struct MyEditStruct
        {
            AZ_TYPE_INFO(MyEditStruct, "{89CCD760-A556-4EDE-98C0-33FD9DD556B9}")

                MyEditStruct()
                : m_data(11)
                , m_specialData(3) {}

            int     Foo(int m) { return 5 * m; }
            bool    IsShowSpecialData() const { return true; }
            int     GetDataOption(int option) { return option * 2; }

            int m_data;
            int m_specialData;
        };

        int MyEditGlobalFunc(int m)
        {
            return 4 * m;
        }

        class MyEditStruct2
        {
        public:
            AZ_TYPE_INFO(MyEditStruct2, "{FFD27958-9856-4CE2-AE13-18878DE5ECE0}");

            MyEditStruct m_myEditStruct;
        };

        class MyEditStruct3
        {
        public:
            enum EditEnum
            {
                ENUM_Test1 = 1,
                ENUM_Test2 = 2,
                ENUM_Test3 = -1,
                ENUM_Test4 = INT_MAX,
            };

            enum class EditEnumClass : AZ::u8
            {
                EEC_1,
                EEC_2,
                EEC_255 = 255,
            };

        public:
            AZ_TYPE_INFO(MyEditStruct3, "{11F859C7-7A15-49C8-8A38-783A1EFC0E06}");

            EditEnum m_enum;
            EditEnum m_enum2;
            EditEnumClass m_enumClass;
        };
    }
} // namespace UnitTest
AZ_TYPE_INFO_SPECIALIZE(UnitTest::EditTest::MyEditStruct3::EditEnum, "{4AF433C2-055E-4E34-921A-A7D16AB548CA}");
AZ_TYPE_INFO_SPECIALIZE(UnitTest::EditTest::MyEditStruct3::EditEnumClass, "{4FEC2F0B-A599-4FCD-836B-89E066791793}");


namespace UnitTest
{
    TEST_F(Serialization, EditContextTest)
    {
        using namespace EditTest;

        class EditContextTest
        {
        public:
            bool BeginSerializationElement(SerializeContext* sc, void* instance, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement)
            {
                (void)instance;
                (void)classData;
                (void)classElement;

                if (classElement)
                {
                    // if we are a pointer, then we may be pointing to a derived type.
                    if (classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER)
                    {
                        // if dataAddress is a pointer in this case, cast it's value to a void* (or const void*) and dereference to get to the actual class.
                        instance = *(void**)(instance);
                        if (instance && classElement->m_azRtti)
                        {
                            AZ::Uuid actualClassId = classElement->m_azRtti->GetActualUuid(instance);
                            if (actualClassId != classElement->m_typeId)
                            {
                                // we are pointing to derived type, adjust class data, uuid and pointer.
                                classData = sc->FindClassData(actualClassId);
                                if (classData)
                                {
                                    instance = classElement->m_azRtti->Cast(instance, classData->m_azRtti->GetTypeId());
                                }
                            }
                        }
                    }
                }

                if (strcmp(classData->m_name, "MyEditStruct") == 0)
                {
                    EXPECT_TRUE(classData->m_editData != nullptr);
                    EXPECT_EQ( 0, strcmp(classData->m_editData->m_name, "MyEditStruct") );
                    EXPECT_EQ( 0, strcmp(classData->m_editData->m_description, "My edit struct class used for ...") );
                    EXPECT_EQ( 2, classData->m_editData->m_elements.size() );
                    EXPECT_EQ( 0, strcmp(classData->m_editData->m_elements.front().m_description, "Special data group") );
                    EXPECT_EQ( 1, classData->m_editData->m_elements.front().m_attributes.size() );
                    EXPECT_TRUE(classData->m_editData->m_elements.front().m_attributes[0].first == AZ_CRC("Callback", 0x79f97426) );
                }
                else if (classElement && classElement->m_editData && strcmp(classElement->m_editData->m_description, "Type") == 0)
                {
                    EXPECT_EQ( 2, classElement->m_editData->m_attributes.size() );
                    // Number of options attribute
                    EXPECT_EQ(classElement->m_editData->m_attributes[0].first, AZ_CRC("NumOptions", 0x90274abc));
                    Edit::AttributeData<int>* intData = azrtti_cast<Edit::AttributeData<int>*>(classElement->m_editData->m_attributes[0].second);
                    EXPECT_TRUE(intData != nullptr);
                    EXPECT_EQ( 3, intData->Get(instance) );
                    // Get options attribute
                    EXPECT_EQ( classElement->m_editData->m_attributes[1].first, AZ_CRC("Options", 0xd035fa87));
                    Edit::AttributeFunction<int(int)>* funcData = azrtti_cast<Edit::AttributeFunction<int(int)>*>(classElement->m_editData->m_attributes[1].second);
                    EXPECT_TRUE(funcData != nullptr);
                    EXPECT_EQ( 20, funcData->Invoke(instance, 10) );
                }
                return true;
            }

            bool EndSerializationElement()
            {
                return true;
            }

            void run()
            {
                SerializeContext serializeContext;

                // We must expose the class for serialization first.
                serializeContext.Class<MyEditStruct>()->
                    Field("data", &MyEditStruct::m_data);

                serializeContext.Class<MyEditStruct2>()->
                    Field("m_myEditStruct", &MyEditStruct2::m_myEditStruct);

                serializeContext.Class<MyEditStruct3>()->
                    Field("m_enum", &MyEditStruct3::m_enum)->
                    Field("m_enum2", &MyEditStruct3::m_enum2)->
                    Field("m_enumClass", &MyEditStruct3::m_enumClass);

                // create edit context
                serializeContext.CreateEditContext();
                EditContext* editContext = serializeContext.GetEditContext();

                // reflect the class for editing
                editContext->Class<MyEditStruct>("MyEditStruct", "My edit struct class used for ...")->
                    ClassElement(AZ::Edit::ClassElements::Group, "Special data group")->
                    Attribute("Callback", &MyEditStruct::IsShowSpecialData)->
                    DataElement("ComboSelector", &MyEditStruct::m_data, "Name", "Type")->
                    Attribute("NumOptions", 3)->
                    Attribute("Options", &MyEditStruct::GetDataOption);

                // reflect class by using the element edit reflection as name/descriptor
                editContext->Class<MyEditStruct2>("MyEditStruct2", "My edit struct class 2 with redirected data element...")->
                    DataElement("ComboSelector", &MyEditStruct2::m_myEditStruct)->
                    Attribute("NumOptions", 3);

                // enumerate elements and verify the class reflection..
                MyEditStruct myObj;
                serializeContext.EnumerateObject(&myObj,
                    AZStd::bind(&EditContextTest::BeginSerializationElement, this, &serializeContext, AZStd::placeholders::_1, AZStd::placeholders::_2, AZStd::placeholders::_3),
                    AZStd::bind(&EditContextTest::EndSerializationElement, this),
                    SerializeContext::ENUM_ACCESS_FOR_READ);

                editContext->Enum<MyEditStruct3::EditEnum>("EditEnum", "The enum for testing the Enum<>() call")->
                    Value("Test1", MyEditStruct3::EditEnum::ENUM_Test1)->
                    Value("Test2", MyEditStruct3::EditEnum::ENUM_Test2)->
                    Value("Test3", MyEditStruct3::EditEnum::ENUM_Test3)->
                    Value("Test4", MyEditStruct3::EditEnum::ENUM_Test4);

                editContext->Enum<MyEditStruct3::EditEnumClass>("EditEnumClass", "The enum class for testing the Enum<>() call")->
                    Value("One", MyEditStruct3::EditEnumClass::EEC_1)->
                    Value("Two", MyEditStruct3::EditEnumClass::EEC_2)->
                    Value("TwoFiftyFive", MyEditStruct3::EditEnumClass::EEC_255);

                AZ_TEST_START_TRACE_SUPPRESSION;
                editContext->Class<MyEditStruct3>("MyEditStruct3", "Used to test enum global reflection")->
                    DataElement("Enum", &MyEditStruct3::m_enum)-> // safe
                    DataElement("Enum2", &MyEditStruct3::m_enum2)-> // safe
                    EnumAttribute(MyEditStruct3::EditEnum::ENUM_Test1, "THIS SHOULD CAUSE AN ERROR")->
                    Attribute(AZ::Edit::Attributes::EnumValues, AZStd::vector<AZ::Edit::EnumConstant<MyEditStruct3::EditEnum>> {
                        AZ::Edit::EnumConstant<MyEditStruct3::EditEnum>(MyEditStruct3::EditEnum::ENUM_Test1, "EnumTest1 - ERROR"),
                        AZ::Edit::EnumConstant<MyEditStruct3::EditEnum>(MyEditStruct3::EditEnum::ENUM_Test2, "EnumTest2 - ERROR"),
                        AZ::Edit::EnumConstant<MyEditStruct3::EditEnum>(MyEditStruct3::EditEnum::ENUM_Test3, "EnumTest3 - ERROR"),
                        AZ::Edit::EnumConstant<MyEditStruct3::EditEnum>(MyEditStruct3::EditEnum::ENUM_Test4, "EnumTest4 - ERROR"),
                    })->
                    ElementAttribute(AZ::Edit::InternalAttributes::EnumValue, AZStd::make_pair(MyEditStruct3::EditEnum::ENUM_Test1, "THIS SHOULD ALSO CAUSE AN ERROR"));
                AZ_TEST_STOP_TRACE_SUPPRESSION(0);
            }
        };

        EditContextTest test;
        test.run();
    }


    /**
    * Test cases when (usually with DLLs) we have to unload parts of the reflected context
    */
    TEST_F(Serialization, UnregisterTest)
    {
        using namespace EditTest;

        auto reflectClasses = [](SerializeContext* context)
        {
            context->Class<MyEditStruct>()->
                Field("data", &MyEditStruct::m_data);
        };

        SerializeContext serializeContext;

        // Register class
        reflectClasses(&serializeContext);

        // enumerate elements and verify the class reflection..
        MyEditStruct myObj;
        EXPECT_TRUE(serializeContext.FindClassData(AzTypeInfo<MyEditStruct>::Uuid()) != nullptr);
        EXPECT_EQ( 0, strcmp(serializeContext.FindClassData(AzTypeInfo<MyEditStruct>::Uuid())->m_name, "MyEditStruct") );

        // remove the class from the context
        serializeContext.EnableRemoveReflection();
        reflectClasses(&serializeContext);
        serializeContext.DisableRemoveReflection();
        EXPECT_EQ( nullptr, serializeContext.FindClassData(AzTypeInfo<MyEditStruct>::Uuid()) );

        // Register class again
        reflectClasses(&serializeContext);
        EXPECT_EQ( nullptr, serializeContext.FindClassData(AzTypeInfo<MyEditStruct>::Uuid())->m_editData ); // no edit data yet

                                                                                                                    // create edit context
        serializeContext.CreateEditContext();
        EditContext* editContext = serializeContext.GetEditContext();

        // reflect the class for editing
        editContext->Class<MyEditStruct>("MyEditStruct", "My edit struct class used for ...")->
            ClassElement(AZ::Edit::ClassElements::Group, "Special data group")->
            Attribute("Callback", &MyEditStruct::IsShowSpecialData)->
            DataElement("ComboSelector", &MyEditStruct::m_data, "Name", "Type")->
            Attribute("NumOptions", 3)->
            Attribute("Options", &MyEditStruct::GetDataOption);

        editContext->Enum<MyEditStruct3::EditEnumClass>("Load Type", "Automatic or Manual loading and unloading")
            ->Value("EEC_1", MyEditStruct3::EditEnumClass::EEC_1)
            ->Value("EEC_2", MyEditStruct3::EditEnumClass::EEC_2)
            ->Value("EEC_255", MyEditStruct3::EditEnumClass::EEC_255);

        EXPECT_TRUE(serializeContext.FindClassData(AzTypeInfo<MyEditStruct>::Uuid())->m_editData != nullptr);
        EXPECT_EQ( 0, strcmp(serializeContext.FindClassData(AzTypeInfo<MyEditStruct>::Uuid())->m_editData->m_name, "MyEditStruct") );

        // remove the class from the context
        serializeContext.EnableRemoveReflection();
        reflectClasses(&serializeContext);
        serializeContext.DisableRemoveReflection();
        EXPECT_EQ( nullptr, serializeContext.FindClassData(AzTypeInfo<MyEditStruct>::Uuid()) );
    }

    namespace LargeData
    {
        class InnerPayload
        {
        public:

            AZ_CLASS_ALLOCATOR(InnerPayload, AZ::SystemAllocator, 0);
            AZ_RTTI(InnerPayload, "{3423157C-C6C5-4914-BB5C-B656439B8D3D}");

            AZStd::string m_textData;

            InnerPayload()
            {
                m_textData = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi sed pellentesque nibh. Mauris ac ipsum ante. Mauris dignissim vehicula dui, et mollis mauris tincidunt non. Aliquam sodales diam ante, in vestibulum nibh ultricies et. Pellentesque accumsan porta vulputate. Donec vel fringilla sem. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nam eu erat eu est mollis condimentum ut eget metus."
                    "Sed nec felis enim.Ut auctor arcu nec tristique volutpat.Nulla viverra vulputate nibh et fringilla.Curabitur sagittis eu libero ullamcorper porta.Ut ac nisi vitae massa luctus tristique.Donec scelerisque, odio at pharetra consectetur, nunc urna porta ligula, tincidunt auctor orci purus non nisi.Nulla at risus at lacus vestibulum varius vitae ac tellus.Etiam ut sem commodo justo tempor congue vel id odio.Duis erat sem, condimentum a neque id, bibendum consectetur ligula.In eget massa lectus.Interdum et malesuada fames ac ante ipsum primis in faucibus.Ut ornare lectus at sem condimentum gravida vel ut est."
                    "Curabitur nisl metus, euismod in enim eu, pulvinar ullamcorper lorem.Morbi et adipiscing nisi.Aliquam id dapibus sapien.Aliquam facilisis, lacus porta interdum mattis, erat metus tempus ligula, nec cursus augue tellus ut urna.Sed sagittis arcu vel magna consequat, eget eleifend quam tincidunt.Maecenas non ornare nisi, placerat ornare orci.Proin auctor in nunc eu ultrices.Vivamus interdum imperdiet sapien nec cursus."
                    "Etiam et iaculis tortor.Nam lacus risus, rutrum a mollis quis, accumsan quis risus.Mauris ac fringilla lectus.Cras posuere massa ultricies libero fermentum, in convallis metus porttitor.Duis hendrerit gravida neque at ultricies.Vestibulum semper congue gravida.Etiam vel mi quis risus ornare convallis nec et elit.Praesent a mollis erat, in eleifend libero.Fusce porttitor malesuada velit, nec pharetra justo rutrum sit amet.Ut vel egestas lacus, sit amet posuere nunc."
                    "Maecenas in eleifend risus.Integer volutpat sodales massa vitae consequat.Cras urna turpis, laoreet sed ante sit amet, dictum commodo sem.Vivamus porta, neque vel blandit dictum, enim metus molestie nisl, a consectetur libero odio eu magna.Maecenas nisi nibh, dignissim et nisi eget, adipiscing auctor ligula.Sed in nisl libero.Maecenas aliquam urna orci, ac ultrices massa sollicitudin vitae.Donec ullamcorper suscipit viverra.Praesent dolor ipsum, tincidunt eu quam sit amet, aliquam cursus orci.Praesent elementum est sit amet lectus imperdiet interdum.Pellentesque et sem et nulla tempus cursus.Sed enim dolor, viverra eu mauris id, ornare congue urna."
                    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi sed pellentesque nibh. Mauris ac ipsum ante. Mauris dignissim vehicula dui, et mollis mauris tincidunt non. Aliquam sodales diam ante, in vestibulum nibh ultricies et. Pellentesque accumsan porta vulputate. Donec vel fringilla sem. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nam eu erat eu est mollis condimentum ut eget metus."
                    "Sed nec felis enim.Ut auctor arcu nec tristique volutpat.Nulla viverra vulputate nibh et fringilla.Curabitur sagittis eu libero ullamcorper porta.Ut ac nisi vitae massa luctus tristique.Donec scelerisque, odio at pharetra consectetur, nunc urna porta ligula, tincidunt auctor orci purus non nisi.Nulla at risus at lacus vestibulum varius vitae ac tellus.Etiam ut sem commodo justo tempor congue vel id odio.Duis erat sem, condimentum a neque id, bibendum consectetur ligula.In eget massa lectus.Interdum et malesuada fames ac ante ipsum primis in faucibus.Ut ornare lectus at sem condimentum gravida vel ut est."
                    "Curabitur nisl metus, euismod in enim eu, pulvinar ullamcorper lorem.Morbi et adipiscing nisi.Aliquam id dapibus sapien.Aliquam facilisis, lacus porta interdum mattis, erat metus tempus ligula, nec cursus augue tellus ut urna.Sed sagittis arcu vel magna consequat, eget eleifend quam tincidunt.Maecenas non ornare nisi, placerat ornare orci.Proin auctor in nunc eu ultrices.Vivamus interdum imperdiet sapien nec cursus."
                    "Etiam et iaculis tortor.Nam lacus risus, rutrum a mollis quis, accumsan quis risus.Mauris ac fringilla lectus.Cras posuere massa ultricies libero fermentum, in convallis metus porttitor.Duis hendrerit gravida neque at ultricies.Vestibulum semper congue gravida.Etiam vel mi quis risus ornare convallis nec et elit.Praesent a mollis erat, in eleifend libero.Fusce porttitor malesuada velit, nec pharetra justo rutrum sit amet.Ut vel egestas lacus, sit amet posuere nunc."
                    "Maecenas in eleifend risus.Integer volutpat sodales massa vitae consequat.Cras urna turpis, laoreet sed ante sit amet, dictum commodo sem.Vivamus porta, neque vel blandit dictum, enim metus molestie nisl, a consectetur libero odio eu magna.Maecenas nisi nibh, dignissim et nisi eget, adipiscing auctor ligula.Sed in nisl libero.Maecenas aliquam urna orci, ac ultrices massa sollicitudin vitae.Donec ullamcorper suscipit viverra.Praesent dolor ipsum, tincidunt eu quam sit amet, aliquam cursus orci.Praesent elementum est sit amet lectus imperdiet interdum.Pellentesque et sem et nulla tempus cursus.Sed enim dolor, viverra eu mauris id, ornare congue urna."
                    ;
            }

            virtual ~InnerPayload()
            {}

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<InnerPayload>()->
                    Version(5, &InnerPayload::ConvertOldVersions)->
                    Field("m_textData", &InnerPayload::m_textData)
                    ;
            }

            static bool ConvertOldVersions(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                (void)context;
                (void)classElement;
                return false;
            }
        };

        class Payload
        {
        public:

            AZ_CLASS_ALLOCATOR(Payload, AZ::SystemAllocator, 0);
            AZ_RTTI(Payload, "{7A14FC65-44FB-4956-B5BC-4CFCBF36E1AE}");

            AZStd::string m_textData;
            AZStd::string m_newTextData;

            InnerPayload m_payload;

            SerializeContext m_context;

            Payload()
            {
                m_textData = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi sed pellentesque nibh. Mauris ac ipsum ante. Mauris dignissim vehicula dui, et mollis mauris tincidunt non. Aliquam sodales diam ante, in vestibulum nibh ultricies et. Pellentesque accumsan porta vulputate. Donec vel fringilla sem. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nam eu erat eu est mollis condimentum ut eget metus."
                    "Sed nec felis enim.Ut auctor arcu nec tristique volutpat.Nulla viverra vulputate nibh et fringilla.Curabitur sagittis eu libero ullamcorper porta.Ut ac nisi vitae massa luctus tristique.Donec scelerisque, odio at pharetra consectetur, nunc urna porta ligula, tincidunt auctor orci purus non nisi.Nulla at risus at lacus vestibulum varius vitae ac tellus.Etiam ut sem commodo justo tempor congue vel id odio.Duis erat sem, condimentum a neque id, bibendum consectetur ligula.In eget massa lectus.Interdum et malesuada fames ac ante ipsum primis in faucibus.Ut ornare lectus at sem condimentum gravida vel ut est."
                    "Curabitur nisl metus, euismod in enim eu, pulvinar ullamcorper lorem.Morbi et adipiscing nisi.Aliquam id dapibus sapien.Aliquam facilisis, lacus porta interdum mattis, erat metus tempus ligula, nec cursus augue tellus ut urna.Sed sagittis arcu vel magna consequat, eget eleifend quam tincidunt.Maecenas non ornare nisi, placerat ornare orci.Proin auctor in nunc eu ultrices.Vivamus interdum imperdiet sapien nec cursus."
                    "Etiam et iaculis tortor.Nam lacus risus, rutrum a mollis quis, accumsan quis risus.Mauris ac fringilla lectus.Cras posuere massa ultricies libero fermentum, in convallis metus porttitor.Duis hendrerit gravida neque at ultricies.Vestibulum semper congue gravida.Etiam vel mi quis risus ornare convallis nec et elit.Praesent a mollis erat, in eleifend libero.Fusce porttitor malesuada velit, nec pharetra justo rutrum sit amet.Ut vel egestas lacus, sit amet posuere nunc."
                    "Maecenas in eleifend risus.Integer volutpat sodales massa vitae consequat.Cras urna turpis, laoreet sed ante sit amet, dictum commodo sem.Vivamus porta, neque vel blandit dictum, enim metus molestie nisl, a consectetur libero odio eu magna.Maecenas nisi nibh, dignissim et nisi eget, adipiscing auctor ligula.Sed in nisl libero.Maecenas aliquam urna orci, ac ultrices massa sollicitudin vitae.Donec ullamcorper suscipit viverra.Praesent dolor ipsum, tincidunt eu quam sit amet, aliquam cursus orci.Praesent elementum est sit amet lectus imperdiet interdum.Pellentesque et sem et nulla tempus cursus.Sed enim dolor, viverra eu mauris id, ornare congue urna."
                    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi sed pellentesque nibh. Mauris ac ipsum ante. Mauris dignissim vehicula dui, et mollis mauris tincidunt non. Aliquam sodales diam ante, in vestibulum nibh ultricies et. Pellentesque accumsan porta vulputate. Donec vel fringilla sem. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nam eu erat eu est mollis condimentum ut eget metus."
                    "Sed nec felis enim.Ut auctor arcu nec tristique volutpat.Nulla viverra vulputate nibh et fringilla.Curabitur sagittis eu libero ullamcorper porta.Ut ac nisi vitae massa luctus tristique.Donec scelerisque, odio at pharetra consectetur, nunc urna porta ligula, tincidunt auctor orci purus non nisi.Nulla at risus at lacus vestibulum varius vitae ac tellus.Etiam ut sem commodo justo tempor congue vel id odio.Duis erat sem, condimentum a neque id, bibendum consectetur ligula.In eget massa lectus.Interdum et malesuada fames ac ante ipsum primis in faucibus.Ut ornare lectus at sem condimentum gravida vel ut est."
                    "Curabitur nisl metus, euismod in enim eu, pulvinar ullamcorper lorem.Morbi et adipiscing nisi.Aliquam id dapibus sapien.Aliquam facilisis, lacus porta interdum mattis, erat metus tempus ligula, nec cursus augue tellus ut urna.Sed sagittis arcu vel magna consequat, eget eleifend quam tincidunt.Maecenas non ornare nisi, placerat ornare orci.Proin auctor in nunc eu ultrices.Vivamus interdum imperdiet sapien nec cursus."
                    "Etiam et iaculis tortor.Nam lacus risus, rutrum a mollis quis, accumsan quis risus.Mauris ac fringilla lectus.Cras posuere massa ultricies libero fermentum, in convallis metus porttitor.Duis hendrerit gravida neque at ultricies.Vestibulum semper congue gravida.Etiam vel mi quis risus ornare convallis nec et elit.Praesent a mollis erat, in eleifend libero.Fusce porttitor malesuada velit, nec pharetra justo rutrum sit amet.Ut vel egestas lacus, sit amet posuere nunc."
                    "Maecenas in eleifend risus.Integer volutpat sodales massa vitae consequat.Cras urna turpis, laoreet sed ante sit amet, dictum commodo sem.Vivamus porta, neque vel blandit dictum, enim metus molestie nisl, a consectetur libero odio eu magna.Maecenas nisi nibh, dignissim et nisi eget, adipiscing auctor ligula.Sed in nisl libero.Maecenas aliquam urna orci, ac ultrices massa sollicitudin vitae.Donec ullamcorper suscipit viverra.Praesent dolor ipsum, tincidunt eu quam sit amet, aliquam cursus orci.Praesent elementum est sit amet lectus imperdiet interdum.Pellentesque et sem et nulla tempus cursus.Sed enim dolor, viverra eu mauris id, ornare congue urna."
                    ;
            }

            virtual ~Payload() {}

            static bool ConvertOldVersions(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                (void)classElement;
                (void)context;
                if (classElement.GetVersion() == 4)
                {
                    // convert from version 0
                    AZStd::string newData;
                    for (int i = 0; i < classElement.GetNumSubElements(); ++i)
                    {
                        AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(i);

                        if (elementNode.GetName() == AZ_CRC("m_textData", 0xfc7870e5))
                        {
                            bool result = elementNode.GetData(newData);
                            EXPECT_TRUE(result);
                            classElement.RemoveElement(i);
                            break;
                        }
                    }

                    for (int i = 0; i < classElement.GetNumSubElements(); ++i)
                    {
                        AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(i);
                        if (elementNode.GetName() == AZ_CRC("m_newTextData", 0x3feafc3d))
                        {
                            elementNode.SetData(context, newData);
                            break;
                        }
                    }
                    return true;
                }

                return false; // just discard unknown versions
            }

            static void Reflect(AZ::SerializeContext& sc)
            {
                sc.Class<Payload>()->
                    Version(5, &Payload::ConvertOldVersions)->
                    Field("m_textData", &Payload::m_textData)->
                    Field("m_newTextData", &Payload::m_newTextData)->
                    Field("m_payload", &Payload::m_payload)
                    ;
            }

            void SaveObjects(ObjectStream* writer)
            {
                bool success = true;
                success = writer->WriteClass(this);
                EXPECT_TRUE(success);
            }

            void TestSave(IO::GenericStream* stream, ObjectStream::StreamType format)
            {
                ObjectStream* objStream = ObjectStream::Create(stream, m_context, format);
                SaveObjects(objStream);
                bool done = objStream->Finalize();
                EXPECT_TRUE(done);
            }
        };
    }

    /*
    * Test serialization using FileUtil.
    * FileUtil interacts with the serialization context through the ComponentApplicationBus.
    */
    class SerializationFileUtil
        : public Serialization
    {
    public:
        void SetUp() override
        {
            Serialization::SetUp();
            m_prevFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(&m_fileIO);

            BaseRtti::Reflect(*m_serializeContext);
        }

        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
            Serialization::TearDown();
        }

        void TestFileUtilsStream(AZ::DataStream::StreamType streamType)
        {
            BaseRtti toSerialize;
            toSerialize.m_data = false;

            // Test Stream Write
            AZStd::vector<char> charBuffer;
            IO::ByteContainerStream<AZStd::vector<char> > charStream(&charBuffer);
            bool success = AZ::Utils::SaveObjectToStream(charStream, streamType, &toSerialize);
            EXPECT_TRUE(success);

            // Test Stream Read
            // Set the stream to the beginning so what was written can be read.
            charStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            BaseRtti* deserialized = AZ::Utils::LoadObjectFromStream<BaseRtti>(charStream);
            EXPECT_TRUE(deserialized);
            EXPECT_EQ( toSerialize.m_data, deserialized->m_data );
            delete deserialized;
            deserialized = nullptr;

            // Test LoadObjectFromBuffer
            // First, save the object to a u8 buffer.
            AZStd::vector<u8> u8Buffer;
            IO::ByteContainerStream<AZStd::vector<u8> > u8Stream(&u8Buffer);
            success = AZ::Utils::SaveObjectToStream(u8Stream, streamType, &toSerialize);
            EXPECT_TRUE(success);
            u8Stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            deserialized = AZ::Utils::LoadObjectFromBuffer<BaseRtti>(&u8Buffer[0], u8Buffer.size());
            EXPECT_TRUE(deserialized);
            EXPECT_EQ( toSerialize.m_data, deserialized->m_data );
            delete deserialized;
            deserialized = nullptr;

            // Write to stream twice, read once.
            // Note that subsequent calls to write to stream will be ignored.
            // Note that many asserts here are commented out because the stream functionality was giving
            // unexpected results. There are rally stories on the backlog backlog (I e-mailed someone to put them on the backlog)
            // related to this.
            AZStd::vector<char> charBufferWriteTwice;
            IO::ByteContainerStream<AZStd::vector<char> > charStreamWriteTwice(&charBufferWriteTwice);
            success = AZ::Utils::SaveObjectToStream(charStreamWriteTwice, streamType, &toSerialize);
            EXPECT_TRUE(success);
            BaseRtti secondSerializedObject;
            secondSerializedObject.m_data = true;
            success = AZ::Utils::SaveObjectToStream(charStreamWriteTwice, streamType, &secondSerializedObject);
            // SaveObjectToStream currently returns success after attempting to save a second object.
            // This does not match up with the later behavior of loading from this stream.
            // Currently, saving twice returns a success on each save, and loading once returns the first object.
            // What should happen, is either the attempt to save onto the stream again should return false,
            // or the read should return the second object first.
            //EXPECT_TRUE(success);
            charStreamWriteTwice.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            deserialized = AZ::Utils::LoadObjectFromStream<BaseRtti>(charStreamWriteTwice);
            EXPECT_TRUE(deserialized);
            // Read the above text. This is here for whoever addresses these backlog items.
            //EXPECT_EQ( toSerialize.m_data, deserialized->m_data );
            //EXPECT_EQ( secondSerializedObject.m_data, deserialized->m_data );
            delete deserialized;
            deserialized = nullptr;
        }

        void TestFileUtilsFile(AZ::DataStream::StreamType streamType)
        {
            BaseRtti toSerialize;
            toSerialize.m_data = false;

            // Test save once, read once.
            AZStd::string filePath = GetTestFolderPath() + "FileUtilsTest";
            bool success = AZ::Utils::SaveObjectToFile(filePath, streamType, &toSerialize);
            EXPECT_TRUE(success);

            BaseRtti* deserialized = AZ::Utils::LoadObjectFromFile<BaseRtti>(filePath);
            EXPECT_TRUE(deserialized);
            EXPECT_EQ( toSerialize.m_data, deserialized->m_data );
            delete deserialized;
            deserialized = nullptr;

            // Test save twice, read once.
            // This is valid with files because saving a file again will overwrite it. Note that streams function differently.
            success = AZ::Utils::SaveObjectToFile(filePath, streamType, &toSerialize);
            EXPECT_TRUE(success);
            success = AZ::Utils::SaveObjectToFile(filePath, streamType, &toSerialize);
            EXPECT_TRUE(success);

            deserialized = AZ::Utils::LoadObjectFromFile<BaseRtti>(filePath);
            EXPECT_TRUE(deserialized);
            EXPECT_EQ( toSerialize.m_data, deserialized->m_data );
            delete deserialized;
            deserialized = nullptr;

            // Test reading from an invalid file. The system should return 'nullptr' when given a bad file path.
            AZ::IO::SystemFile::Delete(filePath.c_str());
            deserialized = AZ::Utils::LoadObjectFromFile<BaseRtti>(filePath);
            EXPECT_EQ( nullptr, deserialized );
        }

        TestFileIOBase m_fileIO;
        AZ::IO::FileIOBase* m_prevFileIO;
    };

    TEST_F(SerializationFileUtil, TestFileUtilsStream_XML)
    {
        TestFileUtilsStream(ObjectStream::ST_XML);
    }

    TEST_F(SerializationFileUtil, TestFileUtilsStream_Binary)
    {
        TestFileUtilsStream(ObjectStream::ST_BINARY);
    }

    TEST_F(SerializationFileUtil, DISABLED_TestFileUtilsFile_XML)
    {
        TestFileUtilsFile(ObjectStream::ST_XML);
    }

    TEST_F(SerializationFileUtil, DISABLED_TestFileUtilsFile_Binary)
    {
        TestFileUtilsFile(ObjectStream::ST_BINARY);
    }

    

    /*
    *
    */
    class SerializeDescendentDataElementTest
        : public AllocatorsFixture
    {
    public:
        struct DataElementTestClass
        {
            AZ_CLASS_ALLOCATOR(DataElementTestClass, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(DataElementTestClass, "{F515B922-BBB9-4216-A2C9-FD665AA30046}");

            DataElementTestClass() {}

            AZStd::unique_ptr<AZ::Entity> m_data;
            AZStd::vector<AZ::Vector2> m_positions;

        private:
            DataElementTestClass(const DataElementTestClass&) = delete;
        };

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_dataElementClass = AZStd::make_unique<DataElementTestClass>();
        }

        void TearDown() override
        {
            m_dataElementClass.reset(); // reset it before the allocators are destroyed

            AllocatorsFixture::TearDown();
        }

        static bool VersionConverter(AZ::SerializeContext& sc, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() == 0)
            {
                auto entityIdElements = AZ::Utils::FindDescendantElements(sc, classElement, AZStd::vector<AZ::Crc32>({ AZ_CRC("m_data"), AZ_CRC("element"), AZ_CRC("Id"), AZ_CRC("id") }));
                EXPECT_EQ(1, entityIdElements.size());
                AZ::u64 id1;
                EXPECT_TRUE(entityIdElements.front()->GetData(id1));
                EXPECT_EQ(47, id1);

                auto vector2Elements = AZ::Utils::FindDescendantElements(sc, classElement, AZStd::vector<AZ::Crc32>({ AZ_CRC("m_positions"), AZ_CRC("element") }));
                EXPECT_EQ(2, vector2Elements.size());
                AZ::Vector2 position;
                EXPECT_TRUE(vector2Elements[0]->GetData(position));
                EXPECT_FLOAT_EQ(1.0f, position.GetX());
                EXPECT_FLOAT_EQ(2.0f, position.GetY());

                EXPECT_TRUE(vector2Elements[1]->GetData(position));
                EXPECT_FLOAT_EQ(2.0f, position.GetX());
                EXPECT_FLOAT_EQ(4.0f, position.GetY());
            }

            return true;
        }

        AZStd::unique_ptr<DataElementTestClass> m_dataElementClass;

        void run()
        {
            m_dataElementClass->m_data = AZStd::make_unique<AZ::Entity>("DataElement");
            m_dataElementClass->m_data->SetId(AZ::EntityId(47));
            m_dataElementClass->m_positions.emplace_back(1.0f, 2.0f);
            m_dataElementClass->m_positions.emplace_back(2.0f, 4.0f);

            // Write original data
            AZStd::vector<AZ::u8> binaryBuffer;
            {
                AZ::SerializeContext sc;
                AZ::Entity::Reflect(&sc);
                sc.Class<DataElementTestClass>()
                    ->Version(0)
                    ->Field("m_data", &DataElementTestClass::m_data)
                    ->Field("m_positions", &DataElementTestClass::m_positions);

                // Binary
                AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > binaryStream(&binaryBuffer);
                AZ::ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
                binaryObjStream->WriteClass(m_dataElementClass.get());
                EXPECT_TRUE(binaryObjStream->Finalize());
            }

            // Test find descendant version converter
            {
                AZ::SerializeContext sc;
                AZ::Entity::Reflect(&sc);
                sc.Class<DataElementTestClass>()
                    ->Version(1, &VersionConverter)
                    ->Field("m_data", &DataElementTestClass::m_data)
                    ->Field("m_positions", &DataElementTestClass::m_positions);

                // Binary
                IO::ByteContainerStream<const AZStd::vector<AZ::u8> > binaryStream(&binaryBuffer);
                binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);

                AZ::ObjectStream::ClassReadyCB readyCB([&](void* classPtr, const AZ::Uuid& classId, AZ::SerializeContext* sc)
                {
                    AZ_UNUSED(classId);
                    AZ_UNUSED(sc);

                    delete reinterpret_cast<DataElementTestClass*>(classPtr);
                });
                ObjectStream::LoadBlocking(&binaryStream, sc, readyCB);
            }
        }
    };

    TEST_F(SerializeDescendentDataElementTest, FindTest)
    {
        run();
    }

    class SerializeDataElementNodeTreeTest
        : public AllocatorsFixture
    {
    public:
        struct EntityWrapperTest
        {
            AZ_CLASS_ALLOCATOR(EntityWrapperTest, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(EntityWrapperTest, "{BCBC25C3-3D6F-4FC4-B73D-51E6FBD38730}");

            AZ::Entity* m_entity = nullptr;
        };

        struct ContainerTest
        {
            AZ_CLASS_ALLOCATOR(ContainerTest, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(ContainerTest, "{88FD1BBA-EE9C-4165-8C66-B8B5F28B9205}");

            AZStd::vector<int> m_addedVector;
            AZStd::unordered_set<int> m_removedSet;
            AZStd::vector<int> m_changedVector;
            AZStd::string m_addedString;
        };

        struct EntityContainerTest
        {
            AZ_CLASS_ALLOCATOR(EntityContainerTest, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(EntityContainerTest, "{A1145D9A-402F-4A40-9B59-52DEAE1070DA}");

            AZStd::unordered_set<AZ::Entity*> m_entitySet;
        };

        struct UnorderedMapContainerTest
        {
            AZ_CLASS_ALLOCATOR(UnorderedMapContainerTest, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(UnorderedMapContainerTest, "{744ADFE1-4BFF-4F3F-8ED0-EA1BDC4A0D2F}");

            AZStd::unordered_map<AZStd::string, int> m_stringIntMap;
        };

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            SerializeDataElementNodeTreeTest::m_wrappedBuffer = AZStd::make_unique<AZStd::vector<AZ::u8>>();
        }

        void TearDown() override
        {
            SerializeDataElementNodeTreeTest::m_wrappedBuffer.reset();
            AllocatorsFixture::TearDown();
        }

        static bool GetDataHierachyVersionConverter(AZ::SerializeContext& sc, AZ::SerializeContext::DataElementNode& rootElement)
        {
            if (rootElement.GetVersion() == 0)
            {
                int entityIndex = rootElement.FindElement(AZ_CRC("m_entity"));
                EXPECT_NE(-1, entityIndex);

                AZ::SerializeContext::DataElementNode& entityElement = rootElement.GetSubElement(entityIndex);
                AZ::Entity newEntity;
                EXPECT_TRUE(entityElement.GetData(newEntity));
                EXPECT_EQ(AZ::EntityId(21434), newEntity.GetId());

                AZStd::vector<AZ::u8> newEntityBuffer;
                {
                    // Binary
                    AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8> > binaryStream(&newEntityBuffer);
                    AZ::ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
                    binaryObjStream->WriteClass(&newEntity);
                    EXPECT_TRUE(binaryObjStream->Finalize());
                }

                // Validate the newEntityBuffer against the wrapped entity.
                EXPECT_EQ(*SerializeDataElementNodeTreeTest::m_wrappedBuffer, newEntityBuffer);
            }

            return true;
        }

        static bool ContainerTestVersionConverter(AZ::SerializeContext& sc, AZ::SerializeContext::DataElementNode& rootElement)
        {
            if (rootElement.GetVersion() == 0)
            {
                int removedSetIndex = rootElement.FindElement(AZ_CRC("m_removedSet"));
                EXPECT_NE(-1, removedSetIndex);

                int changedVectorIndex = rootElement.FindElement(AZ_CRC("m_changedVector"));
                EXPECT_NE(-1, changedVectorIndex);

                auto changedVectorInts = AZ::Utils::FindDescendantElements(sc, rootElement.GetSubElement(changedVectorIndex), { AZ_CRC("element") });
                EXPECT_EQ(2, changedVectorInts.size());
                EXPECT_TRUE(changedVectorInts[0]->SetData(sc, 75));
                EXPECT_TRUE(changedVectorInts[1]->SetData(sc, 50));

                int addedVectorIndex = rootElement.FindElement(AZ_CRC("m_addedVector"));
                EXPECT_EQ(-1, addedVectorIndex);

                ContainerTest containerTest;
                EXPECT_TRUE(rootElement.GetData(containerTest));

                EXPECT_TRUE(containerTest.m_removedSet.empty());
                EXPECT_TRUE(containerTest.m_addedVector.empty());
                EXPECT_EQ(2, containerTest.m_changedVector.size());
                EXPECT_EQ(75, containerTest.m_changedVector[0]);
                EXPECT_EQ(50, containerTest.m_changedVector[1]);

                rootElement.RemoveElement(removedSetIndex);

                // Add an m_addedVector array and remove the zeroth element from the m_changedVector array
                AZStd::vector<int> newInts;
                newInts.push_back(200);
                newInts.push_back(-265);
                newInts.push_back(9451);
                AZStd::string newString("Test");

                AZ::GenericClassInfo* containerGenericInfo = sc.FindGenericClassInfo(azrtti_typeid<AZStd::string>());
                EXPECT_NE(nullptr, containerGenericInfo);
                int addedStringIndex = rootElement.AddElement(sc, "m_addedString", containerGenericInfo); // Add String Element
                EXPECT_NE(-1, addedStringIndex);

                rootElement.GetSubElement(addedStringIndex).SetData(sc, newString); // Set string element data
                rootElement.AddElementWithData(sc, "m_addedVector", newInts); // Add the addedVector vector<int> with initialized data
                AZ::SerializeContext::DataElementNode* changedVectorElementNode = rootElement.FindSubElement(AZ_CRC("m_changedVector"));
                EXPECT_NE(nullptr, changedVectorElementNode);
                changedVectorElementNode->RemoveElement(0);


                ContainerTest containerTest2;
                EXPECT_TRUE(rootElement.GetData(containerTest2));
                EXPECT_TRUE(containerTest2.m_removedSet.empty());
                EXPECT_EQ(3, containerTest2.m_addedVector.size());
                EXPECT_EQ(1, containerTest2.m_changedVector.size());

                EXPECT_EQ(200, containerTest2.m_addedVector[0]);
                EXPECT_EQ(-265, containerTest2.m_addedVector[1]);
                EXPECT_EQ(9451, containerTest2.m_addedVector[2]);

                EXPECT_EQ(50, containerTest2.m_changedVector[0]);
                EXPECT_EQ("Test", containerTest2.m_addedString);
            }
            return true;
        }

        static bool ContainerOfEntitiesVersionConverter(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode& rootElement)
        {
            if (rootElement.GetVersion() == 0)
            {
                int entityContainerIndex = rootElement.FindElement(AZ_CRC("m_entitySet"));
                EXPECT_NE(-1, entityContainerIndex);

                AZ::SerializeContext::DataElementNode& entityContainerElement = rootElement.GetSubElement(entityContainerIndex);
                AZStd::unordered_set<AZ::Entity*> newContainerEntities;
                EXPECT_TRUE(entityContainerElement.GetData(newContainerEntities));
                for (AZ::Entity* entity : newContainerEntities)
                {
                    delete entity;
                }
            }

            return true;
        }

        static bool StringIntMapVersionConverter(AZ::SerializeContext& sc, AZ::SerializeContext::DataElementNode& rootElement)
        {
            if (rootElement.GetVersion() == 0)
            {
                int stringIntMapIndex = rootElement.FindElement(AZ_CRC("m_stringIntMap"));
                EXPECT_NE(-1, stringIntMapIndex);

                UnorderedMapContainerTest containerTest;
                EXPECT_TRUE(rootElement.GetDataHierarchy(sc, containerTest));

                EXPECT_EQ(4, containerTest.m_stringIntMap.size());
                auto foundIt = containerTest.m_stringIntMap.find("Source");
                EXPECT_NE(foundIt, containerTest.m_stringIntMap.end());
                EXPECT_EQ(0, foundIt->second);
                foundIt = containerTest.m_stringIntMap.find("Target");
                EXPECT_NE(containerTest.m_stringIntMap.end(), foundIt);
                EXPECT_EQ(2, foundIt->second);
                foundIt = containerTest.m_stringIntMap.find("In");
                EXPECT_NE(containerTest.m_stringIntMap.end(), foundIt);
                EXPECT_EQ(1, foundIt->second);
                foundIt = containerTest.m_stringIntMap.find("Out");
                EXPECT_NE(containerTest.m_stringIntMap.end(), foundIt);
                EXPECT_EQ(4, foundIt->second);

            }
            return true;
        }

    protected:
        static AZStd::unique_ptr<AZStd::vector<AZ::u8>> m_wrappedBuffer;
    };

    AZStd::unique_ptr<AZStd::vector<AZ::u8>> SerializeDataElementNodeTreeTest::m_wrappedBuffer;

    TEST_F(SerializeDataElementNodeTreeTest, GetDataHierarchyTest)
    {
        EntityWrapperTest entityWrapperTest;
        entityWrapperTest.m_entity = aznew Entity("DataElement");
        entityWrapperTest.m_entity->SetId(AZ::EntityId(21434));

        // Write original data
        AZStd::vector<AZ::u8> binaryBuffer;
        {
            AZ::SerializeContext sc;
            AZ::Entity::Reflect(&sc);
            sc.Class<EntityWrapperTest>()
                ->Version(0)
                ->Field("m_entity", &EntityWrapperTest::m_entity);

            // Binary
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> binaryStream(&binaryBuffer);
            AZ::ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
            binaryObjStream->WriteClass(&entityWrapperTest);
            EXPECT_TRUE(binaryObjStream->Finalize());

            // Write static buffer for wrapped entity data
            binaryStream = AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>>(SerializeDataElementNodeTreeTest::m_wrappedBuffer.get());
            binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
            binaryObjStream->WriteClass(entityWrapperTest.m_entity); // Serialize out the wrapped entity.
            EXPECT_TRUE(binaryObjStream->Finalize());
        }

        // GetDataHierarhyVersionConverter version converter
        {
            AZ::SerializeContext sc;
            AZ::Entity::Reflect(&sc);
            sc.Class<EntityWrapperTest>()
                ->Version(1, &GetDataHierachyVersionConverter)
                ->Field("m_entity", &EntityWrapperTest::m_entity);

            // Binary
            IO::ByteContainerStream<const AZStd::vector<AZ::u8> > binaryStream(&binaryBuffer);
            binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            
            AZ::ObjectStream::ClassReadyCB readyCB([&](void* classPtr, const AZ::Uuid& classId, AZ::SerializeContext* sc)
            {
                AZ_UNUSED(classId);
                AZ_UNUSED(sc);

                EntityWrapperTest* entityWrapper = reinterpret_cast<EntityWrapperTest*>(classPtr);
                delete entityWrapper->m_entity;
                delete entityWrapper;
            });
            ObjectStream::LoadBlocking(&binaryStream, sc, readyCB);
        }

        delete entityWrapperTest.m_entity;
    }

    TEST_F(SerializeDataElementNodeTreeTest, ContainerElementTest)
    {
        ContainerTest containerTest;
        containerTest.m_addedVector.push_back(10);
        containerTest.m_addedVector.push_back(15);
        containerTest.m_removedSet.emplace(25);
        containerTest.m_removedSet.emplace(30);
        containerTest.m_changedVector.push_back(40);
        containerTest.m_changedVector.push_back(45);

        // Write original data
        AZStd::vector<AZ::u8> binaryBuffer;
        {
            AZ::SerializeContext sc;
            sc.Class<ContainerTest>()
                ->Version(0)
                ->Field("m_removedSet", &ContainerTest::m_removedSet)
                ->Field("m_changedVector", &ContainerTest::m_changedVector);

            // Binary
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> binaryStream(&binaryBuffer);
            AZ::ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
            binaryObjStream->WriteClass(&containerTest);
            EXPECT_TRUE(binaryObjStream->Finalize());
        }

        // Test container version converter
        {
            ContainerTest loadedContainer;
            AZ::SerializeContext sc;
            AZ::GenericClassInfo* genericClassInfo = AZ::SerializeGenericTypeInfo<AZStd::unordered_set<int>>::GetGenericInfo();
            genericClassInfo->Reflect(&sc);
            sc.Class<ContainerTest>()
                ->Version(1, &ContainerTestVersionConverter)
                ->Field("m_addedVector", &ContainerTest::m_addedVector)
                ->Field("m_changedVector", &ContainerTest::m_changedVector)
                ->Field("m_addedString", &ContainerTest::m_addedString);

            // Binary
            IO::ByteContainerStream<const AZStd::vector<AZ::u8> > binaryStream(&binaryBuffer);
            binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            ObjectStream::LoadBlocking(&binaryStream, sc, [&loadedContainer](void* objectPtr, const AZ::Uuid typeId, AZ::SerializeContext* serializeContext)
            {
                auto containerTestPtr = static_cast<ContainerTest*>(serializeContext->DownCast(objectPtr, typeId, azrtti_typeid<ContainerTest>()));
                if (containerTestPtr)
                {
                    loadedContainer = *containerTestPtr;
                }
                auto classData = serializeContext->FindClassData(typeId);
                if (classData && classData->m_factory)
                {
                    classData->m_factory->Destroy(objectPtr);
                }
            });

            EXPECT_TRUE(loadedContainer.m_removedSet.empty());
            EXPECT_EQ(1, loadedContainer.m_changedVector.size());
            EXPECT_EQ(3, loadedContainer.m_addedVector.size());

            EXPECT_EQ(50, loadedContainer.m_changedVector[0]);
            EXPECT_EQ(200, loadedContainer.m_addedVector[0]);
            EXPECT_EQ(-265, loadedContainer.m_addedVector[1]);
            EXPECT_EQ(9451, loadedContainer.m_addedVector[2]);
            EXPECT_EQ("Test", loadedContainer.m_addedString);
        }
    }

    TEST_F(SerializeDataElementNodeTreeTest, EntityContainerElementTest)
    {
        EntityContainerTest containerTest;
        containerTest.m_entitySet.insert(aznew AZ::Entity("Test"));

        // Write original data
        AZStd::vector<AZ::u8> binaryBuffer;
        {
            AZ::SerializeContext sc;
            AZ::Entity::Reflect(&sc);
            sc.Class<EntityContainerTest>()
                ->Version(0)
                ->Field("m_entitySet", &EntityContainerTest::m_entitySet);

            // Binary
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> binaryStream(&binaryBuffer);
            AZ::ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
            binaryObjStream->WriteClass(&containerTest);
            EXPECT_TRUE(binaryObjStream->Finalize());
        }

        // Test container version converter
        {
            EntityContainerTest loadedContainer;
            AZ::SerializeContext sc;
            AZ::Entity::Reflect(&sc);
            sc.Class<EntityContainerTest>()
                ->Version(1, &ContainerOfEntitiesVersionConverter)
                ->Field("m_entitySet", &EntityContainerTest::m_entitySet);

            // Binary
            IO::ByteContainerStream<const AZStd::vector<AZ::u8> > binaryStream(&binaryBuffer);
            binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            ObjectStream::LoadBlocking(&binaryStream, sc, [&loadedContainer](void* objectPtr, const AZ::Uuid typeId, AZ::SerializeContext* serializeContext)
            {
                auto containerTestPtr = static_cast<EntityContainerTest*>(serializeContext->DownCast(objectPtr, typeId, azrtti_typeid<EntityContainerTest>()));
                if (containerTestPtr)
                {
                    loadedContainer = *containerTestPtr;
                }
                auto classData = serializeContext->FindClassData(typeId);
                if (classData && classData->m_factory)
                {
                    classData->m_factory->Destroy(objectPtr);
                }
            });

            for (auto&& entityContainer : { containerTest.m_entitySet, loadedContainer.m_entitySet })
            {
                for (AZ::Entity* entity : entityContainer)
                {
                    delete entity;
                }
            }
        }
    }

    TEST_F(SerializeDataElementNodeTreeTest, UnorderedMapContainerElementTest)
    {
        UnorderedMapContainerTest containerTest;
        containerTest.m_stringIntMap.emplace("Source", 0);
        containerTest.m_stringIntMap.emplace("Target", 2);
        containerTest.m_stringIntMap.emplace("In", 1);
        containerTest.m_stringIntMap.emplace("Out", 4);

        // Write original data
        AZStd::vector<AZ::u8> binaryBuffer;
        {
            AZ::SerializeContext sc;
            sc.Class<UnorderedMapContainerTest>()
                ->Version(0)
                ->Field("m_stringIntMap", &UnorderedMapContainerTest::m_stringIntMap);

            // Binary
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> binaryStream(&binaryBuffer);
            AZ::ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
            binaryObjStream->WriteClass(&containerTest);
            EXPECT_TRUE(binaryObjStream->Finalize());
        }

        // Test container version converter
        {
            UnorderedMapContainerTest loadedContainer;
            AZ::SerializeContext sc;
            sc.Class<UnorderedMapContainerTest>()
                ->Version(1, &StringIntMapVersionConverter)
                ->Field("m_stringIntMap", &UnorderedMapContainerTest::m_stringIntMap);

            // Binary
            IO::ByteContainerStream<const AZStd::vector<AZ::u8> > binaryStream(&binaryBuffer);
            binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            EXPECT_TRUE(Utils::LoadObjectFromStreamInPlace(binaryStream, loadedContainer, &sc));
        }
    }

    class SerializeDataElementNodeGetDataTest
        : public AllocatorsFixture
    {
    public:
        struct TemporarilyReflected
        {
            AZ_CLASS_ALLOCATOR(TemporarilyReflected, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(TemporarilyReflected, "{F0909A1D-09BF-44D5-A1D8-E27C8E45579D}");

            AZ::u64 m_num{};
        };

        struct ReflectionWrapper
        {
            AZ_CLASS_ALLOCATOR(ReflectionWrapper, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(ReflectionWrapper, "{EACE8B18-CC31-4E7F-A34C-2A6AA8EB998D}");

            TemporarilyReflected m_tempReflected;
        };

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
        }

        void TearDown() override
        {
            AllocatorsFixture::TearDown();
        }

        static bool GetDataOnNonReflectedClassVersionConverter(AZ::SerializeContext& sc, AZ::SerializeContext::DataElementNode& rootElement)
        {
            (void)sc;
            if (rootElement.GetVersion() == 0)
            {
                // The GetData should not crash
                ReflectionWrapper reflectionWrapper;
                EXPECT_FALSE(rootElement.GetData(reflectionWrapper));

                // Drop the m_tempReflectedElement from the ReflectionWrapper
                EXPECT_TRUE(rootElement.RemoveElementByName(AZ_CRC("m_tempReflected")));

                EXPECT_TRUE(rootElement.GetData(reflectionWrapper));
            }

            return true;
        }
    };

    TEST_F(SerializeDataElementNodeGetDataTest, GetDataOnNonReflectedClassTest)
    {
        ReflectionWrapper testReflectionWrapper;
        AZ::SerializeContext sc;
        sc.Class<TemporarilyReflected>()
            ->Version(0)
            ->Field("m_num", &TemporarilyReflected::m_num)
            ;

        sc.Class<ReflectionWrapper>()
            ->Version(0)
            ->Field("m_tempReflected", &ReflectionWrapper::m_tempReflected)
            ;

        AZStd::vector<AZ::u8> binaryBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> binaryStream(&binaryBuffer);
        AZ::ObjectStream* binaryObjStream = ObjectStream::Create(&binaryStream, sc, ObjectStream::ST_BINARY);
        binaryObjStream->WriteClass(&testReflectionWrapper);
        EXPECT_TRUE(binaryObjStream->Finalize());

        sc.EnableRemoveReflection();
        // Remove the TemporarilyReflected struct so that it is not found when loading
        sc.Class<TemporarilyReflected>()
            ->Version(0)
            ->Field("m_num", &TemporarilyReflected::m_num)
            ;

        // Unreflect ReflectionWrapper version 0 and Reflect it again as version 1
        sc.Class<ReflectionWrapper>()
            ->Version(0)
            ->Field("m_tempReflected", &ReflectionWrapper::m_tempReflected)
            ;
        sc.DisableRemoveReflection();

        sc.Class<ReflectionWrapper>()
            ->Version(1, &GetDataOnNonReflectedClassVersionConverter)
            ->Field("m_tempReflected", &ReflectionWrapper::m_tempReflected)
            ;

        ReflectionWrapper loadReflectionWrapper;
        binaryStream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(Utils::LoadObjectFromStreamInPlace(binaryStream, loadReflectionWrapper, &sc));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    class SerializableAnyFieldTest
        : public AllocatorsFixture
    {
    public:
        struct AnyMemberClass
        {
            AZ_TYPE_INFO(AnyMemberClass, "{67F73D37-5F9E-42FE-AFC9-9867924D87DD}");
            AZ_CLASS_ALLOCATOR(AnyMemberClass, AZ::SystemAllocator, 0);
            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<AnyMemberClass>()
                        ->Field("Any", &AnyMemberClass::m_any)
                        ;
                }
            }
            AZStd::any m_any;
        };

        // We must expose the class for serialization first.
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_serializeContext = AZStd::make_unique<SerializeContext>();
            AnyMemberClass::Reflect(m_serializeContext.get());
            MyClassBase1::Reflect(*m_serializeContext);
            MyClassBase2::Reflect(*m_serializeContext);
            MyClassBase3::Reflect(*m_serializeContext);
            SerializeTestClasses::MyClassMix::Reflect(*m_serializeContext);
            ReflectedString::Reflect(m_serializeContext.get());
            ReflectedSmartPtr::Reflect(m_serializeContext.get());
            NonCopyableClass::Reflect(m_serializeContext.get());
            m_serializeContext->RegisterGenericType<AZStd::shared_ptr<NonCopyableClass>>();
        }

        void TearDown() override
        {

            m_serializeContext->EnableRemoveReflection();
            AnyMemberClass::Reflect(m_serializeContext.get());
            MyClassBase1::Reflect(*m_serializeContext);
            MyClassBase2::Reflect(*m_serializeContext);
            MyClassBase3::Reflect(*m_serializeContext);
            SerializeTestClasses::MyClassMix::Reflect(*m_serializeContext);
            ReflectedString::Reflect(m_serializeContext.get());
            ReflectedSmartPtr::Reflect(m_serializeContext.get());
            NonCopyableClass::Reflect(m_serializeContext.get());
            m_serializeContext->RegisterGenericType<AZStd::shared_ptr<NonCopyableClass>>();
            m_serializeContext->DisableRemoveReflection();

            m_serializeContext.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }
        struct ReflectedString
        {
            AZ_TYPE_INFO(ReflectedString, "{5DE01DEA-119F-43E9-B87C-BF980EBAD896}");
            AZ_CLASS_ALLOCATOR(ReflectedString, AZ::SystemAllocator, 0);
            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    //String class must reflected in at least one field
                    serializeContext->Class<ReflectedString>()
                        ->Field("String", &ReflectedString::m_name)
                        ;
                }
            }
            AZStd::string m_name;
        };

        struct ReflectedSmartPtr
        {
            AZ_TYPE_INFO(ReflectedSmartPtr, "{3EAA2B56-A6A8-46E0-9869-DA4A15AE6704}");
            AZ_CLASS_ALLOCATOR(ReflectedSmartPtr, AZ::SystemAllocator, 0);

            ReflectedSmartPtr() = default;
            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    //String class must reflected in at least one field
                    serializeContext->Class<ReflectedSmartPtr>()
                        ->Field("Field1", &ReflectedSmartPtr::m_uniqueString)
                        ->Field("Field2", &ReflectedSmartPtr::m_sharedString)
                        ;
                }
            }
            AZStd::unique_ptr<ReflectedString> m_uniqueString;
            AZStd::shared_ptr<ReflectedString> m_sharedString;

        private:
            ReflectedSmartPtr(const ReflectedSmartPtr&) = delete;
        };

        struct NonCopyableClass
        {
            AZ_TYPE_INFO(NonCopyableClass, "{5DE8EA5C-9F4A-43F6-9B8B-10EF06319972}");
            AZ_CLASS_ALLOCATOR(NonCopyableClass, AZ::SystemAllocator, 0);
            NonCopyableClass() = default;
            NonCopyableClass(const NonCopyableClass&) = delete;
            NonCopyableClass& operator=(const NonCopyableClass&) = delete;
            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<NonCopyableClass>();
                }
            }
        };

    protected:
        struct NonReflectedClass
        {
            AZ_TYPE_INFO(NonReflectedClass, "{13B8CFB0-601A-4C03-BC19-4EDC71156254}");
            AZ_CLASS_ALLOCATOR(NonReflectedClass, AZ::SystemAllocator, 0);
            AZ::u64 m_num;
            AZStd::string m_name;
        };

        AZStd::unique_ptr<SerializeContext> m_serializeContext;
    };

    TEST_F(SerializableAnyFieldTest, EmptyAnyTest)
    {
        AZStd::any emptyAny;

        // BINARY
        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
        byteObjStream->WriteClass(&emptyAny);
        byteObjStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AZStd::any readAnyData;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, readAnyData, m_serializeContext.get());
        EXPECT_TRUE(readAnyData.empty());

        // JSON
        byteBuffer.clear();
        IO::ByteContainerStream<AZStd::vector<char> > jsonStream(&byteBuffer);
        ObjectStream* jsonObjStream = ObjectStream::Create(&jsonStream, *m_serializeContext, ObjectStream::ST_JSON);
        jsonObjStream->WriteClass(&emptyAny);
        jsonObjStream->Finalize();

        jsonStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZStd::any readAnyDataJson;
        AZ::Utils::LoadObjectFromStreamInPlace(jsonStream, readAnyDataJson, m_serializeContext.get());
        EXPECT_TRUE(readAnyDataJson.empty());

        // JSON
        byteBuffer.clear();
        IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&byteBuffer);
        ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, *m_serializeContext, ObjectStream::ST_XML);
        xmlObjStream->WriteClass(&emptyAny);
        xmlObjStream->Finalize();

        xmlStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZStd::any readAnyDataXml;
        AZ::Utils::LoadObjectFromStreamInPlace(xmlStream, readAnyDataXml, m_serializeContext.get());
        EXPECT_TRUE(readAnyDataXml.empty());
    }

    TEST_F(SerializableAnyFieldTest, MultipleContextsAnyTest)
    {
        SerializeTestClasses::MyClassMix obj;
        obj.Set(5); // Initialize with some value
        AZStd::any testData(obj);

        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_XML);
        byteObjStream->WriteClass(&testData);
        byteObjStream->Finalize();
        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        // create and destroy temporary context to test static context members
        SerializeContext* tmpContext = aznew SerializeContext();
        delete tmpContext;

        AZStd::any readAnyData;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, readAnyData, m_serializeContext.get());
        EXPECT_EQ(SerializeTypeInfo<SerializeTestClasses::MyClassMix>::GetUuid(), readAnyData.type());
        EXPECT_NE(nullptr, AZStd::any_cast<void>(&readAnyData));
        const SerializeTestClasses::MyClassMix& anyMixRef = *AZStd::any_cast<SerializeTestClasses::MyClassMix>(&testData);
        const SerializeTestClasses::MyClassMix& readAnyMixRef = *AZStd::any_cast<SerializeTestClasses::MyClassMix>(&readAnyData);
        EXPECT_EQ(anyMixRef.m_dataMix, readAnyMixRef.m_dataMix);
    }

    TEST_F(SerializableAnyFieldTest, ReflectedFieldTest)
    {
        SerializeTestClasses::MyClassMix obj;
        obj.Set(5); // Initialize with some value

        AZStd::any testData(obj);

        // BINARY
        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_XML);
        byteObjStream->WriteClass(&testData);
        byteObjStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AZStd::any readAnyData;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, readAnyData, m_serializeContext.get());
        EXPECT_EQ(SerializeTypeInfo<SerializeTestClasses::MyClassMix>::GetUuid(), readAnyData.type());
        EXPECT_NE(nullptr, AZStd::any_cast<void>(&readAnyData));
        const SerializeTestClasses::MyClassMix& anyMixRef = *AZStd::any_cast<SerializeTestClasses::MyClassMix>(&testData);
        const SerializeTestClasses::MyClassMix& readAnyMixRef = *AZStd::any_cast<SerializeTestClasses::MyClassMix>(&readAnyData);
        EXPECT_EQ(anyMixRef.m_dataMix, readAnyMixRef.m_dataMix);
    }

    TEST_F(SerializableAnyFieldTest, NonReflectedFieldTest)
    {
        NonReflectedClass notReflected;
        notReflected.m_num = 17;
        notReflected.m_name = "Test";

        AZStd::any testData(notReflected);

        // BINARY
        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
        AZ_TEST_START_TRACE_SUPPRESSION;
        byteObjStream->WriteClass(&testData);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        byteObjStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AZStd::any readAnyData;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, readAnyData, m_serializeContext.get());
        EXPECT_EQ(AZ::Uuid::CreateNull(), readAnyData.type());
        EXPECT_TRUE(readAnyData.empty());
    }

    TEST_F(SerializableAnyFieldTest, EnumerateFieldTest)
    {
        SerializeTestClasses::MyClassMix obj;
        obj.m_dataMix = 5.;
        m_serializeContext->EnumerateObject(&obj,
            [](void* classPtr, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement*)
        {
            if (classData->m_typeId == azrtti_typeid<SerializeTestClasses::MyClassMix>())
            {
                auto mixinClassPtr = reinterpret_cast<SerializeTestClasses::MyClassMix*>(classPtr);
                EXPECT_NE(nullptr, mixinClassPtr);
                EXPECT_DOUBLE_EQ(5.0, mixinClassPtr->m_dataMix);
            }
            return true;
        },
            []() -> bool
        {
            return true;
        },
            SerializeContext::ENUM_ACCESS_FOR_READ);
    }

    TEST_F(SerializableAnyFieldTest, MemberFieldTest)
    {
        SerializeTestClasses::MyClassMix mixedClass;
        mixedClass.m_enum = MyClassBase3::Option3;
        AnyMemberClass anyWrapper;
        anyWrapper.m_any = AZStd::any(mixedClass);

        // BINARY
        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
        byteObjStream->WriteClass(&anyWrapper);
        byteObjStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AnyMemberClass readAnyWrapper;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, readAnyWrapper, m_serializeContext.get());
        EXPECT_EQ(SerializeTypeInfo<SerializeTestClasses::MyClassMix>::GetUuid(), readAnyWrapper.m_any.type());
        EXPECT_NE(nullptr, AZStd::any_cast<void>(&readAnyWrapper.m_any));
        auto* readMixedClass = AZStd::any_cast<SerializeTestClasses::MyClassMix>(&readAnyWrapper.m_any);
        EXPECT_NE(nullptr, readMixedClass);
        EXPECT_EQ(MyClassBase3::Option3, readMixedClass->m_enum);
        SerializeTestClasses::MyClassMix& anyMixRef = *AZStd::any_cast<SerializeTestClasses::MyClassMix>(&anyWrapper.m_any);
        EXPECT_EQ(anyMixRef, *readMixedClass);
    }

    TEST_F(SerializableAnyFieldTest, AZStdStringFieldTest)
    {
        AZStd::string test("Canvas");
        AZStd::any anyString(test);

        // BINARY
        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
        byteObjStream->WriteClass(&anyString);
        byteObjStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AZStd::any readAnyString;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, readAnyString, m_serializeContext.get());
        EXPECT_EQ(azrtti_typeid<AZStd::string>(), readAnyString.type());
        auto* serializedString = AZStd::any_cast<AZStd::string>(&readAnyString);
        EXPECT_NE(nullptr, serializedString);
        EXPECT_EQ(test, *serializedString);
    }

    TEST_F(SerializableAnyFieldTest, AZStdSmartPtrFieldTest)
    {
        /*
        //For some reason that the static_assert inside of AZStd::any about only being able to be constructed with a copyable type
        //or move only type is firing when attempting to move a unique_ptr into it.
        {
            auto testUniquePtr = AZStd::make_unique<ReflectedString>();
            testUniquePtr->m_name = "Script";
            AZStd::any anySmartPtr(AZStd::make_unique<ReflectedString>());

            // BINARY
            AZStd::vector<char> byteBuffer;
            IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
            ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
            byteObjStream->WriteClass(&anySmartPtr);
            byteObjStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            AZStd::any readAnySmartPtr;
            AZ::Utils::LoadObjectFromStreamInPlace(byteStream, readAnySmartPtr, m_serializeContext.get());
            EXPECT_EQ(azrtti_typeid<AZStd::unique_ptr<ReflectedString>>(), readAnySmartPtr.type());
            auto uniquePtrAny = AZStd::any_cast<AZStd::unique_ptr<ReflectedString>>(&readAnySmartPtr);
            EXPECT_NE(nullptr, *uniquePtrAny);

            auto testUniquePtrAny = AZStd::any_cast<AZStd::unique_ptr<ReflectedString>>(&anySmartPtr);
            EXPECT_EQ((*testUniquePtrAny)->m_name, (*uniquePtrAny)->m_name);
        }
        */
        {
            auto testSharedPtr = AZStd::make_shared<ReflectedString>();
            testSharedPtr->m_name = "Canvas";
            AZStd::any anySmartPtr(testSharedPtr);

            // BINARY
            AZStd::vector<char> byteBuffer;
            IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
            ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
            byteObjStream->WriteClass(&anySmartPtr);
            byteObjStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            AZStd::any readAnySmartPtr;
            AZ::Utils::LoadObjectFromStreamInPlace(byteStream, readAnySmartPtr, m_serializeContext.get());
            EXPECT_EQ(azrtti_typeid<AZStd::shared_ptr<ReflectedString>>(), readAnySmartPtr.type());
            auto sharedPtrAny = AZStd::any_cast<AZStd::shared_ptr<ReflectedString>>(&readAnySmartPtr);
            EXPECT_NE(nullptr, *sharedPtrAny);

            EXPECT_EQ(testSharedPtr->m_name, (*sharedPtrAny)->m_name);
        }
    }

    TEST_F(SerializableAnyFieldTest, ReflectedPointerFieldTest)
    {
        SerializeTestClasses::MyClassMix obj;
        obj.Set(26); // Initialize with some value

        AZStd::any testData(&obj);

        // BINARY
        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
        byteObjStream->WriteClass(&testData);
        byteObjStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AZStd::any readAnyData;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, readAnyData, m_serializeContext.get());
        EXPECT_EQ(SerializeTypeInfo<SerializeTestClasses::MyClassMix>::GetUuid(), readAnyData.type());
        EXPECT_NE(nullptr, AZStd::any_cast<void>(&readAnyData));
        const SerializeTestClasses::MyClassMix* anyMixRef = AZStd::any_cast<SerializeTestClasses::MyClassMix*>(testData);
        const SerializeTestClasses::MyClassMix& readAnyMixRef = *AZStd::any_cast<SerializeTestClasses::MyClassMix>(&readAnyData);
        EXPECT_EQ(anyMixRef->m_dataMix, readAnyMixRef.m_dataMix);
    }

    TEST_F(SerializableAnyFieldTest, CreateAnyForSmartPtrWithNonCopyableSmartPtrDoesNotCrash)
    {
        AZStd::any nonCopyableSharedPtr = m_serializeContext->CreateAny(azrtti_typeid<AZStd::shared_ptr<NonCopyableClass>>());
        EXPECT_FALSE(nonCopyableSharedPtr.empty());
    }

    class SerializableOptionalFixture
        : public AllocatorsFixture
    {
    public:
        struct OptionalMemberClass
        {
            AZ_TYPE_INFO(OptionalMemberClass, "{6BC95A2D-FE6B-4FD8-9586-771F47C44C0B}");
            AZ_CLASS_ALLOCATOR(OptionalMemberClass, AZ::SystemAllocator, 0);
            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<OptionalMemberClass>()
                        ->Field("Optional", &OptionalMemberClass::m_optional)
                        ;
                }
            }
            AZStd::optional<int> m_optional;
        };

        // We must expose the class for serialization first.
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_serializeContext = AZStd::make_unique<SerializeContext>();
            OptionalMemberClass::Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {

            m_serializeContext->EnableRemoveReflection();
            OptionalMemberClass::Reflect(m_serializeContext.get());
            m_serializeContext.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }
    protected:
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
    };

    TEST_F(SerializableOptionalFixture, TestHasValueOptionalSerialization)
    {
        AZStd::optional<int> theOpt {42};

        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        AZ::Utils::SaveObjectToStream(byteStream, ObjectStream::ST_XML, &theOpt, m_serializeContext.get());

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AZStd::optional<int> deserializedOptional;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, deserializedOptional, m_serializeContext.get());
        EXPECT_TRUE(deserializedOptional.has_value());
        EXPECT_EQ(deserializedOptional.value(), 42);
    }

    TEST_F(SerializableOptionalFixture, TestNulloptOptionalSerialization)
    {
        AZStd::optional<int> theOpt;

        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        AZ::Utils::SaveObjectToStream(byteStream, ObjectStream::ST_XML, &theOpt, m_serializeContext.get());

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AZStd::optional<int> deserializedOptional;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, deserializedOptional, m_serializeContext.get());
        EXPECT_FALSE(deserializedOptional.has_value());
    }

    TEST_F(Serialization, AttributeTest)
    {
        const AZ::Crc32 attributeCrc = AZ_CRC("TestAttribute");
        const int attributeValue = 5;
        m_serializeContext->Class<SerializeTestClasses::BaseNoRtti>()
            ->Attribute(attributeCrc, attributeValue)
            ;

        auto classData = m_serializeContext->FindClassData(azrtti_typeid<SerializeTestClasses::BaseNoRtti>());
        ASSERT_NE(nullptr, classData);
        auto attribute = AZ::FindAttribute(attributeCrc, classData->m_attributes);
        ASSERT_NE(nullptr, attribute);
        AZ::AttributeReader reader(nullptr, attribute);
        int value = 0;
        EXPECT_TRUE(reader.Read<int>(value));
        EXPECT_EQ(attributeValue, value);
    }

    TEST_F(Serialization, AttributeData_WithCallableType_Succeeds)
    {
        constexpr AZ::Crc32 invokableCrc = AZ_CRC_CE("Invokable");
        constexpr AZ::Crc32 nonInvokableCrc = AZ_CRC_CE("NonInvokable");
        auto ReadFloat = [](SerializeTestClasses::BaseNoRtti* instance) -> float
        {
            auto noRttiInstance = instance;
            if (!noRttiInstance)
            {
                ADD_FAILURE() << "BaseNoRtti instance object should not be nullptr";
                return 0.0f;
            }
            EXPECT_FALSE(noRttiInstance->m_data);
            return 2.0f;
        };

        m_serializeContext->Class<SerializeTestClasses::BaseNoRtti>()
            ->Attribute(invokableCrc, ReadFloat)
            ->Attribute(nonInvokableCrc, 4.0f)
            ;

        SerializeTestClasses::BaseNoRtti baseNoRttiInstance;
        baseNoRttiInstance.Set();
        auto classData = m_serializeContext->FindClassData(azrtti_typeid<SerializeTestClasses::BaseNoRtti>());
        ASSERT_NE(nullptr, classData);
        AZ::Attribute* attribute = AZ::FindAttribute(invokableCrc, classData->m_attributes);
        ASSERT_NE(nullptr, attribute);
        AZ::AttributeInvoker invoker(&baseNoRttiInstance, attribute);
        float value = 0;
        EXPECT_TRUE(invoker.Read<float>(value));
        EXPECT_FLOAT_EQ(2.0f, value);

        AZ::Attribute* nonInvokeAttribute = AZ::FindAttribute(nonInvokableCrc, classData->m_attributes);
        ASSERT_NE(nullptr, nonInvokeAttribute);
        invoker = { &baseNoRttiInstance, nonInvokeAttribute };
        value = {};
        EXPECT_TRUE(invoker.Read<float>(value));
        EXPECT_FLOAT_EQ(4.0f, value);
    }


    class ObjectStreamSerialization
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_serializeContext = AZStd::make_unique<SerializeContext>();
            TemplateInstantiationReflectedWrapper::Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {
            m_serializeContext->EnableRemoveReflection();
            TemplateInstantiationReflectedWrapper::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();

            m_serializeContext.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }
        struct TemplateInstantiationReflectedWrapper
        {
            AZ_TYPE_INFO(TemplateInstantiationReflectedWrapper, "{5A2F60AA-F63E-4106-BD5E-0F77E01DDBAC}");
            AZ_CLASS_ALLOCATOR(TemplateInstantiationReflectedWrapper, AZ::SystemAllocator, 0);
            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    //Reflected Template classes must be reflected in one field
                    serializeContext->Class<TemplateInstantiationReflectedWrapper>()
                        ->Field("m_name", &TemplateInstantiationReflectedWrapper::m_name)
                        ;
                }
            }
            AZStd::string m_name;
        };

        struct DeprecatedClass
        {
            AZ_TYPE_INFO(DeprecatedClass, "{5AB3F3C9-21D9-4AA8-84B2-9ACCC81C77B6}");

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    //Reflected Template classes must be reflected in one field
                    serializeContext->Class<DeprecatedClass>()
                        ->Field("m_value", &DeprecatedClass::m_value)
                        ->Field("m_testFlag", &DeprecatedClass::m_testFlag)
                        ;
                }
            }
            int64_t m_value;
            bool m_testFlag;
        };

        struct ConvertedClass
        {
            AZ_TYPE_INFO(ConvertedClass, "{97733A6F-98B5-4EB7-B782-9F8F69FBD581}");
            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    //Reflected Template classes must be reflected in one field
                    serializeContext->Class<ConvertedClass>()
                        ->Field("m_value", &ConvertedClass::m_value)
                        ->Field("m_testString", &ConvertedClass::m_testString)
                        ;
                }
            }

            int64_t m_value;
            AZStd::string m_testString;
        };

        static bool DeprecatedClassConverter(SerializeContext& serializeContext, SerializeContext::DataElementNode& deprecatedNode)
        {
            return deprecatedNode.Convert<ConvertedClass>(serializeContext) && deprecatedNode.SetData(serializeContext, ConvertedClass{});
        }

        static constexpr const char* c_reflectedFieldNameTypeId{ "{78469836-4D08-42CE-AC22-B2056442D5AF}" };
        static constexpr const char* c_rootReflectedClassTypeId{ "{DED0BFF5-84A8-47E5-8AFB-73B6BED56F0C}" };
        static constexpr unsigned int c_reflectedFieldNameVersion{ 0 };
        // Wraps a DeprecatedClass element that gets written to an ObjectStream
        // and but loaded with a version change using the same typeid into a structure
        // that no longer contains the deprecated class field
        struct ReflectedFieldNameOldVersion1
        {
            AZ_TYPE_INFO(ReflectedFieldNameOldVersion1, c_reflectedFieldNameTypeId);

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    //Reflected Template classes must be reflected in one field
                    serializeContext->Class<ReflectedFieldNameOldVersion1>()
                        ->Version(c_reflectedFieldNameVersion)
                        ->Field("m_deprecatedElement", &ReflectedFieldNameOldVersion1::m_deprecatedElement)
                        ;
                }
            }
            DeprecatedClass m_deprecatedElement;
        };

        struct ReflectedFieldNameNewVersion1
        {
            AZ_TYPE_INFO(ReflectedFieldNameNewVersion1, c_reflectedFieldNameTypeId);

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    //Reflected Template classes must be reflected in one field
                    serializeContext->Class<ReflectedFieldNameNewVersion1>()
                        ->Version(c_reflectedFieldNameVersion)
                        ->Field("newElement", &ReflectedFieldNameNewVersion1::m_newElement)
                        ;
                }
            }
            int m_newElement{};
        };

        struct RootFieldNameV1
        {
            AZ_TYPE_INFO(RootFieldNameV1, c_rootReflectedClassTypeId);

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    //Reflected Template classes must be reflected in one field
                    serializeContext->Class<RootFieldNameV1>()
                        ->Version(c_reflectedFieldNameVersion)
                        ->Field("m_reflectedField", &RootFieldNameV1::m_reflectedField)
                        ->Field("m_rootName", &RootFieldNameV1::m_rootName)
                        ;
                }
            }
            ReflectedFieldNameOldVersion1 m_reflectedField;
            AZStd::string m_rootName;
        };

        struct RootFieldNameV2
        {
            AZ_TYPE_INFO(RootFieldNameV2, c_rootReflectedClassTypeId);

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    //Reflected Template classes must be reflected in one field
                    serializeContext->Class<RootFieldNameV2>()
                        ->Version(c_reflectedFieldNameVersion)
                        ->Field("m_reflectedField", &RootFieldNameV2::m_reflectedField)
                        ->Field("m_rootName", &RootFieldNameV2::m_rootName)
                        ;
                }
            }
            ReflectedFieldNameNewVersion1 m_reflectedField;
            AZStd::string m_rootName;
        };

        struct RootElementMemoryTracker
        {
            AZ_TYPE_INFO(RootElementMemoryTracker, "{772D354F-F6EB-467F-8FA7-9086DDD58324}");
            AZ_CLASS_ALLOCATOR(RootElementMemoryTracker, AZ::SystemAllocator, 0);

            RootElementMemoryTracker()
            {
                ++s_allocatedInstance;
            }
            ~RootElementMemoryTracker()
            {
                --s_allocatedInstance;
            }

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    //Reflected Template classes must be reflected in one field
                    serializeContext->Class<RootElementMemoryTracker>();
                }
            }

            static int32_t s_allocatedInstance;
        };

    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };

    int32_t ObjectStreamSerialization::RootElementMemoryTracker::s_allocatedInstance;

    TEST_F(ObjectStreamSerialization, NewerVersionThanSupportedTest)
    {
        AZStd::string loadString;

        // Set the object stream version to numeric_limits<AZ::u32>::max() "4294967295"
        {
            AZStd::string_view versionMaxStringXml = R"(<ObjectStream version="4294967295">
            <Class name="AZStd::string" field="Name" type="{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}" value="Test" specializationTypeId="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
            </ObjectStream>
            )";

            AZ::IO::MemoryStream versionMaxStream(versionMaxStringXml.data(), versionMaxStringXml.size());
            AZ_TEST_START_TRACE_SUPPRESSION;
            bool result = AZ::Utils::LoadObjectFromStreamInPlace(versionMaxStream, loadString, m_serializeContext.get());
            EXPECT_FALSE(result);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            EXPECT_EQ("", loadString);
        }

        {
            AZStd::string_view versionMaxStringJson = R"({
                "name": "ObjectStream",
                "version": 4294967295,
                "Objects": [
                {
                    "field": "m_textData",
                    "typeName": "AZStd::string",
                    "typeId": "{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}",
                    "specializationTypeId": "{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}",
                    "value": "Test"
                }
            ]
            })";

            AZ::IO::MemoryStream versionMaxStream(versionMaxStringJson.data(), versionMaxStringJson.size());
            AZ_TEST_START_TRACE_SUPPRESSION;
            bool result = AZ::Utils::LoadObjectFromStreamInPlace(versionMaxStream, loadString, m_serializeContext.get());
            EXPECT_FALSE(result);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            EXPECT_EQ("", loadString);
        }

        {
            AZStd::string_view versionMaxStringBinary = "00FFFFFFFF18EF8FF807DDEE4EB0B6784CA3A2C490A40000";
            AZStd::vector<AZ::u8> byteArray;
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> binaryStream(&byteArray);
            AZStd::unique_ptr<AZ::SerializeContext::IDataSerializer> binarySerializer = AZStd::make_unique<AZ::Internal::AZByteStream<AZStd::allocator>>();
            binarySerializer->TextToData(versionMaxStringBinary.data(), 0, binaryStream);
            binarySerializer.reset();

            binaryStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            AZ_TEST_START_TRACE_SUPPRESSION;
            bool result = AZ::Utils::LoadObjectFromStreamInPlace(binaryStream, loadString, m_serializeContext.get());
            EXPECT_FALSE(result);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            EXPECT_EQ("", loadString);
        }
    }

    TEST_F(ObjectStreamSerialization, V1ToCurrentVersionTest)
    {

        // Set the object stream version to "1"
        {
            TemplateInstantiationReflectedWrapper loadXmlWrapper;
            AZStd::string_view versionStringXml = R"(<ObjectStream version="1">
            <Class name="TemplateInstantiationReflectedWrapper" type="{5A2F60AA-F63E-4106-BD5E-0F77E01DDBAC}">
                <Class name="AZStd::string" field="m_name" type="{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}" value="Test"/>
            </Class>
            </ObjectStream>
            )";

            AZ::IO::MemoryStream versionStream(versionStringXml.data(), versionStringXml.size());
            AZ::Utils::LoadObjectFromStreamInPlace(versionStream, loadXmlWrapper, m_serializeContext.get());
            EXPECT_EQ("Test", loadXmlWrapper.m_name);
        }


        {
            TemplateInstantiationReflectedWrapper loadJsonWrapper;
            AZStd::string_view versionStringJson = R"({
                "name": "ObjectStream",
                "version": 1,
                "Objects": [
                    {
                        "typeName": "TemplateInstantiationReflectedWrapper",
                        "typeId": "{5A2F60AA-F63E-4106-BD5E-0F77E01DDBAC}",
                        "Objects": [
                            {
                                "field": "m_name",
                                "typeName": "AZStd::string",
                                "typeId": "{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}",
                                "value": "Test"
                            }
                        ]
                    }
                ]
            })";

            AZ::IO::MemoryStream versionStream(versionStringJson.data(), versionStringJson.size());
            AZ::Utils::LoadObjectFromStreamInPlace(versionStream, loadJsonWrapper, m_serializeContext.get());
            EXPECT_EQ("Test", loadJsonWrapper.m_name);
        }


        {
            TemplateInstantiationReflectedWrapper loadBinaryWrapper;
            AZStd::string_view version1StringBinary = "0000000001085A2F60AAF63E4106BD5E0F77E01DDBAC5CC08C4427EF8FF807DDEE4EB0B6784CA3A2C490A454657374000000";
            AZStd::vector<AZ::u8> byteArray;
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> binaryStream(&byteArray);
            AZStd::unique_ptr<AZ::SerializeContext::IDataSerializer> binarySerializer = AZStd::make_unique<AZ::Internal::AZByteStream<AZStd::allocator>>();
            binarySerializer->TextToData(version1StringBinary.data(), 0, binaryStream);
            binarySerializer.reset();

            binaryStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            AZ::Utils::LoadObjectFromStreamInPlace(binaryStream, loadBinaryWrapper, m_serializeContext.get());
            EXPECT_EQ("Test", loadBinaryWrapper.m_name);
        }

    }

    TEST_F(ObjectStreamSerialization, V2ToCurrentVersionTest)
    {
        AZStd::string loadJsonString;

        // Set the object stream version to "2"
        {
            AZStd::string_view version2StringXml = R"(<ObjectStream version="2">
            <Class name="AZStd::string" type="{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}" value="Test" specializationTypeId="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
            </ObjectStream>
            )";

            AZ::IO::MemoryStream version2Stream(version2StringXml.data(), version2StringXml.size());
            AZ::Utils::LoadObjectFromStreamInPlace(version2Stream, loadJsonString, m_serializeContext.get());
        }

        EXPECT_EQ("Test", loadJsonString);

        AZStd::string loadXmlString;
        {
            AZStd::string_view version2StringJson = R"({
                "name": "ObjectStream",
                "version": 2,
                "Objects": [
                {
                    "typeName": "AZStd::string",
                    "typeId": "{EF8FF807-DDEE-4EB0-B678-4CA3A2C490A4}",
                    "specializationTypeId": "{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}",
                    "value": "Test"
                }
            ]
            })";

            AZ::IO::MemoryStream version2Stream(version2StringJson.data(), version2StringJson.size());
            AZ::Utils::LoadObjectFromStreamInPlace(version2Stream, loadXmlString, m_serializeContext.get());
        }
        EXPECT_EQ("Test", loadXmlString);

        AZStd::string testString = "Test";
        AZStd::vector<AZ::u8> stringArray;
        AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> byteStream(&stringArray);
        AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, &testString, m_serializeContext.get());

        AZStd::string loadBinaryString;
        {
            AZStd::string_view version2StringBinary = "00000000021CEF8FF807DDEE4EB0B6784CA3A2C490A403AAAB3F5C475A669EBCD5FA4DB353C9546573740000";
            AZStd::vector<AZ::u8> byteArray;
            AZ::IO::ByteContainerStream<AZStd::vector<AZ::u8>> binaryStream(&byteArray);
            AZStd::unique_ptr<AZ::SerializeContext::IDataSerializer> binarySerializer = AZStd::make_unique<AZ::Internal::AZByteStream<AZStd::allocator>>();
            binarySerializer->TextToData(version2StringBinary.data(), 0, binaryStream);
            binarySerializer.reset();

            binaryStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
            AZ::Utils::LoadObjectFromStreamInPlace(binaryStream, loadBinaryString, m_serializeContext.get());
        }

        EXPECT_EQ("Test", loadBinaryString);
    }

    TEST_F(ObjectStreamSerialization, UnreflectedChildElementAndDeprecatedClass_XmlTest)
    {
        // Reflect the Deprecated class and the wrapper class 
        // with the deprecated class as a field
        DeprecatedClass::Reflect(m_serializeContext.get());
        ReflectedFieldNameOldVersion1::Reflect(m_serializeContext.get());
        RootFieldNameV1::Reflect(m_serializeContext.get());
        ConvertedClass::Reflect(m_serializeContext.get());

        RootFieldNameV1 oldDeprecatedElement;
        // Test Saving and Loading XML 
        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        EXPECT_TRUE(AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_XML, &oldDeprecatedElement, m_serializeContext.get()));

        // Un-reflect both the deprecated class and the wrapper class with the deprecated field
        {
            m_serializeContext->EnableRemoveReflection();
            DeprecatedClass::Reflect(m_serializeContext.get());
            ReflectedFieldNameOldVersion1::Reflect(m_serializeContext.get());
            RootFieldNameV1::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();
        }

        // Reflect the Deprecation Converter for the DeprecatedClass and the wrapper class
        // with the converter class as a field
        m_serializeContext->ClassDeprecate("DeprecatedClass", AzTypeInfo<DeprecatedClass>::Uuid(), &ObjectStreamSerialization::DeprecatedClassConverter);
        ReflectedFieldNameNewVersion1::Reflect(m_serializeContext.get());
        RootFieldNameV2::Reflect(m_serializeContext.get());

        byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

        RootFieldNameV2 newConvertedElement;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, newConvertedElement, m_serializeContext.get());

        // Un-reflect remaining classes
        {
            m_serializeContext->EnableRemoveReflection();
            ConvertedClass::Reflect(m_serializeContext.get());
            m_serializeContext->ClassDeprecate("DeprecatedClass", AzTypeInfo<DeprecatedClass>::Uuid(), &ObjectStreamSerialization::DeprecatedClassConverter);
            ReflectedFieldNameNewVersion1::Reflect(m_serializeContext.get());
            RootFieldNameV2::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();
        }
    }

    TEST_F(ObjectStreamSerialization, UnreflectedChildElementAndDeprecatedClass_BinaryTest)
    {
        // Reflect the Deprecated class and the wrapper class 
        // with the deprecated class as a field
        DeprecatedClass::Reflect(m_serializeContext.get());
        ReflectedFieldNameOldVersion1::Reflect(m_serializeContext.get());
        RootFieldNameV1::Reflect(m_serializeContext.get());
        ConvertedClass::Reflect(m_serializeContext.get());

        RootFieldNameV1 oldDeprecatedElement;
        // Test Saving and Loading XML 
        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        EXPECT_TRUE(AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, &oldDeprecatedElement, m_serializeContext.get()));

        // Un-reflect both the deprecated class and the wrapper class with the deprecated field
        {
            m_serializeContext->EnableRemoveReflection();
            DeprecatedClass::Reflect(m_serializeContext.get());
            ReflectedFieldNameOldVersion1::Reflect(m_serializeContext.get());
            RootFieldNameV1::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();
        }

        // Reflect the Deprecation Converter for the DeprecatedClass and the wrapper class
        // with the converter class as a field
        m_serializeContext->ClassDeprecate("DeprecatedClass", AzTypeInfo<DeprecatedClass>::Uuid(), &ObjectStreamSerialization::DeprecatedClassConverter);
        ReflectedFieldNameNewVersion1::Reflect(m_serializeContext.get());
        RootFieldNameV2::Reflect(m_serializeContext.get());

        byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

        RootFieldNameV2 newConvertedElement;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, newConvertedElement, m_serializeContext.get());

        // Un-reflect remaining classes
        {
            m_serializeContext->EnableRemoveReflection();
            ConvertedClass::Reflect(m_serializeContext.get());
            m_serializeContext->ClassDeprecate("DeprecatedClass", AzTypeInfo<DeprecatedClass>::Uuid(), &ObjectStreamSerialization::DeprecatedClassConverter);
            ReflectedFieldNameNewVersion1::Reflect(m_serializeContext.get());
            RootFieldNameV2::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();
        }
    }

    TEST_F(ObjectStreamSerialization, UnreflectedChildElementAndDeprecatedClass_JSONTest)
    {
        // Reflect the Deprecated class and the wrapper class 
        // with the deprecated class as a field
        DeprecatedClass::Reflect(m_serializeContext.get());
        ReflectedFieldNameOldVersion1::Reflect(m_serializeContext.get());
        RootFieldNameV1::Reflect(m_serializeContext.get());
        ConvertedClass::Reflect(m_serializeContext.get());

        RootFieldNameV1 oldDeprecatedElement;
        // Test Saving and Loading XML 
        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        EXPECT_TRUE(AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_JSON, &oldDeprecatedElement, m_serializeContext.get()));

        // Un-reflect both the deprecated class and the wrapper class with the deprecated field
        {
            m_serializeContext->EnableRemoveReflection();
            DeprecatedClass::Reflect(m_serializeContext.get());
            ReflectedFieldNameOldVersion1::Reflect(m_serializeContext.get());
            RootFieldNameV1::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();
        }

        // Reflect the Deprecation Converter for the DeprecatedClass and the wrapper class
        // with the converter class as a field
        m_serializeContext->ClassDeprecate("DeprecatedClass", AzTypeInfo<DeprecatedClass>::Uuid(), &ObjectStreamSerialization::DeprecatedClassConverter);
        ReflectedFieldNameNewVersion1::Reflect(m_serializeContext.get());
        RootFieldNameV2::Reflect(m_serializeContext.get());

        byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);

        RootFieldNameV2 newConvertedElement;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, newConvertedElement, m_serializeContext.get());

        // Un-reflect remaining classes
        {
            m_serializeContext->EnableRemoveReflection();
            ConvertedClass::Reflect(m_serializeContext.get());
            m_serializeContext->ClassDeprecate("DeprecatedClass", AzTypeInfo<DeprecatedClass>::Uuid(), &ObjectStreamSerialization::DeprecatedClassConverter);
            ReflectedFieldNameNewVersion1::Reflect(m_serializeContext.get());
            RootFieldNameV1::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();
        }
    }

    // Prove that if a member of a vector of baseclass pointers is unreadable, the container
    // removes the element instead of leaving a null.  This is an arbitrary choice (to remove or leave
    // the null) and this test exists just to prove that the chosen way functions as expected.
    TEST_F(ObjectStreamSerialization, UnreadableVectorElements_LeaveNoGaps_Errors)
    {
        using namespace ContainerElementDeprecationTestData;
        // make sure that when a component is deprecated, it is removed during deserialization
        // and does not leave a hole that is a nullptr.
        ClassWithAVectorOfBaseClasses::Reflect(m_serializeContext.get());

        ClassWithAVectorOfBaseClasses vectorContainer;
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());

        AZStd::vector<char> charBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > containerStream(&charBuffer);
        bool success = AZ::Utils::SaveObjectToStream(containerStream, AZ::ObjectStream::ST_XML, &vectorContainer, m_serializeContext.get());
        EXPECT_TRUE(success);

        // (remove it, but without deprecating)
        m_serializeContext->EnableRemoveReflection();
        DerivedClass2::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();

        // load it, we expect errors:
        ClassWithAVectorOfBaseClasses loadedContainer;
        AZ_TEST_START_TRACE_SUPPRESSION;
        success = AZ::Utils::LoadObjectFromBufferInPlace(charBuffer.data(), charBuffer.size(), loadedContainer, m_serializeContext.get());
        AZ_TEST_STOP_TRACE_SUPPRESSION(2); // 2 classes should have failed and generated warnings/errors
        EXPECT_TRUE(success);
        EXPECT_EQ(loadedContainer.m_vectorOfBaseClasses.size(), 2); // we still preserve the ones we CAN read.
        for (auto baseclass : loadedContainer.m_vectorOfBaseClasses)
        {
            // we should only have baseclass1's in there.
            EXPECT_EQ(baseclass->RTTI_GetType(), azrtti_typeid<DerivedClass1>());
        }
    }

    // Prove that if you properly deprecate a member of a vector of baseclass pointers, the container
    // removes the element instead of leaving a null and does not emit an error
    TEST_F(ObjectStreamSerialization, DeprecatedVectorElements_LeaveNoGaps_DoesNotError)
    {
        using namespace ContainerElementDeprecationTestData;
        // make sure that when a component is deprecated, it is removed during deserialization,
        // and does not leave a hole that is a nullptr.
        ClassWithAVectorOfBaseClasses::Reflect(m_serializeContext.get());

        ClassWithAVectorOfBaseClasses vectorContainer;
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());

        AZStd::vector<char> charBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > containerStream(&charBuffer);
        bool success = AZ::Utils::SaveObjectToStream(containerStream, AZ::ObjectStream::ST_XML, &vectorContainer, m_serializeContext.get());
        EXPECT_TRUE(success);

        // remove it and properly deprecate it
        m_serializeContext->EnableRemoveReflection();
        DerivedClass2::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        m_serializeContext->ClassDeprecate("Dummy UUID", azrtti_typeid<DerivedClass2>());

        ClassWithAVectorOfBaseClasses loadedContainer;
        // it should generate no warnings but the deprecated ones should not be there.
        success = AZ::Utils::LoadObjectFromBufferInPlace(charBuffer.data(), charBuffer.size(), loadedContainer, m_serializeContext.get());
        EXPECT_TRUE(success);
        EXPECT_EQ(loadedContainer.m_vectorOfBaseClasses.size(), 2); // we still preserve the ones we CAN read.
        for (auto baseclass : loadedContainer.m_vectorOfBaseClasses)
        {
            // we should only have baseclass1's in there.
            EXPECT_EQ(baseclass->RTTI_GetType(), azrtti_typeid<DerivedClass1>());
        }
    }

    // Prove that if you deprecate but upgrade a member of a vector of baseclass pointers, the container
    // contains the freshly upgraded element instead of leaving a null and does not emit an error
    TEST_F(ObjectStreamSerialization, DeprecatedVectorElements_ConvertedClass_DoesNotError_DoesNotDiscardData)
    {
        using namespace ContainerElementDeprecationTestData;
        // make sure that when a component is deprecated, it is removed during deserialization
        // and does not leave a hole that is a nullptr.
        ClassWithAVectorOfBaseClasses::Reflect(m_serializeContext.get());

        ClassWithAVectorOfBaseClasses vectorContainer;
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass1());
        vectorContainer.m_vectorOfBaseClasses.push_back(new DerivedClass2());

        AZStd::vector<char> charBuffer;
        AZ::IO::ByteContainerStream<AZStd::vector<char> > containerStream(&charBuffer);
        bool success = AZ::Utils::SaveObjectToStream(containerStream, AZ::ObjectStream::ST_XML, &vectorContainer, m_serializeContext.get());
        EXPECT_TRUE(success);

        // remove it and properly deprecate it with a converter that will upgrade it.
        m_serializeContext->EnableRemoveReflection();
        DerivedClass2::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        m_serializeContext->ClassDeprecate("Dummy UUID", azrtti_typeid<DerivedClass2>(), ConvertDerivedClass2ToDerivedClass3);

        ClassWithAVectorOfBaseClasses loadedContainer;
        // it should generate no warnings but the deprecated ones should not be there.
        success = AZ::Utils::LoadObjectFromBufferInPlace(charBuffer.data(), charBuffer.size(), loadedContainer, m_serializeContext.get());
        EXPECT_TRUE(success);
        ASSERT_EQ(loadedContainer.m_vectorOfBaseClasses.size(), 4); // we still preserve the ones we CAN read.

        // this also proves it does not shuffle elements around.
        EXPECT_EQ(loadedContainer.m_vectorOfBaseClasses[0]->RTTI_GetType(), azrtti_typeid<DerivedClass1>());
        EXPECT_EQ(loadedContainer.m_vectorOfBaseClasses[1]->RTTI_GetType(), azrtti_typeid<DerivedClass3>());
        EXPECT_EQ(loadedContainer.m_vectorOfBaseClasses[2]->RTTI_GetType(), azrtti_typeid<DerivedClass1>());
        EXPECT_EQ(loadedContainer.m_vectorOfBaseClasses[3]->RTTI_GetType(), azrtti_typeid<DerivedClass3>());
    }

    TEST_F(ObjectStreamSerialization, LoadObjectFromStreamInPlaceFailureDoesNotLeak)
    {
        RootElementMemoryTracker::Reflect(m_serializeContext.get());

        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        {
            RootElementMemoryTracker saveTracker;
            EXPECT_TRUE(AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, &saveTracker, m_serializeContext.get()));
            byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        }

        // Attempt to load a RootElementMemoryTracker into an int64_t
        int64_t loadTracker;
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadTracker, m_serializeContext.get()));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_EQ(0U, RootElementMemoryTracker::s_allocatedInstance);
    }

    TEST_F(ObjectStreamSerialization, LoadNonDeprecatedElement_FollowedByZeroSizeDeprecatedElement_DoesNotAssert)
    {
        struct EmptyDeprecatedClass
        {
            AZ_TYPE_INFO(EmptyDeprecatedClass, "{73890A64-9ADB-4639-B0E0-93294CE81B19}");
        };

        struct ConvertedNewClass
        {
            AZ_TYPE_INFO(ConvertedNewClass, "{BE892776-3830-43E5-873C-38A1CA6EF4BB}");
            int32_t m_value{ 5 };
        };

        struct AggregateTestClassV1
        {
            AZ_TYPE_INFO(AggregateTestClassV1, "{088E3B16-4D93-4116-A747-706BE132AF5F}");
            EmptyDeprecatedClass m_testField;
            AZ::Vector3 m_position = AZ::Vector3::CreateZero();
            EmptyDeprecatedClass m_value;
        };

        struct AggregateTestClassV2
        {
            // AggregateTestClassV2 Uuid should match version 1, It isn't the class that
            // is being converted, but it's m_value that is.
            AZ_TYPE_INFO(AggregateTestClassV1, "{088E3B16-4D93-4116-A747-706BE132AF5F}");
            ConvertedNewClass m_testField;
            AZ::Vector3 m_position = AZ::Vector3::CreateZero();
            ConvertedNewClass m_value;
        };

        m_serializeContext->Class<EmptyDeprecatedClass>();
        m_serializeContext->Class<AggregateTestClassV1>()
            ->Field("m_testField", &AggregateTestClassV1::m_testField)
            ->Field("m_position", &AggregateTestClassV1::m_position)
            ->Field("m_value", &AggregateTestClassV1::m_value)
            ;

        // Write out AggrgateTestClassV1 instance
        AggregateTestClassV1 testData;
        testData.m_position = AZ::Vector3(1.0f, 2.0f, 3.0f);
        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> saveStream(&byteBuffer);
        {
            EXPECT_TRUE(AZ::Utils::SaveObjectToStream(saveStream, AZ::DataStream::ST_XML, &testData, m_serializeContext.get()));
            saveStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        }

        // Unreflect AggregateTestClassV1
        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<EmptyDeprecatedClass>();
        m_serializeContext->Class<AggregateTestClassV1>();
        m_serializeContext->DisableRemoveReflection();

        // Reflect AggregateTestClassV2 and load the AggregateTestClassV1 data into memory
        m_serializeContext->Class<ConvertedNewClass>()
            ->Field("m_value", &ConvertedNewClass::m_value)
            ;
        m_serializeContext->Class<AggregateTestClassV2>()
            ->Field("m_testField", &AggregateTestClassV2::m_testField)
            ->Field("m_position", &AggregateTestClassV2::m_position)
            ->Field("m_value", &AggregateTestClassV2::m_value)
            ;

        m_serializeContext->ClassDeprecate("EmptyDeprecatedClass", "{73890A64-9ADB-4639-B0E0-93294CE81B19}",
            [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElementNode) -> bool
        {
            rootElementNode.Convert<ConvertedNewClass>(context);
            return true;
        });

        AggregateTestClassV2 resultData;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(saveStream, resultData, m_serializeContext.get()));
        EXPECT_TRUE(testData.m_position.IsClose(resultData.m_position));
        EXPECT_EQ(5, resultData.m_value.m_value);

        // Cleanup - Unreflect the AggregateTestClassV2, ConvertedNewClass and the EmptyDeprecatedClass
        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<ConvertedNewClass>();
        m_serializeContext->Class<AggregateTestClassV2>();
        m_serializeContext->ClassDeprecate("EmptyDeprecatedClass", "{73890A64-9ADB-4639-B0E0-93294CE81B19}",
            [](AZ::SerializeContext&, AZ::SerializeContext::DataElementNode&) -> bool
        {
            return true;
        });
        m_serializeContext->DisableRemoveReflection();
    }
    struct ClassWithObjectStreamCallback
    {
        AZ_TYPE_INFO(ClassWithObjectStreamCallback, "{780F96D2-9907-439D-94B2-60B915BC12F6}");
        AZ_CLASS_ALLOCATOR(ClassWithObjectStreamCallback, AZ::SystemAllocator, 0);

        ClassWithObjectStreamCallback() = default;
        ClassWithObjectStreamCallback(int32_t value)
            : m_value{ value }
        {}

        static void ReflectWithEventHandler(ReflectContext* context, SerializeContext::IEventHandler* eventHandler)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                //Reflected Template classes must be reflected in one field
                serializeContext->Class<ClassWithObjectStreamCallback>()
                    ->EventHandler(eventHandler)
                    ->Field("m_value", &ClassWithObjectStreamCallback::m_value)
                    ;
            }
        }

        class ObjectStreamEventHandler
            : public SerializeContext::IEventHandler
        {
        public:
            MOCK_METHOD1(OnLoadedFromObjectStream, void(void*));
            MOCK_METHOD1(OnObjectCloned, void(void*));
        };

        int32_t m_value{};
    };

    TEST_F(ObjectStreamSerialization, OnLoadedFromObjectStreamIsInvokedForObjectStreamLoading)
    {
        ClassWithObjectStreamCallback::ObjectStreamEventHandler mockEventHandler;
        EXPECT_CALL(mockEventHandler, OnLoadedFromObjectStream(testing::_)).Times(1);
        ClassWithObjectStreamCallback::ReflectWithEventHandler(m_serializeContext.get(), &mockEventHandler);

        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        {
            ClassWithObjectStreamCallback saveObject{ 1234349 };
            AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, &saveObject, m_serializeContext.get());
            byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        }

        ClassWithObjectStreamCallback loadObject;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadObject, m_serializeContext.get());
    }

    TEST_F(ObjectStreamSerialization, OnLoadedFromObjectStreamIsNotInvokedForCloneObject)
    {
        ClassWithObjectStreamCallback::ObjectStreamEventHandler mockEventHandler;
        EXPECT_CALL(mockEventHandler, OnLoadedFromObjectStream(testing::_)).Times(0);
        EXPECT_CALL(mockEventHandler, OnObjectCloned(testing::_)).Times(1);
        ClassWithObjectStreamCallback::ReflectWithEventHandler(m_serializeContext.get(), &mockEventHandler);

        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        ClassWithObjectStreamCallback saveObject{ 5 };

        ClassWithObjectStreamCallback cloneObject;
        m_serializeContext->CloneObjectInplace(cloneObject, &saveObject);
    }

    TEST_F(ObjectStreamSerialization, OnClonedObjectIsInvokedForCloneObject)
    {
        ClassWithObjectStreamCallback::ObjectStreamEventHandler mockEventHandler;
        EXPECT_CALL(mockEventHandler, OnObjectCloned(testing::_)).Times(2);
        ClassWithObjectStreamCallback::ReflectWithEventHandler(m_serializeContext.get(), &mockEventHandler);

        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        ClassWithObjectStreamCallback saveObject{ 5 };

        ClassWithObjectStreamCallback cloneObject;
        m_serializeContext->CloneObjectInplace(cloneObject, &saveObject);
        
        // Cloning the cloned object should increase the newly cloned object m_value by one again
        ClassWithObjectStreamCallback secondCloneObject;
        m_serializeContext->CloneObjectInplace(secondCloneObject, &cloneObject);
    }

    TEST_F(ObjectStreamSerialization, OnClonedObjectIsNotInvokedForObjectStreamLoading)
    {
        ClassWithObjectStreamCallback::ObjectStreamEventHandler mockEventHandler;
        EXPECT_CALL(mockEventHandler, OnObjectCloned(testing::_)).Times(0);
        EXPECT_CALL(mockEventHandler, OnLoadedFromObjectStream(testing::_)).Times(1);
        ClassWithObjectStreamCallback::ReflectWithEventHandler(m_serializeContext.get(), &mockEventHandler);

        AZStd::vector<AZ::u8> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        {
            ClassWithObjectStreamCallback saveObject{ -396320 };
            AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, &saveObject, m_serializeContext.get());
            byteStream.Seek(0, AZ::IO::GenericStream::SeekMode::ST_SEEK_BEGIN);
        }

        ClassWithObjectStreamCallback loadObject;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadObject, m_serializeContext.get());
    }

    class GenericClassInfoExplicitReflectFixture
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_serializeContext = AZStd::make_unique<SerializeContext>();
            AZ::GenericClassInfo* genericInfo = SerializeGenericTypeInfo<AZStd::vector<AZ::u32>>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(m_serializeContext.get());
            }

            genericInfo = SerializeGenericTypeInfo<AZStd::string>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(m_serializeContext.get());
            }

            genericInfo = SerializeGenericTypeInfo<AZStd::unordered_map<float, float>>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(m_serializeContext.get());
            }
        }

        void TearDown() override
        {
            m_serializeContext->EnableRemoveReflection();
            AZ::GenericClassInfo* genericInfo = SerializeGenericTypeInfo<AZStd::vector<AZ::u32>>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(m_serializeContext.get());
            }

            genericInfo = SerializeGenericTypeInfo<AZStd::string>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(m_serializeContext.get());
            }

            genericInfo = SerializeGenericTypeInfo<AZStd::unordered_map<float, float>>::GetGenericInfo();
            if (genericInfo)
            {
                genericInfo->Reflect(m_serializeContext.get());
            }

            m_serializeContext->DisableRemoveReflection();

            m_serializeContext.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };

    TEST_F(GenericClassInfoExplicitReflectFixture, RootVectorTest)
    {
        AZStd::vector<AZ::u32> rootVector{ 7, 3, 5, 7 };

        {
            // Serializing vector as root class
            AZStd::vector<char> byteBuffer;
            IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
            ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
            byteObjStream->WriteClass(&rootVector);
            byteObjStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            AZStd::vector<AZ::u32> loadedVector;
            AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadedVector, m_serializeContext.get());
            EXPECT_EQ(rootVector, loadedVector);
        }
    }

    TEST_F(GenericClassInfoExplicitReflectFixture, RootStringTest)
    {
        AZStd::string rootString("TestString");

        {
            // Serializing string as root class
            AZStd::vector<char> byteBuffer;
            IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
            ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
            byteObjStream->WriteClass(&rootString);
            byteObjStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            AZStd::string loadedString;
            AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadedString, m_serializeContext.get());
            EXPECT_EQ(rootString, loadedString);
        }
    }

    TEST_F(GenericClassInfoExplicitReflectFixture, RootUnorderedMapTest)
    {
        AZStd::unordered_map<float, float> rootMap;
        rootMap.emplace(7.0f, 20.1f);
        rootMap.emplace(0.0f, 17.0f);

        {
            // Serializing vector as root class
            AZStd::vector<char> byteBuffer;
            IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
            ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
            byteObjStream->WriteClass(&rootMap);
            byteObjStream->Finalize();

            byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            AZStd::unordered_map<float, float> loadedMap;
            AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadedMap, m_serializeContext.get());
            EXPECT_EQ(rootMap, loadedMap);
        }
    }

    class GenericClassInfoInheritanceFixture
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_serializeContext = AZStd::make_unique<SerializeContext>();
            StringUtils::Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {
            m_serializeContext->EnableRemoveReflection();
            StringUtils::Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();

            m_serializeContext.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        class StringUtils : public AZStd::string
        {
        public:

            StringUtils() = default;
            StringUtils(const char* constString)
                : AZStd::string(constString)
            {}

            AZ_TYPE_INFO(StringUtils, "{F3CCCFC0-7890-46A4-9246-067E8A9D2FDE}");

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serialize->Class<StringUtils, AZStd::string>();
                }
            }

            // ... useful string manipulation functions ...
        };

    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };

    TEST_F(GenericClassInfoInheritanceFixture, StringInheritanceTest)
    {
        StringUtils testStringUtils("Custom String");

        // BINARY
        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_BINARY);
        byteObjStream->WriteClass(&testStringUtils);
        byteObjStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        StringUtils loadStringUtils;
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadStringUtils, m_serializeContext.get());
        EXPECT_EQ(testStringUtils, loadStringUtils);
    }

    class SerializableTupleTest
        : public AllocatorsFixture
    {
    public:
        using FloatStringIntTuple = std::tuple<float, AZStd::string, int>;
        using EntityIdEntityTuple = std::tuple<AZ::EntityId, AZ::Entity*>;
        using AnyAnyAnyTuple = std::tuple<AZStd::any, AZStd::any, AZStd::any>;
        using SmartPtrAnyTuple = std::tuple<AZStd::shared_ptr<AZStd::any>>;
        using EmptyTuple = std::tuple<>;
        using TupleCeption = std::tuple<std::tuple<AZStd::string>>;
        using EntityIdVectorStringMap = AZStd::unordered_map<AZ::EntityId, AZStd::vector<AZStd::string>>;
        // We must expose the class for serialization first.
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_serializeContext = AZStd::make_unique<SerializeContext>();
            AZ::Entity::Reflect(m_serializeContext.get());
            AZ::GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<FloatStringIntTuple>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<EntityIdEntityTuple>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<AnyAnyAnyTuple>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<SmartPtrAnyTuple>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<EntityIdVectorStringMap>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<EmptyTuple>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<TupleCeption>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
        }

        void TearDown() override
        {
            m_serializeContext->EnableRemoveReflection();
            AZ::Entity::Reflect(m_serializeContext.get());
            AZ::GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<FloatStringIntTuple>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<EntityIdEntityTuple>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<AnyAnyAnyTuple>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<SmartPtrAnyTuple>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<EntityIdVectorStringMap>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<EmptyTuple>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<TupleCeption>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            m_serializeContext->DisableRemoveReflection();

            m_serializeContext.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

    protected:

        AZStd::unique_ptr<SerializeContext> m_serializeContext;
    };

    TEST_F(SerializableTupleTest, EmptyTupleTest)
    {
        EmptyTuple testTuple;

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&testTuple);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        EmptyTuple loadTuple;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadTuple, m_serializeContext.get()));
        EXPECT_EQ(testTuple, loadTuple);
    }

    TEST_F(SerializableTupleTest, BasicTypeTest)
    {
        FloatStringIntTuple testTuple{ 3.14f, "Tuple", -1 };

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&testTuple);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        FloatStringIntTuple loadTuple;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadTuple, m_serializeContext.get()));
        EXPECT_EQ(testTuple, loadTuple);
    }

    TEST_F(SerializableTupleTest, PointerTupleTest)
    {
        EntityIdEntityTuple testTuple{ AZ::Entity::MakeId(), aznew AZ::Entity("Tuple") };

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&testTuple);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        EntityIdEntityTuple loadTuple;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadTuple, m_serializeContext.get()));
        EXPECT_EQ(std::get<0>(testTuple), std::get<0>(loadTuple));
        EXPECT_EQ(std::get<1>(testTuple)->GetId(), std::get<1>(loadTuple)->GetId());

        delete std::get<1>(testTuple);
        delete std::get<1>(loadTuple);
    }

    TEST_F(SerializableTupleTest, TupleAnyTest)
    {
        AnyAnyAnyTuple testTuple{ AZStd::make_any<AZStd::string>("FirstAny"), AZStd::any(EntityIdVectorStringMap()), AZStd::make_any<AZ::Entity>("Tuple") };

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&testTuple);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        AnyAnyAnyTuple loadTuple;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadTuple, m_serializeContext.get()));
        auto testStringPtr = AZStd::any_cast<AZStd::string>(&std::get<0>(testTuple));
        ASSERT_NE(nullptr, testStringPtr);
        auto loadStringPtr = AZStd::any_cast<AZStd::string>(&std::get<0>(loadTuple));
        ASSERT_NE(nullptr, loadStringPtr);
        auto testMapPtr = AZStd::any_cast<EntityIdVectorStringMap>(&std::get<1>(testTuple));
        ASSERT_NE(nullptr, testMapPtr);
        auto loadMapPtr = AZStd::any_cast<EntityIdVectorStringMap>(&std::get<1>(loadTuple));
        ASSERT_NE(nullptr, loadMapPtr);
        auto testEntityPtr = AZStd::any_cast<AZ::Entity>(&std::get<2>(testTuple));
        ASSERT_NE(nullptr, testEntityPtr);
        auto loadEntityPtr = AZStd::any_cast<AZ::Entity>(&std::get<2>(loadTuple));
        ASSERT_NE(nullptr, loadEntityPtr);

        EXPECT_EQ(*testStringPtr, *loadStringPtr);
        EXPECT_EQ(*testMapPtr, *loadMapPtr);
        EXPECT_EQ(testEntityPtr->GetId(), loadEntityPtr->GetId());
    }

    TEST_F(SerializableTupleTest, UniquePtrAnyTupleTest)
    {
        SmartPtrAnyTuple testTuple{ AZStd::make_shared<AZStd::any>(AZStd::make_any<AZStd::string>("SuperWrappedString")) };

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&testTuple);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        SmartPtrAnyTuple loadTuple;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadTuple, m_serializeContext.get()));
        auto rawTestPtr = std::get<0>(testTuple).get();
        auto rawLoadPtr = std::get<0>(loadTuple).get();
        ASSERT_NE(nullptr, rawLoadPtr);
        auto testStringPtr =  AZStd::any_cast<AZStd::string>(rawTestPtr);
        ASSERT_NE(nullptr, testStringPtr);
        auto loadStringPtr = AZStd::any_cast<AZStd::string>(rawLoadPtr);
        ASSERT_NE(nullptr, loadStringPtr);
        EXPECT_EQ(*testStringPtr, *loadStringPtr);
    }

    TEST_F(SerializableTupleTest, 2Fast2TuplesTest)
    {
        TupleCeption testTuple{ AZStd::make_tuple(AZStd::string("InnerTupleString")) };

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&testTuple);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        TupleCeption loadTuple;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadTuple, m_serializeContext.get()));
        EXPECT_EQ(testTuple, loadTuple);
    }

    class SerializableAZStdArrayTest
        : public AllocatorsFixture
    {
    public:
        using ZeroArray = AZStd::array<float, 0>;
        using FloatFourArray = AZStd::array<float, 4>;
        using ZeroNestedArray = AZStd::array<AZStd::array<float, 0>, 0>;
        using NestedArray = AZStd::array<AZStd::array<FloatFourArray, 3>, 2>;
        // We must expose the class for serialization first.
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_serializeContext = AZStd::make_unique<SerializeContext>();
            AZ::GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<ZeroArray>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<FloatFourArray>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<ZeroNestedArray>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<NestedArray>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
        }

        void TearDown() override
        {
            m_serializeContext->EnableRemoveReflection();
            AZ::GenericClassInfo* genericClassInfo = SerializeGenericTypeInfo<ZeroArray>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<FloatFourArray>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<ZeroNestedArray>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            genericClassInfo = SerializeGenericTypeInfo<NestedArray>::GetGenericInfo();
            if (genericClassInfo)
            {
                genericClassInfo->Reflect(m_serializeContext.get());
            }
            m_serializeContext->DisableRemoveReflection();

            m_serializeContext.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

    protected:
        FloatFourArray m_array;
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
    };

    TEST_F(SerializableAZStdArrayTest, SingleEntryCount)
    {
        Internal::AZStdArrayEvents events;
        events.OnWriteBegin(&m_array);

        for (size_t i = 0; i < 16; ++i)
        {
            EXPECT_EQ(i, events.GetIndex());
            events.Increment();
        }
        for (size_t i = 16; i > 8; --i)
        {
            EXPECT_EQ(i, events.GetIndex());
            events.Decrement();
        }

        events.OnWriteEnd(&m_array);
        EXPECT_TRUE(events.IsEmpty());
    }

    TEST_F(SerializableAZStdArrayTest, MultipleEntriesCount)
    {
        Internal::AZStdArrayEvents events;
        events.OnWriteBegin(&m_array);
        for (size_t i = 0; i < 8; ++i)
        {
            events.Increment();
        }
        for (size_t i = 8; i > 4; --i)
        {
            EXPECT_EQ(i, events.GetIndex());
            events.Decrement();
        }

        events.OnWriteBegin(&m_array);
        for (size_t i = 0; i < 16; ++i)
        {
            EXPECT_EQ(i, events.GetIndex());
            events.Increment();
        }
        for (size_t i = 16; i > 8; --i)
        {
            EXPECT_EQ(i, events.GetIndex());
            events.Decrement();
        }
        events.OnWriteEnd(&m_array);
        EXPECT_EQ(4, events.GetIndex()); // The 8 entries on the first entry of the stack.

        events.OnWriteEnd(&m_array);
        EXPECT_TRUE(events.IsEmpty());
    }

    TEST_F(SerializableAZStdArrayTest, SingleEntryContainerInterface)
    {
        GenericClassInfo* containerInfo = SerializeGenericTypeInfo<decltype(m_array)>::GetGenericInfo();
        ASSERT_NE(nullptr, containerInfo);
        ASSERT_NE(nullptr, containerInfo->GetClassData());
        SerializeContext::IDataContainer* container = containerInfo->GetClassData()->m_container;
        ASSERT_NE(nullptr, container);

        SerializeContext::IEventHandler* eventHandler = containerInfo->GetClassData()->m_eventHandler;
        ASSERT_NE(nullptr, eventHandler);
        eventHandler->OnWriteBegin(&m_array);

        void* element0 = container->ReserveElement(&m_array, nullptr);
        ASSERT_NE(nullptr, element0);
        *reinterpret_cast<float*>(element0) = 42.0f;
        container->StoreElement(&m_array, element0);

        void* element1 = container->ReserveElement(&m_array, nullptr);
        ASSERT_NE(nullptr, element1);
        *reinterpret_cast<float*>(element1) = 142.0f;
        container->StoreElement(&m_array, element1);

        void* deletedElement = container->ReserveElement(&m_array, nullptr);
        ASSERT_NE(nullptr, deletedElement);
        *reinterpret_cast<float*>(deletedElement) = 9000.0f;
        container->RemoveElement(&m_array, deletedElement, nullptr);

        void* element2 = container->ReserveElement(&m_array, nullptr);
        ASSERT_NE(nullptr, element2);
        *reinterpret_cast<float*>(element2) = 242.0f;
        container->StoreElement(&m_array, element2);

        void* element3 = container->ReserveElement(&m_array, nullptr);
        ASSERT_NE(nullptr, element3);
        *reinterpret_cast<float*>(element3) = 342.0f;
        container->StoreElement(&m_array, element2);

        void* overflownElement = container->ReserveElement(&m_array, nullptr);
        EXPECT_EQ(nullptr, overflownElement);

        eventHandler->OnWriteEnd(&m_array);
        eventHandler->OnLoadedFromObjectStream(&m_array);

        EXPECT_FLOAT_EQ( 42.0f, m_array[0]);
        EXPECT_FLOAT_EQ(142.0f, m_array[1]);
        EXPECT_FLOAT_EQ(242.0f, m_array[2]);
        EXPECT_FLOAT_EQ(342.0f, m_array[3]);
    }

    TEST_F(SerializableAZStdArrayTest, SimpleSerialization)
    {
        m_array[0] = 10.0f;
        m_array[1] = 11.1f;
        m_array[2] = 12.2f;
        m_array[3] = 13.3f;

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&m_array);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        FloatFourArray loadedArray;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadedArray, m_serializeContext.get()));

        for (size_t i = 0; i < 4; ++i)
        {
            EXPECT_EQ(m_array[i], loadedArray[i]);
        }
    }

    TEST_F(SerializableAZStdArrayTest, NestedSerialization)
    {
        NestedArray nested;
        nested[0][0][0] = 0.0f;
        nested[0][0][1] = 0.1f;
        nested[0][0][2] = 0.2f;
        nested[0][0][3] = 0.3f;

        nested[0][1][0] = 1.0f;
        nested[0][1][1] = 1.1f;
        nested[0][1][2] = 1.2f;
        nested[0][1][3] = 1.3f;

        nested[0][2][0] = 2.0f;
        nested[0][2][1] = 2.1f;
        nested[0][2][2] = 2.2f;
        nested[0][2][3] = 2.3f;

        nested[1][0][0] = 10.0f;
        nested[1][0][1] = 10.1f;
        nested[1][0][2] = 10.2f;
        nested[1][0][3] = 10.3f;

        nested[1][1][0] = 11.0f;
        nested[1][1][1] = 11.1f;
        nested[1][1][2] = 11.2f;
        nested[1][1][3] = 11.3f;

        nested[1][2][0] = 12.0f;
        nested[1][2][1] = 12.1f;
        nested[1][2][2] = 12.2f;
        nested[1][2][3] = 12.3f;

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&nested);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        NestedArray loadedArray;
        ASSERT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadedArray, m_serializeContext.get()));

        for (size_t l = 0; l < 2; ++l)
        {
            for (size_t k = 0; k < 3; ++k)
            {
                for (size_t i = 0; i < 4; ++i)
                {
                    EXPECT_EQ(nested[l][k][i], loadedArray[l][k][i]);
                }
            }
        }
    }

    TEST_F(SerializableAZStdArrayTest, ZeroSerialization)
    {
        ZeroArray zerroArray;

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&zerroArray);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        ZeroArray loadedArray;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadedArray, m_serializeContext.get()));
    }

    TEST_F(SerializableAZStdArrayTest, ZeroNestedSerialization)
    {
        ZeroNestedArray zerroArray;

        AZStd::vector<char> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        auto objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_XML);
        objStream->WriteClass(&zerroArray);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        ZeroNestedArray loadedArray;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadedArray, m_serializeContext.get()));
    }

    struct VectorTest
    {
        AZ_RTTI(VectorTest, "{2BE9FC5C-14A6-49A7-9A2C-79F6C2F27221}");
        virtual ~VectorTest() = default;
        AZStd::vector<int> m_vec;

        static bool Convert(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            AZStd::vector<int> vec;
            AZ::SerializeContext::DataElementNode* vecElement = classElement.FindSubElement(AZ_CRC("m_vec"));
            EXPECT_TRUE(vecElement != nullptr);
            bool gotData = vecElement->GetData(vec);
            EXPECT_TRUE(gotData);
            vec.push_back(42);
            bool setData = vecElement->SetData(context, vec);
            EXPECT_TRUE(setData);
            return true;
        }
    };

    // Splitting these tests up to make it easier to find memory leaks for specific containers.
    TEST_F(Serialization, ReserveAndFreeWithoutMemLeaks_Array) { ReserveAndFreeWithoutMemLeaks<AZStd::array<float, 5>>(); }
    TEST_F(Serialization, ReserveAndFreeWithoutMemLeaks_FixedVector) { ReserveAndFreeWithoutMemLeaks<AZStd::fixed_vector<float, 5>>(); }
    TEST_F(Serialization, ReserveAndFreeWithoutMemLeaks_ForwardList) { ReserveAndFreeWithoutMemLeaks<AZStd::forward_list<float>>(); }
    TEST_F(Serialization, ReserveAndFreeWithoutMemLeaks_UnorderedSet) { ReserveAndFreeWithoutMemLeaks<AZStd::unordered_set<float>>(); }
    TEST_F(Serialization, ReserveAndFreeWithoutMemLeaks_UnorderedMultiSet) { ReserveAndFreeWithoutMemLeaks<AZStd::unordered_multiset<float>>(); }
    TEST_F(Serialization, ReserveAndFreeWithoutMemLeaks_List) { ReserveAndFreeWithoutMemLeaks<AZStd::list<float>>(); }
    TEST_F(Serialization, ReserveAndFreeWithoutMemLeaks_Set) { ReserveAndFreeWithoutMemLeaks<AZStd::set<float>>(); }
    TEST_F(Serialization, ReserveAndFreeWithoutMemLeaks_Vector) { ReserveAndFreeWithoutMemLeaks<AZStd::vector<float>>(); }

    TEST_F(Serialization, ConvertVectorContainer)
    {
        // Reflect version 1
        m_serializeContext->Class<VectorTest>()
            ->Version(1)
            ->Field("m_vec", &VectorTest::m_vec);

        VectorTest test;
        test.m_vec.push_back(1024);

        // write test to an XML buffer
        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_XML);
        byteObjStream->WriteClass(&test);
        byteObjStream->Finalize();

        // Update the version to 2 and add the converter
        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<VectorTest>();
        m_serializeContext->DisableRemoveReflection();
        m_serializeContext->Class<VectorTest>()
            ->Version(2, &VectorTest::Convert)
            ->Field("m_vec", &VectorTest::m_vec);

        // Reset for read
        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        test = VectorTest{};
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, test, m_serializeContext.get());
        EXPECT_EQ(2, test.m_vec.size());
    }

    class SerializeVectorWithInitialElementsTest
        : public AllocatorsFixture
    {
    public:
        // We must expose the class for serialization first.
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            m_serializeContext = AZStd::make_unique<SerializeContext>();
            VectorWrapper::Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {
            m_serializeContext.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            AllocatorsFixture::TearDown();
        }

        struct VectorWrapper
        {
            AZ_TYPE_INFO(VectorWrapper, "{91F69715-30C3-4F1A-90A0-5F5F7517F375}");
            AZ_CLASS_ALLOCATOR(VectorWrapper, AZ::SystemAllocator, 0);
            VectorWrapper()
                : m_fixedVectorInts(2, 412)
                , m_vectorInts(2, 42)
            {}

            static void Reflect(AZ::ReflectContext* context)
            {
                if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                {
                    serializeContext->Class<VectorWrapper>()
                        ->Field("fixedVectorInts", &VectorWrapper::m_fixedVectorInts)
                        ->Field("VectorInts", &VectorWrapper::m_vectorInts);
                }
            }
            AZStd::fixed_vector<int, 2> m_fixedVectorInts;
            AZStd::vector<int> m_vectorInts;
        };

    protected:
        AZStd::unique_ptr<SerializeContext> m_serializeContext;
    };

    TEST_F(SerializeVectorWithInitialElementsTest, CloneObjectTest)
    {
        VectorWrapper vectorWrapper;
        ASSERT_EQ(2, vectorWrapper.m_fixedVectorInts.size());
        ASSERT_EQ(2, vectorWrapper.m_vectorInts.size());
        vectorWrapper.m_fixedVectorInts[1] = 256;
        vectorWrapper.m_vectorInts[0] = 5;
        vectorWrapper.m_vectorInts[1] = 10;

        VectorWrapper* clonedWrapper = m_serializeContext->CloneObject(&vectorWrapper);
        ASSERT_NE(nullptr, clonedWrapper);

        EXPECT_EQ(vectorWrapper.m_vectorInts.size(), clonedWrapper->m_vectorInts.size());
        EXPECT_EQ(5, clonedWrapper->m_vectorInts[0]);
        EXPECT_EQ(10, clonedWrapper->m_vectorInts[1]);

        EXPECT_EQ(vectorWrapper.m_fixedVectorInts.size(), clonedWrapper->m_fixedVectorInts.size());
        EXPECT_EQ(256, clonedWrapper->m_fixedVectorInts[1]);

        delete clonedWrapper;
    }

    TEST_F(SerializeVectorWithInitialElementsTest, CloneObjectInplaceTest)
    {
        VectorWrapper vectorWrapper;
        ASSERT_EQ(2, vectorWrapper.m_fixedVectorInts.size());
        ASSERT_EQ(2, vectorWrapper.m_vectorInts.size());
        vectorWrapper.m_fixedVectorInts[1] = 256;
        vectorWrapper.m_vectorInts[0] = 5;
        vectorWrapper.m_vectorInts[1] = 10;

        VectorWrapper clonedWrapper;
        m_serializeContext->CloneObjectInplace(clonedWrapper, &vectorWrapper);
        EXPECT_EQ(vectorWrapper.m_vectorInts.size(), clonedWrapper.m_vectorInts.size());
        EXPECT_EQ(5, clonedWrapper.m_vectorInts[0]);
        EXPECT_EQ(10, clonedWrapper.m_vectorInts[1]);

        EXPECT_EQ(vectorWrapper.m_fixedVectorInts.size(), clonedWrapper.m_fixedVectorInts.size());
        EXPECT_EQ(256, clonedWrapper.m_fixedVectorInts[1]);
    }

    TEST_F(SerializeVectorWithInitialElementsTest, ObjectStreamTest)
    {
        VectorWrapper vectorWrapper;
        ASSERT_EQ(2, vectorWrapper.m_fixedVectorInts.size());
        ASSERT_EQ(2, vectorWrapper.m_vectorInts.size());
        vectorWrapper.m_fixedVectorInts[1] = 256;
        vectorWrapper.m_vectorInts[0] = 5;
        vectorWrapper.m_vectorInts[1] = 10;

        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_XML);
        byteObjStream->WriteClass(&vectorWrapper);
        byteObjStream->Finalize();

        byteStream.Seek(0U, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        VectorWrapper loadedWrapper;
        bool loadSuccess = AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadedWrapper, m_serializeContext.get());
        EXPECT_TRUE(loadSuccess);

        EXPECT_EQ(vectorWrapper.m_vectorInts.size(), loadedWrapper.m_vectorInts.size());
        EXPECT_EQ(5, loadedWrapper.m_vectorInts[0]);
        EXPECT_EQ(10, loadedWrapper.m_vectorInts[1]);

        EXPECT_EQ(vectorWrapper.m_fixedVectorInts.size(), loadedWrapper.m_fixedVectorInts.size());
        EXPECT_EQ(256, loadedWrapper.m_fixedVectorInts[1]);
    }

    TEST_F(SerializeVectorWithInitialElementsTest, DataPatchTest)
    {
        VectorWrapper modifiedWrapper;
        ASSERT_EQ(2, modifiedWrapper.m_fixedVectorInts.size());
        ASSERT_EQ(2, modifiedWrapper.m_vectorInts.size());
        modifiedWrapper.m_fixedVectorInts[1] = 256;
        modifiedWrapper.m_vectorInts[0] = 5;
        modifiedWrapper.m_vectorInts[1] = 10;
        modifiedWrapper.m_vectorInts.push_back(15);
        
        VectorWrapper initialWrapper;
        
        DataPatch patch;
        patch.Create(&initialWrapper, azrtti_typeid<VectorWrapper>(), &modifiedWrapper, azrtti_typeid<VectorWrapper>(), DataPatch::FlagsMap(), DataPatch::FlagsMap(), m_serializeContext.get());
        VectorWrapper* patchedWrapper = patch.Apply(&initialWrapper, m_serializeContext.get());

        ASSERT_NE(nullptr, patchedWrapper);
        EXPECT_EQ(modifiedWrapper.m_vectorInts.size(), patchedWrapper->m_vectorInts.size());
        EXPECT_EQ(5, patchedWrapper->m_vectorInts[0]);
        EXPECT_EQ(10, patchedWrapper->m_vectorInts[1]);
        EXPECT_EQ(15, patchedWrapper->m_vectorInts[2]);

        EXPECT_EQ(modifiedWrapper.m_fixedVectorInts.size(), patchedWrapper->m_fixedVectorInts.size());
        EXPECT_EQ(256, patchedWrapper->m_fixedVectorInts[1]);

        delete patchedWrapper;
    }

    struct TestLeafNode
    {
        AZ_RTTI(TestLeafNode, "{D50B136B-82E1-414F-9D84-FEC3A75DC9DF}");

        TestLeafNode() = default;
        TestLeafNode(int field) : m_field(field)
        {}

        virtual ~TestLeafNode() = default;

        int m_field = 0;
    };

    struct TestContainer
    {
        AZ_RTTI(TestContainer, "{6941B3D8-1EE9-4EBD-955A-AB55CFDEE77A}");

        TestContainer() = default;

        virtual ~TestContainer() = default;

        TestLeafNode m_node;
    };

    class TestLeafNodeSerializer
        : public SerializeContext::IDataSerializer
    {
        /// Store the class data into a stream.
        size_t Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
        {
            int tempData;

            tempData = reinterpret_cast<const TestLeafNode*>(classPtr)->m_field;
            AZ_SERIALIZE_SWAP_ENDIAN(tempData, isDataBigEndian);

            return static_cast<size_t>(stream.Write(sizeof(tempData), reinterpret_cast<void*>(&tempData)));
        }

        size_t DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) override
        {
            if (in.GetLength() < sizeof(int))
            {
                return 0;
            }

            int tempData;
            in.Read(sizeof(int), reinterpret_cast<void*>(&tempData));
            char textBuffer[256];

            AZ_SERIALIZE_SWAP_ENDIAN(tempData, isDataBigEndian);
            char* textData = &textBuffer[0];
            azsnprintf(textData, sizeof(textBuffer), "%d", tempData);

            AZStd::string outText = textBuffer;

            return static_cast<size_t>(out.Write(outText.size(), outText.data()));
        }

        size_t TextToData(const char* text, unsigned int /*textVersion*/, IO::GenericStream& stream, bool isDataBigEndian /*= false*/) override
        {
            int value;
            value = atoi(text);
            AZ_SERIALIZE_SWAP_ENDIAN(value, isDataBigEndian);

            stream.Seek(0, IO::GenericStream::ST_SEEK_BEGIN);
            return static_cast<size_t>(stream.Write(sizeof(value), reinterpret_cast<void*>(&value)));
        }

        bool Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian /*= false*/) override
        {
            int tempData = 0;
            if (stream.GetLength() < sizeof(tempData))
            {
                return false;
            }

            stream.Read(sizeof(tempData), reinterpret_cast<void*>(&tempData));

            EXPECT_EQ(version, 1);

            AZ_SERIALIZE_SWAP_ENDIAN(tempData, isDataBigEndian);
            *reinterpret_cast<TestLeafNode*>(classPtr) = TestLeafNode{ tempData };
            return true;
        }

        bool CompareValueData(const void* lhs, const void* rhs) override
        {
            int tempDataLhs = reinterpret_cast<const TestLeafNode*>(lhs)->m_field;;
            int tempDataRhs = reinterpret_cast<const TestLeafNode*>(rhs)->m_field;;

            return tempDataLhs == tempDataRhs;
        }
    };

    // Serializer which sets a reference bool to true on deletion to detect when it's lifetime ends.
    class TestDeleterSerializer
        : public SerializeContext::IDataSerializer
    {
    public:
        TestDeleterSerializer(bool& serializerDeleted)
            : m_serializerDeleted{ serializerDeleted }
        {}
        ~TestDeleterSerializer() override
        {
            m_serializerDeleted = true;
        }
        size_t Save(const void*, IO::GenericStream&, bool) override
        {
            return {};
        }

        size_t DataToText(IO::GenericStream&, IO::GenericStream&, bool) override
        {
            return {};
        }

        size_t TextToData(const char*, unsigned int , IO::GenericStream&, bool) override
        {
            return {};
        }

        bool Load(void*, IO::GenericStream&, unsigned int, bool) override
        {
            return true;
        }

        bool CompareValueData(const void* lhs, const void* rhs) override
        {
            AZ_UNUSED(lhs);
            AZ_UNUSED(rhs);
            return true;
        }

    private:
        bool& m_serializerDeleted;
    };

    TEST_F(Serialization, ConvertWithCustomSerializer)
    {
        m_serializeContext->Class<TestContainer>()
            ->Version(1)
            ->Field("m_node", &TestContainer::m_node);

        m_serializeContext->Class<TestLeafNode>()
            ->Version(1)
            ->Serializer<TestLeafNodeSerializer>();

        const int testValue = 123;
        TestContainer test;
        test.m_node.m_field = testValue;

        // write test to an XML buffer
        AZStd::vector<char> byteBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > byteStream(&byteBuffer);
        ObjectStream* byteObjStream = ObjectStream::Create(&byteStream, *m_serializeContext, ObjectStream::ST_XML);
        byteObjStream->WriteClass(&test);
        byteObjStream->Finalize();

        // Update the version to 2
        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<TestContainer>();
        m_serializeContext->Class<TestLeafNode>();
        m_serializeContext->DisableRemoveReflection();
        m_serializeContext->Class<TestContainer>()
            ->Version(2)
            ->Field("m_node", &TestContainer::m_node);
        m_serializeContext->Class<TestLeafNode>()
            ->Version(2)
            ->Serializer<TestLeafNodeSerializer>();

        // Reset for read
        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        test = {};
        AZ::Utils::LoadObjectFromStreamInPlace(byteStream, test, m_serializeContext.get());

        EXPECT_EQ(test.m_node.m_field, testValue);
    }

    TEST_F(Serialization, CustomSerializerWithDefaultDeleter_IsDeletedOnUnreflect)
    {
        bool serializerDeleted = false;
        AZ::SerializeContext::IDataSerializerPtr customSerializer{ new TestDeleterSerializer{ serializerDeleted }, AZ::SerializeContext::IDataSerializer::CreateDefaultDeleteDeleter() };
        m_serializeContext->Class<TestLeafNode>()
            ->Version(1)
            ->Serializer(AZStd::move(customSerializer));

        EXPECT_FALSE(serializerDeleted);

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<TestLeafNode>();
        m_serializeContext->DisableRemoveReflection();
        EXPECT_TRUE(serializerDeleted);
    }

    TEST_F(Serialization, CustomSerializerWithNoDeleteDeleter_IsNotDeletedOnUnreflect)
    {
        bool serializerDeleted = false;
        TestDeleterSerializer* serializerInstance = new TestDeleterSerializer{ serializerDeleted };
        AZ::SerializeContext::IDataSerializerPtr customSerializer{ serializerInstance, AZ::SerializeContext::IDataSerializer::CreateNoDeleteDeleter() };
        m_serializeContext->Class<TestLeafNode>()
            ->Version(1)
            ->Serializer(AZStd::move(customSerializer));

        EXPECT_FALSE(serializerDeleted);

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<TestLeafNode>();
        m_serializeContext->DisableRemoveReflection();
        ASSERT_FALSE(serializerDeleted);
        delete serializerInstance;
    }

    TEST_F(Serialization, DefaultCtorThatAllocatesMemoryDoesntLeak)
    {
        ClassThatAllocatesMemoryInDefaultCtor::Reflect(*GetSerializeContext());

        AZStd::vector<char> xmlBuffer;
        IO::ByteContainerStream<AZStd::vector<char> > xmlStream(&xmlBuffer);
        {
            ClassThatAllocatesMemoryInDefaultCtor obj;
            ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, *GetSerializeContext(), ObjectStream::ST_XML);
            xmlObjStream->WriteClass(&obj);
            xmlObjStream->Finalize();
        }
        xmlStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        ClassThatAllocatesMemoryInDefaultCtor* deserialized = AZ::Utils::LoadObjectFromStream<ClassThatAllocatesMemoryInDefaultCtor>(xmlStream);
        EXPECT_TRUE(deserialized);
        if (deserialized)
        {
            delete deserialized;
        }

        EXPECT_EQ(ClassThatAllocatesMemoryInDefaultCtor::InstanceTracker::s_instanceCount, 0);
    }

    // Test that loading containers in-place clears any existing data in the
    // containers (
    template <typename T>
    class GenericsLoadInPlaceHolder final
    {
    public:
        AZ_RTTI(((GenericsLoadInPlaceHolder<T>), "{98328203-83F0-4644-B1F6-34DDF50F3416}", T));

        static void Reflect(AZ::SerializeContext& sc)
        {
            sc.Class<GenericsLoadInPlaceHolder>()->Version(1)->Field("data", &GenericsLoadInPlaceHolder::m_data);
        }

        T m_data;
    };

    template <typename T>
    class GenericsLoadInPlaceFixture
        : public Serialization
    {
    public:
        GenericsLoadInPlaceHolder<T> m_holder;
    };

    TYPED_TEST_CASE_P(GenericsLoadInPlaceFixture);

    TYPED_TEST_P(GenericsLoadInPlaceFixture, ClearsOnLoadInPlace)
    {
        using DataType = decltype(this->m_holder);
        DataType::Reflect(*this->GetSerializeContext());

        // Add 3 items to the container
        for (int i = 0; i < 3; ++i)
        {
            this->m_holder.m_data.insert(this->m_holder.m_data.end(), i);
        }

        // Serialize the container
        AZStd::vector<char> xmlBuffer;
        IO::ByteContainerStream<AZStd::vector<char>> xmlStream(&xmlBuffer);
        {
            ObjectStream* xmlObjStream = ObjectStream::Create(&xmlStream, *this->GetSerializeContext(), ObjectStream::ST_XML);
            xmlObjStream->WriteClass(&this->m_holder);
            xmlObjStream->Finalize();
        }
        xmlStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);

        // Put different data in a different instance
        DataType got;
        for (int i = 3; i < 6; ++i)
        {
            got.m_data.insert(got.m_data.end(), i);
        }

        // Verify that the two containers are different
        EXPECT_THAT(got.m_data, ::testing::Ne(this->m_holder.m_data));

        // Deserialize the container into a new one
        AZ::Utils::LoadObjectFromStreamInPlace(xmlStream, got, this->GetSerializeContext());

        // Verify the two containers are the same
        EXPECT_THAT(got.m_data, ::testing::ContainerEq(this->m_holder.m_data));
    }

    REGISTER_TYPED_TEST_CASE_P(GenericsLoadInPlaceFixture, ClearsOnLoadInPlace);

    // The test ClearsOnLoadInPlace is run once for each type in this list
    typedef ::testing::Types<
        AZStd::vector<int>,
        AZStd::list<int>,
        AZStd::forward_list<int>,
        AZStd::set<int>,
        AZStd::unordered_set<int>,
        AZStd::unordered_multiset<int>
    > TypesThatShouldBeClearedWhenLoadedInPlace;
    INSTANTIATE_TYPED_TEST_CASE_P(Clears, GenericsLoadInPlaceFixture, TypesThatShouldBeClearedWhenLoadedInPlace);

    enum TestUnscopedSerializationEnum : int32_t
    {
        TestUnscopedSerializationEnum_Option1,
        TestUnscopedSerializationEnum_Option2,
        TestUnscopedSerializationEnum_Option3,
        TestUnscopedSerializationEnum_Option5NotReflected = 4,
        TestUnscopedSerializationEnum_Option4 = 3,
    };

    enum class TestScopedSerializationEnum
    {
        Option1,
        Option2,
        Option3,
        Option4,
        Option5NotReflected,
    };

    enum class TestUnsignedEnum : uint32_t
    {
        Option42 = 42,
    };
}

AZ_TYPE_INFO_SPECIALIZE(UnitTest::TestUnscopedSerializationEnum, "{83383BFA-F6DA-4124-BE4F-2FAAB7C594E7}");
AZ_TYPE_INFO_SPECIALIZE(UnitTest::TestScopedSerializationEnum, "{17341C5E-81C3-44CB-A40D-F97D49C2531D}");

AZ_TYPE_INFO_SPECIALIZE(UnitTest::TestUnsignedEnum, "{0F91A5AE-DADA-4455-B158-8DB79D277495}");

namespace UnitTest
{
    enum class TestNoTypeInfoEnum
    {
        Zeroth,
        Second = 2,
        Fourth = 4,
    };
    struct NoTypeInfoNonReflectedEnumWrapper
    {
        AZ_TYPE_INFO(NoTypeInfoNonReflectedEnumWrapper, "{500D534D-4535-46FE-8D0C-7EC0782553F7}");
        TestNoTypeInfoEnum m_value{};
    };
    struct TypeInfoReflectedEnumWrapper
    {
        AZ_TYPE_INFO(TypeInfoReflectedEnumWrapper, "{00ACD993-28B4-4951-91E8-16056EA8A8DA}");
        TestScopedSerializationEnum m_value{};
    };

    class EnumTypeSerialization
        : public ScopedAllocatorSetupFixture
    {
    public:
        void SetUp() override
        {
            ScopedAllocatorSetupFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        }

        void TearDown() override
        {
            m_serializeContext.reset();
            ScopedAllocatorSetupFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };

    TEST_F(EnumTypeSerialization, TestUnscopedEnumReflection_Succeeds)
    {
        m_serializeContext->Enum<TestUnscopedSerializationEnum>();
        const AZ::SerializeContext::ClassData* enumClassData = m_serializeContext->FindClassData(azrtti_typeid<TestUnscopedSerializationEnum>());
        ASSERT_NE(nullptr, enumClassData);
        AZ::TypeId underlyingTypeId = AZ::TypeId::CreateNull();
        AttributeReader attrReader(nullptr, enumClassData->FindAttribute(AZ::Serialize::Attributes::EnumUnderlyingType));
        EXPECT_TRUE(attrReader.Read<AZ::TypeId>(underlyingTypeId));
        EXPECT_EQ(azrtti_typeid<int32_t>(), underlyingTypeId);

        // Unreflect Enum type
        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Enum<TestUnscopedSerializationEnum>();
        m_serializeContext->DisableRemoveReflection();
        enumClassData = m_serializeContext->FindClassData(azrtti_typeid<TestUnscopedSerializationEnum>());
        EXPECT_EQ(nullptr, enumClassData);
    }

    TEST_F(EnumTypeSerialization, TestScopedEnumReflection_Succeeds)
    {
        m_serializeContext->Enum<TestScopedSerializationEnum>();
        const AZ::SerializeContext::ClassData* enumClassData = m_serializeContext->FindClassData(azrtti_typeid<TestScopedSerializationEnum>());
        ASSERT_NE(nullptr, enumClassData);

        // Unreflect Enum type
        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Enum<TestScopedSerializationEnum>();
        m_serializeContext->DisableRemoveReflection();
        enumClassData = m_serializeContext->FindClassData(azrtti_typeid<TestScopedSerializationEnum>());
        EXPECT_EQ(nullptr, enumClassData);
    }

    TEST_F(EnumTypeSerialization, TestEnumReflectionWithValues_Succeeds)
    {
        m_serializeContext->Enum<TestUnscopedSerializationEnum>()
            ->Value("Option1", TestUnscopedSerializationEnum::TestUnscopedSerializationEnum_Option1)
            ->Value("Option2", TestUnscopedSerializationEnum::TestUnscopedSerializationEnum_Option2)
            ->Value("Option3", TestUnscopedSerializationEnum::TestUnscopedSerializationEnum_Option3)
            ->Value("Option4", TestUnscopedSerializationEnum::TestUnscopedSerializationEnum_Option4)
            ;

        const AZ::SerializeContext::ClassData* enumClassData = m_serializeContext->FindClassData(azrtti_typeid<TestUnscopedSerializationEnum>());
        ASSERT_NE(nullptr, enumClassData);
        using EnumConstantBase = AZ::SerializeContextEnumInternal::EnumConstantBase;
        using EnumConstantBasePtr = AZStd::unique_ptr<EnumConstantBase>;
        AZStd::vector<AZStd::reference_wrapper<EnumConstantBase>> enumConstants;
        enumConstants.reserve(4);
        for (const AZ::AttributeSharedPair& attrPair : enumClassData->m_attributes)
        {
            if (attrPair.first == AZ::Serialize::Attributes::EnumValueKey)
            {
                auto enumConstantAttribute{ azrtti_cast<AZ::AttributeData<EnumConstantBasePtr>*>(attrPair.second.get()) };
                ASSERT_NE(nullptr, enumConstantAttribute);
                const EnumConstantBasePtr& sourceEnumConstant = enumConstantAttribute->Get(nullptr);
                ASSERT_NE(nullptr, sourceEnumConstant);
                enumConstants.emplace_back(*sourceEnumConstant);
            }
        }

        ASSERT_EQ(4, enumConstants.size());
        EXPECT_EQ("Option1", static_cast<EnumConstantBase&>(enumConstants[0]).GetEnumValueName());
        EXPECT_EQ(0, static_cast<EnumConstantBase&>(enumConstants[0]).GetEnumValueAsUInt());
        EXPECT_EQ("Option2", static_cast<EnumConstantBase&>(enumConstants[1]).GetEnumValueName());
        EXPECT_EQ(1, static_cast<EnumConstantBase&>(enumConstants[1]).GetEnumValueAsUInt());
        EXPECT_EQ("Option3", static_cast<EnumConstantBase&>(enumConstants[2]).GetEnumValueName());
        EXPECT_EQ(2, static_cast<EnumConstantBase&>(enumConstants[2]).GetEnumValueAsUInt());
        EXPECT_EQ("Option4", static_cast<EnumConstantBase&>(enumConstants[3]).GetEnumValueName());
        EXPECT_EQ(3, static_cast<EnumConstantBase&>(enumConstants[3]).GetEnumValueAsUInt());

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Enum<TestUnscopedSerializationEnum>();
        m_serializeContext->DisableRemoveReflection();
    }

    TEST_F(EnumTypeSerialization, TestEnumFieldWithTypeInfoAndReflectedAsEnum_Succeeds)
    {
        m_serializeContext->Enum<TestScopedSerializationEnum>()
            ->Value("Option1", TestScopedSerializationEnum::Option1)
            ->Value("Option2", TestScopedSerializationEnum::Option2)
            ->Value("Option3", TestScopedSerializationEnum::Option3)
            ->Value("Option4", TestScopedSerializationEnum::Option4)
            ;

        m_serializeContext->Class<TypeInfoReflectedEnumWrapper>()
            ->Field("m_value", &TypeInfoReflectedEnumWrapper::m_value)
            ;

        // The TestScopedSerializationEnum is explicitly reflected as an Enum in the SerializeContext and FindClassData
        // should return the EnumType class data
        const AZ::SerializeContext::ClassData* enumClassData = m_serializeContext->FindClassData(azrtti_typeid<TestScopedSerializationEnum>());
        ASSERT_NE(nullptr, enumClassData);
        EXPECT_EQ(azrtti_typeid<TestScopedSerializationEnum>(), enumClassData->m_typeId);

        TypeInfoReflectedEnumWrapper testObject;
        testObject.m_value = TestScopedSerializationEnum::Option3;
        AZStd::vector<uint8_t> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_BINARY);
        objStream->WriteClass(&testObject);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        TypeInfoReflectedEnumWrapper loadObject;
        const bool loadResult = AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadObject, m_serializeContext.get());
        EXPECT_TRUE(loadResult);
        EXPECT_EQ(TestScopedSerializationEnum::Option3, loadObject.m_value);

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<TypeInfoReflectedEnumWrapper>();
        m_serializeContext->Enum<TestScopedSerializationEnum>();
        m_serializeContext->DisableRemoveReflection();
    }

    TEST_F(EnumTypeSerialization, TestEnumFieldWithTypeInfoAndNotReflectedAsEnum_Succeeds)
    {
        m_serializeContext->Class<TypeInfoReflectedEnumWrapper>()
            ->Field("m_value", &TypeInfoReflectedEnumWrapper::m_value)
            ;

        // The TestScopedSerializationEnum is not reflected as an Enum in the SerializeContext, but has specialized AzTypeInfo
        // So FindClassData should return the underlying type in this case, which is an int
        const AZ::SerializeContext::ClassData* underlyingTypeClassData = m_serializeContext->FindClassData(azrtti_typeid<TestScopedSerializationEnum>());
        ASSERT_NE(nullptr, underlyingTypeClassData);
        EXPECT_EQ(azrtti_typeid<int>(), underlyingTypeClassData->m_typeId);

        TypeInfoReflectedEnumWrapper testObject;
        testObject.m_value = TestScopedSerializationEnum::Option3;
        AZStd::vector<uint8_t> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_BINARY);
        objStream->WriteClass(&testObject);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        TypeInfoReflectedEnumWrapper loadObject;
        const bool loadResult = AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadObject, m_serializeContext.get());
        EXPECT_TRUE(loadResult);
        EXPECT_EQ(TestScopedSerializationEnum::Option3, loadObject.m_value);

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<TypeInfoReflectedEnumWrapper>();
        m_serializeContext->DisableRemoveReflection();
    }

    TEST_F(EnumTypeSerialization, TestEnumFieldWithNoTypeInfo_Succeeds)
    {
        m_serializeContext->Class<NoTypeInfoNonReflectedEnumWrapper>()
            ->Field("m_value", &NoTypeInfoNonReflectedEnumWrapper::m_value)
            ;

        static_assert(!AZ::Internal::HasAZTypeInfo<TestNoTypeInfoEnum>::value, "Test enum type should not have AzTypeInfo");
        NoTypeInfoNonReflectedEnumWrapper testObject;
        testObject.m_value = TestNoTypeInfoEnum::Second;
        AZStd::vector<uint8_t> byteBuffer;
        AZ::IO::ByteContainerStream<decltype(byteBuffer)> byteStream(&byteBuffer);
        AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&byteStream, *m_serializeContext, AZ::ObjectStream::ST_BINARY);
        objStream->WriteClass(&testObject);
        objStream->Finalize();

        byteStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        NoTypeInfoNonReflectedEnumWrapper loadObject;
        const bool loadResult = AZ::Utils::LoadObjectFromStreamInPlace(byteStream, loadObject, m_serializeContext.get());
        EXPECT_TRUE(loadResult);
        EXPECT_EQ(TestNoTypeInfoEnum::Second, loadObject.m_value);

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<NoTypeInfoNonReflectedEnumWrapper>();
        m_serializeContext->DisableRemoveReflection();
    }

    TEST_F(EnumTypeSerialization, LoadIntIntoEnumTypeInfoSpecialization_Succeeds)
    {
        AZStd::string_view typeInfoEnumWrapperObjStreamData = R"(<ObjectStream version="3">
            <Class name="TypeInfoReflectedEnumWrapper" type="{00ACD993-28B4-4951-91E8-16056EA8A8DA}">
                <Class name="int" field="m_value" value="72" type="{72039442-EB38-4d42-A1AD-CB68F7E0EEF6}"/>
            </Class>
        </ObjectStream>
        )";

        m_serializeContext->Class<TypeInfoReflectedEnumWrapper>()
            ->Field("m_value", &TypeInfoReflectedEnumWrapper::m_value)
            ;

        // Validate that the "m_value" ClassElement reflected to the TypeInfoReflectedEnumWrapper class
        // is set to the Type of TestScopedSerializationEnum and not the TypeId of int
        // When using enum types in fields previously it always used the underlying type for reflection
        // Now if the enum type is being used in a field and has specialized AzTypeInfo, it uses the specialized TypeID
        const SerializeContext::ClassData* classData = m_serializeContext->FindClassData(azrtti_typeid<TypeInfoReflectedEnumWrapper>());
        ASSERT_NE(nullptr, classData);
        ASSERT_EQ(1, classData->m_elements.size());
        EXPECT_EQ(azrtti_typeid<TestScopedSerializationEnum>(), classData->m_elements[0].m_typeId);
        EXPECT_NE(azrtti_typeid<int>(), classData->m_elements[0].m_typeId);

        AZ::IO::MemoryStream memStream(typeInfoEnumWrapperObjStreamData.data(), typeInfoEnumWrapperObjStreamData.size());
        TypeInfoReflectedEnumWrapper testObject;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(memStream, testObject, m_serializeContext.get()));
        EXPECT_EQ(72, static_cast<int>(testObject.m_value));

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<TypeInfoReflectedEnumWrapper>();
        m_serializeContext->DisableRemoveReflection();
    }

    struct TestUnsignedEnumWrapper
    {
        AZ_TYPE_INFO(TestUnsignedEnumWrapper, "{A5DD32CD-EC5B-4F0D-9D25-239EC76F1860}");
        TestUnsignedEnum m_value{};
    };

    TEST_F(EnumTypeSerialization, VersionConverterRunOnEnum_ConvertsTypeSuccessfully)
    {
        AZStd::string_view typeInfoEnumWrapperObjStreamData = R"(<ObjectStream version="3">
            <Class name="TestUnsignedEnumWrapper" type="{A5DD32CD-EC5B-4F0D-9D25-239EC76F1860}">
                <Class name="unsigned int" field="m_value" value="234343" type="{43DA906B-7DEF-4ca8-9790-854106D3F983}"/>
            </Class>
        </ObjectStream>
        )";

        auto VersionConverter = [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement) -> bool
        {
            if (classElement.GetVersion() < 1)
            {
                int enumIndex = classElement.FindElement(AZ_CRC("m_value"));
                if (enumIndex == -1)
                {
                    return false;
                }

                AZ::SerializeContext::DataElementNode& enumValueNode = classElement.GetSubElement(enumIndex);
                TestUnsignedEnum oldValue{};
                EXPECT_TRUE(enumValueNode.GetData(oldValue));
                EXPECT_EQ(234343U, static_cast<std::underlying_type_t<TestUnsignedEnum>>(oldValue));
                EXPECT_TRUE(enumValueNode.Convert<TestUnsignedEnum>(context));
                EXPECT_TRUE(enumValueNode.SetData(context, TestUnsignedEnum::Option42));
            }
            return true;
        };

        m_serializeContext->Class<TestUnsignedEnumWrapper>()
            ->Version(1, VersionConverter)
            ->Field("m_value", &TestUnsignedEnumWrapper::m_value)
            ;

        AZ::IO::MemoryStream memStream(typeInfoEnumWrapperObjStreamData.data(), typeInfoEnumWrapperObjStreamData.size());
        TestUnsignedEnumWrapper testObject;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(memStream, testObject, m_serializeContext.get()));
        EXPECT_EQ(TestUnsignedEnum::Option42, testObject.m_value);

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<TestUnsignedEnumWrapper>();
        m_serializeContext->DisableRemoveReflection();
    }

    struct TestClassWithEnumField
    {
        AZ_TYPE_INFO(TestClassWithEnumField, "{F1F03A45-3E6D-44C3-A615-A556DEE18E94}");
        TestUnsignedEnum m_value{};
        AZStd::string m_strValue;
    };

    TEST_F(EnumTypeSerialization, LoadingOldVersionOfClassWithEnumFieldStoredUsingTheUnderlying_AndThatClassDoesNotHaveAVersionConverter_Succeeds)
    {
        AZStd::string_view testClassWithEnumFieldData = R"(<ObjectStream version="3">
            <Class name="TestClassWithEnumField" type="{F1F03A45-3E6D-44C3-A615-A556DEE18E94}">
                <Class name="unsigned int" field="m_value" value="42" type="{43DA906B-7DEF-4ca8-9790-854106D3F983}"/>
            </Class>
        </ObjectStream>
        )";

        m_serializeContext->Class<TestClassWithEnumField>()
            ->Version(1)
            ->Field("m_value", &TestClassWithEnumField::m_value)
            ->Field("m_strValue", &TestClassWithEnumField::m_strValue)
            ;

        AZ::IO::MemoryStream memStream(testClassWithEnumFieldData.data(), testClassWithEnumFieldData.size());
        TestClassWithEnumField testObject;
        EXPECT_TRUE(AZ::Utils::LoadObjectFromStreamInPlace(memStream, testObject, m_serializeContext.get()));
        EXPECT_EQ(TestUnsignedEnum::Option42, testObject.m_value);

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<TestClassWithEnumField>();
        m_serializeContext->DisableRemoveReflection();
    }

    struct TestClassWithEnumFieldThatSpecializesTypeInfo
    {
        AZ_TYPE_INFO(TestClassWithEnumFieldThatSpecializesTypeInfo, "{B7E066F4-3598-4678-A331-5AB8789CE391}");
        TestUnsignedEnum m_value{};
    };

    TEST_F(EnumTypeSerialization, CloneObjectAZStdAnyOfEnum_SucceedsWithoutCrashing)
    {

        m_serializeContext->Class<TestClassWithEnumFieldThatSpecializesTypeInfo>()
            ->Version(1)
            ->Field("m_value", &TestClassWithEnumFieldThatSpecializesTypeInfo::m_value)
            ;

        AZStd::any testAny(AZStd::make_any<TestUnsignedEnum>(TestUnsignedEnum::Option42));
        AZStd::any resultAny;
        m_serializeContext->CloneObjectInplace(resultAny, &testAny);
        auto resultEnum = AZStd::any_cast<TestUnsignedEnum>(&resultAny);
        ASSERT_NE(nullptr, resultEnum);
        EXPECT_EQ(TestUnsignedEnum::Option42, *resultEnum);

        m_serializeContext->EnableRemoveReflection();
        m_serializeContext->Class<TestClassWithEnumFieldThatSpecializesTypeInfo>();
        m_serializeContext->DisableRemoveReflection();
    }
}

