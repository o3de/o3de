/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Asset/AssetCommon.h>

namespace LmbrCentral
{
    /*!
    * DecalComponentRequests::Bus
    * Messages serviced by the Decal component.
    */
    class DecalComponentRequests
        : public AZ::ComponentBus
    {
    public:

        virtual ~DecalComponentRequests() {}

        /**
        * Makes the decal visible.
        */
        virtual void Show() = 0;

        /**
        * Hides the decal.
        */
        virtual void Hide() = 0;

        /**
        * Specify the decal's visibility.
        * \param visible true to make the decal visible, false to hide it.
        */
        virtual void SetVisibility(bool visible) = 0;
    };

    using DecalComponentRequestBus = AZ::EBus<DecalComponentRequests>;

    /*!
    * DecalComponentEditorRequests::Bus
    * Editor/UI messages serviced by the Decal component.
    */
    class DecalComponentEditorRequests
        : public AZ::ComponentBus
    {
    public:

        using Bus = AZ::EBus<DecalComponentEditorRequests>;

        virtual ~DecalComponentEditorRequests() {}

        virtual void RefreshDecal() {}
    };

    /*!
    * DecalComponentEvents::Bus
    * Events dispatched by the Decal component.
    */
    class DecalComponentEvents
        : public AZ::ComponentBus
    {
    public:

        using Bus = AZ::EBus<DecalComponentEvents>;

        virtual ~DecalComponentEvents() {}
    };
} // namespace LmbrCentral
