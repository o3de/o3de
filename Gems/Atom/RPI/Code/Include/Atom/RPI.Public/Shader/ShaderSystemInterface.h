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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/Event.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionTypes.h>
#include <Atom/RHI.Reflect/NameIdReflectionMap.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderSystemInterface
        {
        public:
            AZ_RTTI(ShaderSystemInterface, "{D14E323A-2240-46DA-9126-6746D10A93F1}");

            ShaderSystemInterface() = default;
            virtual ~ShaderSystemInterface() = default;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(ShaderSystemInterface);

            static ShaderSystemInterface* Get();

            using GlobalShaderOptionUpdatedEvent = Event<const AZ::Name& /*shaderOptionName*/, ShaderOptionValue /*value*/>;
            using GlobalShaderOptionMap = AZStd::unordered_map<Name, ShaderOptionValue>;

            //! Set a global shader option value that can be used by any shader with a matching shader option name.
            virtual void SetGlobalShaderOption(const AZ::Name& shaderOptionName, ShaderOptionValue value) = 0;

            //! Return the value of a global shader option, or Null if the value is not set.
            virtual ShaderOptionValue GetGlobalShaderOption(const AZ::Name& shaderOptionName) = 0;

            //! Returns the collection of all global shader options and their values.
            virtual const GlobalShaderOptionMap& GetGlobalShaderOptions() const = 0;

            //! Connect a handler for GlobalShaderOptionUpdatedEvent's
            virtual void Connect(GlobalShaderOptionUpdatedEvent::Handler& handler) = 0;
        };

    }   // namespace RPI
}   // namespace AZ
