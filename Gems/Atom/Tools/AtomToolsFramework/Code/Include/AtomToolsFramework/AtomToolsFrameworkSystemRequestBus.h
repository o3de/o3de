/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AtomToolsFramework
{
    //! AtomToolsFrameworkSystemRequestBus provides an interface for globally accessible atom tools functions
    class AtomToolsFrameworkSystemRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Returns true if registry settings are configured to ignore the specified path, hiding it from the asset browser and file
        //! enumeration functions. This is primarily used to ignore files in the asset cache or intermediate assets folder.
        virtual bool IsPathIgnored(const AZStd::string& path) const = 0;

        //! Returns true to registry settings are configured to allow the file at the specified path to be opened and edited.
        virtual bool IsPathEditable(const AZStd::string& path) const = 0;

        //! Returns true if registry settings are configured to allow the file at the specified path to be used for thumbnails and previews.
        virtual bool IsPathPreviewable(const AZStd::string& path) const = 0;
    };

    using AtomToolsFrameworkSystemRequestBus = AZ::EBus<AtomToolsFrameworkSystemRequests>;

} // namespace AtomToolsFramework
