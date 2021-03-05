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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/Entity/SliceEntityOwnershipService.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipService.h>

namespace UnitTest
{
    using EntityList = AZStd::vector<AZ::Entity*>;

    class EntityOwnershipServiceTestFixture
        : public AllocatorsTestFixture
    {
    protected:
        AzFramework::RootSliceAsset GetRootSliceAsset();
        void SetUpEntityOwnershipServiceTest();
        void TearDownEntityOwnershipServiceTest();
        AzFramework::SliceInstantiationTicket AddSlice(const EntityList& entityList, const bool isAsynchronous = false);
        void AddEditorSlice(
            AZ::Data::Asset<AZ::SliceAsset>& sliceAsset, const AZ::Transform& worldTransform, const EntityList& entityList);
        AzFramework::SliceInstantiationTicket AddSlice(const EntityList& entityList, const bool isAsynchronous,
            AZ::Data::Asset<AZ::SliceAsset>& sliceAsset);
        void HandleEntitiesAdded(const AzFramework::EntityList& entityList);
        void HandleEntitiesRemoved(const AzFramework::EntityIdList& entityIds);
        bool ValidateEntities(const AzFramework::EntityList&);

        void AddSliceComponentToAsset(AZ::Data::Asset<AZ::SliceAsset>& sliceAsset, const EntityList& entityList);

        AZStd::unique_ptr<AzFramework::Application> m_app;
        bool m_entitiesAddedCallbackTriggered = false;
        bool m_entityRemovedCallbackTriggered = false;
        bool m_validateEntitiesCallbackTriggered = false;
        bool m_areEntitiesValidForContext = true;

        class EntityOwnershipServiceApplication : public AzToolsFramework::ToolsApplication
        {
        public:
            AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        };
    };
}


