/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Configuration.h>
#include <AzCore/base.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    namespace RPI
    {
        //! A place where material functors register to, so that functor source data can be retrieved by name at build time.
        //! As part of deserialization, registration can be done in Reflect() call for each functor.
        class ATOM_RPI_EDIT_API MaterialFunctorSourceDataRegistration
        {
        public:
            AZ_RTTI(MaterialFunctorSourceDataRegistration, "{20D1E55A-737B-43AF-B1F5-054574DCF400}");
            AZ_CLASS_ALLOCATOR(MaterialFunctorSourceDataRegistration, AZ::SystemAllocator);

            MaterialFunctorSourceDataRegistration() = default;
            virtual ~MaterialFunctorSourceDataRegistration() = default;

            //! Get the Interface singleton for the registration.
            //! On some host platforms shader processing is not supported and this interface is not available, so this may return nullptr.
            static MaterialFunctorSourceDataRegistration* Get();

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(MaterialFunctorSourceDataRegistration);

            //! Register Interface for this class.
            void Init();

            //! Unregister Interface for this class.
            void Shutdown();

            //! Register the functor's name and type to allow retrieving type by name.
            void RegisterMaterialFunctor(const AZStd::string& functorName, const Uuid& typeId);

            //! Retrieving the type of functor by its registered name.
            //! If it is not registered, return 0.
            Uuid FindMaterialFunctorTypeIdByName(const AZStd::string& functorName);

            const AZStd::string FindMaterialFunctorNameByTypeId(const Uuid& typeId);

        private:
            AZStd::unordered_map<AZStd::string, Uuid> m_materialfunctorMap; //! Look-up map for names and types.
            AZStd::unordered_map<Uuid, AZStd::string> m_inverseMap;
        };
    }
}
