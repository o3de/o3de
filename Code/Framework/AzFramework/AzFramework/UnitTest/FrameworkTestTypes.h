/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        : public LeakDetectionFixture
    {
    public:
        ScopedAllocatorsFileIOFixture()
            :LeakDetectionFixture()
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
