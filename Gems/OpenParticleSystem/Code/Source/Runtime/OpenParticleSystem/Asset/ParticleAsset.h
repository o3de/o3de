/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <OpenParticleSystem/Asset/ParticleArchive.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <OpenParticleSystem/ParticleConfig.h>

namespace OpenParticle
{
    struct ParticleLOD
    {
        static void Reflect(AZ::ReflectContext* context);
        float m_distance;
        AZStd::vector<AZ::u32> m_emitters;
    };

    struct ParticleCurve
    {
        static void Reflect(AZ::ReflectContext* context);
        SimuCore::ParticleCore::CurveExtrapMode extrapModeLeft;
        SimuCore::ParticleCore::CurveExtrapMode extrapModeRight;
        float valueFactor;
        float timeFactor;
        SimuCore::ParticleCore::CurveTickMode tickMode;
        AZStd::vector<SimuCore::ParticleCore::KeyPoint> keyPoints;
    };

    struct ParticleRandom
    {
        static void Reflect(AZ::ReflectContext* context);
        float min;
        float max;
        SimuCore::ParticleCore::RandomTickMode tickMode;
    };

    struct ParticleDistribution
    {
        static void Reflect(AZ::ReflectContext* context);
        AZStd::vector<ParticleRandom> randoms;
        AZStd::vector<ParticleCurve> curves;
    };

    class ParticleAsset final
        : public AZ::Data::AssetData
        , public AZ::Data::AssetBus::Handler
    {
    public:
        friend class ParticleAssetData;

        AZ_RTTI(OpenParticle::ParticleAsset, "{86cf03d6-d1b2-4a5c-a4b8-1d3bdeb5a507}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(ParticleAsset, AZ::SystemAllocator, 0);

        static const char* DisplayName;
        static const char* Group;
        static const char* Extension;

        static void Reflect(AZ::ReflectContext* context);

        ParticleAsset();
        virtual ~ParticleAsset();

        void SetReady();

        ParticleArchive& GetParticleArchive();

        AZStd::vector<ParticleLOD>& GetLODs();

        ParticleDistribution& GetDistribution();

    private:
        ParticleArchive m_particleArchive;
        AZStd::vector<ParticleLOD> m_lods;
        ParticleDistribution m_distribution;
    };
} // namespace OpenParticle

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ParticleLOD, "{FF949B5B-971E-4082-9BED-958A1E7689FD}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ParticleRandom, "{080AD23D-73EA-47C5-8621-BF64B45FF73B}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ParticleCurve, "{fcf88e1f-60bf-432d-a848-d11b3ce355f0}");
    AZ_TYPE_INFO_SPECIALIZE(OpenParticle::ParticleDistribution, "{9e77a6a2-2b7b-43fb-89ed-2399079319e9}");
} // namespace AZ
