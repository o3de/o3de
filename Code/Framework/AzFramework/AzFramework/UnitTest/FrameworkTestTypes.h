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
#pragma once


#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    /**
    * Helper class to set up a test fixture that uses the system alloctors and the local file IO
    */
    class ScopedAllocatorsFileIOFixture
        : public ScopedAllocatorSetupFixture
    {
    public:
        ScopedAllocatorsFileIOFixture()
            :ScopedAllocatorSetupFixture()
            , m_prevFileIO(AZ::IO::FileIOBase::GetInstance())
        {
            if (m_prevFileIO)
            {
                AZ::IO::FileIOBase::SetInstance(nullptr);
            }

            m_localFileIO = aznew AZ::IO::LocalFileIO();
            AZ::IO::FileIOBase::SetInstance(m_localFileIO);
        }

        ~ScopedAllocatorsFileIOFixture()
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_localFileIO;
            m_localFileIO = nullptr;

            if (m_prevFileIO)
            {
                AZ::IO::FileIOBase::SetInstance(m_prevFileIO);
            }
        }

    private:
        AZ::IO::FileIOBase* m_prevFileIO = nullptr;
        AZ::IO::FileIOBase* m_localFileIO = nullptr;
    };
}
