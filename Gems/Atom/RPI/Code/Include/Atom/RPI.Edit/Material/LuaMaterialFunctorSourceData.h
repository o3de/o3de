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

#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>

namespace UnitTest
{
    class LuaMaterialFunctorTests;
}

namespace AZ
{
    namespace RPI
    {
        //! Builds a LuaMaterialFunctor.
        //! Materials can use this functor to create custom scripted operations.
        class LuaMaterialFunctorSourceData final
            : public AZ::RPI::MaterialFunctorSourceData
        {
            friend class UnitTest::LuaMaterialFunctorTests;
        public:
            AZ_RTTI(AZ::RPI::LuaMaterialFunctorSourceData, "{E6F6D022-340C-47E3-A0BA-4EFE79C0CD1A}", RPI::MaterialFunctorSourceData);

            static void Reflect(AZ::ReflectContext* context);

            FunctorResult CreateFunctor(const RuntimeContext& context) const override;
            FunctorResult CreateFunctor(const EditorContext& context) const override;

            AZStd::vector<AssetDependency> GetAssetDependencies() const override;

            virtual AZStd::vector<AZ::Name> GetShaderOptionDependencies() const override { return m_shaderOptionDependencies; }

        private:
            // Calls a lua function that returns a list of strings.
            Outcome<AZStd::vector<Name>, void> GetNameListFromLuaScript(AZ::ScriptContext& scriptContext, const char* luaFunctionName) const;

            FunctorResult CreateFunctor(const AZStd::string& materialTypeSourceFilePath, const MaterialPropertiesLayout* propertiesLayout) const;

            // Only one of these should have data
            AZStd::string m_luaSourceFile;
            AZStd::string m_luaScript;

            // These are prefix strings that will be applied to every name lookup in the lua functor.
            // This allows the lua script to be reused in different contexts.
            AZStd::string m_propertyNamePrefix;
            AZStd::string m_srgNamePrefix;
            AZStd::string m_optionsNamePrefix;

            // This is mutable because it gets initialized in CreateFunctor which is const. This is okay because CreateFunctor() is called before GetShaderOptionDependencies()
            mutable AZStd::vector<AZ::Name> m_shaderOptionDependencies;
        };

    } // namespace Render
} // namespace AZ
