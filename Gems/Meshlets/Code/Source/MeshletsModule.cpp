

#include <MeshletsModuleInterface.h>
#include <MeshletsSystemComponent.h>

namespace Meshlets
{
    class MeshletsModule
        : public MeshletsModuleInterface
    {
    public:
        AZ_RTTI(MeshletsModule, "{19bbf909-a4fc-48ec-915a-316046feb2f9}", MeshletsModuleInterface);
        AZ_CLASS_ALLOCATOR(MeshletsModule, AZ::SystemAllocator, 0);
    };
}// namespace Meshlets

AZ_DECLARE_MODULE_CLASS(Gem_Meshlets, Meshlets::MeshletsModule)
