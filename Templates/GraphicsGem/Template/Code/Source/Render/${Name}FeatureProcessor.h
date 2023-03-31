#pragma once

#include <${Name}/${Name}FeatureProcessorInterface.h>

namespace AZ::Render
{
    class ${Name}FeatureProcessor final
        : public ${Name}FeatureProcessorInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(${Name}FeatureProcessor, SystemAllocator)
        AZ_RTTI(${Name}FeatureProcessor, "{${Random_Uuid}}", ${Name}FeatureProcessorInterface);

        static void Reflect(AZ::ReflectContext* context);

        ${Name}FeatureProcessor() = default;
        virtual ~${Name}FeatureProcessor() = default;

        // FeatureProcessor overrides
        void Activate() override;
        void Deactivate() override;
        void Simulate(const FeatureProcessor::SimulatePacket& packet) override;

    };
}