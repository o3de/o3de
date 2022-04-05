
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace ROS2
{
    class ROS2Requests
    {
    public:
        AZ_RTTI(ROS2Requests, "{fa9316fc-3eff-46e7-83de-c07a29c94109}");
        virtual ~ROS2Requests() = default;
        // Put your public methods here
    };
    
    class ROS2BusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using ROS2RequestBus = AZ::EBus<ROS2Requests, ROS2BusTraits>;
    using ROS2Interface = AZ::Interface<ROS2Requests>;

} // namespace ROS2
