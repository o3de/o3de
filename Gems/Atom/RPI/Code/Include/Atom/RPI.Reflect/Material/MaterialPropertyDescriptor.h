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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Name/Name.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>

namespace AZ
{
    namespace RPI
    {
        struct MaterialPropertyIndexType {
            AZ_TYPE_INFO(MaterialPropertyIndexType, "{cfc09268-f3f1-4474-bd8f-f2c8de27c5f1}");
        };

        using MaterialPropertyIndex = RHI::Handle<uint32_t, MaterialPropertyIndexType>;

        enum class MaterialPropertyOutputType
        {
            ShaderInput,  //!< Maps to a ShaderResourceGroup input
            ShaderOption, //!< Maps to a shader variant option
            Invalid,
            Count = Invalid
        };

        static const uint32_t MaterialPropertyOutputTypeCount = static_cast<uint32_t>(MaterialPropertyOutputType::Count);
        const char* ToString(MaterialPropertyOutputType materialPropertyOutputType);

        //! Represents a specific output data binding for the material property layer. 
        class MaterialPropertyOutputId
        {
        public:
            AZ_TYPE_INFO(MaterialPropertyOutputId, "{98AAD47F-3603-4CE3-B218-FB920B74027D}")

            static void Reflect(ReflectContext* context);

            MaterialPropertyOutputType m_type = MaterialPropertyOutputType::Invalid;

            //! For m_type==ShaderOption, this is the index of a specific ShaderAsset (see MaterialTypeSourceData's ShaderCollection). 
            //! For m_type==ShaderInput, this field is not used (because there is only one material ShaderResourceGroup in a MaterialAsset).
            RHI::Handle<uint32_t> m_containerIndex;

            //! Index to the specific setting that the material property maps to. 
            //! The MaterialPropertyDataType, MaterialPropertyOutputType, and m_containerIndex determine which list this refers to.
            RHI::Handle<uint32_t> m_itemIndex;
        };
                
        enum class MaterialPropertyDataType : uint32_t
        {
            Invalid, 

            Bool,
            Int,
            UInt,
            Float,
            Vector2,
            Vector3,
            Vector4,
            Color,
            Image,
            Enum,   //!< This type is only used in source data files, not runtime data

            Count
        };

        static const uint32_t MaterialPropertyDataTypeCount = static_cast<uint32_t>(MaterialPropertyDataType::Count);

        const char* ToString(MaterialPropertyDataType materialPropertyDataType);

        AZStd::string GetMaterialPropertyDataTypeString(AZ::TypeId typeId);

        //! Visibility for each material property.
        //! If the data field is empty, use default as editable.
        enum class MaterialPropertyVisibility : uint32_t
        {
            Enabled,  //< The property is visible and editable
            Disabled, //< The property is visible but non-editable
            Hidden,   //< The property is invisible

            Default = Enabled
        };

        struct MaterialPropertyRange
        {
            MaterialPropertyRange() = default;
            MaterialPropertyRange(
                const MaterialPropertyValue& max,
                const MaterialPropertyValue& min,
                const MaterialPropertyValue& softMax,
                const MaterialPropertyValue& softMin
            )
                : m_max(max)
                , m_min(min)
                , m_softMax(softMax)
                , m_softMin(softMin)
            {}

            MaterialPropertyValue m_max;
            MaterialPropertyValue m_min;
            MaterialPropertyValue m_softMax;
            MaterialPropertyValue m_softMin;
        };

        //! Used by material functors to dynamically control property metadata in tools.
        //! For example, show/hide a property based on some other 'enable' flag property.
        //! Normally we wouldn't want editor-related code mixed in with runtime code, but
        //! since this data can be modified dynamically, keeping it in the runtime makes
        //! the overall material functor design simpler and more user-friendly.
        struct MaterialPropertyDynamicMetadata
        {
            AZ_TYPE_INFO(MaterialPropertyDynamicMetadata, "{A89F215F-3235-499F-896C-9E63ACC1D657}");

            AZ::RPI::MaterialPropertyVisibility m_visibility;
            AZStd::string m_description;
            AZ::RPI::MaterialPropertyRange m_propertyRange;
        };

        //! A material property is any data input to a material, like a bool, float, Vector, Image, Buffer, etc.
        //! This descriptor defines a single input property, including it's name ID, and how it maps
        //! to the shader system.
        //!
        //! Each property can be directly connected to various outputs like ShaderResourceGroup fields, shader variant options, etc.
        //! In most cases there will be a single output connection, but multiple connections are possible.
        //! Alternatively, the property may not have any direct connections and would be processed by a custom material functor
        //! instead (see MaterialFunctor.h). Note that having direct output connections does not preclude the use of a functor;
        //! a property with a direct connection may also be processed by a material functor for additional indirect handling.
        class MaterialPropertyDescriptor
        {
            friend class MaterialTypeAssetCreator;
        public:
            AZ_TYPE_INFO(MaterialPropertyDescriptor, "{FC440E30-297E-4827-A28E-ED35AF1719AF}");

            using OutputList = AZStd::vector<MaterialPropertyOutputId>;

            static void Reflect(ReflectContext* context);
            
            MaterialPropertyDescriptor() = default;

            MaterialPropertyDataType GetDataType() const;
            //! Returns the TypeId that is used to store values for this material property.
            AZ::TypeId GetStorageDataTypeId() const;

            //! Returns the value of the enum from its name. If this property is not an enum or the name is undefined, InvalidEnumValue is returned.
            static constexpr uint32_t InvalidEnumValue = -1;
            uint32_t GetEnumValue(const AZ::Name& enumName) const;

            //! Returns the unique name ID of this property
            const Name& GetName() const;
            
            //! Returns the list of shader setting the property is directly connected to.
            const OutputList& GetOutputConnections() const;

        private:
            MaterialPropertyDataType m_dataType = MaterialPropertyDataType::Invalid;
            AZStd::vector<AZ::Name> m_enumNames;
            Name m_nameId;
            OutputList m_outputConnections;
        };
    } // namespace RPI

    AZ_TYPE_INFO_SPECIALIZE(RPI::MaterialPropertyOutputType, "{42A6E5E8-0FE6-4D7B-884A-1F478E4ADD97}");
    AZ_TYPE_INFO_SPECIALIZE(RPI::MaterialPropertyVisibility, "{318B43A2-79E3-4502-8FD0-5815209EA123}");
    AZ_TYPE_INFO_SPECIALIZE(RPI::MaterialPropertyDataType, "{3D903D5C-C6AA-452E-A2F8-8948D30833FF}");
} // namespace AZ

