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

#include "CrySystem_precompiled.h"

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>

#include <AzCore/Memory/OSAllocator.h>

#include "CrySizerImpl.h"

namespace UnitTest
{
    class CrySizerTest
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
            AZ::AllocatorInstance<CryStringAllocator>::Create();

            m_sizer = new CrySizerImpl();
        }

        void TearDown() override
        {
            delete m_sizer;

            AZ::AllocatorInstance<CryStringAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy();
        }
    protected:
        CrySizerImpl* m_sizer;
    }; //class StatisticsTest

    /**
     * The key data structures fed to ICrySizer in CTerrain::GetMemoryUsage(class ICrySizer* pSizer):
     * 1. structs and classes.
     * 2. PodArray< of structs / classes>
     * 3. PodArray< of pointers>
     */
    TEST_F(CrySizerTest, CrySizerTest_AddSomeObjectsUsedInCTerrain_GetExpectedSize)
    {
        struct TmpStruct 
        {
            AZ::u32 a;
            AZ::u32 b;
        };
        TmpStruct tmpStructObj;
        //Tracking a simple struct.
        m_sizer->AddObjectSize(&tmpStructObj);

        //The AddObject method is only available when using the ICrySizer base class.
        ICrySizer* sizer = static_cast<ICrySizer*>(m_sizer);

        const int numItemsPerArray = 1024;

        //PodArray of structs
        PodArray<TmpStruct> podArrayOfTmpStruct;
        podArrayOfTmpStruct.resize(numItemsPerArray);
        sizer->AddObject(podArrayOfTmpStruct);

        //PodArray of pointers
        PodArray<TmpStruct*> podArrayOfTmpStructPointers;
        podArrayOfTmpStructPointers.resize(numItemsPerArray);
        sizer->AddObject(podArrayOfTmpStructPointers);

        //PodArray of Array2d of pointers.
        const int array2dAxisSize = 64;
        PodArray<Array2d<TmpStruct*>> podArrayOfArray2d;
        podArrayOfArray2d.resize(numItemsPerArray);
        for (int i = 0; i < numItemsPerArray; ++i)
        {
            //Array2d will allocate array2dAxisSize * array2dAxisSize elements.
            podArrayOfArray2d[i].Allocate(array2dAxisSize);
        }
        sizer->AddObject(podArrayOfArray2d);

        //Calculate the total expected size
        const size_t expectedSizeOfArray2d = sizeof(Array2d<TmpStruct*>)
            + (array2dAxisSize * array2dAxisSize) * sizeof(TmpStruct*);
        const size_t expectedTotalSize = sizeof(TmpStruct)
            + numItemsPerArray * sizeof(TmpStruct)
            + numItemsPerArray * sizeof(TmpStruct*)
            + numItemsPerArray * expectedSizeOfArray2d;

        EXPECT_EQ( m_sizer->GetTotalSize(), expectedTotalSize);
    }

}//namespace UnitTest
