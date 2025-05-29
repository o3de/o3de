/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyCollection.h>
#include <Atom/RPI.Reflect/Material/MaterialPipelineState.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

#include <AtomCore/Instance/InstanceData.h>

namespace AZ
{
    namespace RHI
    {
        class ShaderResourceGroup;
    }

    namespace RPI
    {
        class Shader;
        class ShaderResourceGroup;
        class MaterialPropertyDescriptor;
        class MaterialPropertiesLayout;

        //! Provides runtime material functionality based on a MaterialAsset. The material operates on a
        //! set of properties, which are configured primarily at build-time through the MaterialAsset. 
        //! These properties are used to configure shader system inputs at runtime.
        //! 
        //! Material property values can be accessed at runtime, using the SetPropertyValue() and GetPropertyValue().
        //! After applying all property changes, Compile() must be called to apply those changes to the shader system.
        //! 
        //! If RPI validation is enabled, the class will perform additional error checking. If a setter method fails
        //! an error is emitted and the call returns false without performing the requested operation. Likewise, if 
        //! a getter method fails, an error is emitted and an empty value is returned. If validation is disabled, the 
        //! operation is always performed.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API Material
            : public Data::InstanceData
            , public ShaderReloadNotificationBus::MultiHandler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class MaterialSystem;
        public:
            AZ_INSTANCE_DATA(Material, "{C99F75B2-8BD5-4CD8-8672-1E01EF0A04CF}");
            AZ_CLASS_ALLOCATOR(Material, SystemAllocator);

            //! Material objects use a ChangeId to track when changes have been made to the material at runtime. See GetCurrentChangeId()
            using ChangeId = size_t;

            //! GetCurrentChangeId() will never return this value, so client code can use this to initialize a ChangeId that is immediately dirty
            static constexpr ChangeId DEFAULT_CHANGE_ID = 0;

            static Data::Instance<Material> FindOrCreate(const Data::Asset<MaterialAsset>& materialAsset);
            static Data::Instance<Material> Create(const Data::Asset<MaterialAsset>& materialAsset);

            virtual ~Material();

            //! Finds the material property index from the material property ID
            //! @param wasRenamed optional parameter that is set to true if @propertyId is an old name and an automatic rename was applied to find the index.
            //! @param newName optional parameter that is set to the new property name, if the property was renamed.
            MaterialPropertyIndex FindPropertyIndex(const Name& propertyId, bool* wasRenamed = nullptr, Name* newName = nullptr) const;

            //! Sets the value of a material property. The template data type must match the property's data type.
            //! @return true if property value was changed
            template<typename Type>
            bool SetPropertyValue(MaterialPropertyIndex index, const Type& value);

            //! Gets the value of a material property. The template data type must match the property's data type.
            template<typename Type>
            const Type& GetPropertyValue(MaterialPropertyIndex index) const;

            //! Sets the value of a material property. The @value data type must match the property's data type.
            //! @return true if property value was changed
            bool SetPropertyValue(MaterialPropertyIndex index, const MaterialPropertyValue& value);

            const MaterialPropertyCollection& GetPropertyCollection() const;

            const MaterialPropertyValue& GetPropertyValue(MaterialPropertyIndex index) const;
            const AZStd::vector<MaterialPropertyValue>& GetPropertyValues() const;
            
            //! Gets flags indicating which properties have been modified.
            const MaterialPropertyFlags& GetPropertyDirtyFlags() const;

            //! Gets the material properties layout.
            RHI::ConstPtr<MaterialPropertiesLayout> GetMaterialPropertiesLayout() const;

            //! Must be called after changing any material property values in order to apply those changes to the shader.
            //! Does nothing if NeedsCompile() is false or CanCompile() is false.
            //! @return whether compilation occurred
            bool Compile();
            
            //! Returns an ID that can be used to track whether the material has changed since the last time client code read it.
            //! This gets incremented every time a change is made, like by calling SetPropertyValue().
            ChangeId GetCurrentChangeId() const;

            //! Return the general purpose shader collection that applies to any render pipeline.
            const ShaderCollection& GetGeneralShaderCollection() const;

            //! Returns the shader collection for a specific material pipeline.
            //! @param forPipeline the name of the material pipeline to query for shaders.
            const ShaderCollection& GetShaderCollection(const Name& forPipeline) const;

            //! Iterates through all shader items in the material, for all render pipelines, including the general shader collection.
            //! @param callback function is called for each shader item
            //! @param materialPipelineName the name of the shader's material pipeline, or empty (MaterialPipelineNone) for items in the general shader collection.
            void ForAllShaderItems(AZStd::function<bool(const Name& materialPipelineName, const ShaderCollection::Item& shaderItem)> callback) const;

            //! Returns whether this material owns a particular shader option. In that case, SetSystemShaderOption may not be used.
            bool MaterialOwnsShaderOption(const Name& shaderOptionName) const;

            //! Attempts to set the value of a system-level shader option that is controlled by this material.
            //! This applies to all shaders in the material's ShaderCollection.
            //! Note, this may only be used to set shader options that are not "owned" by the material, see MaterialOwnsShaderOption().
            //! @param shaderOptionName the name of the shader option(s) to set
            //! @param value the new value for the shader option(s)
            //! @param return the number of shader options that were updated, or Failure if the material owns the indicated shader option.
            AZ::Outcome<uint32_t> SetSystemShaderOption(const Name& shaderOptionName, RPI::ShaderOptionValue value);

            //! Apply all global shader options to this material
            void ApplyGlobalShaderOptions();

            //! Override the material's default PSO handling setting.
            //! This is normally used in tools like Asset Processor or Material Editor to allow changes that impact
            //! Pipeline State Objects which is not allowed at runtime. See MaterialPropertyPsoHandling for more details.
            //! Do not set this in the shipping runtime unless you know what you are doing.
            void SetPsoHandlingOverride(MaterialPropertyPsoHandling psoHandlingOverride);

            Data::Instance<RPI::ShaderResourceGroup> GetShaderResourceGroup();

            const RHI::ShaderResourceGroup* GetRHIShaderResourceGroup() const;

            const Data::Asset<MaterialAsset>& GetAsset() const;

            //! Returns whether the material is ready to compile pending changes. (Materials can only be compiled once per frame because SRGs can only be compiled once per frame).
            bool CanCompile() const;

            //! Returns whether the material has property changes that have not been compiled yet.
            bool NeedsCompile() const;

            using OnMaterialShaderVariantReadyEvent = AZ::Event<>;
            //! Connect a handler to listen to the event that a shader variant asset of the shaders used by this material is ready.
            //! This is a thread safe function.
            void ConnectEvent(OnMaterialShaderVariantReadyEvent::Handler& handler);

        private:
            Material();

            //! Standard init path from asset data.
            static Data::Instance<Material> CreateInternal(MaterialAsset& materialAsset);
            RHI::ResultCode Init(MaterialAsset& materialAsset);

            ///////////////////////////////////////////////////////////////////
            // ShaderReloadNotificationBus overrides...
            void OnShaderReinitialized(const Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const ShaderVariant& shaderVariant) override;
            ///////////////////////////////////////////////////////////////////

            //! Helper function to reinitialize the material while preserving property values.
            void ReInitKeepPropertyValues();

            //! Helper function for setting the value of a shader constant input, allowing for specialized handling of specific types,
            //! converting to the native type before passing to the ShaderResourceGroup.
            bool SetShaderConstant(RHI::ShaderInputConstantIndex shaderInputIndex, const MaterialPropertyValue& value);

            //! Helper function for setting the value of a shader option, allowing for specialized handling of specific types.
            //! This template is explicitly specialized in the cpp file.
            bool SetShaderOption(ShaderOptionGroup& options, ShaderOptionIndex shaderOptionIndex, const MaterialPropertyValue & value);

            bool TryApplyPropertyConnectionToShaderInput(
                const MaterialPropertyValue & value,
                const MaterialPropertyOutputId & connection,
                const MaterialPropertyDescriptor * propertyDescriptor);
            bool TryApplyPropertyConnectionToShaderOption(
                const MaterialPropertyValue & value,
                const MaterialPropertyOutputId & connection);
            bool TryApplyPropertyConnectionToShaderEnable(
                const MaterialPropertyValue & value,
                const MaterialPropertyOutputId & connection);
            bool TryApplyPropertyConnectionToInternalProperty(
                const MaterialPropertyValue & value,
                const MaterialPropertyOutputId & connection);

            void ProcessDirectConnections();
            void ProcessMaterialFunctors();
            void ProcessInternalDirectConnections();
            void ProcessInternalMaterialFunctors();

            // Note we can't overload the ForAllShaderItems name, because the compiler fails to resolve the public
            // version of the function when a private overload is present, just based on a lambda signature.
            void ForAllShaderItemsWriteable(AZStd::function<bool(ShaderCollection::Item& shaderItem)> callback);

            static constexpr const char* s_debugTraceName = "Material";

            //! The corresponding material asset that provides material type data and initial property values.
            Data::Asset<MaterialAsset> m_materialAsset;

            //! Holds all runtime data for the shader resource group, and provides functions to easily manipulate that data
            Data::Instance<RPI::ShaderResourceGroup> m_shaderResourceGroup;

            //! The RHI shader resource group owned by m_shaderResourceGroup. Held locally to avoid an indirection.
            const RHI::ShaderResourceGroup* m_rhiShaderResourceGroup = nullptr;

            //! These the main material properties, exposed in the Material Editor, and configured directly by users.
            MaterialPropertyCollection m_materialProperties;

            ShaderCollection m_generalShaderCollection;

            MaterialPipelineDataMap m_materialPipelineData;

            //! Tracks each change made to material properties.
            //! Initialized to DEFAULT_CHANGE_ID+1 to ensure that GetCurrentChangeId() will not return DEFAULT_CHANGE_ID (a value that client 
            //! code can use to initialize a ChangeId that is immediately dirty).
            ChangeId m_currentChangeId = DEFAULT_CHANGE_ID + 1;

            //! Records the m_currentChangeId when the material was last compiled.
            ChangeId m_compiledChangeId = DEFAULT_CHANGE_ID;

            bool m_isInitializing = false;

            MaterialPropertyPsoHandling m_psoHandling = MaterialPropertyPsoHandling::Warning;

            //! AZ::Event is not thread safe, so we have to do our own thread safe code
            //! because MeshDrawPacket can connect to this event from different threads.
            AZStd::recursive_mutex m_shaderVariantReadyEventMutex;
            OnMaterialShaderVariantReadyEvent m_shaderVariantReadyEvent;
        };

    } // namespace RPI
} // namespace AZ
