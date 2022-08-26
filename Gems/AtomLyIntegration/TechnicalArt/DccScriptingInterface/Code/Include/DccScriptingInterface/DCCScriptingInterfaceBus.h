
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace DccScriptingInterface
{
    class DccScriptingInterfaceRequests
    {
    public:
        AZ_RTTI(DccScriptingInterfaceRequests, "{9A315465-4463-400B-8576-0A4386398013}");
        virtual ~DccScriptingInterfaceRequests() = default;
        // Put your public methods here
    };
    
    class DccScriptingInterfaceBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using DccScriptingInterfaceRequestBus = AZ::EBus<DccScriptingInterfaceRequests, DccScriptingInterfaceBusTraits>;
    using DccScriptingInterfaceInterface = AZ::Interface<DccScriptingInterfaceRequests>;

} // namespace DccScriptingInterface
