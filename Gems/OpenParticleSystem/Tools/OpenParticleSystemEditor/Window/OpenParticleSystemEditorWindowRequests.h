/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>

namespace OpenParticleSystemEditor
{
    class EditorWindowRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Bring main window to foreground
        virtual void ActivateWindow() = 0;
        virtual void OpenDocument(const AZStd::string& path) = 0;
        virtual void CreateParticleFile(const AZStd::string& path) = 0;
        virtual void SaveDocument() = 0;
    };

    using EditorWindowRequestsBus = AZ::EBus<EditorWindowRequests>;
} // namespace OpenParticleSystemEditor
