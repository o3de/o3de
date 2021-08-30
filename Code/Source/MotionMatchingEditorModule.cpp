
#include <MotionMatchingModuleInterface.h>
#include <MotionMatchingEditorSystemComponent.h>

namespace MotionMatching
{
    class MotionMatchingEditorModule
        : public MotionMatchingModuleInterface
    {
    public:
        AZ_RTTI(MotionMatchingEditorModule, "{cf4381d1-0207-4ef8-85f0-6c88ec28a7b6}", MotionMatchingModuleInterface);
        AZ_CLASS_ALLOCATOR(MotionMatchingEditorModule, AZ::SystemAllocator, 0);

        MotionMatchingEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                MotionMatchingEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<MotionMatchingEditorSystemComponent>(),
            };
        }
    };
}// namespace MotionMatching

AZ_DECLARE_MODULE_CLASS(Gem_MotionMatching, MotionMatching::MotionMatchingEditorModule)
