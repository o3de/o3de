/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/EBus/Event.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionTypes.h>
#include <Atom/RHI.Reflect/NameIdReflectionMap.h>

namespace AZ
{
    namespace RPI
    {
        class ATOM_RPI_PUBLIC_API ShaderSystemInterface
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

            //! The ShaderSystem supervariantName is used by the ShaderAsset to search for an additional supervariant permutation.
            //! This is done by appending the supervariantName set here to the user-specified supervariant name.
            //! Currently this is used for NoMSAA supervariant support.
            virtual void SetSupervariantName(const AZ::Name& supervariantName) = 0;
            virtual const AZ::Name& GetSupervariantName() const = 0;
        };

    }   // namespace RPI
}   // namespace AZ
