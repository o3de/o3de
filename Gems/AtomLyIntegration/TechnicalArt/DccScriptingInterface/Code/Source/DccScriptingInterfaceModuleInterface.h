
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace DccScriptingInterface
{
    class DccScriptingInterfaceModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(DccScriptingInterfaceModuleInterface, "{BCD5EF47-6B00-4EA6-B28E-19F464316414}", AZ::Module);
        AZ_CLASS_ALLOCATOR(DccScriptingInterfaceModuleInterface, AZ::SystemAllocator, 0);

        DccScriptingInterfaceModuleInterface()
        {
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
            };
        }
    };
}// namespace DccScriptingInterface
