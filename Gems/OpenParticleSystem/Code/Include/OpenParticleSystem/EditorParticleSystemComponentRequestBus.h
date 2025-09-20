/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace OpenParticle
{
    //! EditorParticleSystemComponentRequests provides an interface to communicate with ParticleEditor
    class EditorParticleSystemComponentRequests
        : public AZ::EBusTraits
    {
    public:
        // Only a single handler is allowed
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Open document in particle editor
        virtual void OpenParticleEditor(const AZStd::string& sourcePath) = 0;
        virtual void CreateNewParticle(const AZStd::string& sourcePath) = 0;

        virtual AZ::TypeId GetParticleSystemConfigType() const = 0;

        virtual AZ::TypeId GetEmitterConfigType() const = 0;

        virtual AZStd::vector<AZ::TypeId> GetEmitTypes() const = 0;

        virtual AZStd::vector<AZ::TypeId> GetSpawnTypes() const = 0;

        virtual AZStd::vector<AZ::TypeId> GetUpdateTypes() const = 0;

        virtual AZStd::vector<AZ::TypeId> GetRenderTypes() const = 0;

        virtual AZ::TypeId GetDefaultEmitType() const = 0;

        virtual AZStd::vector<AZ::TypeId> GetDefaultSpawnTypes() const = 0;

        virtual AZ::TypeId GetDefaultRenderType() const = 0;

        virtual AZ::Data::AssetId GetDefaultEmitterMaterialId() const = 0;
    };
    using EditorParticleSystemComponentRequestBus = AZ::EBus<EditorParticleSystemComponentRequests>;

    class EditorParticleRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual AZStd::vector<AZStd::string> GetEmitterNames() = 0;
        virtual int GetEmitterIndex(size_t index) = 0;
    };

    using EditorParticleRequestsBus = AZ::EBus<EditorParticleRequests>;

    class ParticleSourceData;
    class EditorParticleDocumentBusRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual void OnEmitterNameChanged(ParticleSourceData*) = 0;
    };

    using EditorParticleDocumentBusRequestsBus = AZ::EBus<EditorParticleDocumentBusRequests>;

    class EditorParticleOpenParticleRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void OpenParticleFile(const AZStd::string& sourcePath) = 0;
    };

    using EditorParticleOpenParticleRequestsBus = AZ::EBus<EditorParticleOpenParticleRequests>;

} // namespace OpenParticle
