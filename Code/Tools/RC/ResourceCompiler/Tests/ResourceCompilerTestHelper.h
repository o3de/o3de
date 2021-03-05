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

#include <AzCore/Memory/AllocatorScope.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/AzTest.h>


class ResourceCompilerTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    virtual ~ResourceCompilerTestEnvironment(){}

protected:
    void SetupEnvironment() override
    {
        m_allocatorScope.ActivateAllocators();
        AZ::IO::FileIOBase::SetInstance(nullptr); // The API requires the old instance to be destroyed first
        AZ::IO::FileIOBase::SetInstance(new AZ::IO::LocalFileIO());
    }

    void TeardownEnvironment() override
    {
        m_allocatorScope.DeactivateAllocators();
    }
    AZ::AllocatorScope<AZ::OSAllocator, AZ::SystemAllocator, AZ::LegacyAllocator, CryStringAllocator> m_allocatorScope;
};
