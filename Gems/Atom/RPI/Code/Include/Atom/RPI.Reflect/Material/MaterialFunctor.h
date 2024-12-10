/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>
#include <Atom/RPI.Reflect/Material/MaterialDynamicMetadata.h>
#include <Atom/RPI.Reflect/Material/MaterialPipelineState.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class Material;
        class Image;
        class ShaderResourceGroup;
        class MaterialPropertiesLayout;
        class MaterialPropertyCollection;

        //! Indicates how the material system should respond to any material property changes that
        //! impact Pipeline State Object configuration. This is significant because some platforms
        //! require that PSOs be pre-compiled and shipped with the game.
        enum class MaterialPropertyPsoHandling
        {
            //! PSO-impacting property changes are not allowed, are ignored, and will report an error.
            //! This should be used at runtime. It is recommended to do this on all platforms, not just the restricted ones,
            //! to encourage best-practices. However, if a game project is not shipping on any restricted platforms,
            //! then the team could decide to allow PSO changes.
            Error,

            //! PSO-impacting property changes are allowed, but produce a warning message.
            Warning,
 
            //! PSO-impacting property changes are allowed. This can be used during asset processing, in developer tools, or on platforms that don't restrict PSO changes.
            Allowed
        };

        namespace LuaMaterialFunctorAPI
        {
            class ConfigureShaders;
        }

        namespace MaterialFunctorAPI
        {
            //! Provides functions that are common to all runtime execution contexts
            class ATOM_RPI_REFLECT_API CommonRuntimeConfiguration
            {
            public:
                virtual ~CommonRuntimeConfiguration() = default;

                MaterialPropertyPsoHandling GetMaterialPropertyPsoHandling() const { return m_psoHandling; }

            protected:
                CommonRuntimeConfiguration(MaterialPropertyPsoHandling psoHandling) : m_psoHandling(psoHandling)
                {
                }

            private:
                MaterialPropertyPsoHandling m_psoHandling = MaterialPropertyPsoHandling::Error;
            };

            //! Provides commonly used functions for reading material property values
            class ATOM_RPI_REFLECT_API ReadMaterialPropertyValues
            {
            public:
                virtual ~ReadMaterialPropertyValues() = default;

                //! Get the property value. The type must be one of those in MaterialPropertyValue.
                //! Otherwise, a compile error will be reported.
                template<typename Type>
                const Type& GetMaterialPropertyValue(const Name& propertyId) const;
                template<typename Type>
                const Type& GetMaterialPropertyValue(const MaterialPropertyIndex& index) const;
                //! Get the property value. GetMaterialPropertyValue<T>() is equivalent to GetMaterialPropertyValue().GetValue<T>().
                const MaterialPropertyValue& GetMaterialPropertyValue(const Name& propertyId) const;
                const MaterialPropertyValue& GetMaterialPropertyValue(const MaterialPropertyIndex& index) const;

                const MaterialPropertiesLayout* GetMaterialPropertiesLayout() const;

            protected:
                ReadMaterialPropertyValues(
                    const MaterialPropertyCollection& materialProperties,
                    const MaterialPropertyFlags* materialPropertyDependencies
                );

                const MaterialPropertyCollection& m_materialProperties;
                const MaterialPropertyFlags* m_materialPropertyDependencies = nullptr;
            };

            //! Provides commonly used functions for configuring shaders
            class ATOM_RPI_REFLECT_API ConfigureShaders
            {
                friend LuaMaterialFunctorAPI::ConfigureShaders;

            public:
                virtual ~ConfigureShaders() = default;

                //! Set the value of a shader option in all applicable shaders.
                bool SetShaderOptionValue(const Name& optionName, ShaderOptionValue value);
                bool SetShaderOptionValue(const Name& optionName, const Name& value);

                //! Return how many shaders are in the local ShaderCollection.
                AZStd::size_t GetShaderCount() const;

                //! Enable/disable the specific shader with the index.
                //! @param shaderIndex the index of a shader in the material's ShaderCollection
                //! @param shaderTag the tag name of a shader in the material's ShaderCollection
                void SetShaderEnabled(AZStd::size_t shaderIndex, bool enabled);
                void SetShaderEnabled(const AZ::Name& shaderTag, bool enabled);

                //! Set runtime draw list override. It will override the draw list defined in the shader variant source.
                //! @param shaderIndex the index of a shader in the material's ShaderCollection
                //! @param shaderTag the tag name of a shader in the material's ShaderCollection
                //! @param drawListTagName  The tag name of the draw list that should be used for this shader
                void SetShaderDrawListTagOverride(AZStd::size_t shaderIndex, const Name& drawListTagName);
                void SetShaderDrawListTagOverride(const AZ::Name& shaderTag, const Name& drawListTagName);

                //! Set runtime render states overlay. It will override the render states defined in the shader variant source, for each valid overlay member.
                //! Note RenderStates are initialized to default values which will override all states.
                //! Utilize GetInvalidRenderStates, and only assign the state(s) that should be modified.
                //! @param shaderIndex the index of a shader in the material's ShaderCollection
                //! @param shaderTag the tag name of a shader in the material's ShaderCollection
                //! @param renderStatesOverlay a RenderStates struct that will be merged with this shader's RenderStates (using RHI::MergeStateInto())
                void ApplyShaderRenderStateOverlay(AZStd::size_t shaderIndex, const RHI::RenderStates& renderStatesOverlay);
                void ApplyShaderRenderStateOverlay(const AZ::Name& shaderTag, const RHI::RenderStates& renderStatesOverlay);

            protected:
                ConfigureShaders(
                    ShaderCollection* localShaderCollection
                );

                virtual void ForAllShaderItems(AZStd::function<bool(ShaderCollection::Item& shaderItem)> callback);

                template<typename ValueType>
                bool SetShaderOptionValueHelper(const Name& name, const ValueType& value);

                ShaderCollection* m_localShaderCollection;
            };

            //! This execution context operates at a high level, and is not specific to a particular material pipeline.
            //! It can read material property values.
            //! It can set internal material property values (to pass data to pipeline-specific functors which use PipelineRuntimeContext).
            //! It can configure the Material ShaderResourceGroup because there is one for the entire material,
            //! it's not specific to a material pipeline or particular shader.
            //! It can configure shaders that are not specific to a particular material pipeline (i.e. the MaterialPipelineNone ShaderCollection).
            //! It can set shader option values (Note this does impact the material-pipeline-specific shaders in order to automatically
            //! propagate the values to all shaders in the material).
            class ATOM_RPI_REFLECT_API RuntimeContext
                : public CommonRuntimeConfiguration
                , public ReadMaterialPropertyValues
                , public ConfigureShaders
            {
            public:

                //! Get the shader resource group for editing.
                ShaderResourceGroup* GetShaderResourceGroup();

                //! Set the value of an internal material property. These are used to pass data to one of the material pipelines.
                bool SetInternalMaterialPropertyValue(const Name& propertyId, const MaterialPropertyValue& value);

                RuntimeContext(
                    const MaterialPropertyCollection& materialProperties,
                    const MaterialPropertyFlags* materialPropertyDependencies,
                    MaterialPropertyPsoHandling psoHandling,
                    ShaderResourceGroup* shaderResourceGroup,
                    ShaderCollection* generalShaderCollection,
                    MaterialPipelineDataMap* materialPipelineData
                );

            private:

                void ForAllShaderItems(AZStd::function<bool(ShaderCollection::Item& shaderItem)> callback) override;

                ShaderResourceGroup* m_shaderResourceGroup;
                MaterialPipelineDataMap* m_materialPipelineData;
            };

            //! This execution context operates on a specific MaterialPipelinePayload's shaders.
            //! It can read "internal" material properties used for passing data to the material pipeline.
            class ATOM_RPI_REFLECT_API PipelineRuntimeContext
                : public CommonRuntimeConfiguration
                , public ReadMaterialPropertyValues
                , public ConfigureShaders
            {
            public:

                PipelineRuntimeContext(
                    const MaterialPropertyCollection& internalProperties,
                    const MaterialPropertyFlags* internalMaterialPropertyDependencies,
                    MaterialPropertyPsoHandling psoHandling,
                    ShaderCollection* pipelineShaderCollections
                );

            };

            //! This execution context is used by tools for configuring UI metadata.
            class ATOM_RPI_REFLECT_API EditorContext
                : public ReadMaterialPropertyValues
            {
            public:
                const MaterialPropertyDynamicMetadata* GetMaterialPropertyMetadata(const Name& propertyId) const;
                const MaterialPropertyDynamicMetadata* GetMaterialPropertyMetadata(const MaterialPropertyIndex& index) const;

                const MaterialPropertyGroupDynamicMetadata* GetMaterialPropertyGroupMetadata(const Name& propertyId) const;

                //! Set the visibility dynamic metadata of a material property.
                bool SetMaterialPropertyVisibility(const Name& propertyId, MaterialPropertyVisibility visibility);
                bool SetMaterialPropertyVisibility(const MaterialPropertyIndex& index, MaterialPropertyVisibility visibility);

                bool SetMaterialPropertyDescription(const Name& propertyId, AZStd::string description);
                bool SetMaterialPropertyDescription(const MaterialPropertyIndex& index, AZStd::string description);

                bool SetMaterialPropertyMinValue(const Name& propertyId, const MaterialPropertyValue& min);
                bool SetMaterialPropertyMinValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& min);

                bool SetMaterialPropertyMaxValue(const Name& propertyId, const MaterialPropertyValue& max);
                bool SetMaterialPropertyMaxValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& max);

                bool SetMaterialPropertySoftMinValue(const Name& propertyId, const MaterialPropertyValue& min);
                bool SetMaterialPropertySoftMinValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& min);

                bool SetMaterialPropertySoftMaxValue(const Name& propertyId, const MaterialPropertyValue& max);
                bool SetMaterialPropertySoftMaxValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& max);

                bool SetMaterialPropertyGroupVisibility(const Name& propertyGroupName, MaterialPropertyGroupVisibility visibility);

                // [GFX TODO][ATOM-4168] Replace the workaround for unlink-able RPI.Public classes in MaterialFunctor
                // const AZStd::vector<AZStd::any>&, AZStd::unordered_map<MaterialPropertyIndex, Image*>&, RHI::ConstPtr<MaterialPropertiesLayout>
                // can be all replaced by const Material*, but Material definition is separated in RPI.Public, we can't use it at this point.
                EditorContext(
                    const MaterialPropertyCollection& materialProperties,
                    AZStd::unordered_map<Name, MaterialPropertyDynamicMetadata>& propertyMetadata,
                    AZStd::unordered_map<Name, MaterialPropertyGroupDynamicMetadata>& propertyGroupMetadata,
                    AZStd::unordered_set<Name>& updatedPropertiesOut,
                    AZStd::unordered_set<Name>& updatedPropertyGroupsOut,
                    const MaterialPropertyFlags* materialPropertyDependencies
                );

            private:
                MaterialPropertyDynamicMetadata* QueryMaterialPropertyMetadata(const Name& propertyId) const;
                MaterialPropertyGroupDynamicMetadata* QueryMaterialPropertyGroupMetadata(const Name& propertyGroupId) const;

                AZStd::unordered_map<Name, MaterialPropertyDynamicMetadata>& m_propertyMetadata;
                AZStd::unordered_map<Name, MaterialPropertyGroupDynamicMetadata>& m_propertyGroupMetadata;
                AZStd::unordered_set<Name>& m_updatedPropertiesOut;
                AZStd::unordered_set<Name>& m_updatedPropertyGroupsOut;
            };

        }

        //! MaterialFunctor objects provide custom logic and calculations to configure shaders, render states,
        //! editor metadata, and more.
        //! Atom also provides a LuaMaterialFunctor subclass that uses a script to define the custom logic
        //! for a convenient workflow. Developers may also provide their own custom hard-coded implementations
        //! as an optimization rather than taking the scripted approach.
        //! Any custom subclasses of MaterialFunctor will also need a corresponding MaterialFunctorSourceData subclass
        //! to create the functor at build-time. Depending on the builder context, clients can choose to create a runtime
        //! specific functor, an editor functor or one functor used in both circumstances (see usage examples and MaterialFunctor::Process blow).
        //! Usage example:
        //!     (MainRuntime) Modify the material's main ShaderCollection;
        //!         (This allows a material type to include custom logic that dynamically enables or disables particular shaders,
        //!          or set shader options)
        //!     (MainRuntime) Perform client-specified calculations on material property values to produce shader input values;
        //!         (For example, there may be a "RotationDegrees" material property but the underlying shader requires
        //!         a rotation matrix, so a MaterialFunctor converts the data.)
        //!     (MainRuntime) Set internal material property values that are passed to a material pipeline script for further processing;
        //!         (For example, if a an "opacityValue" property is less than 1.0, set a "isTransparent" flag. Another material pipeline
        //!          functor can use this to enable the transparent pass shader.)
        //!     (Editor) Modify metadata of a property when other related properties have changed their value.
        //!         (For example, if a flag property use texture is checked, the texture property will show up,
        //!         otherwise, it should hide.)
        //!     (PipelineRuntime) Enable or disable material pipeline shaders.
        //!         (The material pipeline has an "isTransparent" flag. Some other part of the material type sets this value as needed
        //!         based on material property values from the user. If this value is true, the functor disables the depth pass and
        //!         forward pass, and instead enables the transparent pass shader.)
        //! Note: Although it is reasonable to have a MaterialFunctor subclass that implements both Process(RuntimeContext) and
        //!       Process(EditorContext) (Atom has several functors that work like this), there is no reason for a functor to combine
        //!       either of these with Process(PipelineRuntimeContext). The role of regular material functors vs pipeline material functors
        //!       are so different it could actually be more proper to have separate MaterialFunctor and MaterialPipelineFunctor classes.
        //!       However, we avoid making that split because in practice it would create unnecessary clutter, as it would lead to splitting
        //!       Material[Pipeline]SourceData, JsonMaterial[Pipeline]FunctorSourceDataSerializer, LuaMaterial[Pipeline]Functor,
        //!       LuaMaterial[Pipeline]FunctorSourceData, and all their relevant unit tests. These are so similar functionally that it ends
        //!       up being easier to just keep them all together, and solely rely on the different execution context objections to keep the
        //!       APIs separated.
        class ATOM_RPI_REFLECT_API MaterialFunctor :
            public AZStd::intrusive_base
        {
            friend class MaterialFunctorSourceData;

        public:
            AZ_RTTI(AZ::RPI::MaterialFunctor, "{4F2EDF30-71C0-4E00-9CB0-9EA97587712E}");
            AZ_CLASS_ALLOCATOR(MaterialFunctor, SystemAllocator);

            static void Reflect(ReflectContext* context);

            //! Check if dependent properties are dirty.
            bool NeedsProcess(const MaterialPropertyFlags& propertyDirtyFlags);

            //! Get all dependent properties of this functor.
            const MaterialPropertyFlags& GetMaterialPropertyDependencies() const;

            //! Process(RuntimeContext) is called at runtime to configure the pipeline-agnostic ShaderCollection and
            //! material ShaderResourceGroup based on material property values.
            virtual void Process([[maybe_unused]] MaterialFunctorAPI::RuntimeContext& context) {}

            //! Process(EditorContext) is called in tools to configure UI, such as property visibility.
            virtual void Process([[maybe_unused]] MaterialFunctorAPI::EditorContext& context) {}
            
            //! Process(PipelineRuntimeContext) is called at runtime to configure a pipeline-specific ShaderCollection
            //! based on some internal material property values.
            virtual void Process([[maybe_unused]] MaterialFunctorAPI::PipelineRuntimeContext& context) {}

        private:

            //! The material properties associated with this functor.
            //! It defines what properties should trigger this functor to process.
            //! Bit position uses MaterialPropertyIndex of the property.
            MaterialPropertyFlags m_materialPropertyDependencies;
        };


        using MaterialFunctorList = AZStd::vector<Ptr<MaterialFunctor>>;

    } // namespace RPI
} // namespace AZ
