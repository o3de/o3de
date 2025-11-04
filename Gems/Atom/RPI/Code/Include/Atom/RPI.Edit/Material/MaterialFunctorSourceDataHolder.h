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
#include <Atom/RPI.Edit/Configuration.h>
#include <Atom/RPI.Edit/Material/MaterialFunctorSourceData.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        //! The wrapper class for derived material functors.
        //! It is used in deserialization so that derived material functors can be deserialized by name.
        class ATOM_RPI_EDIT_API MaterialFunctorSourceDataHolder final
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
        private:
            Ptr<MaterialFunctorSourceData> m_actualSourceData = nullptr; // The derived material functor instance.
        };
    } // namespace RPI

} // namespace AZ
