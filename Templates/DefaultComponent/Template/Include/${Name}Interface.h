#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}Requests
    {
    public:
        AZ_RTTI(${SanitizedCppName}Requests, "{${Random_Uuid}}");
        virtual ~${SanitizedCppName}Requests() = default;
        // Put your public methods here

        // Put notification events here
        // void RegisterEvent(AZ::EventHandler<...> notifyHandler);
        // AZ::Event<...> m_notifyEvent1;
    };

    class ${SanitizedCppName}BusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        using BusIdType = AZ::EntityId;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        //////////////////////////////////////////////////////////////////////////
    };

    using ${SanitizedCppName}RequestBus = AZ::EBus<${SanitizedCppName}Requests, ${SanitizedCppName}BusTraits>;
    inline namespace ${SanitizedCppName}Interface
    {
        inline constexpr auto Get = [](${SanitizedCppName}BusTraits::BusIdType busId) {return ${SanitizedCppName}RequestBus::FindFirstHandler(busId); };
    }

} // namespace ${SanitizedCppName}
