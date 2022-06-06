

#include <RemoteToolsModuleInterface.h>
#include <RemoteToolsSystemComponent.h>

namespace RemoteTools
{
    class RemoteToolsModule
        : public RemoteToolsModuleInterface
    {
    public:
        AZ_RTTI(RemoteToolsModule, "{86ed333f-1f40-497f-ac31-9de31dee9371}", RemoteToolsModuleInterface);
        AZ_CLASS_ALLOCATOR(RemoteToolsModule, AZ::SystemAllocator, 0);
    };
}// namespace RemoteTools

AZ_DECLARE_MODULE_CLASS(Gem_RemoteTools, RemoteTools::RemoteToolsModule)
