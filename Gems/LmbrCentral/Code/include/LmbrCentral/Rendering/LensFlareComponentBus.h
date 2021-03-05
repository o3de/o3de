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

namespace LmbrCentral
{
    /*!
     * LensFlareComponentRequestsBus
     * Messages serviced by the Lens Flare component.
     */
    class LensFlareComponentRequests
        : public AZ::ComponentBus
    {
    public:
        enum class State
        {
            Off,
            On,
        };

        virtual ~LensFlareComponentRequests() {}

        //! Control lens flare state.
        virtual void SetLensFlareState(State state) { (void)state; }

        //! Turns lens flare on
        virtual void TurnOnLensFlare() {}

        //! Turns lens flare off
        virtual void TurnOffLensFlare() {}

        //! Toggles lens flare state.
        virtual void ToggleLensFlare() {}
    };

    using LensFlareComponentRequestBus = AZ::EBus<LensFlareComponentRequests>;


    /*!
     * LensFlareEditorRequestBus
     * Editor/UI messages serviced by the Lens Flare component.
     */
    class EditorLensFlareComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~EditorLensFlareComponentRequests() {}

        //! Recreates the lens flare.
        virtual void RefreshLensFlare() = 0;
    };

    using EditorLensFlareComponentRequestBus = AZ::EBus<EditorLensFlareComponentRequests>;

    /*!
     * LensFlareComponentEventBus
     * Events dispatched by the Lens Flare component.
     */
    class LensFlareComponentNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual ~LensFlareComponentNotifications() {}

        //! Lens flare has been turned on
        virtual void LensFlareTurnedOn() {}

        //! Lens flare has been turned off
        virtual void LensFlareTurnedOff() {}
    };

    using LensFlareComponentNotificationBus = AZ::EBus<LensFlareComponentNotifications>;
} // namespace LmbrCentral
