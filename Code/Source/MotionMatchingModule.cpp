

#include <MotionMatchingModuleInterface.h>
#include <MotionMatchingSystemComponent.h>

namespace MotionMatching
{
    class MotionMatchingModule
        : public MotionMatchingModuleInterface
    {
    public:
        AZ_RTTI(MotionMatchingModule, "{cf4381d1-0207-4ef8-85f0-6c88ec28a7b6}", MotionMatchingModuleInterface);
        AZ_CLASS_ALLOCATOR(MotionMatchingModule, AZ::SystemAllocator, 0);
    };
}// namespace MotionMatching

AZ_DECLARE_MODULE_CLASS(Gem_MotionMatching, MotionMatching::MotionMatchingModule)
