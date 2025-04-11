/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <Atom/RPI.Edit/Configuration.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <Atom/RPI.Reflect/Material/MaterialDynamicMetadata.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class JsonMaterialPropertySerializer;

        //! Stores data that defines one material property, for use in JSON source files such as .materialtype and .materialpipeline.
        struct ATOM_RPI_EDIT_API MaterialPropertySourceData
        {
            friend class JsonMaterialPropertySerializer;

            AZ_CLASS_ALLOCATOR(MaterialPropertySourceData, SystemAllocator);
            AZ_TYPE_INFO(AZ::RPI::MaterialPropertySourceData, "{E0DB3C0D-75DB-4ADB-9E79-30DA63FA18B7}");

            struct ATOM_RPI_EDIT_API Connection
            {
            public:
                AZ_TYPE_INFO(AZ::RPI::MaterialPropertySourceData::Connection, "{C2F37C26-D7EF-4142-A650-EF50BB18610F}");

                Connection() = default;
                Connection(MaterialPropertyOutputType type, AZStd::string_view name);

                MaterialPropertyOutputType m_type = MaterialPropertyOutputType::Invalid;

                //! The name of a specific shader setting. This will either be a ShaderResourceGroup input, a ShaderOption, or a shader tag, depending on m_type.
                AZStd::string m_name;
            };

            using ConnectionList = AZStd::vector<Connection>;

            static constexpr float DefaultMin = AZStd::numeric_limits<float>::lowest();
            static constexpr float DefaultMax = AZStd::numeric_limits<float>::max();
            static constexpr float DefaultStep = 0.1f;

            static void Reflect(ReflectContext* context);

            MaterialPropertySourceData() = default;

            explicit MaterialPropertySourceData(AZStd::string_view name);

            const AZStd::string& GetName() const;

            MaterialPropertyVisibility m_visibility = MaterialPropertyVisibility::Default;

            MaterialPropertyDataType m_dataType = MaterialPropertyDataType::Invalid;

            ConnectionList m_outputConnections; //!< List of connections from material property to shader settings

            MaterialPropertyValue m_value; //!< Value for the property. The type must match the MaterialPropertyDataType.

            AZStd::vector<AZStd::string> m_enumValues; //!< Only used if property is Enum type
            bool m_enumIsUv = false;  //!< Indicates if the enum value should use m_enumValues or those extracted from m_uvNameMap.

            // Editor metadata ...
            AZStd::string m_displayName;
            AZStd::string m_description;
            AZStd::vector<AZStd::string> m_vectorLabels;
            MaterialPropertyValue m_min;
            MaterialPropertyValue m_max;
            MaterialPropertyValue m_softMin;
            MaterialPropertyValue m_softMax;
            MaterialPropertyValue m_step;

        private:

            // Even though most data in this struct is public, m_name is protected because the MaterialTypeSourceData keeps tight
            // control over how material properties and groups are created and named.
            AZStd::string m_name; //!< The name of the property within the property group. The full property ID will be groupName.propertyName.
        };

        //using MaterialPropertySourceDataList = AZStd::vector<AZStd::unique_ptr<MaterialPropertySourceData>>;

    } // namespace RPI

} // namespace AZ
