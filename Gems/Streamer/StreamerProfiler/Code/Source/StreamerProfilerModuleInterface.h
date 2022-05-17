
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <StreamerProfilerSystemComponent.h>

namespace Streamer
{
    class StreamerProfilerModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(StreamerProfilerModuleInterface, "{27fc3f53-8e85-43be-b121-3fef086f8f22}", AZ::Module);
        AZ_CLASS_ALLOCATOR(StreamerProfilerModuleInterface, AZ::SystemAllocator, 0);

        StreamerProfilerModuleInterface()
        {
            m_descriptors.insert(m_descriptors.end(), {
                StreamerProfilerSystemComponent::CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<StreamerProfilerSystemComponent>(),
            };
        }
    };
}// namespace Streamer
