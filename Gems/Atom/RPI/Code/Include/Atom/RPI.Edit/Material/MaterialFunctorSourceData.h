/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionTypes.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>

namespace AZ
{
    class ReflectContext;

    namespace RHI
    {
        class ShaderResourceGroupLayout;
    }

    namespace RPI
    {
        class ShaderCollection;
        class MaterialPropertiesLayout;
        class ShaderOptionGroupLayout;
        class JsonMaterialFunctorSourceDataSerializer;

        //! This is an abstract base class for initializing MaterialFunctor objects.
        //! Material functors provide custom logic and calculations to configure shaders, render states, and more.
        //! See MaterialFunctor.h for details.
        class MaterialFunctorSourceData
            : public AZStd::intrusive_base
        {
            friend class JsonMaterialFunctorSourceDataSerializer;

        public:
            AZ_RTTI(MaterialFunctorSourceData, "{2E8C6884-E136-4494-AEC1-5F23473278DC}");
            AZ_CLASS_ALLOCATOR(MaterialFunctorSourceData, AZ::SystemAllocator, 0);

            //! This generally corresponds to AssetBuilderSDK's Job Dependencies.
            struct AssetDependency
            {
                AZStd::string m_sourceFilePath; //!< Can be relative to asset root, or relative to the .materialtype source file
                AZStd::string m_jobKey;         //!< The AssetBuilderSDK's job key name for the asset produced by m_sourceFilePath
            };

            struct RuntimeContext
            {
            public:
                RuntimeContext(const AZStd::string& materialTypeFilePath, const MaterialPropertiesLayout* materialPropertiesLayout, const RHI::ShaderResourceGroupLayout* shaderResourceGroupLayout, const ShaderCollection* shaderCollection)
                    : m_materialTypeFilePath(materialTypeFilePath)
                    , m_materialPropertiesLayout(materialPropertiesLayout)
                    , m_shaderResourceGroupLayout(shaderResourceGroupLayout)
                    , m_shaderCollection(shaderCollection)
                {}

                const AZStd::string& GetMaterialTypeSourceFilePath() const { return m_materialTypeFilePath; }

                const MaterialPropertiesLayout* GetMaterialPropertiesLayout() const;
                const RHI::ShaderResourceGroupLayout* GetShaderResourceGroupLayout() const;

                //! Returns the number of shaders available in this material type.
                AZStd::size_t GetShaderCount() const;

                //! Return the shader option group layout of a shader by index. If the index is invalid, it will report error and return nullptr.
                const ShaderOptionGroupLayout* GetShaderOptionGroupLayout(AZStd::size_t shaderIndex) const;
                const ShaderOptionGroupLayout* GetShaderOptionGroupLayout(const AZ::Name& shaderTag) const;

                //! Returns the shader tags in this material type
                AZStd::vector<AZ::Name> GetShaderTags() const;

                //! Find a property's index by its name. It will report error and return a Null index if it fails.
                MaterialPropertyIndex FindMaterialPropertyIndex(const Name& propertyName) const;

                //! Find a shader option index using an option name from the shader by index or tag.
                //! @param shaderIndex index into the material type's list of shader options
                //! @param shaderTag tag name to index into the material type's list of shader options
                //! @param optionName the name of the option to find
                //! @param reportErrors if true, report an error message when if the option name was not found.
                //! @return the found index, or an empty handle if the option could not be found
                ShaderOptionIndex FindShaderOptionIndex(AZStd::size_t shaderIndex, const Name& optionName, bool reportErrors = true) const;
                ShaderOptionIndex FindShaderOptionIndex(const AZ::Name& shaderTag, const Name& optionName, bool reportErrors = true) const;

                //! Return true if a shaderIndex is within the range of the number of shaders defined in this material.
                //! And also report an error message if the index is invalid.
                bool CheckShaderIndexValid(AZStd::size_t shaderIndex) const;

                //! Return true if shaderTag refers to a valid shader in this material's shader collection.
                //! And also report an error message if shaderTag is invalid.
                bool CheckShaderTagValid(const AZ::Name& shaderTag) const;

            private:
                const AZStd::string m_materialTypeFilePath;
                const MaterialPropertiesLayout*       m_materialPropertiesLayout;
                const RHI::ShaderResourceGroupLayout* m_shaderResourceGroupLayout;
                const ShaderCollection*               m_shaderCollection;
            };

            struct EditorContext
            {
            public:
                EditorContext(const AZStd::string& materialTypeFilePath, const MaterialPropertiesLayout* materialPropertiesLayout)
                    : m_materialTypeFilePath(materialTypeFilePath)
                    , m_materialPropertiesLayout(materialPropertiesLayout)
                {}

                const AZStd::string& GetMaterialTypeSourceFilePath() const { return m_materialTypeFilePath; }

                const MaterialPropertiesLayout* GetMaterialPropertiesLayout() const;

                //! Find a property's index by its name. It will report error and return a Null index if it fails.
                MaterialPropertyIndex FindMaterialPropertyIndex(const Name& propertyName) const;

            private:
                const AZStd::string m_materialTypeFilePath;
                const MaterialPropertiesLayout* m_materialPropertiesLayout;
            };

            //! Creates a fully initialized MaterialFunctor object that is ready to be serialized to the cache.
            //! Override either or both, depending on where the functor should take place.
            //! The reason we provide two separate paths to create a functor is there could be different C++ build dependencies for the runtime vs editor functors.
            //! Otherwise it's fine to override both these functions and make them return the same MaterialFunctor. You could create one MaterialFunctor subclass
            //! that handles both runtime and editor processing. Or if there are performance or dependency reasons, create two separate MaterialFunctor subclasses
            //! for runtime and editor.
            using FunctorResult = Outcome<Ptr<MaterialFunctor>>;
            virtual FunctorResult CreateFunctor([[maybe_unused]] const RuntimeContext& runtimeContext) const { return Success(Ptr<MaterialFunctor>(nullptr)); }
            virtual FunctorResult CreateFunctor([[maybe_unused]] const EditorContext& editorContext) const { return Success(Ptr<MaterialFunctor>(nullptr)); }

            //! Add a new dependent property to this functor.
            void AddMaterialPropertyDependency(Ptr<MaterialFunctor> functor, MaterialPropertyIndex index) const;

            //! Returns a list of all shader options that this functor can set.
            virtual AZStd::vector<AZ::Name> GetShaderOptionDependencies() const { return {}; }
            
            //! Returns a list of other assets that this functor depends on.
            //! Any change to one of these assets will trigger the .materialtype to rebuild.
            virtual AZStd::vector<AssetDependency> GetAssetDependencies() const { return {}; }
        };
    } // namespace RPI
} // namespace AZ
