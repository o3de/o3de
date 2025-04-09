/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <Atom/RPI.Edit/Configuration.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>

namespace AZ
{
    class ReflectContext;

    namespace RHI
    {
        class ShaderResourceGroupLayout;
    }

    namespace RPI
    {
        class JsonMaterialFunctorSourceDataSerializer;
        class MaterialNameContext;

        //! This is an abstract base class for initializing MaterialFunctor objects.
        //! Material functors provide custom logic and calculations to configure shaders, render states, and more.
        //! See MaterialFunctor.h for details.
        class ATOM_RPI_EDIT_API MaterialFunctorSourceData
            : public AZStd::intrusive_base
        {
            friend class JsonMaterialFunctorSourceDataSerializer;

        public:
            AZ_RTTI(MaterialFunctorSourceData, "{2E8C6884-E136-4494-AEC1-5F23473278DC}");
            AZ_CLASS_ALLOCATOR(MaterialFunctorSourceData, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            //! This generally corresponds to AssetBuilderSDK's Job Dependencies.
            struct AssetDependency
            {
                AZStd::string m_sourceFilePath; //!< Can be relative to asset root, or relative to the .materialtype source file
                AZStd::string m_jobKey;         //!< The AssetBuilderSDK's job key name for the asset produced by m_sourceFilePath
            };

            struct ATOM_RPI_EDIT_API RuntimeContext
            {
            public:
                RuntimeContext(
                    const AZStd::string& materialTypeFilePath,
                    const MaterialPropertiesLayout* materialPropertiesLayout,
                    const RHI::ShaderResourceGroupLayout* shaderResourceGroupLayout,
                    const MaterialNameContext* materialNameContext)
                    : m_materialTypeFilePath(materialTypeFilePath)
                    , m_materialPropertiesLayout(materialPropertiesLayout)
                    , m_shaderResourceGroupLayout(shaderResourceGroupLayout)
                    , m_materialNameContext(materialNameContext)
                {}

                const AZStd::string& GetMaterialTypeSourceFilePath() const { return m_materialTypeFilePath; }

                const MaterialPropertiesLayout* GetMaterialPropertiesLayout() const;
                const RHI::ShaderResourceGroupLayout* GetShaderResourceGroupLayout() const;

                //! Find the index of a ShaderResourceGroup input. This will automatically apply the MaterialNameContext.
                RHI::ShaderInputConstantIndex FindShaderInputConstantIndex(Name inputName) const;
                RHI::ShaderInputImageIndex FindShaderInputImageIndex(Name inputName) const;

                //! Find a property's index by its name. It will report error and return a Null index if it fails.
                //! This will also automatically apply the MaterialNameContext.
                MaterialPropertyIndex FindMaterialPropertyIndex(Name propertyId) const;

                //! Returns the name context for the functor.
                //! It acts like a namespace for any names that the MaterialFunctorSourceData might reference. The namespace
                //! is automatically applied by the other relevant functions of this RuntimeContext class.
                //! Note that by default the MaterialNameContext is not saved as part of the final MaterialFunctor class
                //! (most CreateFunctor() implementations should convert names to indexes anyway) but CreateFunctor() can
                //! copy it to the created MaterialFunctor for use at runtime if needed.
                const MaterialNameContext* GetNameContext() const { return m_materialNameContext; }

            private:
                const AZStd::string m_materialTypeFilePath;
                const MaterialPropertiesLayout*          m_materialPropertiesLayout;
                const RHI::ShaderResourceGroupLayout*    m_shaderResourceGroupLayout;
                const MaterialNameContext*               m_materialNameContext;
            };

            struct ATOM_RPI_EDIT_API EditorContext
            {
            public:
                EditorContext(
                    const AZStd::string& materialTypeFilePath,
                    const MaterialPropertiesLayout* materialPropertiesLayout,
                    const MaterialNameContext* materialNameContext)
                    : m_materialTypeFilePath(materialTypeFilePath)
                    , m_materialPropertiesLayout(materialPropertiesLayout)
                    , m_materialNameContext(materialNameContext)
                {}

                const AZStd::string& GetMaterialTypeSourceFilePath() const { return m_materialTypeFilePath; }

                const MaterialPropertiesLayout* GetMaterialPropertiesLayout() const;

                //! Find a property's index by its name. It will report error and return a Null index if it fails.
                //! This will also automatically apply the MaterialNameContext.
                MaterialPropertyIndex FindMaterialPropertyIndex(Name propertyId) const;

                //! Returns the name context for the functor.
                //! It acts like a namespace for any names that the MaterialFunctorSourceData might reference. The namespace
                //! is automatically applied by the other relevant functions of this RuntimeContext class.
                //! Note that by default the MaterialNameContext is not saved as part of the final MaterialFunctor class
                //! (most CreateFunctor() implementations should convert names to indexes anyway) but CreateFunctor() can
                //! copy it to the created MaterialFunctor for use at runtime if needed.
                const MaterialNameContext* GetNameContext() const { return m_materialNameContext; }

            private:
                const AZStd::string m_materialTypeFilePath;
                const MaterialPropertiesLayout* m_materialPropertiesLayout;
                const MaterialNameContext* m_materialNameContext;
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
