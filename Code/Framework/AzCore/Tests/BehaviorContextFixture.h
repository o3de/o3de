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
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace UnitTest
{
    // Basic fixture for managing a BehaviorContext
    class BehaviorContextFixture
        : public AllocatorsFixture
        , public AZ::ComponentApplicationBus::Handler
    {
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_behaviorContext = aznew AZ::BehaviorContext();

            AZ::ComponentApplicationBus::Handler::BusConnect();
        }

        void TearDown() override
        {            
            AZ::ComponentApplicationBus::Handler::BusDisconnect();

            // Just destroy everything before we complete the tear down.
            delete m_behaviorContext;
            m_behaviorContext = nullptr;

            AllocatorsFixture::TearDown();
        }

        // ComponentApplicationBus
        AZ::ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override {}

        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override {}
        bool AddEntity(AZ::Entity*) override { return true; }
        bool RemoveEntity(AZ::Entity*) override { return true; }
        bool DeleteEntity(const AZ::EntityId&) override { return true; }
        AZ::Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
        AZ::SerializeContext* GetSerializeContext() override { return nullptr; }
        AZ::BehaviorContext*  GetBehaviorContext() override { return m_behaviorContext; }
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        const char* GetAppRoot() const override { return nullptr; }
        AZ::Debug::DrillerManager* GetDrillerManager() override { return nullptr; }
        void EnumerateEntities(const EntityCallback& /*callback*/) override {}
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        ////

    protected:    
        AZ::BehaviorContext* m_behaviorContext;
    };
}
