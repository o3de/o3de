/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshShaderOptions.h>

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <AtomCore/Instance/Instance.h>
#include <AzCore/EBus/Event.h>

namespace AZ
{
    namespace RPI
    {
        class Shader;
    }

    namespace Render
    {
        class CachedSkinnedMeshShaderOptions;

        //! Notifies listeners that the skinned mesh shader has reloaded and the shader options need to be updated
        class SkinnedMeshShaderOptionNotifications
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = CachedSkinnedMeshShaderOptions*;
            using MutexType = AZStd::mutex;

            virtual void OnShaderReinitialized(const CachedSkinnedMeshShaderOptions* cachedShaderOptions) = 0;
        };
        using SkinnedMeshShaderOptionNotificationBus = AZ::EBus<SkinnedMeshShaderOptionNotifications>;

        //! This class caches the indices of the skinned mesh shader options and uses them to more optimally create a ShaderOptionGroup
        class CachedSkinnedMeshShaderOptions
        {
        public:
            void SetShader(Data::Instance<RPI::Shader> shader);
            void ConnectToShaderReinitializedEvent(SkinnedMeshShaderOptionNotificationBus::Handler& shaderReinitializedEventHandler);
            RPI::ShaderOptionGroup CreateShaderOptionGroup(const SkinnedMeshShaderOptions& shaderOptions) const;
        private:
            Data::Instance<RPI::Shader> m_shader;
            RPI::ShaderOptionIndex m_skinningMethodOptionIndex;
            RPI::ShaderOptionValue m_skinningMethodLinearSkinningValue;
            RPI::ShaderOptionValue m_skinningMethodDualQuaternionValue;
            RPI::ShaderOptionValue m_skinningMethodNoSkinningValue;

            RPI::ShaderOptionIndex m_applyMorphTargetOptionIndex;
            RPI::ShaderOptionValue m_applyMorphTargetFalseValue;
            RPI::ShaderOptionValue m_applyMorphTargetTrueValue;
        };
    } // namespace Render
} // namespace AZ
