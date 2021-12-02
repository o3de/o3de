/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

namespace AZ
{
    namespace Render
    {
        class SharedBuffer;

        namespace Hair
        {
            class HairSystemComponent final
                : public Component
            {
            public:
                AZ_COMPONENT(HairSystemComponent, "{F3A56326-1D2F-462D-A9E8-0405A76601A5}");

                HairSystemComponent();
                ~HairSystemComponent();

                static void Reflect(ReflectContext* context);

                static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
                static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
                static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);

            private:
                //! Loads the pass templates mapping file 
                void LoadPassTemplateMappings();

                ////////////////////////////////////////////////////////////////////////
                // Component interface implementation
                void Init() override;
                void Activate() override;
                void Deactivate() override;
                ////////////////////////////////////////////////////////////////////////

                //! Used for loading the pass templates of the hair gem.
                RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
            };
        } // namespace Hair
    } // End Render namespace
} // End AZ namespace
