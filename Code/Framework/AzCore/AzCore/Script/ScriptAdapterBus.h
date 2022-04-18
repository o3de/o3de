/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: AdapterEvents.
 * Create: 2021-06-28
 */
#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/Policies.h>
#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class AdapterEvents : public AZ::EBusTraits
    {
    public:
        // Only one Handler can be online at any one time per address
        static const AZ::EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;

        virtual void SaveLuaFile() = 0;
        virtual void DeleteLuaFile() = 0;
    };
    using AdapterBus = AZ::EBus<AdapterEvents>;
}