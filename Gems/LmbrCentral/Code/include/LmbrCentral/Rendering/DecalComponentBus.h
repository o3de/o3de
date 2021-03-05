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
