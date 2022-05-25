
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace RecastNavigation
{
    class RecastNavigationRequests
    {
    public:
        AZ_RTTI(RecastNavigationRequests, "{d1c2f552-287d-4aa1-a5b8-5b234c9106f3}");
        virtual ~RecastNavigationRequests() = default;
        // Put your public methods here
    };
    
    class RecastNavigationBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using RecastNavigationRequestBus = AZ::EBus<RecastNavigationRequests, RecastNavigationBusTraits>;
    using RecastNavigationInterface = AZ::Interface<RecastNavigationRequests>;

} // namespace RecastNavigation
