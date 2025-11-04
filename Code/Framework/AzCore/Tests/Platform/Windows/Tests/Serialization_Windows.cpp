/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class SerializationFixture
        : public LeakDetectionFixture
    {
    public:

        void SetUp() override
        {
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        }

        void TearDown() override
        {
            m_serializeContext.reset();
        }

    protected:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    };

    TEST_F(SerializationFixture, CallingDeletePointerDataOnIDataContainerObject_DoesNotAccessInvalidMemory)
    {
        struct TestDataContainer
            : AZ::SerializeContext::IDataContainer
        {
            TestDataContainer(uint32_t* testInt)
                : m_testInt(testInt)
            {
                // Create SerializeContext ClassElement for a uint32_t type that is not a pointer
                m_classElement.m_name = "Test";
                m_classElement.m_nameCrc = AZ_CRC_CE("Test");
                m_classElement.m_typeId = azrtti_typeid<uint32_t>();
                m_classElement.m_dataSize = sizeof(uint32_t);
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
            void EnumElements(void*, const ElementCB&) override
            {}
            void EnumTypes(const ElementTypeCB&) override
            {}
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
            bool RemoveElement(void*, const void*, AZ::SerializeContext*  serializeContext) override
            {
                DeletePointerData(serializeContext, &m_classElement, m_testInt);
                return {};
            }
            size_t RemoveElements(void*, const void**, size_t, AZ::SerializeContext* serializeContext) override
            {
                DeletePointerData(serializeContext, &m_classElement, m_testInt);
                return {};
            }
            void ClearElements(void*, AZ::SerializeContext*) override
            {}

            uint32_t* m_testInt;
            AZ::SerializeContext::ClassElement m_classElement;
        };


        // strategy:  Allocate 2 adjacent pages.
        //            Mark the second one as NO READ / NO WRITE
        //            Hand the buffer to the function with a pointer
        //            at the very end of the first page, expect
        //            a failure to read if it crosses it

        // get the page size, just in case this is running on special
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        EXPECT_NE(sysInfo.dwPageSize, 0);
        uint32_t pageSize = static_cast<uint32_t>(sysInfo.dwPageSize);
        uint32_t blockSize = pageSize * 2;
        uint32_t numberOfU32s = blockSize / sizeof(uint32_t);
        uint32_t* testBuffer = reinterpret_cast<uint32_t*>(VirtualAlloc(nullptr, blockSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE ));
        EXPECT_NE(testBuffer, nullptr);

        uint32_t halfWayIndex = (numberOfU32s/2);
        uint32_t* halfwayPointer = &testBuffer[halfWayIndex]; // pointer to the first byte in the second page

        uint32_t newProtectFlags = PAGE_NOACCESS;
        unsigned long oldProtectFlags = 0;
        unsigned long resultCode = VirtualProtect(halfwayPointer, pageSize, newProtectFlags, &oldProtectFlags);
        EXPECT_NE(resultCode, 0);

        // pointer to the last four bytes in the first page:
        uint32_t oneBeforeHalfway = halfWayIndex - 1;
        uint32_t* lastElementInFirstPage = &testBuffer[oneBeforeHalfway];
        *lastElementInFirstPage = 0x12345678;

        TestDataContainer testContainer(lastElementInFirstPage);

        // the above is the very last element in the container.
        // the following line should NOT try to read memory beyond that.
        testContainer.RemoveElement(nullptr, nullptr, m_serializeContext.get());

        // clean up!
        VirtualFree(testBuffer, 0, MEM_RELEASE);
    }
}

