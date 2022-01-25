/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <PythonAssetBuilder/PythonAssetBuilderBus.h>

namespace PythonAssetBuilder
{
    class PythonBuilderWorker;
    class PythonBuilderMessageSink;

    class PythonAssetBuilderSystemComponent
        : public AZ::Component
        , protected PythonAssetBuilderRequestBus::Handler
    {
    public:
        AZ_COMPONENT(PythonAssetBuilderSystemComponent, "{E2872C13-D103-4534-9A95-76A66C8DDB5D}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // PythonAssetBuilderRequestBus
        AZ::Outcome<bool, AZStd::string> RegisterAssetBuilder(const AssetBuilderSDK::AssetBuilderDesc& desc) override;
        AZ::Outcome<AZStd::string, AZStd::string> GetExecutableFolder() const override;

    private:
        using PythonBuilderWorkerPointer = AZStd::shared_ptr<PythonBuilderWorker>;
        using PythonBuilderWorkerMap = AZStd::unordered_map<AZ::Uuid, PythonBuilderWorkerPointer>;
        PythonBuilderWorkerMap m_pythonBuilderWorkerMap;
        AZStd::shared_ptr <PythonBuilderMessageSink> m_messageSink;
    };

    constexpr const char PythonBuilder[] = "PythonBuilder";
} 
