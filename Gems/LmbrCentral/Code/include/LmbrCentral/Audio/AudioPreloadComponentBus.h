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
     * AudioPreloadComponentRequests EBus Interface
     * Messages serviced by AudioPreloadComponents.
     * Preloads are ATL controls that specify one
     * or more assets to load.
     * See \ref AudioPreloadComponent for details.
     */
    class AudioPreloadComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //! Loads the default preload (specified in editor).
        virtual void Load() = 0;

        //! Unloads the default preload (specified in editor).
        virtual void Unload() = 0;

        //! Loads the specified preload.
        virtual void LoadPreload(const char* preloadName) = 0;

        //! Unloads the specified preload.
        virtual void UnloadPreload(const char* preloadName) = 0;
        
        //! Checks if a preload is loaded.
        virtual bool IsLoaded(const char* preloadName) = 0;
    };

    using AudioPreloadComponentRequestBus = AZ::EBus<AudioPreloadComponentRequests>;

} // namespace LmbrCentral
