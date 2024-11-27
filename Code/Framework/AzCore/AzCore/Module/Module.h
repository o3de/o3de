/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Console/ConsoleFunctor.h>
#include <AzCore/Name/NameDictionary.h>

namespace AZ
{
    class ReflectContext;
    class Entity;

    /**
     * AZ::Module enables static and dynamic modules (aka LIBs and DLLs) to
     * connect with the running \ref AZ::ComponentApplication.
     *
     * Each module should contain a class which inherits from AZ::Module.
     * This class must perform tasks such as reflecting the classes within
     * the module and adding critical components to the system entity.
     */
    class Module
    {
    public:
        AZ_RTTI(Module, "{59682E0E-731F-4361-BC0B-039BC5376CA1}");
        AZ_CLASS_ALLOCATOR(Module, AZ::SystemAllocator);

        /**
         * Override to register the module's ComponentDescriptors.
         *
         * Example:
         * \code
         * MyModule::MyModule()
         *     : AZ::Module()
         * {
         *     m_descriptors.insert(m_descriptors.end(), {
         *         MyAwesomeComponent::CreateDescriptor(),
         *     });
         * };
         * \endcode
         */
        Module();
        /**
         * Releases all descriptors in m_descriptors.
         */
        virtual ~Module();

        // no copy
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;

        /**
         * DO NOT OVERRIDE. This method will return in the future, but at this point things reflected here are not unreflected for all ReflectContexts (Serialize, Editor, Network, Script, etc.)
         * Place all calls to non-component reflect functions inside of a component reflect function to ensure that your types are unreflected.
         */
        void Reflect(AZ::ReflectContext*) {}

        /**
         * Override to require specific components on the system entity.
         * \return The type-ids of required components.
         */
        virtual ComponentTypeList GetRequiredSystemComponents() const { return ComponentTypeList(); }

        // Non virtual, just registers component descriptors
        void RegisterComponentDescriptors();

        AZStd::list<AZ::ComponentDescriptor*> GetComponentDescriptors() const { return m_descriptors; }

    protected:
        AZStd::list<AZ::ComponentDescriptor*> m_descriptors;
    };
} // namespace AZ


#if defined(AZ_MONOLITHIC_BUILD)
// Calls an _IMPL macro to allow the MODULE_NAME and MODULE_CLASSNAME to resolve any macros
#   define AZ_DECLARE_MODULE_CLASS_IMPL(MODULE_NAME, MODULE_CLASSNAME) \
    extern "C" AZ::Module * CreateModuleClass_##MODULE_NAME() { return aznew MODULE_CLASSNAME; }
#else
#   define AZ_DECLARE_MODULE_CLASS_IMPL(MODULE_NAME, MODULE_CLASSNAME)                           \
    AZ_DECLARE_MODULE_INITIALIZATION                                                             \
    extern "C" AZ_DLL_EXPORT AZ::Module* CreateModuleClass()                                     \
    {                                                                                            \
        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)               \
        {                                                                                        \
             console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());           \
             console->ExecuteDeferredConsoleCommands();                                          \
        }                                                                                        \
        if (AZ::NameDictionary::IsReady())                                                       \
        {                                                                                        \
            AZ::NameDictionary::Instance().LoadDeferredNames(AZ::Name::GetDeferredHead());       \
        }                                                                                        \
        return aznew MODULE_CLASSNAME;                                                           \
    }                                                                                            \
    extern "C" AZ_DLL_EXPORT void DestroyModuleClass(AZ::Module* module) { delete module; }
#endif

/// \def AZ_DECLARE_MODULE_CLASS(MODULE_NAME, MODULE_CLASSNAME)
/// For modules which define a \ref AZ::Module class.
/// This macro provides default implementations of functions that \ref
/// AZ::ComponentApplication expects to find in a module.
///
/// \note
/// For modules with no \ref AZ::Module class, use \ref AZ_DECLARE_MODULE_INITIALIZATION instead.
///
/// \note
/// In a monolithic build, all modules are compiled as static libraries.
/// In a non-monolithic build, all modules are compiled as dynamic libraries.
///
/// \param MODULE_NAME      Name of module.
/// \param MODULE_CLASSNAME Name of AZ::Module class (include namespace).
///
/// Execute any deferred console commands after linking any new deferred functors
/// This allows deferred console commands defined within the module to now execute
/// at this point now that the module has been loaded
#define AZ_DECLARE_MODULE_CLASS(MODULE_NAME, MODULE_CLASSNAME) AZ_DECLARE_MODULE_CLASS_IMPL(MODULE_NAME, MODULE_CLASSNAME)
