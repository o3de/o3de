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

#include <AssetMemoryAnalyzer/AssetMemoryAnalyzerBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AssetMemoryAnalyzer
{
    class FrameAnalysis;

    class AssetMemoryAnalyzerSystemComponent
        : public AZ::Component
        , protected AssetMemoryAnalyzerRequestBus::Handler
    {
    public:
        AZ_COMPONENT(AssetMemoryAnalyzerSystemComponent, "{84428E10-24FF-48A7-B5EC-0A28D25C3C68}");

        AssetMemoryAnalyzerSystemComponent();
        ~AssetMemoryAnalyzerSystemComponent();

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        static const char** GetVRAMCategories();
        static const char** GetVRAMSubCategories();

        bool IsEnabled() const;

        ////////////////////////////////////////////////////////////////////////
        // AssetMemoryAnalyzerRequestBus interface implementation
        void SetEnabled(bool enabled) override;
        void ExportCSVFile(const char* path) override;
        void ExportJSONFile(const char* path) override;
        AZStd::shared_ptr<FrameAnalysis> GetAnalysis() override;
        ////////////////////////////////////////////////////////////////////////

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        class Impl;

    private:
        AZStd::unique_ptr<Impl> m_impl;
    };
}
