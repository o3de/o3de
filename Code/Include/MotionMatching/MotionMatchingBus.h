
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace MotionMatching
{
    class MotionMatchingRequests
    {
    public:
        AZ_RTTI(MotionMatchingRequests, "{b08f73cc-a922-49ef-8c0e-07166b43ea65}");
        virtual ~MotionMatchingRequests() = default;
        // Put your public methods here
    };
    
    class MotionMatchingBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using MotionMatchingRequestBus = AZ::EBus<MotionMatchingRequests, MotionMatchingBusTraits>;
    using MotionMatchingInterface = AZ::Interface<MotionMatchingRequests>;

} // namespace MotionMatching
