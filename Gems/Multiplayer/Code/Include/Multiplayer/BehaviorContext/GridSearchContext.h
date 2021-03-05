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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <GridMate/NetworkGridMateSessionEvents.h>

namespace GridMate
{
    class GridSession;
    class GridSearch;
    struct SearchInfo;
}

namespace Multiplayer
{
    struct SessionDesc;
    struct GridSearchTicket;
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Multiplayer::GridSearchTicket, "{1C8A155B-123F-4E71-8B33-C043248FB164}");
    AZ_TYPE_INFO_SPECIALIZE(GridMate::GridSearch, "{5FDCA36D-8284-46DC-9387-81A7A70EDBA8}");
    AZ_TYPE_INFO_SPECIALIZE(GridMate::SearchInfo, "{D7BBA18F-5F7C-4E28-8446-6E0E709B1CDD}");
}

namespace Multiplayer
{
    /**
      * An interface to expose Grid searching for an EBus
      */
    class GridSearchInterface
        : public AZ::ComponentBus
    {
    public:
        // Signal events
        virtual const GridSearchTicket* StartSearch(const SessionDesc& sessionDesc) = 0;
        virtual bool StopSearch(GridSearchTicket* search) = 0;
        virtual bool JoinSession(const GridMate::SearchInfo* searchInfo) = 0;

        // Sink callbacks
        virtual void OnSearchComplete(const GridSearchTicket* gridSearch) = 0;
        virtual void OnSearchError(const GridMate::string& errorMsg) = 0;
        virtual void OnSearchInfo(const GridMate::SearchInfo* searchInfo) = 0;
        virtual void OnSearchClosed(bool isJoiningSession) = 0;
        virtual void OnJoinComplete(const GridMate::GridSession* gridSession) = 0;
    };

    using GridSearchBus = AZ::EBus<GridSearchInterface>;

    /**
    * Exposes Grid searching events and callbacks to a behavior context such as Lua
    */
    namespace GridSearchBehavior
    {
        void Reflect(AZ::ReflectContext* reflectContext);
    }
}

