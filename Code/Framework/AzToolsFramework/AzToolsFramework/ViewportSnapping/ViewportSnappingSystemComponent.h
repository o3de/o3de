/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AzToolsFramework
{
    namespace ViewportSnapping
    {
        class ViewportSnapper;

        //! System component owning the editor's geometry snapping service.
        //!
        //! Its only job is lifetime: it constructs the implementation and registers it with
        //! AZ::Interface for as long as the editor is running. The implementation is held behind a
        //! forward declaration rather than by value so that nothing about its internals - or the
        //! containers it caches geometry in - ends up on this exported class's ABI.
        class AZTF_API ViewportSnappingSystemComponent final : public AZ::Component
        {
        public:
            AZ_COMPONENT(ViewportSnappingSystemComponent, "{2F7A6D18-4C93-4E5B-B0A7-8D3E1C4F92B6}");

            ViewportSnappingSystemComponent();
            ~ViewportSnappingSystemComponent();

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        private:
            // AZ::Component overrides ...
            void Activate() override;
            void Deactivate() override;

            AZStd::unique_ptr<ViewportSnapper> m_snapper;
        };
    } // namespace ViewportSnapping
} // namespace AzToolsFramework
