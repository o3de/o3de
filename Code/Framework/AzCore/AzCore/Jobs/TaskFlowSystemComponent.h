/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Component/Component.h>
#include <AzCore/Jobs/TaskFlowBus.h>

namespace AZ
{
    /**
     *
     */
    class TaskFlowSystemComponent
        : public Component
        , public TaskFlowExecutorInterface
    {
    public:
        AZ_COMPONENT(AZ::TaskFlowSystemComponent, "{F6EF3DC5-2230-441C-80BD-9D5BDAEBFB11}")

        TaskFlowSystemComponent();

        //////////////////////////////////////////////////////////////////////////
        // TaskFlowExecutorInterface
        tf::Executor* GetExecutor() override { return m_taskflowExecutor; }
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
        /// \red ComponentDescriptor::Reflect
        static void Reflect(ReflectContext* reflection);

        tf::Executor*  m_taskflowExecutor;
    };
}
