/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {

        struct MaterialFunctorShaderParameter
        {
            AZ_RTTI(MaterialFunctorShaderParameter, "{A6220690-E78E-4B37-A349-5944E13F766B}");
            static void Reflect(AZ::ReflectContext* context);

            virtual ~MaterialFunctorShaderParameter() = default;
            AZStd::string m_name;
            AZStd::string m_typeName;
            size_t m_typeSize = 0;
        };

        //! The wrapper class for derived material functors.
        //! It is used in deserialization so that derived material functors can be deserialized by name.
        class MaterialFunctorSourceDataHolder final
            : public AZStd::intrusive_base
        {
            friend class JsonMaterialFunctorSourceDataSerializer;

        public:
            AZ_RTTI(MaterialFunctorSourceDataHolder, "{073C98F6-9EA4-411A-A6D2-A47428A0EFD4}");
            AZ_CLASS_ALLOCATOR(MaterialFunctorSourceDataHolder, AZ::SystemAllocator);

            MaterialFunctorSourceDataHolder() = default;
            MaterialFunctorSourceDataHolder(Ptr<MaterialFunctorSourceData> actualSourceData);
            ~MaterialFunctorSourceDataHolder() = default;

            static void Reflect(AZ::ReflectContext* context);

            MaterialFunctorSourceData::FunctorResult CreateFunctor(const MaterialFunctorSourceData::RuntimeContext& runtimeContext) const;
            MaterialFunctorSourceData::FunctorResult CreateFunctor(const MaterialFunctorSourceData::EditorContext& editorContext) const;

            Ptr<MaterialFunctorSourceData> GetActualSourceData() const;
            const AZStd::vector<MaterialFunctorShaderParameter>& GetShaderParameters() const;

        private:
            Ptr<MaterialFunctorSourceData> m_actualSourceData = nullptr; // The derived material functor instance.
            AZStd::vector<MaterialFunctorShaderParameter> m_shaderParameters;
        };
    } // namespace RPI

} // namespace AZ
