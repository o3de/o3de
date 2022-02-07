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
#include <Atom/RPI.Reflect/Limits.h>
#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>
#include <Atom/RPI.Reflect/Material/MaterialDynamicMetadata.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class Material;
        class Image;
        class ShaderResourceGroup;
        class MaterialPropertiesLayout;

        using MaterialPropertyFlags = AZStd::bitset<Limits::Material::PropertyCountMax>;
        
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

        //! MaterialFunctor objects provide custom logic and calculations to configure shaders, render states,
        //! editor metadata, and more.
        //! Atom will provide an implementation of this class that uses a script to define the custom logic
        //! for a convenient workflow. Clients may also provide their own custom hard-coded implementations
        //! as an optimization rather than taking the scripted approach.
        //! Any custom subclasses of MaterialFunctor will also need a corresponding MaterialFunctorSourceData subclass
        //! to create the functor at build-time. Depending on the builder context, clients can choose to create a runtime
        //! specific functor, an editor functor or one functor used in both circumstances (see usage examples and MaterialFunctor::Process blow).
        //! Usage example:
        //!     (Runtime) Modify the material's ShaderCollections;
        //!         (This allows a material type to include custom logic that dynamically select shaders variants
        //!         or disable specific shaders. This can result in changing the performance characteristics
        //!         of the material by selecting alternate shader variants, or by excluding entries shaders/passes.)
        //!     (Runtime) Perform client-specified calculations on material property values to produce shader input values;
        //!         (For example, there may be a "RotationDegrees" material property but the underlying shader requires
        //!         a rotation matrix, so a MaterialFunctor converts the data.)
        //!     (Editor) Modify metadata of a property when other related properties have changed their value.
        //!         (For example, if a flag property use texture is checked, the texture property will show up,
        //!         otherwise, it should hide.)
        class MaterialFunctor
            : public AZStd::intrusive_base
        {
            friend class MaterialFunctorSourceData;
        private:
            //! The material properties associated with this functor.
            //! In another word, it defines what properties should trigger this functor to process.
            //! Bit position uses MaterialPropertyIndex of the property.
            MaterialPropertyFlags m_materialPropertyDependencies;

        public:
            AZ_RTTI(AZ::RPI::MaterialFunctor, "{4F2EDF30-71C0-4E00-9CB0-9EA97587712E}");
            AZ_CLASS_ALLOCATOR(MaterialFunctor, SystemAllocator, 0);

            using ShaderAssetList = AZStd::vector<Data::Asset<ShaderAsset>>;

            static void Reflect(ReflectContext* context);

            class RuntimeContext
            {
                friend class LuaMaterialFunctorRuntimeContext;
            public:
                //! Get the property value. The type must be one of those in MaterialPropertyValue.
                //! Otherwise, a compile error will be reported.
                template<typename Type>
                const Type& GetMaterialPropertyValue(const Name& propertyName) const;
                template<typename Type>
                const Type& GetMaterialPropertyValue(const MaterialPropertyIndex& index) const;
                //! Get the property value. GetMaterialPropertyValue<T>() is equivalent to GetMaterialPropertyValue().GetValue<T>().
                const MaterialPropertyValue& GetMaterialPropertyValue(const Name& propertyName) const;
                const MaterialPropertyValue& GetMaterialPropertyValue(const MaterialPropertyIndex& index) const;

                const MaterialPropertiesLayout* GetMaterialPropertiesLayout() const { return m_materialPropertiesLayout.get(); }
                
                MaterialPropertyPsoHandling GetMaterialPropertyPsoHandling() const { return m_psoHandling; }

                //! Set the value of a shader option
                //! @param shaderIndex the index of a shader in the material's ShaderCollection
                //! @param shaderTag the tag name of a shader in the material's ShaderCollection
                //! @param optionIndex the index of the shader option to set
                //! @param value the new value for the shader option
                bool SetShaderOptionValue(AZStd::size_t shaderIndex, ShaderOptionIndex optionIndex, ShaderOptionValue value);
                bool SetShaderOptionValue(const AZ::Name& shaderTag, ShaderOptionIndex optionIndex, ShaderOptionValue value);

                //! Get the shader resource group for editing.
                ShaderResourceGroup* GetShaderResourceGroup();

                //! Return how many shaders are in this material.
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

                // [GFX TODO][ATOM-4168] Replace the workaround for unlink-able RPI.Public classes in MaterialFunctor
                // const AZStd::vector<AZStd::any>&, AZStd::unordered_map<MaterialPropertyIndex, Image*>&, RHI::ConstPtr<MaterialPropertiesLayout>
                // can be all replaced by const Material*, but Material definition is separated in RPI.Public, we can't use it at this point.
                RuntimeContext(
                    const AZStd::vector<MaterialPropertyValue>& propertyValues,
                    RHI::ConstPtr<MaterialPropertiesLayout> materialPropertiesLayout,
                    ShaderCollection* shaderCollection,
                    ShaderResourceGroup* shaderResourceGroup,
                    const MaterialPropertyFlags* materialPropertyDependencies,
                    MaterialPropertyPsoHandling psoHandling
                );
            private:
                bool SetShaderOptionValue(ShaderCollection::Item& shaderItem, ShaderOptionIndex optionIndex, ShaderOptionValue value);

                const AZStd::vector<MaterialPropertyValue>& m_materialPropertyValues;
                RHI::ConstPtr<MaterialPropertiesLayout> m_materialPropertiesLayout;
                ShaderCollection* m_shaderCollection;
                ShaderResourceGroup* m_shaderResourceGroup;
                const MaterialPropertyFlags* m_materialPropertyDependencies = nullptr;
                MaterialPropertyPsoHandling m_psoHandling = MaterialPropertyPsoHandling::Error;
            };

            class EditorContext
            {
                friend class LuaMaterialFunctorEditorContext;
            public:
                const MaterialPropertyDynamicMetadata* GetMaterialPropertyMetadata(const Name& propertyName) const;
                const MaterialPropertyDynamicMetadata* GetMaterialPropertyMetadata(const MaterialPropertyIndex& index) const;

                const MaterialPropertyGroupDynamicMetadata* GetMaterialPropertyGroupMetadata(const Name& propertyName) const;

                //! Get the property value. The type must be one of those in MaterialPropertyValue.
                //! Otherwise, a compile error will be reported.
                template<typename Type>
                const Type& GetMaterialPropertyValue(const Name& propertyName) const;
                template<typename Type>
                const Type& GetMaterialPropertyValue(const MaterialPropertyIndex& index) const;
                //! Get the property value. GetMaterialPropertyValue<T>() is equivalent to GetMaterialPropertyValue().GetValue<T>().
                const MaterialPropertyValue& GetMaterialPropertyValue(const Name& propertyName) const;
                const MaterialPropertyValue& GetMaterialPropertyValue(const MaterialPropertyIndex& index) const;

                const MaterialPropertiesLayout* GetMaterialPropertiesLayout() const { return m_materialPropertiesLayout.get(); }
                
                MaterialPropertyPsoHandling GetMaterialPropertyPsoHandling() const { return MaterialPropertyPsoHandling::Allowed; }

                //! Set the visibility dynamic metadata of a material property.
                bool SetMaterialPropertyVisibility(const Name& propertyName, MaterialPropertyVisibility visibility);
                bool SetMaterialPropertyVisibility(const MaterialPropertyIndex& index, MaterialPropertyVisibility visibility);

                bool SetMaterialPropertyDescription(const Name& propertyName, AZStd::string description);
                bool SetMaterialPropertyDescription(const MaterialPropertyIndex& index, AZStd::string description);

                bool SetMaterialPropertyMinValue(const Name& propertyName, const MaterialPropertyValue& min);
                bool SetMaterialPropertyMinValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& min);

                bool SetMaterialPropertyMaxValue(const Name& propertyName, const MaterialPropertyValue& max);
                bool SetMaterialPropertyMaxValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& max);

                bool SetMaterialPropertySoftMinValue(const Name& propertyName, const MaterialPropertyValue& min);
                bool SetMaterialPropertySoftMinValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& min);

                bool SetMaterialPropertySoftMaxValue(const Name& propertyName, const MaterialPropertyValue& max);
                bool SetMaterialPropertySoftMaxValue(const MaterialPropertyIndex& index, const MaterialPropertyValue& max);

                bool SetMaterialPropertyGroupVisibility(const Name& propertyGroupName, MaterialPropertyGroupVisibility visibility);

                // [GFX TODO][ATOM-4168] Replace the workaround for unlink-able RPI.Public classes in MaterialFunctor
                // const AZStd::vector<AZStd::any>&, AZStd::unordered_map<MaterialPropertyIndex, Image*>&, RHI::ConstPtr<MaterialPropertiesLayout>
                // can be all replaced by const Material*, but Material definition is separated in RPI.Public, we can't use it at this point.
                EditorContext(
                    const AZStd::vector<MaterialPropertyValue>& propertyValues,
                    RHI::ConstPtr<MaterialPropertiesLayout> materialPropertiesLayout,
                    AZStd::unordered_map<Name, MaterialPropertyDynamicMetadata>& propertyMetadata,
                    AZStd::unordered_map<Name, MaterialPropertyGroupDynamicMetadata>& propertyGroupMetadata,
                    AZStd::unordered_set<Name>& updatedPropertiesOut,
                    AZStd::unordered_set<Name>& updatedPropertyGroupsOut,
                    const MaterialPropertyFlags* materialPropertyDependencies
                );

            private:
                MaterialPropertyDynamicMetadata* QueryMaterialPropertyMetadata(const Name& propertyName) const;
                MaterialPropertyGroupDynamicMetadata* QueryMaterialPropertyGroupMetadata(const Name& propertyGroupName) const;

                const AZStd::vector<MaterialPropertyValue>& m_materialPropertyValues;
                RHI::ConstPtr<MaterialPropertiesLayout> m_materialPropertiesLayout;
                AZStd::unordered_map<Name, MaterialPropertyDynamicMetadata>& m_propertyMetadata;
                AZStd::unordered_map<Name, MaterialPropertyGroupDynamicMetadata>& m_propertyGroupMetadata;
                AZStd::unordered_set<Name>& m_updatedPropertiesOut;
                AZStd::unordered_set<Name>& m_updatedPropertyGroupsOut;
                const MaterialPropertyFlags* m_materialPropertyDependencies = nullptr;
            };

            //! Check if dependent properties are dirty.
            bool NeedsProcess(const MaterialPropertyFlags& propertyDirtyFlags);

            //! Get all dependent properties of this functor.
            const MaterialPropertyFlags& GetMaterialPropertyDependencies() const;

            //! Process() is called at runtime to build a ShaderCollection object given some material property values.
            //! Material properties will be accessed by MaterialPropertyIndex. The corresponding MaterialFunctorSourceData
            //! should collect the necessary MaterialPropertyIndex values at build-time.
            //! Or Process() is to process material property values and make changes to shader settings.
            virtual void Process([[maybe_unused]] RuntimeContext& context) {}

            //! Process metadata used in the editor, such as property visibility.
            //! While original metadata is stored in MaterialDocument::m_propertyMetadata, this temporary context has a
            //! a reference to it which can be accessed by MaterialPropertyIndex or property Name,
            //! both can use the EditorContext API to convert to the other.
            virtual void Process([[maybe_unused]] EditorContext& context) {}
        };

        using MaterialFunctorList = AZStd::vector<Ptr<MaterialFunctor>>;

    } // namespace RPI
} // namespace AZ
