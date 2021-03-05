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

#include <AzCore/Interface/Interface.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderVariantAsyncLoader.h>


namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        /**
         * Manages system-wide initialization and support for shader classes
         */
        class ShaderSystem
            : public ShaderSystemInterface
        {
        public:
            AZ_TYPE_INFO(ShaderSystem, "{F57DB8D9-0701-4C96-92DB-A8E07DEA09A6}");

            static void Reflect(AZ::ReflectContext* context);
            static void GetAssetHandlers(AssetHandlerPtrList& assetHandlers);

            void Init();
            void Shutdown();

            ///////////////////////////////////////////////////////////////////
            // ShaderSystemInterface overrides
            void SetGlobalShaderOption(const AZ::Name& shaderOptionName, ShaderOptionValue value) override;
            ShaderOptionValue GetGlobalShaderOption(const AZ::Name& shaderOptionName) override;
            const GlobalShaderOptionMap& GetGlobalShaderOptions() const override;
            void Connect(GlobalShaderOptionUpdatedEvent::Handler& handler) override;
            ///////////////////////////////////////////////////////////////////

        private:
            AZStd::unordered_map<Name, ShaderOptionValue> m_globalShaderOptionValues;
            GlobalShaderOptionUpdatedEvent m_globalShaderOptionUpdatedEvent;
            ShaderVariantAsyncLoader m_shaderVariantAsyncLoader;
        };



    } // namespace RPI
} // namespace AZ
