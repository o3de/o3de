/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h> ///< Platform/compiler specific defines
#include <AzCore/EBus/EBus.h>

#if defined(AZ_MONOLITHIC_BUILD)
    #define AZCORE_API
    #define AZCORE_API_EXTERN
#else
    #if defined(AZCORE_EXPORTS)
        #define AZCORE_API        AZ_DLL_EXPORT
        #define AZCORE_API_EXTERN AZ_DLL_EXPORT_EXTERN
    #else
        #define AZCORE_API        AZ_DLL_IMPORT
        #define AZCORE_API_EXTERN AZ_DLL_IMPORT_EXTERN
    #endif
#endif

// The following allow heavily-used busses to be declared extern, in order to speed up compile time where the same header
// with the same bus is included in many different source files.
// to use it, declare the EBus extern using DECLARE_EBUS_EXTERN or DECLARE_EBUS_EXTERN_WITH_TRAITS in the header file
// and then use DECLARE_EBUS_INSTANTIATION or DECLARE_EBUS_INSTANTIATION_WITH_TRAITS in a file that everything that includes the header
// will link to (for example, in a static library, dynamic library with export library, or .inl that everyone must include in a compile unit).

// The following must be declared AT GLOBAL SCOPE and the namespace AZ is assumed due to the rule that extern template declarations must occur
// in their enclosing scope.

#if defined(AZ_MONOLITHIC_BUILD)

//! Declares an EBus class template, which uses EBusAddressPolicy::Single and is instantiated in a shared library, as extern using only the
//! interface argument for both the EBus Interface and BusTraits template parameters
#define AZ_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(a) \
{ \
    extern template class EBus<a, b>; \
}
 
//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(a) \
namespace AZ \
{ \
    template class EBus<a, b>; \
}
 
//! Declares an EBus class template, which uses an address policy different from EBusAddressPolicy::Single and is instantiated in a shared
//! library, as extern using only the interface argument for both the EBus Interface and BusTraits template parameters
#define AZ_DECLARE_EBUS_EXTERN_MULTI_ADDRESS(a) \
namespace AZ \
{ \
    extern template class EBus<a, b>; \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(a) \
namespace AZ \
{ \
    template class EBus<a, b>; \
}

//! Declares an EBus class template, which uses EBusAddressPolicy::Single and is instantiated in a shared library, as extern with both the
//! interface and bus traits arguments
#define AZ_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
    extern template class EBus<a, b>; \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
    template class EBus<a, b>; \
}

//! Declares an EBus class template, which uses an address policy different from EBusAddressPolicy::Single and is instantiated in a shared
//! library, as extern with both the interface and bus traits arguments
#define AZ_DECLARE_EBUS_EXTERN_MULTI_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
    extern template class EBus<a, b>; \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
    template class EBus<a, b>; \
}

#else

//! Declares an EBus class template, which uses EBusAddressPolicy::Single and is instantiated in a shared library, as extern using only the
//! interface argument for both the EBus Interface and BusTraits template parameters
#define AZ_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(a) \
namespace AZ \
{ \
   AZCORE_API_EXTERN template class AZCORE_API EBus<a, a>; \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   AZCORE_API_EXTERN template class AZCORE_API Internal::NonIdHandler<a, a, EBus<a, a>::BusesContainer>; \
   AZCORE_API_EXTERN template struct AZCORE_API Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, a>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS(a) \
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
#define AZ_DECLARE_EBUS_EXTERN_MULTI_ADDRESS(a) \
namespace AZ \
{ \
   AZCORE_API_EXTERN template class AZCORE_API EBus<a, a>;  \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   AZCORE_API_EXTERN template class AZCORE_API Internal::IdHandler<a, a, EBus<a, a>::BusesContainer>; \
   AZCORE_API_EXTERN template class AZCORE_API Internal::MultiHandler<a, a, EBus<a, a>::BusesContainer>; \
   AZCORE_API_EXTERN template struct AZCORE_API Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, a>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS(a) \
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
#define AZ_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
   AZCORE_API_EXTERN template class AZCORE_API EBus<a, b>;     \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   AZCORE_API_EXTERN template class AZCORE_API Internal::NonIdHandler<a, b, EBus<a, b>::BusesContainer>; \
   AZCORE_API_EXTERN template struct AZCORE_API Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, b>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZ_DECLARE_EBUS_INSTANTIATION_SINGLE_ADDRESS_WITH_TRAITS(a, b) \
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
#define AZ_DECLARE_EBUS_EXTERN_MULTI_ADDRESS_WITH_TRAITS(a, b) \
namespace AZ \
{ \
   AZCORE_API_EXTERN template class AZCORE_API EBus<a, b>;  \
   AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
   AZCORE_API_EXTERN template class AZCORE_API Internal::IdHandler<a, b, EBus<a, b>::BusesContainer>; \
   AZCORE_API_EXTERN template class AZCORE_API Internal::MultiHandler<a, b, EBus<a, b>::BusesContainer>; \
   AZCORE_API_EXTERN template struct AZCORE_API Internal::EBusCallstackStorage<Internal::CallstackEntryBase<a, b>, true>; \
   AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING \
}

//! Explicitly instantiates an EBus which was declared with the function directly above
#define AZ_DECLARE_EBUS_INSTANTIATION_MULTI_ADDRESS_WITH_TRAITS(a, b) \
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
