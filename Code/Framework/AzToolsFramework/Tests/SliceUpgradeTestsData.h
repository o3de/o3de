/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/Asset/SimpleAsset.h>

namespace AZ
{
    // Added definition of type info and rtti for the DataPatchTypeUpgrade class
    // to this Unit Test file to allow rtti functions to be accessible from the SerializeContext::TypeChange
    // call
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME_IMPL(SerializeContext::DataPatchTypeUpgrade, \
        "DataPatchTypeUpgrade", "{E5A2F519-261C-4B81-925F-3730D363AB9C}", AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS);
    AZ_RTTI_NO_TYPE_INFO_IMPL((SerializeContext::DataPatchTypeUpgrade, \
        AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_CLASS), DataPatchUpgrade);
}

namespace UnitTest
{
    static const float TestDataA_ExpectedVal = 1.5f;

    class TestDataA
    {
    public:
        float m_val = TestDataA_ExpectedVal;

    public:
        AZ_RTTI(TestDataA, "{3B7949D0-07BF-408E-8101-264466AEC403}");

        virtual ~TestDataA() = default;

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestDataA>()
                    // Version defaults to 0
                    ->Field("Val", &TestDataA::m_val)
                    ;
            }
        }
    };

    inline constexpr AZ::TypeId TestComponentATypeId{ "{C802148B-7EDC-4518-9780-FB9F99880446}" };
    class TestComponentA_V0
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        TestDataA m_data;

    public:
        AZ_EDITOR_COMPONENT(TestComponentA_V0, TestComponentATypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentA_V0, AzToolsFramework::Components::EditorComponentBase>()
                    // Version defaults to 0
                    ->Field("Data", &TestComponentA_V0::m_data)
                    ;
            }
        }
    };

    class NewTestDataA
    {
    public:
        float m_val = 2.5f;

    public:
        AZ_RTTI(NewTestDataA, "{2CEC8357-5156-4C8C-B664-501EA19213CB}");

        virtual ~NewTestDataA() = default;

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<NewTestDataA>()
                    // Version defaults to 0
                    ->Field("Val", &NewTestDataA::m_val)
                    ;
            }
        }
    };

    class TestComponentA_V1
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        NewTestDataA m_data;

    public:
        AZ_EDITOR_COMPONENT(TestComponentA_V1, TestComponentATypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static int ConvertV0Float_to_V1Int(float in)
        {
            return (int)in;
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentA_V1, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(1)
                    ->Field("NewData", &TestComponentA_V1::m_data)
                    ->TypeChange<TestDataA, NewTestDataA>("Data", 0, 1, [](TestDataA in) -> NewTestDataA {
                        return NewTestDataA();
                    })
                    ->NameChange(0, 1, "Data", "NewData")
                    ;
            }
        }
    };

    inline constexpr AZ::TypeId TestDataB_TypeId { "{20E6777B-6857-409B-B27F-9E505D4378EF}" };

    struct TestDataB_V0
    {
        static AZ::u64 PersistentIdCounter;

        AZ_RTTI(TestDataB_V0, TestDataB_TypeId);

        TestDataB_V0()
            : m_persistentId(++PersistentIdCounter)
            , m_data(0)
        {}

        TestDataB_V0(int data)
            : m_persistentId(++PersistentIdCounter)
            , m_data(data)
        {}

        virtual ~TestDataB_V0() = default;


        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestDataB_V0>()
                    ->Version(0)
                    ->PersistentId([](const void* instance) -> AZ::u64 { return reinterpret_cast<const TestDataB_V0*>(instance)->m_persistentId; })
                    ->Field("PersistentId", &TestDataB_V0::m_persistentId)
                    ->Field("Data", &TestDataB_V0::m_data)
                    ;
            }
        }

        AZ::u64 m_persistentId;
        int m_data;
    };
    AZ::u64 TestDataB_V0::PersistentIdCounter = 1024;

    struct TestDataB_V1
    {
        AZ_RTTI(TestDataB_V1, TestDataB_TypeId);

        TestDataB_V1()
            : m_persistentId(++TestDataB_V0::PersistentIdCounter)
            , m_info(0)
        {}

        virtual ~TestDataB_V1() = default;

        static float TestDataB_V0_V1(int in)
        {
            return in + 13.5f;
        }

        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // conversion from Version 0 to Version 1
            // - Data (int) becomes Info (float) with the conversion Info = Data + 13.5f
            if (classElement.GetVersion() == 0)
            {
                int dataIndex = classElement.FindElement(AZ_CRC_CE("Data"));

                if (dataIndex < 0)
                {
                    return false;
                }

                AZ::SerializeContext::DataElementNode& dataElement = classElement.GetSubElement(dataIndex);
                int data;

                dataElement.GetData<int>(data);

                //Create a new info value
                int infoIndex = classElement.AddElement<float>(context, "Info");
                AZ::SerializeContext::DataElementNode& infoElement = classElement.GetSubElement(infoIndex);

                //Set width and height data to x and y from the old size
                infoElement.SetData<float>(context, data + 13.5f);

                classElement.RemoveElement(dataIndex);
            }

            return true;
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestDataB_V1>()
                    ->Version(1, &VersionConverter)
                    ->PersistentId([](const void* instance) -> AZ::u64 { return reinterpret_cast<const TestDataB_V1*>(instance)->m_persistentId; })
                    ->Field("PersistentId", &TestDataB_V1::m_persistentId)
                    ->Field("Info", &TestDataB_V1::m_info)
                    ->TypeChange<int, float>("Data", 0, 1, [](int in)-> float {
                        float result = TestDataB_V0_V1(in);
                        return result;
                    })
                    ->NameChange(0, 1, "Data", "Info")
                    ;
            }
        }

        AZ::u64 m_persistentId;
        float m_info = 27.5f;
    };

    inline constexpr AZ::TypeId TestComponentBTypeId{ "{10778D96-4860-4690-9A0E-B1066C00136B}" };

    class TestComponentB_V0
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZStd::unordered_map<int, TestDataB_V0> m_unorderedMap;

    public:
        AZ_EDITOR_COMPONENT(TestComponentB_V0, TestComponentBTypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentB_V0, AzToolsFramework::Components::EditorComponentBase>()
                    ->Field("UnorderedMap", &TestComponentB_V0::m_unorderedMap)
                    ;
            }
        }
    };

    // TestComponentB_V0_1 is NOT a version upgrade of TestComponentB_V0. It is TestComponentB_V0.
    // We have to create a different class to represent TestComponentB_V0 so we can simulate
    // version upgrade of TestDataB.
    class TestComponentB_V0_1
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZStd::unordered_map<int, TestDataB_V1> m_unorderedMap;

    public:
        AZ_EDITOR_COMPONENT(TestComponentB_V0_1, TestComponentBTypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentB_V0_1, AzToolsFramework::Components::EditorComponentBase>()
                    ->Field("UnorderedMap", &TestComponentB_V0_1::m_unorderedMap)
                    ;
            }
        }
    };

    class TestComponentC_V0
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZStd::vector<TestDataB_V0> m_vec;

    public:
        AZ_EDITOR_COMPONENT(TestComponentC_V0, TestComponentBTypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentC_V0, AzToolsFramework::Components::EditorComponentBase>()
                    ->Field("Vector", &TestComponentC_V0::m_vec)
                    ;
            }
        }
    };

    // TestComponentC_V0_1 is NOT a version upgrade of TestComponentC_V0. It is TestComponentC_V0.
    // We have to create a different class to represent TestComponentC_V0 so we can simulate
    // version upgrade of TestDataB.
    class TestComponentC_V0_1
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZStd::vector<TestDataB_V1> m_vec;

    public:
        AZ_EDITOR_COMPONENT(TestComponentC_V0_1, TestComponentBTypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentC_V0_1, AzToolsFramework::Components::EditorComponentBase>()
                    ->Field("Vector", &TestComponentC_V0_1::m_vec)
                    ;
            }
        }
    };

    inline constexpr AZ::TypeId TestComponentDTypeId{ "{77655B67-3E03-418C-B010-D272DBCEAE25}" };

    // Initial Test Values
    static const int Value1_Initial = 3;
    static const float Value2_Initial = 7;
    static const char* AssetPath_Initial = "C:/ly/dev/assets/myslicetestasset.NaN";

    // Data Patch Override Values
    static const int Value1_Override = 5;
    static const float Value2_Override = 9;
    static const char* AssetPath_Override = "C:/ly/dev/assets/SliceTestAssets/myslicetestasset.NaN";

    // Final Test Values
    static const AZStd::string_view Value1_Final = "Five";
    static const AZStd::string_view Value2_Final = "Nine";

    class TestComponentD_V1
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        int m_firstData = Value1_Initial;
        float m_secondData = Value2_Initial;
        AZStd::string m_asset = AssetPath_Initial;

    public:
        AZ_EDITOR_COMPONENT(TestComponentD_V1, TestComponentDTypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentD_V1, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(1)
                    ->Field("IntData", &TestComponentD_V1::m_firstData)
                    ->Field("FloatData", &TestComponentD_V1::m_secondData)
                    ->Field("AssetData", &TestComponentD_V1::m_asset)
                    ;
            }
        }
    };

    class SliceUpgradeTestAsset
    {
    public:
        AZ_TYPE_INFO(SliceUpgradeTestAsset, "{10A39071-9287-49FE-93C8-55F7715FC758}")
            static const char* GetFileFilter()
        {
            return "*.NaN";
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SliceUpgradeTestAsset>()
                    ->Version(1);
            }
        }
    };

    class TestComponentD_V2
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZStd::string m_firstData;
        AZStd::string m_secondData;
        AzFramework::SimpleAssetReference<SliceUpgradeTestAsset> m_asset;

    public:
        AZ_EDITOR_COMPONENT(TestComponentD_V2, TestComponentDTypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static AZStd::string IntToString(int val)
        {
            switch (val)
            {
            case 1:
                return "One";
            case 2:
                return "Two";
            case 3:
                return "Three";
            case 4:
                return "Four";
            case 5:
                return "Five";
            case 6:
                return "Six";
            case 7:
                return "Seven";
            case 8:
                return "Eight";
            case 9:
                return "Nine";
            case 0:
                return "Zero";
            default:
                return "NaN";
            }
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentD_V2, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(2)
                    ->Field("StringData", &TestComponentD_V2::m_firstData)
                    ->TypeChange<int, AZStd::string>("IntData", 1, 2, &IntToString)
                    ->NameChange(1, 2, "IntData", "StringData")
                    ->Field("SecondStringData", &TestComponentD_V2::m_secondData)
                    ->TypeChange<float, AZStd::string>("FloatData", 1, 2, [](float in)->AZStd::string {return IntToString(int(in)); })
                    ->NameChange(1, 2, "FloatData", "SecondStringData")
                    ->Field("AssetData", &TestComponentD_V2::m_asset)
                    ->TypeChange<AZStd::string, AzFramework::SimpleAssetReference<SliceUpgradeTestAsset>>("AssetData", 1, 2, [](const AZStd::string& in)->AzFramework::SimpleAssetReference<SliceUpgradeTestAsset>
                        {
                            AzFramework::SimpleAssetReference<SliceUpgradeTestAsset> sliceUpgradeAsset;
                            sliceUpgradeAsset.SetAssetPath(in.c_str());
                            return sliceUpgradeAsset;
                        });
            }
        }
    };

    // Test Data for: UpgradeSkipVersion_TypeChange_FloatToDouble
    // This test makes sure the data patch upgrade system can
    // properly select upgrades. It will attempt to perform
    // each of the following upgrades:
    // 1. float (V4) -> int (V5)                                // Applies a single upgrade to convert a data patch originally created using TestComponentE_V4 to one that can be applied to TestComponentE_V5
    // 2. float (V4) -> int (V5), int (V5) -> double (V6)       // Applies 2 incremental upgrades to upgrade a data patch created using TestComponentE_V4 so that it can be applied to TestComponentE_V6_1 (Expected data loss)
    // 3. float (V4) -> double (V6)                             // Applies a skip-version patch to go directly from TestComponentE_V4 to TestComponentE_V6_2 to avoid the data loss in the previous scenario.

    inline constexpr AZ::TypeId TestComponentETypeId{ "{835E5A78-2283-4113-91BC-BFC022619388}" };

    // Original data for our test TestComponentE_V4
    static const float V4_DefaultData = 3.75f;

    // Overridden value in our data patch created using TestComponentE_V4
    static const float V4_OverrideData = 6.33f;

    // Expected value of the override when converting the patch to TestComponentE_V5
    static const int V5_ExpectedData = 3;

    // Expected value of the override when converting the patch to TestComponentE_V6_1 using upgrade method (2)
    static const double V6_ExpectedData_NoSkip = 30.0;

    // Expected value of the override when converting the patch to TestComponentE_V6_2 using upgrade method (3)
    static const double V6_ExpectedData_Skip = 12.66;

    class TestComponentE_V4
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        float m_data;

        static const float ExpectedData;

    public:
        AZ_EDITOR_COMPONENT(TestComponentE_V4, TestComponentETypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentE_V4, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(4)
                    ->Field("FloatData", &TestComponentE_V4::m_data);
            }
        }
    };

    class TestComponentE_V5
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        int m_data;

    public:
        AZ_EDITOR_COMPONENT(TestComponentE_V5, TestComponentETypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static int ConvertV4Float_to_V5Int(float in)
        {
            return ((int)in) / 2;
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentE_V5, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(5)
                    ->Field("IntData", &TestComponentE_V5::m_data)
                    ->TypeChange<float, int>("FloatData", 4, 5, &ConvertV4Float_to_V5Int)
                    ->NameChange(4, 5, "FloatData", "IntData");
            }
        }
    };

    class TestComponentE_V6_1
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        double m_data;

    public:
        AZ_EDITOR_COMPONENT(TestComponentE_V6_1, TestComponentETypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static int ConvertV4Float_to_V5Int(float in)
        {
            return ((int)in) / 2;
        }

        static double ConvertV5Int_to_V6Double(int in)
        {
            return (double)(in * 10);
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentE_V6_1, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(6)
                    ->Field("DoubleData", &TestComponentE_V6_1::m_data)
                    ->TypeChange<float, int>("FloatData", 4, 5, &ConvertV4Float_to_V5Int)
                    ->NameChange(4, 5, "FloatData", "IntData")
                    ->TypeChange<int, double>("IntData", 5, 6, &ConvertV5Int_to_V6Double)
                    ->NameChange(5, 6, "IntData", "DoubleData");
            }
        }
    };

    class TestComponentE_V6_2
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        double m_data;

    public:
        AZ_EDITOR_COMPONENT(TestComponentE_V6_2, TestComponentETypeId);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override {}
        void Activate() override {}
        void Deactivate() override {}
        //////////////////////////////////////////////////////////////////////////

        static int ConvertV4Float_to_V5Int(float in)
        {
            return (int)in;
        }

        static double ConvertV5Int_to_V6Double(int in)
        {
            return (double)(in * 10);
        }

        static double ConvertV4Float_to_V6Double(float in)
        {
            return ((double)in) * 2.0;
        }

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = AZ::RttiCast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<TestComponentE_V6_2, AzToolsFramework::Components::EditorComponentBase>()
                    ->Version(6)
                    ->Field("DoubleData", &TestComponentE_V6_2::m_data)
                    ->TypeChange<float, int>("FloatData", 4, 5, &ConvertV4Float_to_V5Int)
                    ->NameChange(4, 5, "FloatData", "IntData")
                    ->TypeChange<int, double>("IntData", 5, 6, &ConvertV5Int_to_V6Double)
                    ->TypeChange<float, double>("FloatData", 4, 6, &ConvertV4Float_to_V6Double) // The skip-version converter to preserve the floating point data from V4.
                    ->NameChange(5, 6, "IntData", "DoubleData");
            }
        }
    };

} // namespace UnitTest
