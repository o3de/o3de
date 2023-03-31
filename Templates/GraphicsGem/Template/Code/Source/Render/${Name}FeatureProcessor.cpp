#include "${Name}FeatureProcessor.h"

namespace AZ::Render
{
    void ${Name}FeatureProcessor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext
                ->Class<${Name}FeatureProcessor, FeatureProcessor>()
                ;
        }
    }

    void ${Name}FeatureProcessor::Activate()
    {

    }

    void ${Name}FeatureProcessor::Deactivate()
    {
        
    }

    void ${Name}FeatureProcessor::Simulate([[maybe_unused]] const FeatureProcessor::SimulatePacket& packet)
    {
        
    }    
}