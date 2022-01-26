
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace MaterialEditor
{
    class MaterialEditorRequests
    {
    public:
        AZ_RTTI(MaterialEditorRequests, "{68bb1a2f-33b1-4906-adf3-c74c460400b1}");
        virtual ~MaterialEditorRequests() = default;
        // Put your public methods here
    };
    
    class MaterialEditorBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using MaterialEditorRequestBus = AZ::EBus<MaterialEditorRequests, MaterialEditorBusTraits>;
    using MaterialEditorInterface = AZ::Interface<MaterialEditorRequests>;

} // namespace MaterialEditor
