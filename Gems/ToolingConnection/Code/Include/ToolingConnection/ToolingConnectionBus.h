
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace ToolingConnection
{
    class ToolingConnectionRequests
    {
    public:
        AZ_RTTI(ToolingConnectionRequests, "{84181a04-921d-4efc-b4cc-0afb838e30c4}");
        virtual ~ToolingConnectionRequests() = default;
        // Put your public methods here
    };
    
    class ToolingConnectionBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using ToolingConnectionRequestBus = AZ::EBus<ToolingConnectionRequests, ToolingConnectionBusTraits>;
    using ToolingConnectionInterface = AZ::Interface<ToolingConnectionRequests>;

} // namespace ToolingConnection
