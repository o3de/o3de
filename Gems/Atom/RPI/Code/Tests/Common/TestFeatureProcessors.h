/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace UnitTest
{
    // A feature processor which has scene notification enabled
    class TestFeatureProcessor1 final
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(TestFeatureProcessor1, "{CCC3EB15-D80E-4F5A-93F4-B0F993A5E7F5}", AZ::RPI::FeatureProcessor);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestFeatureProcessor1, AZ::RPI::FeatureProcessor>()
                    ->Version(1)
                    ;
            }
        }

        // Variables
        // notification counters
        int m_pipelineCount = 0;
        int m_viewSetCount = 0;
        int m_pipelineChangedCount = 0;
        AZ::RPI::RenderPipeline* m_lastPipeline = nullptr;

        // FeatureProcessor overrides...
        void Activate() override
        {
            EnableSceneNotification();
        }

        void Deactivate() override
        {
            DisableSceneNotification();
        }

        void Render(const RenderPacket&)  override {};

        // Overrides for scene notification bus handler
        void OnRenderPipelineAdded(AZ::RPI::RenderPipelinePtr pipeline) override
        {
            m_pipelineCount++;
            m_lastPipeline = pipeline.get();
        }

        void OnRenderPipelineRemoved(AZ::RPI::RenderPipeline* pipeline) override
        {
            m_pipelineCount--;
            m_lastPipeline = pipeline;
        }

        void OnRenderPipelinePersistentViewChanged(AZ::RPI::RenderPipeline* renderPipeline, AZ::RPI::PipelineViewTag viewTag, AZ::RPI::ViewPtr newView, AZ::RPI::ViewPtr previousView) override
        {
            m_viewSetCount++;
            m_lastPipeline = renderPipeline;
        }

        void OnRenderPipelinePassesChanged(AZ::RPI::RenderPipeline* renderPipeline) override
        {
            m_pipelineChangedCount++;
            m_lastPipeline = renderPipeline;
        }

    }; 
    
    class TestFeatureProcessor2 final
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(TestFeatureProcessor2, "{1DB411E1-0C0D-4FA1-A0AA-9935CFF671D5}", AZ::RPI::FeatureProcessor);
        
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestFeatureProcessor2, AZ::RPI::FeatureProcessor>()
                    ->Version(1)
                    ;
            }
        }
        void Render(const RenderPacket&)  override {};
    };

    class TestFeatureProcessorInterface
        : public AZ::RPI::FeatureProcessor
    {
    public:
        AZ_RTTI(TestFeatureProcessorInterface, "{F1766CA0-B3A6-40F5-8ADE-5E93EB0DDE9D}", AZ::RPI::FeatureProcessor);

        virtual void SetValue(int value) = 0;
        virtual int GetValue() const = 0;

    };

    class TestFeatureProcessorImplementation final
        : public TestFeatureProcessorInterface
    {
    public:
        AZ_RTTI(TestFeatureProcessorImplementation, "{2FEB6299-A03E-4341-9234-47786F5A53C3}", TestFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestFeatureProcessorImplementation, TestFeatureProcessorInterface>()
                    ->Version(1)
                    ;
            }
        }
        void Render(const RenderPacket&)  override {};
        void SetValue(int value) override { m_value = value; }
        int GetValue() const override { return m_value; };
    private:
        int m_value = 0;
    };

    class TestFeatureProcessorImplementation2 final
        : public TestFeatureProcessorInterface
    {
    public:
        AZ_RTTI(TestFeatureProcessorImplementation2, "{48E98E91-373E-43D4-BFD2-991B9FF8CEE8}", TestFeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestFeatureProcessorImplementation2, TestFeatureProcessorInterface>()
                    ->Version(1)
                    ;
            }
        }
        void Render(const RenderPacket&)  override {};
        void SetValue(int value) override { m_value = value; }
        int GetValue() const override { return m_value; };
    private:
        int m_value = 0;
    };
}  // namespace UnitTest
