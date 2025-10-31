/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/sort.h>
#include <AzToolsFramework/Fingerprinting/TypeFingerprinter.h>
#include "AzCore/Component/Component.h"

namespace AzToolsFramework
{
    namespace Fingerprinting
    {
        TypeFingerprinter::TypeFingerprinter(const AZ::SerializeContext& serializeContext)
            : m_serializeContext(serializeContext)
        {
            m_serializeContext.EnumerateAll(
                [this](const AZ::SerializeContext::ClassData* classData, const AZ::TypeId&)
            {
                CreateFingerprint(*classData);
                return true;
            });
        }

        TypeFingerprint TypeFingerprinter::CreateFingerprint(const AZ::SerializeContext::ClassData& classData)
        {
            // Ensures hash will change if a service is moved from, say, required to dependent.
            static const size_t kRequiredServiceKey = AZ_CRC_CE("RequiredServiceKey");
            static const size_t kDependentServiceKey = AZ_CRC_CE("DependentServiceKey");
            static const size_t kProvidedServiceKey = AZ_CRC_CE("ProvidedServiceKey");
            static const size_t kIncompatibleServiceKey = AZ_CRC_CE("IncompatibleServiceKey");

            TypeFingerprint fingerprint = 0;

            AZStd::hash_combine(fingerprint, classData.m_typeId);
            AZStd::hash_combine(fingerprint, classData.m_version);

            bool isComponentObject = classData.m_azRtti ? classData.m_azRtti->IsTypeOf(azrtti_typeid<AZ::Component>()) : false;

            if(isComponentObject)
            {
                AZ::ComponentDescriptor::DependencyArrayType services;
                AZ::ComponentDescriptor* componentDescriptor = nullptr;
                AZ::ComponentDescriptorBus::EventResult(componentDescriptor, classData.m_azRtti->GetTypeId(), &AZ::ComponentDescriptorBus::Events::GetDescriptor);

                if (componentDescriptor)
                {
                    services.clear();
                    componentDescriptor->GetRequiredServices(services, nullptr);
                    AZStd::sort(services.begin(), services.end());
                    AZStd::hash_combine(fingerprint, kRequiredServiceKey);
                    AZStd::hash_range(fingerprint, services.begin(), services.end());

                    services.clear();
                    componentDescriptor->GetDependentServices(services, nullptr);
                    AZStd::sort(services.begin(), services.end());
                    AZStd::hash_combine(fingerprint, kDependentServiceKey);
                    AZStd::hash_range(fingerprint, services.begin(), services.end());

                    services.clear();
                    componentDescriptor->GetProvidedServices(services, nullptr);
                    AZStd::sort(services.begin(), services.end());
                    AZStd::hash_combine(fingerprint, kProvidedServiceKey);
                    AZStd::hash_range(fingerprint, services.begin(), services.end());

                    services.clear();
                    componentDescriptor->GetIncompatibleServices(services, nullptr);
                    AZStd::sort(services.begin(), services.end());
                    AZStd::hash_combine(fingerprint, kIncompatibleServiceKey);
                    AZStd::hash_range(fingerprint, services.begin(), services.end());
                }
            }

            for (const AZ::SerializeContext::ClassElement& element : classData.m_elements)
            {
                AZStd::hash_combine(fingerprint, element.m_typeId);
                AZStd::hash_combine(fingerprint, AZStd::string_view(element.m_name));
                AZStd::hash_combine(fingerprint, element.m_flags);
            }

            m_typeFingerprints[classData.m_typeId] = fingerprint;
            return fingerprint;
        }

        void TypeFingerprinter::GatherAllTypesInObject(const void* instance, const AZ::TypeId& typeId, TypeCollection& outTypeCollection) const
        {
            AZ::SerializeContext::BeginElemEnumCB elementCallback =
                [&outTypeCollection](void* /* instance pointer */, const AZ::SerializeContext::ClassData* classData, const AZ::SerializeContext::ClassElement* /* classElement*/)
            {
                outTypeCollection.insert(classData->m_typeId);
                return true;
            };

            m_serializeContext.EnumerateInstanceConst(instance, typeId, elementCallback, nullptr, AZ::SerializeContext::ENUM_ACCESS_FOR_READ, nullptr, nullptr);
        }

        TypeCollection TypeFingerprinter::GatherAllTypesForComponents() const
        {
            TypeCollection types;

            m_serializeContext.EnumerateDerived(
                [this, &types]
            (const AZ::SerializeContext::ClassData* classData, const AZ::Uuid& /*knownType*/)
                {
                    if (!classData->m_azRtti->IsAbstract())
                    {
                        types.insert(classData->m_typeId);
                    }

                    // We need to recursively include all the types referenced by the component in the fingerprint
                    AZStd::queue<const AZ::SerializeContext::ClassElement*> elementQueue;

                    for (const auto& element : classData->m_elements)
                    {
                        elementQueue.push(&element);
                    }

                    while (!elementQueue.empty())
                    {
                        const auto* element = elementQueue.front();
                        elementQueue.pop();

                        auto [insertedElement, didInsert] = types.insert(element->m_typeId);

                        if (didInsert)
                        {
                            const auto* elementClassData = m_serializeContext.FindClassData(element->m_typeId);

                            if (elementClassData)
                            {
                                for (const auto& subElement : elementClassData->m_elements)
                                {
                                    elementQueue.push(&subElement);
                                }
                            }
                            else
                            {
                                AZ_Warning("TypeFingerprinter", false, "Failed to find class data for type %s while generating fingerprint in hierarchy of %s component.  Fingerprint may not be accurate.",
                                    element->m_typeId.ToString<AZStd::string>().c_str(),
                                    classData->m_name);
                            }
                        }
                    }

                    return true;
                },
                azrtti_typeid<AZ::Component>(), azrtti_typeid<AZ::Component>());

            return types;
        }

        TypeFingerprint TypeFingerprinter::GenerateFingerprintForAllTypes(const TypeCollection& typeCollection) const
        {
            // Sort all types before fingerprinting
            AZStd::vector<AZ::TypeId> sortedTypes;
            sortedTypes.reserve(typeCollection.size());
            sortedTypes.insert(sortedTypes.begin(), typeCollection.begin(), typeCollection.end());
            AZStd::sort(sortedTypes.begin(), sortedTypes.end());

            TypeFingerprint fingerprint = 0;

            for (const AZ::TypeId& typeId : sortedTypes)
            {
                TypeFingerprint typeFingerprint = GetFingerprint(typeId);
                AZStd::hash_combine(fingerprint, typeFingerprint);
            }

            return fingerprint;
        }

        TypeFingerprint TypeFingerprinter::GenerateFingerprintForAllTypesInObject(const void* instance, const AZ::TypeId& typeId) const
        {
            TypeCollection types;
            GatherAllTypesInObject(instance, typeId, types);
            return GenerateFingerprintForAllTypes(types);
        }

    } // namespace Fingerprinting
} // namespace AzToolsFramework
