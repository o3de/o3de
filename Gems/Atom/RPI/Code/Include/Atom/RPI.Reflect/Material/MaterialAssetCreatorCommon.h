/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Name/Name.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>

// These classes are not directly referenced in this header only because the SetPropertyValue()
// function is templatized. But the API is still specific to these data types so we include them here.
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

namespace AZ
{
    namespace RPI
    {
        class StreamingImageAsset;
        class AttachmentImageAsset;

        //! Provides common functionality to both MaterialTypeAssetCreator and MaterialAssetCreator.
        class MaterialAssetCreatorCommon
        {
        public:
            void SetPropertyValue(const Name& name, const Data::Asset<ImageAsset>& imageAsset);
            void SetPropertyValue(const Name& name, const Data::Asset<StreamingImageAsset>& imageAsset);
            void SetPropertyValue(const Name& name, const Data::Asset<AttachmentImageAsset>& imageAsset);

            //! Sets a property value using data in AZStd::variant-based MaterialPropertyValue. The contained data must match
            //! the data type of the property. For type Image, the value must be a Data::Asset<ImageAsset>.
            void SetPropertyValue(const Name& name, const MaterialPropertyValue& value);

        protected:
            MaterialAssetCreatorCommon() = default;

            void OnBegin(
                const MaterialPropertiesLayout* propertyLayout,
                AZStd::vector<MaterialPropertyValue>* propertyValues,
                const AZStd::function<void(const char*)>& warningFunc,
                const AZStd::function<void(const char*)>& errorFunc);
            void OnEnd();

        private:
            bool PropertyCheck(TypeId typeId, const Name& name);

            //! Returns the MaterialPropertyDataType value that corresponds to typeId
            MaterialPropertyDataType GetMaterialPropertyDataType(TypeId typeId) const;

            //! Checks that the TypeId typeId matches the type expected by materialPropertyDescriptor
            bool ValidateDataType(TypeId typeId, const Name& propertyName, const MaterialPropertyDescriptor* materialPropertyDescriptor);

            const MaterialPropertiesLayout* m_propertyLayout = nullptr;
            //! Points to the m_propertyValues list in a MaterialAsset or MaterialTypeAsset
            AZStd::vector<MaterialPropertyValue>* m_propertyValues = nullptr;

            AZStd::function<void(const char*)> m_reportWarning = nullptr;
            AZStd::function<void(const char*)> m_reportError = nullptr;
        };

    } // namespace RPI
} // namespace AZ
