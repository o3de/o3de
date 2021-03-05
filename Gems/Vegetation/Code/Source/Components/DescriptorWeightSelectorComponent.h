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
#include <Vegetation/Ebuses/DescriptorSelectorRequestBus.h>
#include <Vegetation/Ebuses/DescriptorWeightSelectorRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class DescriptorWeightSelectorConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(DescriptorWeightSelectorConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(DescriptorWeightSelectorConfig, "{382116B1-5843-42A3-915B-A3BFC3CFAB78}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        SortBehavior m_sortBehavior = SortBehavior::Unsorted;
        GradientSignal::GradientSampler m_gradientSampler;
    };

    static const AZ::Uuid DescriptorWeightSelectorComponentTypeId = "{D282AF06-4D89-4353-B4E5-92E5389C8EF7}";

    /**
    * Default placement logic for vegetation in an area
    */
    class DescriptorWeightSelectorComponent
        : public AZ::Component
        , private DescriptorSelectorRequestBus::Handler
        , private DescriptorWeightSelectorRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(DescriptorWeightSelectorComponent, DescriptorWeightSelectorComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        DescriptorWeightSelectorComponent(const DescriptorWeightSelectorConfig& configuration);
        DescriptorWeightSelectorComponent() = default;
        ~DescriptorWeightSelectorComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // DescriptorSelectorRequestBus
        void SelectDescriptors(const DescriptorSelectorParams& params, DescriptorPtrVec& descriptors) const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // DescriptorWeightSelectorRequestBus
        SortBehavior GetSortBehavior() const override;
        void SetSortBehavior(SortBehavior behavior) override;
        GradientSignal::GradientSampler& GetGradientSampler() override;

    private:
        DescriptorWeightSelectorConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}