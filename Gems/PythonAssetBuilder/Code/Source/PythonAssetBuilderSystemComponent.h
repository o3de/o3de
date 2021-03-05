/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

#include <PythonAssetBuilder/PythonAssetBuilderBus.h>
#include <PythonAssetBuilder/PythonBuilderRequestBus.h>

namespace PythonAssetBuilder
{
    class PythonBuilderWorker;
    class PythonBuilderMessageSink;

    class PythonAssetBuilderSystemComponent
        : public AZ::Component
        , protected PythonAssetBuilderRequestBus::Handler
        , protected PythonBuilderRequestBus::Handler
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

        // PythonBuilderRequestBus
        AZ::Outcome<AZ::EntityId, AZStd::string> CreateEditorEntity(const AZStd::string& name) override;
        AZ::Outcome<AZ::Data::AssetType, AZStd::string> WriteSliceFile(
            AZStd::string_view filename,
            AZStd::vector<AZ::EntityId> entityList,
            bool makeDynamic) override;

    private:
        using PythonBuilderWorkerPointer = AZStd::shared_ptr<PythonBuilderWorker>;
        using PythonBuilderWorkerMap = AZStd::unordered_map<AZ::Uuid, PythonBuilderWorkerPointer>;
        PythonBuilderWorkerMap m_pythonBuilderWorkerMap;
        AZStd::shared_ptr <PythonBuilderMessageSink> m_messageSink;
    };

    constexpr const char PythonBuilder[] = "PythonBuilder";
} 
