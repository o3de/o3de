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

#include <AzCore/std/containers/vector.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleWindowRequestBus provides 
    class ShaderManagementConsoleWindowRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        /// Creates and shows main window
        virtual void CreateShaderManagementConsoleWindow() = 0;

        //! Destroys main window
        virtual void DestroyShaderManagementConsoleWindow() = 0;

        virtual void GenerateShaderVariantListForShaderMaterials(const char* shaderFileName) = 0;
    };
    using ShaderManagementConsoleWindowRequestBus = AZ::EBus<ShaderManagementConsoleWindowRequests>;

} // namespace ShaderManagementConsole
