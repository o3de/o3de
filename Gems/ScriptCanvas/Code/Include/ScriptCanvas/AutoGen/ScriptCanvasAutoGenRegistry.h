/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

 //!
 //! Script Canvas nodes need to be registered into O3DE in order to be
 //! available to the different systems. Because ScriptCanvas nodes are
 //! components, this registration system allows the collecting
 //! AZ::ComponentDescriptors in order to register them with the
 //! application.
 //!
 //! The goal is to make ScriptCanvas node creation as straightforward
 //! as possible. By creating a small helper factory that registers
 //! nodes into an AZ::EnvironmentVariable we remove all registration
 //! related boilerplate code, developers need simply to add their
 //! ScriptCanvas autogen drivers, header, and source files to the
 //! CMake files list in order for ScriptCanvas nodes to be registered.
 //!

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ComponentDescriptor;
    class ReflectContext;
}

namespace ScriptCanvas
{
    class Node;
}



#define CONCATENATION_IMPL(a, b) a##b
#define CONCATENATION(a, b) CONCATENATION_IMPL(a, b)

#define SCRIPTCANVAS_REGISTER_EXTERN(cls) \
    template <typename T> \
    extern void RegistrationCall(); \
    template <> \
    void RegistrationCall<cls>(); \
    static const int CONCATENATION(temp, __COUNTER__ ) = \
        RegistrationHelper::Register<cls>(&RegistrationCall<cls>)

#define SCRIPTCANVAS_REGISTER(cls) \
    template <> \
    void RegistrationCall<cls>()

namespace RegistrationHelper
{
    template <typename T>
    inline int Register(void(*f)())
    {
        static const int s = [&f]() {
            f();
            return 0;
        }();
        return s;
    }
}

//! Holds the complete list of Script Canvas nodes (grammar and nodeables), all
//! gems and modules will register their nodes into this model
class ScriptCanvasModel
{
public:

    static ScriptCanvasModel& Instance();

    ScriptCanvasModel() = default;
    ~ScriptCanvasModel() = default;

    void Init();
    void Release();
    void RemoveDescriptor(AZ::ComponentDescriptor* descriptor);

    using ReflectFunction = AZStd::function<void(AZ::ReflectContext*)>;
    bool RegisterReflection(const AZStd::string& name, ReflectFunction reflect, AZ::ComponentDescriptor* descriptor = nullptr);

    void Reflect(AZ::ReflectContext* context);

    const AZStd::list<AZ::ComponentDescriptor*>& GetDescriptors() const { return m_descriptors; }

private:

    AZStd::list<AZ::ComponentDescriptor*> m_descriptors;

    AZStd::unordered_map<AZStd::string, ReflectFunction> m_registeredReflections;

    bool m_verbose = false;
};

