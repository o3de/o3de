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

            RPI::ShaderOptionIndex m_applyMorphTargetOptionIndex;
            RPI::ShaderOptionValue m_applyMorphTargetFalseValue;
            RPI::ShaderOptionValue m_applyMorphTargetTrueValue;

            RPI::ShaderOptionIndex m_applyColorMorphTargetOptionIndex;
            RPI::ShaderOptionValue m_applyColorMorphTargetFalseValue;
            RPI::ShaderOptionValue m_applyColorMorphTargetTrueValue;
        };
    } // namespace Render
} // namespace AZ
