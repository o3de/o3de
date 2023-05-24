/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Viewport/LocalViewBookmarkLoader.h>

namespace AzToolsFramework
{
    //! @class ViewBookmarkSystemComponent
    //! @brief System Component that holds functionality for the ViewBookmarks
    class ViewBookmarkSystemComponent final
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ViewBookmarkSystemComponent, "{FDD852BA-5F9E-4676-B121-D4B2FDEA7F55}");

        ViewBookmarkSystemComponent() = default;

        virtual ~ViewBookmarkSystemComponent() = default;

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

    private:
        //! Used for loading/saving View Bookmarks.
        LocalViewBookmarkLoader m_viewBookmarkLoader;
    };
} // namespace AzToolsFramework
