/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Quality/QualitySystemBus.h>

namespace AzFramework
{
    class QualityCVarGroup;

    // QualitySystemComponent manages quality groups, levels and cvar settings
    // stored in the SettingsRegistry
    class QualitySystemComponent final
        : public AZ::Component
        , public QualitySystemEvents::Bus::Handler
    {
    public:
        AZ_COMPONENT(QualitySystemComponent, "{CA269E6A-A420-4B68-93E9-2E09A604D29A}", AZ::Component);

        QualitySystemComponent();
        ~QualitySystemComponent() override;

        AZ_DISABLE_COPY(QualitySystemComponent);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzFramework::QualitySystemBus
        void LoadDefaultQualityGroup(QualityLevel qualityLevel = QualityLevel::LevelFromDeviceRules) override;

    private:
        void EvaluateDeviceRules();
        void RegisterCvars();

        AZStd::string m_defaultGroupName;
        AZStd::vector<AZStd::unique_ptr<QualityCVarGroup>> m_settingsGroupCVars;
    };
} // AzFramework

