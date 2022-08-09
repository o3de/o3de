/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include<AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace RPI
    {
        class Material;
    }
}

namespace AtomImGuiTools
{
    //! AtomImGuiToolsBus provides an interface to interact with Atom ImGui debug tools
    class AtomImGuiToolsRequests : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void ShowMaterialShaderDetails(AZ::Data::Instance<AZ::RPI::Material> material) = 0;
    };
    using AtomImGuiToolsBus = AZ::EBus<AtomImGuiToolsRequests>;

} 
