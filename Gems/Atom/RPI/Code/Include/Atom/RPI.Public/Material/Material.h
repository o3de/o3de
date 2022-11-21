/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>

// These classes are not directly referenced in this header only because the Set/GetPropertyValue()
// functions are templatized. But the API is still specific to these data types so we include them here.
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
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
        class Material
            : public Data::InstanceData
            , public ShaderReloadNotificationBus::MultiHandler
        {
            friend class MaterialSystem;
        public:
            AZ_INSTANCE_DATA(Material, "{C99F75B2-8BD5-4CD8-8672-1E01EF0A04CF}");
            AZ_CLASS_ALLOCATOR(Material, SystemAllocator, 0);

            //! Material objects use a ChangeId to track when changes have been made to the material at runtime. See GetCurrentChangeId()
            using ChangeId = size_t;

            //! GetCurrentChangeId() will never return this value, so client code can use this to initialize a ChangeId that is immediately dirty
            static const ChangeId DEFAULT_CHANGE_ID = 0;

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

            //! Return the set of shaders to be run by this material.
            const ShaderCollection& GetShaderCollection() const;

            //! Attempts to set the value of a system-level shader option that is controlled by this material.
            //! This applies to all shaders in the material's ShaderCollection.
            //! Note, this may only be used to set shader options that are not "owned" by the material.
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

            const RHI::ShaderResourceGroup* GetRHIShaderResourceGroup() const;

            const Data::Asset<MaterialAsset>& GetAsset() const;

            //! Returns whether the material is ready to compile pending changes. (Materials can only be compiled once per frame because SRGs can only be compiled once per frame).
            bool CanCompile() const;

            //! Returns whether the material has property changes that have not been compiled yet.
            bool NeedsCompile() const;

        private:
            Material() = default;

            //! Standard init path from asset data.
            static Data::Instance<Material> CreateInternal(MaterialAsset& materialAsset);
            RHI::ResultCode Init(MaterialAsset& materialAsset);

            ///////////////////////////////////////////////////////////////////
            // ShaderReloadNotificationBus overrides...
            void OnShaderReinitialized(const Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const ShaderVariant& shaderVariant) override;
            ///////////////////////////////////////////////////////////////////

            template<typename Type>
            bool ValidatePropertyAccess(const MaterialPropertyDescriptor* propertyDescriptor) const;

            //! Helper function for setting the value of a shader constant input, allowing for specialized handling of specific types.
            //! This template is explicitly specialized in the cpp file.
            template<typename Type>
            void SetShaderConstant(RHI::ShaderInputConstantIndex shaderInputIndex, const Type& value);

            //! Helper function for setting the value of a shader option, allowing for specialized handling of specific types.
            //! This template is explicitly specialized in the cpp file.
            template<typename Type>
            bool SetShaderOption(ShaderOptionGroup& options, ShaderOptionIndex shaderOptionIndex, Type value);

            static const char* s_debugTraceName;

            //! The corresponding material asset that provides material type data and initial property values.
            Data::Asset<MaterialAsset> m_materialAsset;

            //! Holds all runtime data for the shader resource group, and provides functions to easily manipulate that data
            Data::Instance<RPI::ShaderResourceGroup> m_shaderResourceGroup;

            //! The RHI shader resource group owned by m_shaderResourceGroup. Held locally to avoid an indirection.
            const RHI::ShaderResourceGroup* m_rhiShaderResourceGroup = nullptr;

            //! Provides a description of the set of available material properties, cached locally so we don't have to keep fetching it from the MaterialTypeSourceData.
            RHI::ConstPtr<MaterialPropertiesLayout> m_layout;

            //! Values for all properties in MaterialPropertiesLayout
            AZStd::vector<MaterialPropertyValue> m_propertyValues;

            //! Flags indicate which properties have been modified so that related functors will update.
            MaterialPropertyFlags m_propertyDirtyFlags;

            //! Used to track which properties have been modified at runtime so they can be preserved if the material has to reinitialiize.
            MaterialPropertyFlags m_propertyOverrideFlags;

            //! A copy of the MaterialAsset's ShaderCollection is stored here to allow material-specific changes to the default collection.
            ShaderCollection m_shaderCollection;

            //! Tracks each change made to material properties.
            //! Initialized to DEFAULT_CHANGE_ID+1 to ensure that GetCurrentChangeId() will not return DEFAULT_CHANGE_ID (a value that client 
            //! code can use to initialize a ChangeId that is immediately dirty).
            ChangeId m_currentChangeId = DEFAULT_CHANGE_ID + 1;

            //! Records the m_currentChangeId when the material was last compiled.
            ChangeId m_compiledChangeId = DEFAULT_CHANGE_ID;

            bool m_isInitializing = false;
                
            MaterialPropertyPsoHandling m_psoHandling = MaterialPropertyPsoHandling::Warning;
        };

    } // namespace RPI
} // namespace AZ
