

#include <JavascriptModuleInterface.h>
#include <JavascriptSystemComponent.h>

namespace Javascript
{
    class JavascriptModule
        : public JavascriptModuleInterface
    {
    public:
        AZ_RTTI(JavascriptModule, "{996d5f8b-c41d-4644-a0b7-8d439c5fbd3a}", JavascriptModuleInterface);
        AZ_CLASS_ALLOCATOR(JavascriptModule, AZ::SystemAllocator, 0);
    };
}// namespace Javascript

AZ_DECLARE_MODULE_CLASS(Gem_Javascript, Javascript::JavascriptModule)
