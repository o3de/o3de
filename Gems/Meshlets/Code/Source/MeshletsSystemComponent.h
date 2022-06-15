
/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <Meshlets/MeshletsBus.h>

namespace AZ
{
    namespace Meshlets
    {
        class MeshletsSystemComponent
            : public AZ::Component
            , protected MeshletsRequestBus::Handler
            , public AZ::TickBus::Handler
        {
        public:
            AZ_COMPONENT(MeshletsSystemComponent, "{0a55656c-08ac-440d-a55c-f7e3c1b91712}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            MeshletsSystemComponent();
            ~MeshletsSystemComponent();

        protected:
            ////////////////////////////////////////////////////////////////////////
            // MeshletsRequestBus interface implementation

            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // AZTickBus interface implementation
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            ////////////////////////////////////////////////////////////////////////

            //! Loads the pass templates mapping file 
            void LoadPassTemplateMappings();

            //! Used for loading the pass templates of the hair gem.
            AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
        };

    } // namespace Meshlets
} // namespace AZ
