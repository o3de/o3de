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

#include <PythonBindingsInterface.h>
#include <AzCore/IO/Path/Path.h> 
#include <AzCore/std/parallel/semaphore.h>

namespace O3DE::ProjectManager
{
    class PythonBindings 
        : public PythonBindingsInterface::Registrar
    {
    public:
        PythonBindings() = default;
        PythonBindings(const AZ::IO::PathView& enginePath);
        ~PythonBindings() override;

        // PythonBindings overrides
        ProjectInfo GetCurrentProject() override;

    private:
        AZ_DISABLE_COPY_MOVE(PythonBindings);

        void ExecuteWithLock(AZStd::function<void()> executionCallback);
        bool StartPython();
        bool StopPython();

        AZ::IO::FixedMaxPath m_enginePath;
        AZStd::recursive_mutex m_lock;
    };
}
