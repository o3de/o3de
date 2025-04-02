/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h> ///< Platform/compiler specific defines

#if defined(AZ_MONOLITHIC_BUILD)
    #define AZF_API
    #define AZF_API_EXTERN
#else
    #if defined(AZF_EXPORTS)
        #define AZF_API           AZ_DLL_EXPORT
        #define AZF_API_EXTERN    AZ_DLL_EXPORT_EXTERN
    #else
        #define AZF_API           AZ_DLL_IMPORT
        #define AZF_API_EXTERN    AZ_DLL_IMPORT_EXTERN
    #endif
#endif

#if defined(AZ_MONOLITHIC_BUILD)
//! Declares an EBus class template, which uses EBusAddressPolicy::Single and is instantiated in a shared library, as extern using only the
//! interface argument for both the EBus Interface and BusTraits template parameters
#define AZF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(a) \
{ \
    extern template class EBus<a, b>; \
}
 
//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(a) \
namespace AZ \
{ \
    template class EBus<a, b>; \
}
 
//! Declares an EBus class template, which uses an address policy different from EBusAddressPolicy::Single and is instantiated in a shared
//! library, as extern using only the interface argument for both the EBus Interface and BusTraits template parameters
#define AZF_DECLARE_EBUS_EXTERN_MULTI_ADDRESS(a) \
namespace AZ \
{ \
    extern template class EBus<a, b>; \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(a) \
namespace AZ \
{ \
    template class EBus<a, b>; \
}

//! Declares an EBus class template, which uses EBusAddressPolicy::Single and is instantiated in a shared library, as extern with both the
//! interface and bus traits arguments
#define AZF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
    extern template class EBus<a, b>; \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
    template class EBus<a, b>; \
}

//! Declares an EBus class template, which uses an address policy different from EBusAddressPolicy::Single and is instantiated in a shared
//! library, as extern with both the interface and bus traits arguments
#define AZF_DECLARE_EBUS_EXTERN_MULTI_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
    extern template class EBus<a, b>; \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
    template class EBus<a, b>; \
}

#else

//! Declares an EBus class template, which uses EBusAddressPolicy::Single and is instantiated in a shared library, as extern using only the
//! interface argument for both the EBus Interface and BusTraits template parameters
#define AZF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(a) \
namespace AZ \
{ \
   AZF_API_EXTERN template class AZF_API EBus<a, a>; \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   AZF_API_EXTERN template class AZF_API Internal::NonIdHandler<a, a, EBus<a, a>::BusesContainer>; \
   AZF_API_EXTERN template struct AZF_API Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, a>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(a) \
namespace AZ \
{ \
   template class AZ_DLL_EXPORT EBus<a, a>; \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   template class AZ_DLL_EXPORT Internal::NonIdHandler<a, a, EBus<a, a>::BusesContainer>; \
   template struct AZ_DLL_EXPORT Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, a>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Declares an EBus class template, which uses an address policy different from EBusAddressPolicy::Single and is instantiated in a shared
//! library, as extern using only the interface argument for both the EBus Interface and BusTraits template parameters
#define AZF_DECLARE_EBUS_EXTERN_MULTI_ADDRESS(a) \
namespace AZ \
{ \
   AZF_API_EXTERN template class AZF_API EBus<a, a>;  \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   AZF_API_EXTERN template class AZF_API Internal::IdHandler<a, a, EBus<a, a>::BusesContainer>; \
   AZF_API_EXTERN template class AZF_API Internal::MultiHandler<a, a, EBus<a, a>::BusesContainer>; \
   AZF_API_EXTERN template struct AZF_API Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, a>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(a) \
namespace AZ \
{ \
   template class AZ_DLL_EXPORT EBus<a, a>; \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   template class AZ_DLL_EXPORT Internal::IdHandler<a, a, EBus<a, a>::BusesContainer>; \
   template class AZ_DLL_EXPORT Internal::MultiHandler<a, a, EBus<a, a>::BusesContainer>; \
   template struct AZ_DLL_EXPORT Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, a>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Declares an EBus class template, which uses EBusAddressPolicy::Single and is instantiated in a shared library, as extern with both the
//! interface and bus traits arguments
#define AZF_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
   AZF_API_EXTERN template class AZF_API EBus<a, b>;     \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   AZF_API_EXTERN template class AZF_API Internal::NonIdHandler<a, b, EBus<a, b>::BusesContainer>; \
   AZF_API_EXTERN template struct AZF_API Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, b>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZF_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
   template class AZ_DLL_EXPORT EBus<a, b>; \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   template class AZ_DLL_EXPORT Internal::NonIdHandler<a, b, EBus<a, b>::BusesContainer>; \
   template struct AZ_DLL_EXPORT Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, b>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Declares an EBus class template, which uses an address policy different from EBusAddressPolicy::Single and is instantiated in a shared
//! library, as extern with both the interface and bus traits arguments
#define AZF_DECLARE_EBUS_EXTERN_MULTI_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
   AZF_API_EXTERN template class AZF_API EBus<a, b>;  \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   AZF_API_EXTERN template class AZF_API Internal::IdHandler<a, b, EBus<a, b>::BusesContainer>; \
   AZF_API_EXTERN template class AZF_API Internal::MultiHandler<a, b, EBus<a, b>::BusesContainer>; \
   AZF_API_EXTERN template struct AZF_API Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, b>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZF_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
   template class AZ_DLL_EXPORT EBus<a, b>; \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   template class AZ_DLL_EXPORT Internal::IdHandler<a, b, EBus<a, b>::BusesContainer>; \
   template class AZ_DLL_EXPORT Internal::MultiHandler<a, b, EBus<a, b>::BusesContainer>; \
   template struct AZ_DLL_EXPORT Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, b>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

#endif //defined(AZ_MONOLITHIC_BUILD)
