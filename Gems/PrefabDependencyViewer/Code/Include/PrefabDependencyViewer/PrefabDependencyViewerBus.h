
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace PrefabDependencyViewer
{
    class PrefabDependencyViewerRequests
    {
    public:
        AZ_RTTI(PrefabDependencyViewerRequests, "{7aa2ed62-3899-471a-b0f9-4a5f868ee033}");
        virtual ~PrefabDependencyViewerRequests() = default;
        // Put your public methods here
    };
    
    class PrefabDependencyViewerBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using PrefabDependencyViewerRequestBus = AZ::EBus<PrefabDependencyViewerRequests, PrefabDependencyViewerBusTraits>;
    using PrefabDependencyViewerInterface = AZ::Interface<PrefabDependencyViewerRequests>;

} // namespace PrefabDependencyViewer
