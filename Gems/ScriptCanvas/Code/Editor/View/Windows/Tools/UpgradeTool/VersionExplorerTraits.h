#include <AzCore/EBus/EBus.h>

namespace ScriptCanvasEditor
{
    class VersionExplorerNotificationsTraits : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = VersionExplorerNotificationsTraits*;
    };
    using VersionExplorerNotificationsBus = AZ:EBus<VersionExplorerNotificationsTraits>;

    class VersionExplorerRequestsTraits : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy = AZ::EBusHandlerPolicy::Single;
    };
    using VersionExplorerRequestsBus = AZ:EBus<VersionExplorerRequestsTraits>;
}
