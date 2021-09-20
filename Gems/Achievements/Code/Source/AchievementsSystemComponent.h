/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <Achievements/AchievementRequestBus.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Component/TickBus.h>

namespace Achievements
{
    ////////////////////////////////////////////////////////////////////////////////////////
    // A system component providing an interface to query and unlock achievements
    class AchievementsSystemComponent
        : public AZ::Component
        , protected AchievementRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////
        // Component setup
        AZ_COMPONENT(AchievementsSystemComponent, "{07CFF8FE-668E-476A-95D9-A3B0CCCE2414}");

        ////////////////////////////////////////////////////////////////////////////////////////
        // Component overrides
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////////////////////
        // AchievementsRequestBus interface implementation
        void UnlockAchievement(const UnlockAchievementParams& params) override;
        void QueryAchievementDetails(const QueryAchievementParams& params) override;
    
    public:
        ////////////////////////////////////////////////////////////////////////////////////////
        // Base class for platform specific implementations
        class Implementation
        {
        public:
            AZ_CLASS_ALLOCATOR(Implementation, AZ::SystemAllocator, 0);

            static Implementation* Create(AchievementsSystemComponent& achievementsSystemComponent);
            Implementation(AchievementsSystemComponent& achievementsSystemComponent);

            AZ_DISABLE_COPY_MOVE(Implementation);

            virtual ~Implementation();

            virtual void UnlockAchievement(const UnlockAchievementParams& params) = 0;
            virtual void QueryAchievementDetails(const QueryAchievementParams& params) = 0;

            static void OnUnlockAchievementComplete(const UnlockAchievementParams& params);
            static void OnQueryAchievementDetailsComplete(const QueryAchievementParams& params, const AchievementDetails& details);

            AchievementsSystemComponent& m_achievementsSystemComponent;
        };

        private:
            ////////////////////////////////////////////////////////////////////////////////////////
            // Private pointer to the platform specific implementation
            AZStd::unique_ptr<Implementation> m_pimpl;
    };
}
