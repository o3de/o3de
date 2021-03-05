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
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <SurfaceData/SurfaceDataTagEnumeratorRequestBus.h>
#include <SurfaceData/SurfaceDataTagEnumeratorRequestBus.h>
#include <Vegetation/Ebuses/DescriptorListCombinerRequestBus.h>
#include <Vegetation/Ebuses/DescriptorProviderRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class DescriptorListCombinerConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(DescriptorListCombinerConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(DescriptorListCombinerConfig, "{A62E9C87-093C-4534-AB48-DEF8EC80C190}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        AZStd::vector<AZ::EntityId> m_descriptorProviders;

        size_t GetNumDescriptors() const;
        AZ::EntityId GetDescriptorEntityId(int index) const;
        void RemoveDescriptorEntityId(int index);
        void SetDescriptorEntityId(int index, AZ::EntityId entityId);
        void AddDescriptorEntityId(AZ::EntityId entityId);
    };

    static const AZ::Uuid DescriptorListCombinerComponentTypeId = "{1A1267EA-8A29-42AE-A385-BB0E60899EEF}";

    /**
    * Retrieve a list of descriptors from multiple providers
    */
    class DescriptorListCombinerComponent
        : public AZ::Component
        , private DescriptorProviderRequestBus::Handler
        , private DescriptorListCombinerRequestBus::Handler
        , private SurfaceData::SurfaceDataTagEnumeratorRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(DescriptorListCombinerComponent, DescriptorListCombinerComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        DescriptorListCombinerComponent(const DescriptorListCombinerConfig& configuration);
        DescriptorListCombinerComponent() = default;
        ~DescriptorListCombinerComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // DescriptorProviderRequestBus
        void GetDescriptors(DescriptorPtrVec& descriptors) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceData::SurfaceDataTagEnumeratorRequestBus
        void GetInclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags, bool& includeAll) const override;
        void GetExclusionSurfaceTags(SurfaceData::SurfaceTagVector& tags) const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // DescriptorListCombinerRequestBus
        size_t GetNumDescriptors() const override;
        AZ::EntityId GetDescriptorEntityId(int index) const override;
        void RemoveDescriptorEntityId(int index) override;
        void SetDescriptorEntityId(int index, AZ::EntityId entityId) override;
        void AddDescriptorEntityId(AZ::EntityId entityId) override;

    private:
        DescriptorListCombinerConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;

        void SetupDependencies();
    };
}