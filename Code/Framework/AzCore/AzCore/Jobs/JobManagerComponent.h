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
#ifndef AZCORE_JOB_MANAGER_COMPONENT_H
#define AZCORE_JOB_MANAGER_COMPONENT_H

#include <AzCore/Component/Component.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    /**
     *
     */
    class JobManagerComponent
        : public Component
        , public JobManagerBus::Handler
    {
    public:
        AZ_COMPONENT(AZ::JobManagerComponent, "{CAE3A025-FAC9-4537-B39E-0A800A2326DF}")

        JobManagerComponent();

        //////////////////////////////////////////////////////////////////////////
        // JobManager bus
        JobManager* GetManager() override { return m_jobManager; }
        JobContext* GetGlobalContext() override { return m_jobGlobalContext; }
        //////////////////////////////////////////////////////////////////////////

    private:
        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        /// \ref ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
        /// \ref ComponentDescriptor::GetDependentServices
        static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);
        /// \red ComponentDescriptor::Reflect
        static void Reflect(ReflectContext* reflection);

        JobManager*  m_jobManager;
        JobContext*  m_jobGlobalContext;
        int          m_numberOfWorkerThreads;   ///< Number of worked threads to spawn for this process. If <= 0 we will use all cores.
        int          m_firstThreadCPU;          ///< ID of the first thread, afterwards we just increment. If == -1, no CPU will be set.(TODO: We can have a full array)
    };
}

#endif // AZCORE_JOB_MANAGER_COMPONENT_H
#pragma once
