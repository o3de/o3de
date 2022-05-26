

#include <ToolingConnectionModuleInterface.h>
#include <ToolingConnectionSystemComponent.h>

namespace ToolingConnection
{
    class ToolingConnectionModule
        : public ToolingConnectionModuleInterface
    {
    public:
        AZ_RTTI(ToolingConnectionModule, "{86ed333f-1f40-497f-ac31-9de31dee9371}", ToolingConnectionModuleInterface);
        AZ_CLASS_ALLOCATOR(ToolingConnectionModule, AZ::SystemAllocator, 0);
    };
}// namespace ToolingConnection

AZ_DECLARE_MODULE_CLASS(Gem_ToolingConnection, ToolingConnection::ToolingConnectionModule)
