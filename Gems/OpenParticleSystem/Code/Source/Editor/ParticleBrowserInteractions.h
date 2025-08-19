/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

namespace OpenParticle
{
    class ParticleBrowserInteractions
        : public AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ParticleBrowserInteractions, AZ::SystemAllocator, 0);

        ParticleBrowserInteractions();
        ~ParticleBrowserInteractions();

    private:
        //! AssetBrowserInteractionNotificationBus::Handler overrides...
        void AddSourceFileOpeners(
            const char* fullSourceFileName,
            const AZ::Uuid& sourceUUID,
            AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;

        bool HandlesSource(AZStd::string_view fileName) const;
    };
} // namespace OpenParticle
