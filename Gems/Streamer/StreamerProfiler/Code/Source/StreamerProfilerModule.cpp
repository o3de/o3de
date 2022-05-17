

#include <StreamerProfilerModuleInterface.h>
#include <StreamerProfilerSystemComponent.h>

namespace Streamer
{
    class StreamerProfilerModule
        : public StreamerProfilerModuleInterface
    {
    public:
        AZ_RTTI(StreamerProvfilerModule, "{9d4ba060-2f72-4c52-8c32-63c788517a2f}", StreamerProfilerModuleInterface);
        AZ_CLASS_ALLOCATOR(StreamerProfilerModule, AZ::SystemAllocator, 0);
    };
}// namespace Streamer

AZ_DECLARE_MODULE_CLASS(Gem_StreamerProfiler, Streamer::StreamerProfilerModule)
