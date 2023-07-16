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

class IScriptCanvasNodeFactory;

//! Abstract class that provides the information need to register
//! Script Canvas nodes
class IScriptCanvasNodeFactory
{
public:

    virtual const AZStd::string GetGemOrModuleName() const = 0;
    virtual const AZStd::string GetName() const = 0;
    virtual const AZStd::string GetID() const = 0;

    virtual AZ::ComponentDescriptor* GetDescriptor() const = 0;
};

//! Holds the complete list of Script Canvas nodes (grammar and nodeables), all
//! gems and modules will register their nodes into this model
class ScriptCanvasModel
{
public:

    static ScriptCanvasModel& Instance();

    ScriptCanvasModel() = default;
    virtual ~ScriptCanvasModel()
    {
        m_registeredReflections.clear();
        m_registry.clear();
    }

    void Init();
    void Register(const char* gemOrModuleName, const char* typeName, const char* typeHash, IScriptCanvasNodeFactory* factory);

    using ReflectFunction = AZStd::function<void(AZ::ReflectContext*)>;
    void RegisterReflection(const AZStd::string& name, ReflectFunction reflect);

    void Reflect(AZ::ReflectContext* context);

    const AZ::ComponentDescriptor* GetDescriptor(const char* typehash) const;

private:

    // Minimum data required for runtime operations
    struct Entry
    {
        AZStd::string m_gemOrModuleName;
        IScriptCanvasNodeFactory* m_factory;
        AZ::ComponentDescriptor* m_descriptor;
    };

    using RegisteredItems = AZStd::unordered_map<AZStd::string, Entry>;
    RegisteredItems m_registry;

    AZStd::unordered_map<AZStd::string, ReflectFunction> m_registeredReflections;

    bool m_verbose = false;

public:

    const RegisteredItems& GetEntries() const
    {
        return m_registry;
    }

};

//! Helper object to compute the hash of a given string
class ScriptCanvasNodeHash
{
public:

    static inline AZStd::string Encode(AZStd::string input)
    {
        AZStd::hash<AZStd::string> hash_string;
        auto h = hash_string(input);
        return AZStd::string::format("%llX", h);
    }
};

//! This class creates the registration information for a given
//! ScriptCanvas node. It registers into the ScriptCanvasModel
template <typename T>
class ScriptCanvasNodeFactory : public IScriptCanvasNodeFactory
{
public:

    ScriptCanvasNodeFactory(const char* gemOrModuleName, const char* className)
        : m_gemOrModuleName(gemOrModuleName)
        , m_className(className)
    {
        AZStd::string input = AZStd::string::format("%s_%s", gemOrModuleName, className).c_str();
        m_typeHash = ScriptCanvasNodeHash::Encode(input);

        ScriptCanvasModel::Instance().Register(gemOrModuleName, className, m_typeHash.c_str(), this);

        if (m_verbose)
        {
            AZ_TracePrintf("ScriptCanvas", "Registered Node: %s::%s (%s)\n", gemOrModuleName, className, m_typeHash.c_str());
        }
    }

    virtual ~ScriptCanvasNodeFactory() = default;

    const AZStd::string GetGemOrModuleName() const override { return m_gemOrModuleName; }
    const AZStd::string GetName() const override { return m_className; }
    const AZStd::string GetID() const override { return m_typeHash; }


private:

    AZStd::string m_className;
    AZStd::string m_gemOrModuleName;
    AZStd::string m_typeHash;

    bool m_verbose = false;

};

// Declares a custom factory object for a Script Canvas node, this object will be used
// to automatically register a node into the environment
#define SCRIPTCANVAS_NODE_FACTORY_DECL(GemOrModuleName, ClassName)                                                                                          \
    class ScriptCanvasNodeFactory_##ClassName : public ScriptCanvasNodeFactory<ClassName> { public:                                                         \
                ScriptCanvasNodeFactory_##ClassName() : ScriptCanvasNodeFactory(GemOrModuleName, #ClassName) {}                                             \
                static ScriptCanvasNodeFactory_##ClassName* Impl() { static ScriptCanvasNodeFactory_##ClassName factory; return &factory; }                 \
                AZ::ComponentDescriptor* GetDescriptor() const override { return ClassName::CreateDescriptor(); }                                           \
    };                                                                                                                                                      \
                static ScriptCanvasNodeFactory_##ClassName* s_factory_##ClassName;

// Implements the static object for the specified class name, this ensures that the
// node will get registered into the environment on construction
#define SCRIPTCANVAS_NODE_FACTORY_IMPL(Namespace, ClassName) Namespace::ClassName::ScriptCanvasNodeFactory_##ClassName* s_factory_##ClassName = Namespace::ClassName::ScriptCanvasNodeFactory_##ClassName::Impl();

