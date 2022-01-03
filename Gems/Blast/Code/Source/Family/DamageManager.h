/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzCore/Interface/Interface.h"

#include <Blast/BlastMaterial.h>
#include <Family/ActorTracker.h>
#include <Blast/BlastSystemBus.h>
#include <AzCore/Interface/Interface.h>

namespace Blast
{
    class BlastActor;

    // Class responsible to handling damage and how it applies in the Blast family.
    class DamageManager
    {
    public:
        struct RadialDamage
        {
        };
        struct CapsuleDamage
        {
        };
        struct ShearDamage
        {
        };
        struct TriangleDamage
        {
        };
        struct ImpactSpreadDamage
        {
        };

        template<typename T>
        using DamagePair = AZStd::pair<AZStd::unique_ptr<T>, AZStd::unique_ptr<NvBlastExtProgramParams>>;

        DamageManager(const BlastMaterial& blastMaterial, ActorTracker& actorTracker)
            : m_blastMaterial(blastMaterial)
            , m_actorTracker(actorTracker)
        {
        }

        template<class T, class... Args>
        void Damage(T damageType, float damage, const Args&... args);

        template<class T, class... Args>
        void Damage(T damageType, BlastActor& actor, float damage, const Args&... args);

    private:
        template<class T, class... Args>
        auto CalculateDamage(T damageType, BlastActor& actor, float damage, const Args&... args);

        template<typename T>
        static void DelegateToSystem(
            AZStd::unique_ptr<T> desc, AZStd::unique_ptr<NvBlastExtProgramParams> programParams);

        [[nodiscard]] DamagePair<NvBlastExtRadialDamageDesc> RadialDamageInternal(
            BlastActor& actor, float damage, const AZ::Vector3& localPosition, float minRadius, float maxRadius);
        [[nodiscard]] DamagePair<NvBlastExtShearDamageDesc> ShearDamageInternal(
            BlastActor& actor, float damage, const AZ::Vector3& localPosition, float minRadius, float maxRadius,
            const AZ::Vector3& normal);
        [[nodiscard]] DamagePair<NvBlastExtImpactSpreadDamageDesc> ImpactSpreadDamageInternal(
            BlastActor& actor, float damage, const AZ::Vector3& localPosition, float minRadius, float maxRadius);
        [[nodiscard]] DamagePair<NvBlastExtCapsuleRadialDamageDesc> CapsuleDamageInternal(
            BlastActor& actor, float damage, const AZ::Vector3& localPosition0, const AZ::Vector3& localPosition1,
            float minRadius, float maxRadius);
        [[nodiscard]] DamagePair<NvBlastExtTriangleIntersectionDamageDesc> TriangleDamageInternal(
            BlastActor& actor, float damage, const AZ::Vector3& localPosition0, const AZ::Vector3& localPosition1,
            const AZ::Vector3& localPosition2);

        static AZStd::vector<BlastActor*> OverlapSphere(
            ActorTracker& actorTracker, float radius, const AZ::Transform& pose);
        static AZStd::vector<BlastActor*> OverlapCapsule(
            ActorTracker& actorTracker, const AZ::Vector3& position0, const AZ::Vector3& position1, float maxRadius);

        static AZ::Vector3 TransformToLocal(BlastActor& actor, const AZ::Vector3& globalPosition);

        BlastMaterial m_blastMaterial;
        ActorTracker& m_actorTracker;
    };

    template<class T, class... Args>
    void DamageManager::Damage(T damageType, float damage, const Args&... args)
    {
        const float normalizedDamage = m_blastMaterial.GetNormalizedDamage(damage);
        if (normalizedDamage <= 0.f)
        {
            return;
        }

        std::tuple<const Args&...> tuple(args...);

        AZStd::vector<BlastActor*> actors;

        if constexpr (
            std::is_same<T, struct RadialDamage>::value || std::is_same<T, struct ImpactSpreadDamage>::value ||
            std::is_same<T, struct ShearDamage>::value)
        {
            actors =
                OverlapSphere(m_actorTracker, std::get<2>(tuple), AZ::Transform::CreateTranslation(std::get<0>(tuple)));
        }
        else if constexpr (std::is_same<T, struct CapsuleDamage>::value)
        {
            actors = OverlapCapsule(m_actorTracker, std::get<0>(tuple), std::get<1>(tuple), std::get<3>(tuple));
        }
        else if constexpr (std::is_same<T, struct TriangleDamage>::value)
        {
            AZStd::copy(
                m_actorTracker.GetActors().begin(), m_actorTracker.GetActors().end(), AZStd::back_inserter(actors));
        }

        for (auto* actor : actors)
        {
            auto [desc, programParams] = CalculateDamage(damageType, *actor, normalizedDamage, args...);
            DelegateToSystem(AZStd::move(desc), AZStd::move(programParams));
        }
    }

    template<class T, class... Args>
    void DamageManager::Damage(T damageType, BlastActor& actor, float damage, const Args&... args)
    {
        const float normalizedDamage = m_blastMaterial.GetNormalizedDamage(damage);
        if (normalizedDamage <= 0.f)
        {
            return;
        }

        auto [desc, programParams] = CalculateDamage(damageType, actor, normalizedDamage, args...);
        DelegateToSystem(AZStd::move(desc), AZStd::move(programParams));
    }

    template<class T, class... Args>
    auto DamageManager::CalculateDamage([[maybe_unused]] T damageType, BlastActor& actor, float damage, const Args&... args)
    {
        if constexpr (std::is_same<T, struct RadialDamage>::value)
        {
            std::tuple<const Args&...> tuple(args...);
            return RadialDamageInternal(
                actor, damage, TransformToLocal(actor, std::get<0>(tuple)), std::get<1>(tuple), std::get<2>(tuple));
        }
        else if constexpr (std::is_same<T, struct ShearDamage>::value)
        {
            std::tuple<const Args&...> tuple(args...);
            AZ::Vector3 position = TransformToLocal(actor, std::get<0>(tuple));
            AZ::Vector3 normal = TransformToLocal(actor, std::get<3>(tuple));
            return ShearDamageInternal(actor, damage, position, std::get<1>(tuple), std::get<2>(tuple), normal);
        }
        else if constexpr (std::is_same<T, struct ImpactSpreadDamage>::value)
        {
            std::tuple<const Args&...> tuple(args...);
            return ImpactSpreadDamageInternal(
                actor, damage, TransformToLocal(actor, std::get<0>(tuple)), std::get<1>(tuple), std::get<2>(tuple));
        }
        else if constexpr (std::is_same<T, struct CapsuleDamage>::value)
        {
            std::tuple<const Args&...> tuple(args...);
            return CapsuleDamageInternal(
                actor, damage, TransformToLocal(actor, std::get<0>(tuple)), TransformToLocal(actor, std::get<1>(tuple)),
                std::get<2>(tuple), std::get<3>(tuple));
        }
        else if constexpr (std::is_same<T, struct TriangleDamage>::value)
        {
            std::tuple<const Args&...> tuple(args...);
            return TriangleDamageInternal(
                actor, damage, TransformToLocal(actor, std::get<0>(tuple)), TransformToLocal(actor, std::get<1>(tuple)),
                TransformToLocal(actor, std::get<2>(tuple)));
        }
    }

    template<typename T>
    void DamageManager::DelegateToSystem(
        AZStd::unique_ptr<T> desc, AZStd::unique_ptr<NvBlastExtProgramParams> programParams)
    {
        AZ::Interface<Blast::BlastSystemRequests>::Get()->AddDamageDesc(AZStd::move(desc));
        AZ::Interface<Blast::BlastSystemRequests>::Get()->AddProgramParams(AZStd::move(programParams));
    }
} // namespace Blast
