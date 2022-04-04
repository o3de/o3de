
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
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                StreamerProfilerSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<StreamerProfilerSystemComponent>(),
            };
        }
    };
}// namespace Streamer
