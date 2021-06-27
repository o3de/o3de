/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Multiplayer/Components/FilteredReplicationInterface.h>
#include <Multiplayer/Components/FilteredServerToClientBus.h>

namespace Multiplayer
{
    //! @class FilteredServerToClientComponent
    //! @brief Allows specification of a filtering interface that can omit entities from being replicated from servers to clients.
    //! @details This component ought be to attached to player prefabs, then one has to implement @FilteredReplicationInterface and
    //!          set it via @SetFilteredInterface.
    class FilteredServerToClientComponent final
        : public AZ::Component
        , public FilteredServerToClientRequestBus::Handler
    {
    public:
        AZ_COMPONENT(FilteredServerToClientComponent, "{B6CB4668-6994-4457-85EA-24E9A2333918}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ~FilteredServerToClientComponent() override = default;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! Multiplayer::FilteredServerToClientBus overrides.
        //! @{
        void SetFilteredReplicationHandlerChanged(FilteredReplicationHandlerChanged::Handler handler) override;
        void SetFilteredInterface(FilteredReplicationInterface* filteredReplication) override;
        FilteredReplicationInterface* GetFilteredInterface() override;
        //! @}

    private:
        FilteredReplicationInterface* m_filteringHandler = nullptr;
        FilteredReplicationHandlerChanged m_filteringHandlerChanged;
    };
}
