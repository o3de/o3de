
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace Javascript
{
    class JavascriptRequests
    {
    public:
        AZ_RTTI(JavascriptRequests, "{34a3df39-1501-499f-9196-110026b49299}");
        virtual ~JavascriptRequests() = default;
        // Put your public methods here
    };
    
    class JavascriptBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using JavascriptRequestBus = AZ::EBus<JavascriptRequests, JavascriptBusTraits>;
    using JavascriptInterface = AZ::Interface<JavascriptRequests>;

} // namespace Javascript
