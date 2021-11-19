
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace ${Name}
{
    class ${Name}Requests
    {
    public:
        AZ_RTTI(${Name}Requests, "{a3801fac-eb63-4f0c-9fed-e6233b3d8524}");
        virtual ~${Name}Requests() = default;
        // Put your public methods here
    };

    class ${Name}BusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using ${Name}RequestBus = AZ::EBus<${Name}Requests, ${Name}BusTraits>;
    using ${Name}Interface = AZ::Interface<${Name}Requests>;

} // namespace ${Name}
