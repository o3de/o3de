/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AzToolsFramework
{
    namespace Fingerprinting
    {
        /**
         * A fingerprint based on a type's reflected data in the AZ::SerializeContext.
         */
        using TypeFingerprint = size_t;

        static const TypeFingerprint InvalidTypeFingerprint = 0;

        using TypeCollection = AZStd::unordered_set<AZ::TypeId>;

        /**
         * Generates fingerprints for each type known to the AZ::SerializeContext.
         */
        class TypeFingerprinter
        {
        public:
            AZ_CLASS_ALLOCATOR(TypeFingerprinter, AZ::SystemAllocator);

            TypeFingerprinter(const AZ::SerializeContext& serializeContext);

            // Get the fingerprint for a type.
            TypeFingerprint GetFingerprint(const AZ::TypeId& typeId) const;

            template<class T>
            TypeFingerprint GetFingerprint() const;

            // Gather all types contained within this object and add them to the collection.
            void GatherAllTypesInObject(const void* object, const AZ::TypeId& objectTypeId, TypeCollection& outTypeCollection) const;

            template<class T>
            void GatherAllTypesInObject(const T* object, TypeCollection& outTypeCollection) const;

            TypeCollection GatherAllTypesForComponents() const;

            // Generate a fingerprint based on all types in the collection.
            TypeFingerprint GenerateFingerprintForAllTypes(const TypeCollection& typeCollection) const;

            // Generate a fingerprint based on all types contained within this object.
            // Calling this is the same as calling GatherAllTypesInObject() followed by GenerateFingerprintForAllTypes().
            TypeFingerprint GenerateFingerprintForAllTypesInObject(const void* object, const AZ::TypeId& objectTypeId) const;

            template<class T>
            TypeFingerprint GenerateFingerprintForAllTypesInObject(const T* object) const;

        private:

            TypeFingerprint CreateFingerprint(const AZ::SerializeContext::ClassData& classData);

            AZStd::unordered_map<AZ::TypeId, TypeFingerprint> m_typeFingerprints;
            const AZ::SerializeContext& m_serializeContext;
        };

        ////////////////////////////////////////////////////////////////////////////
        // Implementation

        inline TypeFingerprint TypeFingerprinter::GetFingerprint(const AZ::TypeId& typeId) const
        {
            auto found = m_typeFingerprints.find(typeId);
            if (found != m_typeFingerprints.end())
            {
                return found->second;
            }
            return InvalidTypeFingerprint;
        }

        template<class T>
        inline TypeFingerprint TypeFingerprinter::GetFingerprint() const
        {
            return GetFingerprint(AZ::SerializeTypeInfo<T>::GetUuid());
        }

        template<class T>
        inline void TypeFingerprinter::GatherAllTypesInObject(const T* object, TypeCollection& outTypeCollection) const
        {
            const void* classPtr = AZ::SerializeTypeInfo<T>::RttiCast(object, AZ::SerializeTypeInfo<T>::GetRttiTypeId(object));
            const AZ::TypeId& classId = AZ::SerializeTypeInfo<T>::GetUuid(object);
            return GatherAllTypesInObject(classPtr, classId, outTypeCollection);
        }

        template<class T>
        inline TypeFingerprint TypeFingerprinter::GenerateFingerprintForAllTypesInObject(const T* object) const
        {
            const void* classPtr = AZ::SerializeTypeInfo<T>::RttiCast(object, AZ::SerializeTypeInfo<T>::GetRttiTypeId(object));
            const AZ::TypeId& classId = AZ::SerializeTypeInfo<T>::GetUuid(object);
            return GenerateFingerprintForAllTypesInObject(classPtr, classId);
        }

    } // namespace Fingerprinting
} // namespace AzToolsFramework
