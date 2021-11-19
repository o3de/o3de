
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "${Name}SystemComponent.h"

namespace ${Name}
{
    class ${Name}Module
        : public AZ::Module
    {
    public:
        AZ_RTTI(${Name}Module, "{1498c951-971a-4f09-b7c4-b675e4714949}", AZ::Module);
        AZ_CLASS_ALLOCATOR(${Name}Module, AZ::SystemAllocator, 0);

        ${Name}Module()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                ${Name}SystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<${Name}SystemComponent>(),
            };
        }
    };
}// namespace ${Name}

AZ_DECLARE_MODULE_CLASS(Gem_${Name}, ${Name}::${Name}Module)
