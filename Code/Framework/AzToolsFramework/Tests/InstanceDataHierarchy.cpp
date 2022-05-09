/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Component/Entity.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <random>
#include <QDebug>


using namespace AZ;
using namespace AZ::IO;
using namespace AZ::Debug;

namespace UnitTest
{
    class TestComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(TestComponent, "{94D5C952-FD65-4997-B517-F36003F8018A}");

        struct SubData
        {
            AZ_TYPE_INFO(SubData, "{A0165FCA-A311-4FED-B36A-DC5FD2AF2857}");
            AZ_CLASS_ALLOCATOR(SubData, AZ::SystemAllocator, 0);

            SubData() {}
            SubData(int v) : m_int(v) {}
            ~SubData() = default;

            int m_int = 0;
        };

        class SerializationEvents
            : public AZ::SerializeContext::IEventHandler
        {
            void OnReadBegin(void* classPtr) override
            {
                TestComponent* component = reinterpret_cast<TestComponent*>(classPtr);
                component->m_serializeOnReadBegin++;
            }
            void OnReadEnd(void* classPtr) override
            {
                TestComponent* component = reinterpret_cast<TestComponent*>(classPtr);
                component->m_serializeOnReadEnd++;
            }
            void OnWriteBegin(void* classPtr) override
            {
                TestComponent* component = reinterpret_cast<TestComponent*>(classPtr);
                component->m_serializeOnWriteBegin++;
            }
            void OnWriteEnd(void* classPtr) override
            {
                TestComponent* component = reinterpret_cast<TestComponent*>(classPtr);
                component->m_serializeOnWriteEnd++;
            }
        };

        TestComponent() = default;
        ~TestComponent() override
        {
            for (SubData* data : m_pointerContainer)
            {
                delete data;
            }
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SubData>()
                    ->Version(1)
                    ->Field("Int", &SubData::m_int)
                    ;

                serializeContext->Class<TestComponent, AZ::Component>()
                    ->EventHandler<SerializationEvents>()
                    ->Version(1)
                    ->Field("Float", &TestComponent::m_float)
                    ->Field("String", &TestComponent::m_string)
                    ->Field("NormalContainer", &TestComponent::m_normalContainer)
                    ->Field("PointerContainer", &TestComponent::m_pointerContainer)
                    ->Field("SubData", &TestComponent::m_subData)
                    ;

                if (AZ::EditContext* edit = serializeContext->GetEditContext())
                {
                    edit->Class<TestComponent>("Test Component", "A test component")
                        ->DataElement(nullptr, &TestComponent::m_float, "Float Field", "A float field")
                        ->DataElement(nullptr, &TestComponent::m_string, "String Field", "A string field")
                        ->DataElement(nullptr, &TestComponent::m_normalContainer, "Normal Container", "A container")
                        ->DataElement(nullptr, &TestComponent::m_pointerContainer, "Pointer Container", "A container")
                        ->DataElement(nullptr, &TestComponent::m_subData, "Struct Field", "A sub data type")
                        ;

                    edit->Class<SubData>("Test Component", "A test component")
                        ->DataElement(nullptr, &SubData::m_int, "Int Field", "An int")
                        ;
                }
            }
        }

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }

        float m_float = 0.f;
        AZStd::string m_string;

        AZStd::vector<SubData> m_normalContainer;
        AZStd::vector<SubData*> m_pointerContainer;

        SubData m_subData;

        size_t m_serializeOnReadBegin = 0;
        size_t m_serializeOnReadEnd = 0;
        size_t m_serializeOnWriteBegin = 0;
        size_t m_serializeOnWriteEnd = 0;
    };
    
    bool operator==(const TestComponent::SubData& lhs, const TestComponent::SubData& rhs)
    {
        return lhs.m_int == rhs.m_int;
    }

    /**
    * InstanceDataHierarchyBasicTest
    */
    class InstanceDataHierarchyBasicTest
        : public AllocatorsFixture
    {
    public:
        InstanceDataHierarchyBasicTest()
        {
        }

        ~InstanceDataHierarchyBasicTest() override
        {
        }

        void run()
        {
            using namespace AzToolsFramework;

            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();
            Entity::Reflect(&serializeContext);
            TestComponent::Reflect(&serializeContext);

            // Test building of hierarchies, and copying of data from testEntity1 to testEntity2->
            {
                AZStd::unique_ptr<AZ::Entity> testEntity1(new AZ::Entity());
                testEntity1->CreateComponent<TestComponent>();
                AZStd::unique_ptr<AZ::Entity> testEntity2(serializeContext.CloneObject(testEntity1.get()));

                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadBegin == 1);
                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadEnd == 1);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnWriteBegin == 1);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnWriteEnd == 1);

                testEntity1->FindComponent<TestComponent>()->m_float = 1.f;
                testEntity1->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(1));
                testEntity1->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(2));
                testEntity1->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(1));
                testEntity1->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(2));

                // First entity has more entries, so we'll be adding elements to testEntity2->
                testEntity2->FindComponent<TestComponent>()->m_float = 2.f;
                testEntity2->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(1));
                testEntity2->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(1));

                InstanceDataHierarchy idh1;
                idh1.AddRootInstance(testEntity1.get());
                idh1.Build(&serializeContext, 0);

                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadBegin == 2);
                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadEnd == 2);

                InstanceDataHierarchy idh2;
                idh2.AddRootInstance(testEntity2.get());
                idh2.Build(&serializeContext, 0);

                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnReadBegin == 1);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnReadEnd == 1);

                // Verify IDH structure.
                InstanceDataNode* root1 = idh1.GetRootNode();
                AZ_TEST_ASSERT(root1);

                InstanceDataNode* root2 = idh2.GetRootNode();
                AZ_TEST_ASSERT(root2);

                auto secondChildIter = root1->GetChildren().begin();
                AZStd::advance(secondChildIter, 1);
                InstanceDataNode::Address addr = secondChildIter->ComputeAddress();
                AZ_TEST_ASSERT(!addr.empty());
                InstanceDataNode* foundIn2 = idh2.FindNodeByAddress(addr);
                AZ_TEST_ASSERT(foundIn2);

                // Find the TestComponent in entity1's IDH.
                AZStd::stack<InstanceDataNode*> nodeStack;
                nodeStack.push(root1);
                InstanceDataNode* componentNode1 = nullptr;
                while (!nodeStack.empty())
                {
                    InstanceDataNode* node = nodeStack.top();
                    nodeStack.pop();

                    if (node->GetClassMetadata()->m_typeId == AZ::AzTypeInfo<TestComponent>::Uuid())
                    {
                        componentNode1 = node;
                        break;
                    }

                    for (InstanceDataNode& child : node->GetChildren())
                    {
                        nodeStack.push(&child);
                    }
                }

                // Verify we found the component node in both hierarchies.
                AZ_TEST_ASSERT(componentNode1);
                addr = componentNode1->ComputeAddress();
                foundIn2 = idh2.FindNodeByAddress(addr);
                AZ_TEST_ASSERT(foundIn2);

                //// Try copying data from entity 1 to entity 2.
                bool result = InstanceDataHierarchy::CopyInstanceData(componentNode1, foundIn2, &serializeContext);
                AZ_TEST_ASSERT(result);

                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadBegin == 2);
                AZ_TEST_ASSERT(testEntity1->FindComponent<TestComponent>()->m_serializeOnReadEnd == 2);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnWriteBegin == 2);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_serializeOnWriteEnd == 2);

                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_normalContainer.size() == 2);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_pointerContainer.size() == 2);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_float == 1.f);
            }

            // Test removal of container elements during instance data copying.
            {
                AZStd::unique_ptr<AZ::Entity> testEntity1(new AZ::Entity());
                testEntity1->CreateComponent<TestComponent>();
                AZStd::unique_ptr<AZ::Entity> testEntity2(serializeContext.CloneObject(testEntity1.get()));

                // First entity has more in container 1, fewer in container 2 as compared to second entity.
                testEntity1->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(1));
                testEntity1->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(2));
                testEntity1->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(1));

                testEntity2->FindComponent<TestComponent>()->m_normalContainer.push_back(TestComponent::SubData(1));
                testEntity2->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(1));
                testEntity2->FindComponent<TestComponent>()->m_pointerContainer.push_back(aznew TestComponent::SubData(2));

                // Change a field.
                testEntity2->FindComponent<TestComponent>()->m_float = 2.f;

                InstanceDataHierarchy idh1;
                idh1.AddRootInstance(testEntity1.get());
                idh1.Build(&serializeContext, 0);

                InstanceDataHierarchy idh2;
                idh2.AddRootInstance(testEntity2.get());
                idh2.Build(&serializeContext, 0);

                InstanceDataNode* root1 = idh1.GetRootNode();

                // Find the TestComponent in entity1's IDH.
                AZStd::stack<InstanceDataNode*> nodeStack;
                nodeStack.push(root1);
                InstanceDataNode* componentNode1 = nullptr;
                while (!nodeStack.empty())
                {
                    InstanceDataNode* node = nodeStack.top();
                    nodeStack.pop();

                    if (node->GetClassMetadata()->m_typeId == AZ::AzTypeInfo<TestComponent>::Uuid())
                    {
                        componentNode1 = node;
                        break;
                    }

                    for (InstanceDataNode& child : node->GetChildren())
                    {
                        nodeStack.push(&child);
                    }
                }

                // Verify we found the component node in both hierarchies.
                AZ_TEST_ASSERT(componentNode1);
                InstanceDataNode::Address addr = componentNode1->ComputeAddress();
                InstanceDataNode* foundIn2 = idh2.FindNodeByAddress(addr);
                AZ_TEST_ASSERT(foundIn2);

                // Do a comparison test
                {
                    size_t newNodes = 0;
                    size_t removedNodes = 0;
                    size_t changedNodes = 0;

                    InstanceDataHierarchy::CompareHierarchies(componentNode1, foundIn2,
                        &InstanceDataHierarchy::DefaultValueComparisonFunction,
                        &serializeContext,
                        // New node
                        [&](InstanceDataNode* targetNode, AZStd::vector<AZ::u8>& data)
                        {
                            (void)targetNode;
                            (void)data;
                            ++newNodes;
                        },

                        // Removed node (container element).
                        [&](const InstanceDataNode* sourceNode, InstanceDataNode* targetNodeParent)
                        {
                            (void)sourceNode;
                            (void)targetNodeParent;
                            ++removedNodes;
                        },

                        // Changed node
                        [&](const InstanceDataNode* sourceNode, InstanceDataNode* targetNode,
                            AZStd::vector<AZ::u8>& sourceData, AZStd::vector<AZ::u8>& targetData)
                        {
                            (void)sourceNode;
                            (void)targetNode;
                            (void)sourceData;
                            (void)targetData;
                            ++changedNodes;
                        }
                        );

                    AZ_TEST_ASSERT(newNodes == 2);  // 2 because child nodes of new nodes are now also flagged as new
                    AZ_TEST_ASSERT(removedNodes == 1);
                    AZ_TEST_ASSERT(changedNodes == 1);
                }

                //// Try copying data from entity 1 to entity 2.
                bool result = InstanceDataHierarchy::CopyInstanceData(componentNode1, foundIn2, &serializeContext);
                AZ_TEST_ASSERT(result);

                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_normalContainer.size() == 2);
                AZ_TEST_ASSERT(testEntity2->FindComponent<TestComponent>()->m_pointerContainer.size() == 1);
            }

            // Test FindNodeByPartialAddress functionality and Read/Write of InstanceDataNode
            {
                const AZStd::string testString = "this is a test";
                const float testFloat = 123.0f;
                const int testInt = 7;
                const TestComponent::SubData testSubData(testInt);
                const AZStd::vector<TestComponent::SubData> testNormalContainer{ TestComponent::SubData(1), TestComponent::SubData(2), TestComponent::SubData(3) };

                // create a test component with some initial values
                AZStd::unique_ptr<TestComponent> testComponent(new TestComponent);
                testComponent.get()->m_float = testFloat;
                testComponent.get()->m_string = testString;
                testComponent.get()->m_normalContainer = testNormalContainer;
                testComponent.get()->m_subData.m_int = testInt;

                // create an InstanceDataHierarchy for the test component
                InstanceDataHierarchy idhTestComponent;
                idhTestComponent.AddRootInstance(testComponent.get());
                idhTestComponent.Build(&serializeContext, 0);

                // create some partial addresses to search for fields in InstanceDataHierarchy
                // note: reflection serialization context values are used for lookup (crcs stored)
                //       if a more specific address is required, start from field and work up to structures/components etc
                //       (see addrSubDataInt below as an example)
                InstanceDataNode::Address addrFloat = { AZ_CRC("Float") };
                InstanceDataNode::Address addrString = { AZ_CRC("String") };
                InstanceDataNode::Address addrNormalContainer = { AZ_CRC("NormalContainer") };
                InstanceDataNode::Address addrSubData = { AZ_CRC("SubData") };
                InstanceDataNode::Address addrSubDataInt = { AZ_CRC("Int"), AZ_CRC("SubData") };

                // find InstanceDataNodes using partial address
                InstanceDataNode* foundFloat = idhTestComponent.FindNodeByPartialAddress(addrFloat);
                InstanceDataNode* foundString = idhTestComponent.FindNodeByPartialAddress(addrString);
                InstanceDataNode* foundNormalContainer = idhTestComponent.FindNodeByPartialAddress(addrNormalContainer);
                InstanceDataNode* foundSubData = idhTestComponent.FindNodeByPartialAddress(addrSubData);
                InstanceDataNode* foundSubDataInt = idhTestComponent.FindNodeByPartialAddress(addrSubDataInt);

                // ensure each has been returned successfully
                AZ_TEST_ASSERT(foundFloat);
                AZ_TEST_ASSERT(foundString);
                AZ_TEST_ASSERT(foundNormalContainer);
                AZ_TEST_ASSERT(foundSubData);
                AZ_TEST_ASSERT(foundSubDataInt);

                // check a case where we know the address is incorrect and we will not find an InstanceDataNode
                InstanceDataNode::Address addrInvalid = { AZ_CRC("INVALID") };
                InstanceDataNode* foundInvalid = idhTestComponent.FindNodeByPartialAddress(addrInvalid);
                AZ_TEST_ASSERT(foundInvalid == nullptr);

                ///////////////////////////////////////////////////////////////////////////////

                // test the values read from the InstanceDataNodes are the same as the ones our TestComponent were constructed with
                float readTestFloat;
                foundFloat->Read(readTestFloat);
                AZ_TEST_ASSERT(readTestFloat == testFloat);

                AZStd::string readTestString;
                foundString->Read(readTestString);
                AZ_TEST_ASSERT(readTestString == testString);

                int readTestInt;
                foundSubDataInt->Read(readTestInt);
                AZ_TEST_ASSERT(readTestInt == testInt);

                TestComponent::SubData readTestSubData;
                foundSubData->Read(readTestSubData);
                AZ_TEST_ASSERT(readTestSubData == testSubData);

                AZStd::vector<TestComponent::SubData> readTestNormalContainer;
                foundNormalContainer->Read(readTestNormalContainer);
                AZ_TEST_ASSERT(readTestNormalContainer == testNormalContainer);

                // create some new test values to write to the InstanceDataNode
                const AZStd::string newTestString = "this string has been updated!";
                const float newTestFloat = 456.0f;
                const int newTestInt = 94;
                const TestComponent::SubData newTestSubData(newTestInt);
                const AZStd::vector<TestComponent::SubData> newTestNormalContainer{ TestComponent::SubData(20), TestComponent::SubData(40), TestComponent::SubData(60) };

                // actually write the values to each InstanceDataNode
                foundFloat->Write(newTestFloat);
                foundString->Write(newTestString);
                foundSubData->Write(newTestSubData);
                foundNormalContainer->Write(newTestNormalContainer);

                // read the values back to make sure the are the same as the newly set values
                AZStd::string updatedTestString;
                foundString->Read(updatedTestString);
                AZ_TEST_ASSERT(updatedTestString == newTestString);

                float updatedTestFloat;
                foundFloat->Read(updatedTestFloat);
                AZ_TEST_ASSERT(updatedTestFloat == newTestFloat);

                TestComponent::SubData updatedTestSubData;
                foundSubData->Read(updatedTestSubData);
                AZ_TEST_ASSERT(updatedTestSubData == newTestSubData);

                AZStd::vector<TestComponent::SubData> updatedNormalContainer;
                foundNormalContainer->Read(updatedNormalContainer);
                AZ_TEST_ASSERT(updatedNormalContainer == newTestNormalContainer);
            }
        }
    };

    static AZ::u8 s_persistentIdCounter = 0;

    class InstanceDataHierarchyCopyContainerChangesTest
        : public AllocatorsFixture
    {
    public:

        InstanceDataHierarchyCopyContainerChangesTest()
        {
        }

        ~InstanceDataHierarchyCopyContainerChangesTest() override
        {
        }

        class StructInner
        {
        public:

            AZ_TYPE_INFO(StructInner, "{4BFA2A4F-8568-43AA-941C-8361DBA13CBB}");

            AZ::u8 m_persistentId;
            AZ::u32 m_value;

            StructInner()
            {
                m_value = 1;
                m_persistentId = ++s_persistentIdCounter;
            }

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<StructInner>()->
                    PersistentId([](const void* instance) -> u64 { return static_cast<u64>(reinterpret_cast<const StructInner*>(instance)->m_persistentId); })->
                    Field("Id", &StructInner::m_persistentId)->
                    Field("Value", &StructInner::m_value)
                    ;
            }
        };

        class StructOuter
        {
        public:
            AZ_TYPE_INFO(StructInner, "{FEDCED26-8D5A-41CB-BA97-AB687CF51FC6}");

            AZStd::vector<StructInner> m_vector;

            StructOuter()
            {
            }

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<StructOuter>()->
                    Field("Vector", &StructOuter::m_vector)
                    ;
            }
        };

        void DoCopy(StructOuter& source, StructOuter& target, AZ::SerializeContext& ctx)
        {
            AzToolsFramework::InstanceDataHierarchy sourceHier;
            sourceHier.AddRootInstance(&source, AZ::AzTypeInfo<StructOuter>::Uuid());
            sourceHier.Build(&ctx, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
            AzToolsFramework::InstanceDataHierarchy targetHier;
            targetHier.AddRootInstance(&target, AZ::AzTypeInfo<StructOuter>::Uuid());
            targetHier.Build(&ctx, AZ::SerializeContext::ENUM_ACCESS_FOR_READ);
            AzToolsFramework::InstanceDataHierarchy::CopyInstanceData(&sourceHier, &targetHier, &ctx);
        }

        void VerifyMatch(StructOuter& source, StructOuter& target)
        {
            AZ_TEST_ASSERT(source.m_vector.size() == target.m_vector.size());

            // Make sure that matching elements have the same data (we're using persistent Ids, so order can be whatever).
            for (auto& sourceElement : source.m_vector)
            {
                for (auto& targetElement : target.m_vector)
                {
                    if (targetElement.m_persistentId == sourceElement.m_persistentId)
                    {
                        AZ_TEST_ASSERT(targetElement.m_value == sourceElement.m_value);
                        break;
                    }
                }
            }
        }

        void run()
        {
            using namespace AzToolsFramework;

            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();

            StructInner::Reflect(serializeContext);
            StructOuter::Reflect(serializeContext);

            StructOuter outerSource;
            StructOuter outerTarget;

            StructOuter originalSource;
            originalSource.m_vector.emplace_back();
            originalSource.m_vector.emplace_back();
            originalSource.m_vector.emplace_back();

            {
                outerSource = originalSource;

                DoCopy(outerSource, outerTarget, serializeContext);

                AZ_TEST_ASSERT(outerTarget.m_vector.size() == 3);
            }

            {
                outerSource = originalSource;
                outerTarget = outerSource;

                // Pluck from the start of the array so elements get shifted.
                // Also modify something in the last element so it's written to the target.
                // This verifies that removals are applied safely alongside data changes.
                outerSource.m_vector.erase(outerSource.m_vector.begin());
                outerSource.m_vector.begin()->m_value = 2;

                DoCopy(outerSource, outerTarget, serializeContext);

                VerifyMatch(outerSource, outerTarget);
            }

            {
                outerSource = originalSource;
                outerTarget = outerSource;

                // Remove an element from the target and SHRINK the array to fit so it's
                // guaranteed to grow when the missing element is copied from the source.
                // This verifies that additions are being applied safely alongside data changes.
                outerTarget.m_vector.erase(outerTarget.m_vector.begin());
                outerTarget.m_vector.set_capacity(outerTarget.m_vector.size()); // Force grow on insert
                outerSource.m_vector.back().m_value = 5;

                DoCopy(outerSource, outerTarget, serializeContext);

                VerifyMatch(outerSource, outerTarget);
            }

            {
                outerSource = originalSource;
                outerTarget = outerSource;

                // Add elements to the source.
                // Add an element to the target.
                // Change a different element.
                // This tests removals, additions, and changes occurring together, with net growth in the target container.
                outerSource.m_vector.emplace_back();
                outerSource.m_vector.emplace_back();
                outerTarget.m_vector.emplace_back();
                outerTarget.m_vector.set_capacity(outerTarget.m_vector.size()); // Force grow on insert
                outerTarget.m_vector.begin()->m_value = 10;

                DoCopy(outerSource, outerTarget, serializeContext);

                VerifyMatch(outerSource, outerTarget);
            }
        }
    };

    enum class TestEnum
    {
        Value1 = 0x01,
        Value2 = 0x02,
        Value3 = 0xFF,
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(UnitTest::TestEnum, "{52DBDCC6-0829-4602-A650-E6FC32AFC5F2}");
}

namespace UnitTest
{
    class InstanceDataHierarchyEnumContainerTest
        : public AllocatorsFixture
    {
    public:
        class EnumContainer
        {
        public:
            AZ_TYPE_INFO(EnumContainer, "{7F9EED53-7587-4616-B4A7-10B3AF95475E}");
            AZ_CLASS_ALLOCATOR(EnumContainer, AZ::SystemAllocator, 0);

            TestEnum m_enum;
            AZStd::vector<TestEnum> m_enumVector;

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<EnumContainer>()
                    ->Field("Enum", &EnumContainer::m_enum)
                    ->Field("EnumVector", &EnumContainer::m_enumVector)
                    ;

                if (EditContext* edit = context.GetEditContext())
                {
                    edit->Enum<UnitTest::TestEnum>("TestEnum", "No Description")
                        ->Value("Value1", UnitTest::TestEnum::Value1)
                        ->Value("Value2", UnitTest::TestEnum::Value2)
                        ->Value("Value3", UnitTest::TestEnum::Value3)
                        ;

                    edit->Class<EnumContainer>("Enum Container", "Test container that has an external enum")
                        ->DataElement(nullptr, &EnumContainer::m_enum, "Enum Field", "An enum value")
                        ->DataElement(nullptr, &EnumContainer::m_enumVector, "Enum Vector Field", "A vector of enum values")
                        ;
                }
            }
        };

        void run()
        {
            using namespace AzToolsFramework;

            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();
            EnumContainer::Reflect(serializeContext);

            EnumContainer ec;
            ec.m_enumVector.emplace_back(UnitTest::TestEnum::Value3);

            InstanceDataHierarchy idh;
            idh.AddRootInstance(&ec, azrtti_typeid<EnumContainer>());
            idh.Build(&serializeContext, 0);

            InstanceDataNode* enumNode = idh.FindNodeByPartialAddress({ AZ_CRC("Enum") });
            InstanceDataNode* enumVectorNode = idh.FindNodeByPartialAddress({ AZ_CRC("EnumVector") });

            ASSERT_NE(enumNode, nullptr);
            ASSERT_NE(enumVectorNode, nullptr);

            auto getEnumData = [&ec](const AzToolsFramework::InstanceDataNode& node) -> Uuid
            {
                Uuid id = Uuid::CreateNull();
                auto attribute = node.GetElementMetadata()->FindAttribute(AZ_CRC("EnumType"));
                auto attributeData = azrtti_cast<AttributeData<AZ::TypeId>*>(attribute);
                if (attributeData)
                {
                    id = attributeData->Get(&ec);
                }
                return id;
            };
            EXPECT_EQ(getEnumData(*enumNode), RttiTypeId<UnitTest::TestEnum>());

            const auto& vectorEntries = enumVectorNode->GetChildren();
            ASSERT_EQ(vectorEntries.size(), 1);
            EXPECT_EQ(getEnumData(*vectorEntries.begin()), RttiTypeId<UnitTest::TestEnum>());
        }

    };

    class GroupTestComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(GroupTestComponent, "{C088C81D-D59D-43F1-85F8-B2E591BABA36}")

        GroupTestComponent() = default;

        struct SubData
        {
            AZ_TYPE_INFO(SubData, "{983316B5-17C0-476E-9CEB-CA749B3ABE5D}");
            AZ_CLASS_ALLOCATOR(SubData, AZ::SystemAllocator, 0);

            SubData() {}
            explicit SubData(int v) : m_int(v) {}
            explicit SubData(bool b) : m_bool(b) {}
            explicit SubData(float f) : m_float(f) {}
            ~SubData() = default;

            float m_float = 0.f;
            int m_int = 0;
            bool m_bool = true;
        };

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SubData>()
                    ->Version(1)
                    ->Field("SubInt", &SubData::m_int)
                    ->Field("SubToggle", &SubData::m_bool)
                    ->Field("SubFloat", &SubData::m_float)
                    ;

                serializeContext->Class<GroupTestComponent, AZ::Component>()
                    ->Version(1)
                    ->Field("Float", &GroupTestComponent::m_float)
                    ->Field("GroupToggle", &GroupTestComponent::m_groupToggle)
                    ->Field("GroupFloat", &GroupTestComponent::m_groupFloat)
                    ->Field("ToggleGroupInt", &GroupTestComponent::m_toggleGroupInt)
                    ->Field("SubDataNormal", &GroupTestComponent::m_subGroupForNormal)
                    ->Field("SubDataToggle", &GroupTestComponent::m_subGroupForToggle)
                    ;

                if (AZ::EditContext* edit = serializeContext->GetEditContext())
                {
                    edit->Class<GroupTestComponent>("Group Test Component", "Testing normal groups and toggle groups")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->DataElement(nullptr, &GroupTestComponent::m_float, "Float Field", "A float field")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Normal Group")
                        ->DataElement(nullptr, &GroupTestComponent::m_groupFloat, "Float Field", "A float field")
                        ->DataElement(nullptr, &GroupTestComponent::m_subGroupForNormal, "Struct Field", "A sub data type")
                        ->GroupElementToggle("Group Toggle", &GroupTestComponent::m_groupToggle)
                        ->DataElement(nullptr, &GroupTestComponent::m_toggleGroupInt, "Normal Integer", "An Integer")
                        ->DataElement(nullptr, &GroupTestComponent::m_subGroupForToggle, "Struct Field", "A sub data type")
                        ;

                    edit->Class<SubData>("SubGroup Test Component", "Testing nested normal groups and toggle groups")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->ClassElement(AZ::Edit::ClassElements::Group, "Normal SubGroup")
                        ->DataElement(nullptr, &SubData::m_int, "SubGroup Int Field", "An int")
                        ->GroupElementToggle("SubGroup Toggle", &SubData::m_bool)
                        ->DataElement(nullptr, &SubData::m_float, "SubGroup Float Field", "An int")
                        ;
                }
            }
        }

        void Activate() override
        {
        }

        void Deactivate() override
        {
        }

        float m_float = 0.f;
        float m_groupFloat = 0.f;
        int m_toggleGroupInt = 0;
        AZStd::string m_string;
        bool m_groupToggle = false;

        SubData m_subGroupForNormal;
        SubData m_subGroupForToggle;
    };

    class InstanceDataHierarchyGroupTestFixture : public AllocatorsFixture
    {
    public:
        InstanceDataHierarchyGroupTestFixture() = default;

        AZStd::unique_ptr<SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::Entity> testEntity1;
        AzToolsFramework::InstanceDataHierarchy* instanceDataHierarchy;
        AzToolsFramework::InstanceDataNode* componentNode1 = nullptr;

        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            using AzToolsFramework::InstanceDataHierarchy;
            using AzToolsFramework::InstanceDataNode;

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();

            m_serializeContext.reset(aznew AZ::SerializeContext());
            m_serializeContext.get()->CreateEditContext();
            Entity::Reflect(m_serializeContext.get());
            GroupTestComponent::Reflect(m_serializeContext.get());

            testEntity1.reset(new AZ::Entity());
            testEntity1->CreateComponent<GroupTestComponent>();

            instanceDataHierarchy = aznew InstanceDataHierarchy();
            instanceDataHierarchy->AddRootInstance(testEntity1.get());
            instanceDataHierarchy->Build(m_serializeContext.get(), 0);

            // Adding the nodes to a node stack
            auto rootNode = instanceDataHierarchy->GetRootNode();
            AZStd::stack<InstanceDataNode*> nodeStack;
            nodeStack.push(rootNode);
            while (!nodeStack.empty())
            {
                InstanceDataNode* node = nodeStack.top();
                nodeStack.pop();
                if (node->GetClassMetadata()->m_typeId == AZ::AzTypeInfo<GroupTestComponent>::Uuid())
                {
                    componentNode1 = node;
                    break;
                }
                for (InstanceDataNode& child : node->GetChildren())
                {
                    nodeStack.push(&child);
                }
            }
        }

         void TearDown() override
        {
            m_serializeContext.reset();
            testEntity1.reset();
            delete instanceDataHierarchy;
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
            AllocatorsFixture::TearDown();
        }
    };

    class InstanceDataHierarchyKeyedContainerTest
        : public AllocatorsFixture
    {
    public:
        class CustomKeyWithoutStringRepresentation
        {
        public:
            AZ_TYPE_INFO(CustomKeyWithoutStringRepresentation, "{54E838DE-1A8D-4BBA-BD3A-D41886C439A9}");
            AZ_CLASS_ALLOCATOR(CustomKeyWithoutStringRepresentation, AZ::SystemAllocator, 0);

            int m_value = 0;

            int operator<(const CustomKeyWithoutStringRepresentation& other) const
            {
                return m_value < other.m_value;
            }
        };

        class CustomKeyWithStringRepresentation
        {
        public:
            AZ_TYPE_INFO(CustomKeyWithStringRepresentation, "{51F7FB74-2991-4CC9-850A-8D5AA0732282}");
            AZ_CLASS_ALLOCATOR(CustomKeyWithStringRepresentation, AZ::SystemAllocator, 0);

            static const char* KeyPrefix() { return "CustomKey"; }

            int m_value = 0;

            int operator<(const CustomKeyWithStringRepresentation& other) const
            {
                return m_value < other.m_value;
            }

            AZStd::string ToString() const
            {
                return AZStd::string::format("%s %i", KeyPrefix(), m_value);
            }
        };

        class KeyedContainer
        {

        public:
            AZ_TYPE_INFO(KeyedContainer, "{53A7416F-2D84-4256-97B0-BE4B6EF6DBAF}");
            AZ_CLASS_ALLOCATOR(KeyedContainer, AZ::SystemAllocator, 0);

            AZStd::map<AZStd::string, float> m_map;
            AZStd::unordered_map<AZStd::pair<int, double>, int> m_unorderedMap;
            AZStd::set<int> m_set;
            AZStd::unordered_set<AZ::u64> m_unorderedSet;
            AZStd::unordered_multimap<int, AZStd::string> m_multiMap;
            AZStd::unordered_map<int, AZStd::unordered_map<int, int>> m_nestedMap;
            AZStd::map<CustomKeyWithoutStringRepresentation, int> m_uncollapsableMap;
            AZStd::map<CustomKeyWithStringRepresentation, int> m_collapsableMap;

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<CustomKeyWithoutStringRepresentation>()
                    ->Field("value", &CustomKeyWithoutStringRepresentation::m_value);

                context.Class<CustomKeyWithStringRepresentation>()
                    ->Field("value", &CustomKeyWithStringRepresentation::m_value);

                context.Class<KeyedContainer>()
                    ->Field("map", &KeyedContainer::m_map)
                    ->Field("unorderedMap", &KeyedContainer::m_unorderedMap)
                    ->Field("set", &KeyedContainer::m_set)
                    ->Field("unorderedSet", &KeyedContainer::m_unorderedSet)
                    ->Field("multiMap", &KeyedContainer::m_multiMap)
                    ->Field("nestedMap", &KeyedContainer::m_nestedMap)
                    ->Field("uncollapsableMap", &KeyedContainer::m_uncollapsableMap)
                    ->Field("collapsableMap", &KeyedContainer::m_collapsableMap);

                if (auto editContext = context.GetEditContext())
                {
                    editContext->Class<CustomKeyWithStringRepresentation>("CustomKeyWithStringRepresentation", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::ConciseEditorStringRepresentation, &CustomKeyWithStringRepresentation::ToString);
                }
            }
        };

        struct KeyTestData
        {
            virtual void InsertAndVerifyKeys(AZ::SerializeContext::IDataContainer* container, void* key, void* instance, const AZ::SerializeContext::ClassElement* classElement) const = 0;
            virtual AZ::Uuid ExpectedKeyType() const = 0;
            virtual size_t NumberOfKeys() const = 0;
            virtual ~KeyTestData() {}
        };

        template <class T>
        struct TypedKeyTestData : public KeyTestData
        {
            AZStd::vector<T> keysToInsert;
            TypedKeyTestData(std::initializer_list<T> keys)
                : keysToInsert(keys)
            {
            }

            void InsertAndVerifyKeys(AZ::SerializeContext::IDataContainer* container, void* key, void* instance, const AZ::SerializeContext::ClassElement* classElement) const override
            {
                T* keyContainer = reinterpret_cast<T*>(key);
                for (const T& keyToInsert : keysToInsert)
                {
                    *keyContainer = keyToInsert;
                    void* element = container->ReserveElement(instance, classElement);
                    auto associativeInterface = container->GetAssociativeContainerInterface();
                    associativeInterface->SetElementKey(element, key);
                    container->StoreElement(instance, element);
                    auto lookupKey = associativeInterface->GetElementByKey(instance, classElement, (void*)(&keyToInsert));
                    EXPECT_NE(lookupKey, nullptr);
                }
            }

            AZ::Uuid ExpectedKeyType() const override
            {
                return azrtti_typeid<AZ::Internal::RValueToLValueWrapper<T>>();
            }

            size_t NumberOfKeys() const override
            {
                return keysToInsert.size();
            }

            static AZStd::unique_ptr<TypedKeyTestData<T>> Create(std::initializer_list<T> keys)
            {
                return AZStd::make_unique<TypedKeyTestData<T>>(keys);
            }
        };

        void run()
        {
            using namespace AzToolsFramework;

            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();
            KeyedContainer::Reflect(serializeContext);

            KeyedContainer kc;

            InstanceDataHierarchy idh;
            idh.AddRootInstance(&kc, azrtti_typeid<KeyedContainer>());
            idh.Build(&serializeContext, 0);

            AZStd::unordered_map<AZ::u32, AZStd::unique_ptr<KeyTestData>> keyTestData;
            keyTestData[AZ_CRC("map")] = TypedKeyTestData<AZStd::string>::Create({"A", "B", "lorem ipsum"});
            keyTestData[AZ_CRC("unorderedMap")] = TypedKeyTestData<AZStd::pair<int, double>>::Create({ {5, 1.0}, {5, -2.0} });
            keyTestData[AZ_CRC("set")] = TypedKeyTestData<int>::Create({2, 4, -255, 999});
            keyTestData[AZ_CRC("unorderedSet")] = TypedKeyTestData<AZ::u64>::Create({500000, 9, 0, 42, 42});
            keyTestData[AZ_CRC("multiMap")] = TypedKeyTestData<int>::Create({-1, 2, -3, 4, -5, 6});
            keyTestData[AZ_CRC("nestedMap")] = TypedKeyTestData<int>::Create({1, 10, 100, 1000});
            keyTestData[AZ_CRC("uncollapsableMap")] = TypedKeyTestData<CustomKeyWithoutStringRepresentation>::Create({{0}, {1}});
            keyTestData[AZ_CRC("collapsableMap")] = TypedKeyTestData<CustomKeyWithStringRepresentation>::Create({{0}, {1}});

            auto insertKeysIntoContainer = [&serializeContext](AzToolsFramework::InstanceDataNode& node, KeyTestData* keysToInsert)
            {
                const AZ::SerializeContext::ClassElement* element = node.GetElementMetadata();
                AZ::SerializeContext::IDataContainer* container = node.GetClassMetadata()->m_container;

                ASSERT_NE(element, nullptr);
                ASSERT_NE(container, nullptr);

                const AZ::SerializeContext::ClassElement* containerClassElement = container->GetElement(container->GetDefaultElementNameCrc());
                auto associativeInterface = container->GetAssociativeContainerInterface();
                ASSERT_NE(associativeInterface, nullptr);
                auto key = associativeInterface->CreateKey();

                auto attribute = containerClassElement ->FindAttribute(AZ_CRC("KeyType"));
                auto attributeData = azrtti_cast<AttributeData<AZ::TypeId>*>(attribute);
                ASSERT_NE(attributeData, nullptr);
                auto keyId = attributeData->Get(node.FirstInstance());
                ASSERT_EQ(keyId, keysToInsert->ExpectedKeyType());

                // Ensure we can build an InstanceDataHierarchy at runtime from the container's KeyType
                InstanceDataHierarchy idh2;
                idh2.AddRootInstance(key.get(), keyId);
                idh2.Build(&serializeContext, 0);
                auto children = idh2.GetChildren();
                EXPECT_EQ(children.size(), 1);

                keysToInsert->InsertAndVerifyKeys(container, key.get(), node.FirstInstance(), element);
            };

            for (InstanceDataNode& node : idh.GetChildren())
            {
                const AZ::SerializeContext::ClassElement* element = node.GetElementMetadata();
                auto insertIterator = keyTestData.find(element->m_nameCrc);
                ASSERT_NE(insertIterator, keyTestData.end());
                auto keysToInsert = insertIterator->second.get();

                insertKeysIntoContainer(node, keysToInsert);
            }

            auto nestedKeys = TypedKeyTestData<int>::Create({2, 4, 8, 16});
            idh.Build(&serializeContext, 0);
            for (InstanceDataNode& node : idh.GetChildren())
            {
                const AZ::SerializeContext::ClassElement* element = node.GetElementMetadata();
                if (element->m_nameCrc == AZ_CRC("nestedMap"))
                {
                    auto children = node.GetChildren();
                    // We should have entries for each inserted key in the nested map
                    EXPECT_EQ(children.size(), keyTestData[AZ_CRC("nestedMap")]->NumberOfKeys());
                    for (AzToolsFramework::InstanceDataNode& child : children)
                    {
                        insertKeysIntoContainer(child.GetChildren().back(), nestedKeys.get());
                    }
                }
                else if (element->m_nameCrc == AZ_CRC("collapsableMap"))
                {
                    auto children = node.GetChildren();
                    EXPECT_GT(children.size(), 0);
                    for (AzToolsFramework::InstanceDataNode& child : children)
                    {
                        // Ensure we're getting keys with the correct prefix based on the ConciseEditorStringRepresentation
                        AZStd::string name = child.GetElementEditMetadata()->m_name;
                        EXPECT_NE(name.find(CustomKeyWithStringRepresentation::KeyPrefix()), AZStd::string::npos);
                    }
                }
                else if (element->m_nameCrc == AZ_CRC("uncollapsableMap"))
                {
                    auto children = node.GetChildren();
                    EXPECT_GT(children.size(), 0);
                    for (AzToolsFramework::InstanceDataNode& child : children)
                    {
                        auto keyValueChildren = child.GetChildren();
                        EXPECT_EQ(keyValueChildren.size(), 2);
                        auto keyValueChildrenIterator = keyValueChildren.begin();
                        auto keyNode = *keyValueChildrenIterator;
                        ++keyValueChildrenIterator;
                        auto valueNode = *keyValueChildrenIterator;

                        // Ensure key/value pairs that can't be collapsed get labels based on type
                        EXPECT_EQ(AZ::Crc32(keyNode.GetElementEditMetadata()->m_name), AZ_CRC("Key<CustomKeyWithoutStringRepresentation>"));
                        EXPECT_EQ(AZ::Crc32(valueNode.GetElementEditMetadata()->m_name), AZ_CRC("Value<int>"));
                    }
                }
            }

            // Ensure IgnoreKeyValuePairs is respected
            idh.SetBuildFlags(InstanceDataHierarchy::Flags::IgnoreKeyValuePairs);
            idh.Build(&serializeContext, 0);
            for (InstanceDataNode& node : idh.GetChildren())
            {
                const AZ::SerializeContext::ClassElement* element = node.GetElementMetadata();
                if (element->m_nameCrc == AZ_CRC("map") || element->m_nameCrc == AZ_CRC("unorderedMap") || element->m_nameCrc == AZ_CRC("nestedMap"))
                {
                    for (InstanceDataNode& pair : node.GetChildren())
                    {
                        EXPECT_EQ(pair.GetChildren().size(), 2);
                    }
                }
            }
        }
    };

    class InstanceDataHierarchyCompareAssociativeContainerTest
        : public AllocatorsFixture
    {
    public:
        class Container
        {
        public:
            AZ_TYPE_INFO(Container, "{9920B5BD-F21C-4353-9449-9C3FD38E50FC}");
            AZ_CLASS_ALLOCATOR(Container, AZ::SystemAllocator, 0);

            AZStd::unordered_map<AZStd::string, int> m_map;

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<Container>()
                    ->Field("map", &Container::m_map);
            }
        };

        void run()
        {
            using namespace AzToolsFramework;

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();

            AZ::SerializeContext serializeContext;
            Container::Reflect(serializeContext);

            Container c1;
            c1.m_map = {
                {"A", 1},
                {"B", 2},
                {"C", 3}
            };

            Container c2;
            c2.m_map = {
                {"C", 1},
                {"A", 2},
                {"B", 3}
            };

            Container c3;
            c3.m_map = {
                {"A", 2},
                {"D", 3}
            };

            auto testComparison = [&](Container& baseInstance, 
                Container& compareInstance, 
                AZStd::unordered_set<AZStd::string> expectedAdds,
                AZStd::unordered_set<AZStd::string> expectedRemoves,
                AZStd::unordered_set<AZStd::string> expectedChanges)
            {
                InstanceDataHierarchy idhBase;
                idhBase.AddRootInstance(&baseInstance, azrtti_typeid<Container>());
                idhBase.Build(&serializeContext, 0);

                InstanceDataHierarchy idhCompare;
                idhCompare.AddRootInstance(&compareInstance, azrtti_typeid<Container>());
                idhCompare.Build(&serializeContext, 0);

                AZStd::unordered_set<AZStd::string> actualAdds;
                AZStd::unordered_set<AZStd::string> actualRemoves;
                AZStd::unordered_set<AZStd::string> actualChanges;

                auto newNodeCB = [&](InstanceDataNode* newNode, AZStd::vector<AZ::u8>&)
                {
                    actualAdds.insert(newNode->GetElementEditMetadata()->m_name);
                };

                auto removedNodeCB = [&](const InstanceDataNode* sourceNode, InstanceDataNode*)
                {
                    actualRemoves.insert(sourceNode->GetElementEditMetadata()->m_name);
                };

                auto changedNodeCB = [&](const InstanceDataNode* sourceNode, const InstanceDataNode*, AZStd::vector<AZ::u8>&, AZStd::vector<AZ::u8>&)
                {
                    actualChanges.insert(sourceNode->GetParent()->GetElementEditMetadata()->m_name);
                };

                InstanceDataHierarchy::CompareHierarchies(&idhBase, 
                    &idhCompare, 
                    &InstanceDataHierarchy::DefaultValueComparisonFunction,
                    &serializeContext,
                    newNodeCB,
                    removedNodeCB,
                    changedNodeCB
                );

                EXPECT_EQ(expectedAdds, actualAdds);
                EXPECT_EQ(expectedRemoves, actualRemoves);
                EXPECT_EQ(expectedChanges, actualChanges);
            };

            Container cCopy = c1;
            testComparison(c1, cCopy, {}, {}, {});
            testComparison(c1, c3, {"D", "[0]", "[1]"}, {"B", "C"}, {"A"});
            testComparison(c3, c1, {"B", "C", "[0]", "[1]"}, {"D"}, {"A"});
            testComparison(c1, c2, {}, {}, {"A", "B", "C"});

            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        }
    };


    class InstanceDataHierarchyElementTest
        : public AllocatorsFixture
    {
    public:
        class UIElementContainer
        {
        public:
            AZ_TYPE_INFO(UIElementContainer, "{83B7BDFD-8B60-4C52-B7C5-BF3C824620F5}");
            AZ_CLASS_ALLOCATOR(UIElementContainer, AZ::SystemAllocator, 0);

            int m_data;

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<UIElementContainer>()
                    ->Field("data", &UIElementContainer::m_data);

                if (auto editContext = context.GetEditContext())
                {
                    editContext->Class<UIElementContainer>("Test", "")
                        ->UIElement("TestHandler", "UIElement")
                        ->DataElement(nullptr, &UIElementContainer::m_data)
                        ->UIElement(AZ_CRC("TestHandler2"), "UIElement2")
                    ;
                }
            }
        };

        void run()
        {
            using namespace AzToolsFramework;

            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();
            UIElementContainer::Reflect(serializeContext);

            UIElementContainer test;
            InstanceDataHierarchy idh;
            idh.AddRootInstance(&test, azrtti_typeid<UIElementContainer>());
            idh.Build(&serializeContext, 0);

            auto children = idh.GetChildren();
            ASSERT_EQ(children.size(), 3);
            auto it = children.begin();

            Crc32 uiHandler = 0;
            EXPECT_EQ(it->ReadAttribute(AZ::Edit::UIHandlers::Handler, uiHandler), true);
            EXPECT_EQ(uiHandler, AZ_CRC("TestHandler"));
            EXPECT_STREQ(it->GetElementMetadata()->m_name, "UIElement");
            EXPECT_EQ(it->GetElementMetadata()->m_nameCrc, AZ_CRC("UIElement"));

            uiHandler = 0;
            ++it;
            ++it;
            EXPECT_EQ(it->ReadAttribute(AZ::Edit::UIHandlers::Handler, uiHandler), true);
            EXPECT_EQ(uiHandler, AZ_CRC("TestHandler2"));
            EXPECT_STREQ(it->GetElementMetadata()->m_name, "UIElement2");
            EXPECT_EQ(it->GetElementMetadata()->m_nameCrc, AZ_CRC("UIElement2"));
        }
    };

    class InstanceDataHierarchyAggregateInstanceTest
        : public AllocatorsFixture
    {
    public:
        class AggregatedContainer
        {
        public:
            AZ_TYPE_INFO(AggregatedContainer, "{42E09F38-2D26-4FED-9901-06003A030ED5}");
            AZ_CLASS_ALLOCATOR(AggregatedContainer, AZ::SystemAllocator, 0);

            int m_aggregated;
            int m_notAggregated;

            static void Reflect(AZ::SerializeContext& context)
            {
                context.Class<AggregatedContainer>()
                    ->Field("aggregatedDataElement", &AggregatedContainer::m_aggregated)
                    ->Field("notAggregatedDataElement", &AggregatedContainer::m_notAggregated)
                ;

                if (auto editContext = context.GetEditContext())
                {
                    // By default, DataElements accept multi-edit and UIElements do not
                    editContext->Class<AggregatedContainer>("Test", "")
                        ->DataElement(nullptr, &AggregatedContainer::m_aggregated)
                        ->DataElement(nullptr, &AggregatedContainer::m_notAggregated)
                            ->Attribute(AZ::Edit::Attributes::AcceptsMultiEdit, false)
                        ->UIElement("TestHandler", "aggregatedUIElement")
                            ->Attribute(AZ::Edit::Attributes::AcceptsMultiEdit, true)
                        ->UIElement(AZ_CRC("TestHandler2"), "notAggregatedUIElement")
                    ;
                }
            }
        };

        void run()
        {
            using namespace AzToolsFramework;

            AZ::SerializeContext serializeContext;
            serializeContext.CreateEditContext();
            AggregatedContainer::Reflect(serializeContext);

            InstanceDataHierarchy idh;
            AZStd::list<AggregatedContainer> containers;
            for (int i = 0; i < 5; ++i)
            {
                containers.push_back();
                AggregatedContainer& container = containers.back();
                idh.AddRootInstance(&container, azrtti_typeid<AggregatedContainer>());
                idh.Build(&serializeContext, 0);

                auto children = idh.GetChildren();
                // If we have multiple instances, the two non-aggregating elements should go away
                ASSERT_EQ(children.size(), i == 0 ? 4 : 2);

                auto it = children.begin();

                EXPECT_STREQ(it->GetElementMetadata()->m_name, "aggregatedDataElement");
                ++it;

                if (i == 0)
                {
                    EXPECT_STREQ(it->GetElementMetadata()->m_name, "notAggregatedDataElement");
                    ++it;
                }

                EXPECT_STREQ(it->GetElementMetadata()->m_name, "aggregatedUIElement");
                ++it;

                if (i == 0)
                {
                    EXPECT_STREQ(it->GetElementMetadata()->m_name, "notAggregatedUIElement");
                    ++it;
                }
            }
        }
    };

    TEST_F(InstanceDataHierarchyBasicTest, Test)
    {
        run();
    }

    TEST_F(InstanceDataHierarchyCopyContainerChangesTest, Test)
    {
        run();
    }

    TEST_F(InstanceDataHierarchyEnumContainerTest, Test)
    {
        run();
    }

    TEST_F(InstanceDataHierarchyKeyedContainerTest, Test)
    {
        run();
    }

    TEST_F(InstanceDataHierarchyKeyedContainerTest, RemovingMultipleItemsFromContainerDoesNotCrash)
    {
        using TestMap = AZStd::unordered_map<double, double>;
        TestMap testMap;
        AZStd::initializer_list<AZStd::pair<double, double>> valuesToInsert{ {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0} };

        AZ::GenericClassInfo* mapGenericClassInfo = AZ::SerializeGenericTypeInfo<TestMap>::GetGenericInfo();
        AZ::SerializeContext::ClassData* mapClassData = mapGenericClassInfo->GetClassData();
        ASSERT_NE(nullptr, mapClassData);
        AZ::SerializeContext::IDataContainer* mapDataContainer = mapClassData->m_container;
        ASSERT_NE(nullptr, mapDataContainer);
        auto associativeInterface = mapDataContainer->GetAssociativeContainerInterface();

        AZ::SerializeContext::ClassElement classElement;
        AZ::SerializeContext::DataElement dataElement;
        dataElement.m_nameCrc = mapDataContainer->GetDefaultElementNameCrc();
        EXPECT_TRUE(mapDataContainer->GetElement(classElement, dataElement));

        AZStd::vector<double> keyRemovalContainer;
        keyRemovalContainer.reserve(valuesToInsert.size());
        for (const AZStd::pair<double, double>& valueToInsert : valuesToInsert)
        {   
            void* newElement = mapDataContainer->ReserveElement(&testMap, &classElement);
            *reinterpret_cast<typename TestMap::value_type*>(newElement) = valueToInsert;
            mapDataContainer->StoreElement(&testMap, newElement);
            keyRemovalContainer.push_back(valueToInsert.first);
        }

        EXPECT_EQ(valuesToInsert.size(), testMap.size());
        for (const AZStd::pair<double, double>& testValue : valuesToInsert)
        {
            // Make sure all elements within initializer_list is in the map
            void* lookupValue = associativeInterface->GetElementByKey(&testMap, &classElement, &testValue.first);
            EXPECT_NE(nullptr, lookupValue);

        }
        
        // Shuffle the keys around and attempt to remove the keys using IDataContainer::RemoveElement
        SerializeContext serializeContext;
        const uint32_t rngSeed = std::random_device{}();
        std::mt19937 mtTwisterRng(rngSeed);
        std::shuffle(keyRemovalContainer.begin(), keyRemovalContainer.end(), mtTwisterRng);
        for (double key : keyRemovalContainer)
        {
            void* valueToRemove = associativeInterface->GetElementByKey(&testMap, &classElement, &key);
            EXPECT_TRUE(mapDataContainer->RemoveElement(&testMap, valueToRemove, &serializeContext));
        }

        EXPECT_EQ(0, mapDataContainer->Size(&testMap));
    }

    TEST_F(InstanceDataHierarchyCompareAssociativeContainerTest, TestComparingAssociativeContainers)
    {
        run();
    }

    TEST_F(InstanceDataHierarchyElementTest, TestLayingOutUIAndDataElements)
    {
        run();
    }

    TEST_F(InstanceDataHierarchyAggregateInstanceTest, TestRespectingAggregateInstanceVisibility)
    {
        run();
    }

    // Test to validate that the only ClassElement::Group nodes are ToggleGroups
    TEST_F(InstanceDataHierarchyGroupTestFixture, GroupToggleIsClassElementGroup)
    {
        using AzToolsFramework::InstanceDataHierarchy;
        using AzToolsFramework::InstanceDataNode;

        for (auto child : componentNode1->GetChildren())
        {
            AZStd::string childName(child.GetElementMetadata()->m_name);
            if (childName.compare("GroupToggle") == 0)
            {
                EXPECT_EQ(child.GetElementEditMetadata()->m_elementId, AZ::Edit::ClassElements::Group);
            }
            if ((childName.compare("SubDataNormal") == 0) || (childName.compare("SubDataToggle") == 0))
            {
                for (auto subChild : child.GetChildren())
                {
                    childName = subChild.GetElementMetadata()->m_name;
                    if (childName.compare("SubToggle") == 0)
                    {
                        EXPECT_EQ(subChild.GetElementEditMetadata()->m_elementId, AZ::Edit::ClassElements::Group);
                    }
                    else
                    {
                        EXPECT_NE(subChild.GetElementEditMetadata()->m_elementId, AZ::Edit::ClassElements::Group);
                    }
                }
            }
        }
    }

    // Test to ensure that each node has been assigned under the proper group and the group hierarchy is structured correctly
    TEST_F(InstanceDataHierarchyGroupTestFixture, ValidatingGroupAndSubGroupHierarchy)
    {
        using AzToolsFramework::InstanceDataHierarchy;
        using AzToolsFramework::InstanceDataNode;

        for (auto child : componentNode1->GetChildren())
        {
            AZStd::string childName(child.GetElementMetadata()->m_name);
            if (childName.compare("GroupFloat") == 0)
            {
                EXPECT_STREQ(child.GetGroupElementMetadata()->m_description, "Normal Group");
            }
            if (childName.compare("ToggleGroupInt") == 0)
            {
                EXPECT_STREQ(child.GetGroupElementMetadata()->m_description, "Group Toggle");
            }
            if ((childName.compare("SubDataNormal") == 0) || (childName.compare("SubDataToggle") == 0))
            {
                for (auto subChild : child.GetChildren())
                {
                    childName = subChild.GetElementMetadata()->m_name;
                    if (childName.compare("SubInt") == 0)
                    {
                        EXPECT_STREQ(subChild.GetGroupElementMetadata()->m_description, "Normal SubGroup");
                    }
                    if (childName.compare("SubFloat") == 0)
                    {
                        EXPECT_STREQ(subChild.GetGroupElementMetadata()->m_description, "SubGroup Toggle");
                    }
                }
            }
        }
    }

    class InstanceDataHierarchyGroupTestFixtureParameterized
        : public InstanceDataHierarchyGroupTestFixture
        , public ::testing::WithParamInterface<const char*>
    {
    };

    INSTANTIATE_TEST_CASE_P(
        InstanceDataHierarchyGroupTestFixture,
        InstanceDataHierarchyGroupTestFixtureParameterized,
        ::testing::Values("GroupFloat", "GroupToggle", "ToggleGroupInt", "SubInt", "SubToggle", "SubFloat"));

    // Test to validate that each node in a group and Subgroup has the correct parent
    TEST_P(InstanceDataHierarchyGroupTestFixtureParameterized, ValidatingGroupAndSubGroupParents)
    {
        using AzToolsFramework::InstanceDataHierarchy;
        using AzToolsFramework::InstanceDataNode;

        const char* paramName = GetParam();
        for (auto child : componentNode1->GetChildren())
        {
            AZStd::string childName(child.GetElementMetadata()->m_name);
            if (childName.compare(paramName) == 0)
            {
                EXPECT_STREQ(child.GetParent()->GetClassMetadata()->m_name, "GroupTestComponent");
            }
            if ((childName.compare("SubDataNormal") == 0) || (childName.compare("SubDataToggle") == 0))
            {
                for (auto subChild : child.GetChildren())
                {
                    childName = subChild.GetElementMetadata()->m_name;
                    if (childName.compare(paramName) == 0)
                    {
                        EXPECT_STREQ(subChild.GetParent()->GetClassMetadata()->m_name, "SubData");
                    }
                }
            }
        }
    }
}   // namespace UnitTest
