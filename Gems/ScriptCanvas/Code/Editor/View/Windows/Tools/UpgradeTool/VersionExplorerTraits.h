#include <AzCore/EBus/EBus.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        class RequestsTraits : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy HandlerPolicy = AZ::EBusAddressPolicy::Single;
        };
        using RequestsBus = AZ::EBus<RequestsTraits>;

        class NotificationsTraits : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            using BusIdType = NotificationsTraits*;
        };
        using NotificationsBus = AZ::EBus<NotificationsTraits>;
    }
}
