/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZCORE_EXCLUDE_LUA)

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

extern "C" {
#   include <Lua/lualib.h>
#   include <Lua/lauxlib.h>
}

#include <AzFramework/Script/ScriptComponent.h>

AZ_DEFINE_BUDGET(Script);

constexpr static const char* ScriptComponentHotReloadPath = "/O3DE/AzFramework/Script/HotReloadEnabled";

namespace ScriptComponentCpp
{
    template<typename T>
    bool WriteToStream(AZ::IO::GenericStream& stream, const T* t)
    {
        return stream.Write(sizeof(T), t) == sizeof(T);
    }

    int LuaStreamWriter(lua_State*, const void* data, size_t size, void* userData)
    {
        using namespace AZ::IO;
        GenericStream* stream = static_cast<GenericStream*>(userData);

        SizeType bytesToWrite = aznumeric_caster(size);
        SizeType bytesWritten = stream->Write(bytesToWrite, data);

        // Return whether or not there was an error
        return bytesWritten == bytesToWrite ? 0 : 1;
    }

    bool LuaDumpToStream(AZStd::string_view errorWindow, AZ::IO::GenericStream& stream, lua_State* l)
    {
        AZ_UNUSED(errorWindow);
        if (!lua_isfunction(l, -1))
        {
            AZ_TracePrintf(errorWindow.data(), "Top of stack is not function!");
        }

        const int stripBinaryRepresentation = 1;
        return lua_dump(l, LuaStreamWriter, &stream, stripBinaryRepresentation) == 0;
    }
}

namespace AzFramework
{
    void ConstructScriptAssetPaths(ScriptCompileRequest& request)
    {
        // Reset filename to .luac, reconstruct full path
        AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.data(), request.m_destFileName);
        AzFramework::StringFunc::Path::ReplaceExtension(request.m_destFileName, "luac");
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.data(), request.m_destFileName.data(), request.m_destPath, true);
    }

    // Ensures condition is true, otherwise fails the compile job.
#define COMPILE_VERIFY(condition, ...)                              \
    if (!(condition))                                           \
    {                                                           \
        AZ_Error(request.m_errorWindow.data(), false, __VA_ARGS__);    \
        return AZ::Failure(AZStd::string::format(__VA_ARGS__));  \
    }

    AZ::Outcome<void, AZStd::string> CompileScript(ScriptCompileRequest& request)
    {
        AZ::ScriptContext scriptContext(AZ::DefaultScriptContextId);
        return CompileScript(request, scriptContext);
    }

    AZ::Outcome<void, AZStd::string> CompileScriptAndAsset(ScriptCompileRequest& request)
    {
        return CompileScript(request);
    }

    AZ::Outcome<void, AZStd::string> CompileScript(ScriptCompileRequest& request, AZ::ScriptContext& scriptContext)
    {
        AZ_TracePrintf(request.m_errorWindow.data(), "Starting script compile.\n");

        AZ::LuaScriptData& assetData = request.m_luaScriptDataOut;
        assetData.m_debugName = "@";
        assetData.m_debugName += request.m_sourceFile;
        AZStd::to_lower(assetData.m_debugName.begin(), assetData.m_debugName.end());
        auto scriptStream = assetData.CreateScriptWriteStream();

        // parse source
        COMPILE_VERIFY(scriptContext.LoadFromStream(request.m_input, assetData.m_debugName.c_str(), AZ::k_scriptLoadRawText), "%s", lua_tostring(scriptContext.NativeContext(), -1));
        AZ_TracePrintf(request.m_errorWindow.data(), "Beginning writing of script data.\n");
        COMPILE_VERIFY(ScriptComponentCpp::LuaDumpToStream(request.m_errorWindow, scriptStream, scriptContext.NativeContext()), "Failed to write lua script to stream.");
        return AZ::Success();
    }

    AZ::Outcome<AZStd::string, AZStd::string> CompileScriptAndSaveAsset(ScriptCompileRequest& request)
    {
        using namespace AZ::IO;
        FileIOStream outputStream;
        if (!outputStream.Open(request.m_destPath.c_str(), OpenMode::ModeWrite | OpenMode::ModeBinary))
        {
            return AZ::Failure(AZStd::string("Failed to open output file %s", request.m_destPath.data()));
        }

        auto compileOutcome = CompileScript(request);
        if (!compileOutcome.IsSuccess())
        {
            return AZ::Failure(compileOutcome.TakeError());
        }

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        COMPILE_VERIFY(serializeContext, "Unable to retrieve serialize context.");
        COMPILE_VERIFY(AZ::Utils::SaveObjectToStream<AZ::LuaScriptData>
            ( outputStream
            , AZ::ObjectStream::ST_BINARY
            , &request.m_luaScriptDataOut, serializeContext)
            , "Failed to write asset data to stream");

        return AZ::Success(request.m_destFileName);
    }

    bool SaveLuaAssetData(const AZ::LuaScriptData& data, AZ::IO::GenericStream& stream)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Unable to retrieve serialize context.");
        return AZ::Utils::SaveObjectToStream<AZ::LuaScriptData>(stream, AZ::ObjectStream::ST_BINARY, &data, serializeContext);
    }

    //  We generally have the following data types which we can use for editor properties
    //  simple: numbers, strings, booleans, arrays, entity
    //  complex: colors, vectors, normals, matrix,
    //
    // MyScriptComponent = {
    //      Properties = {
    //          m_speed = {
    //              default = 0,   // Support for numerical, string or boolean
    //              min = 0,
    //              max = 100,
    //              step = 1,
    //              description = "Speed in m/s for the ...",
    //          },
    //          Colors = {
    //              front = {
    //                  default = Vector4(255,255,255),
    //              },
    //              back = {
    //                  default = Vector4(128,0,0,128)
    //              }
    //          },
    //          Jump = {
    //              enabled = false,
    //              m_jumpDirection = {
    //                  default = Vector3(0,1,0),
    //              },
    //              m_jumpPosition = {
    //                  default = Vector3(0,0,0),
    //              },
    //          },
    //          m_target = {
    //              default = EntityId(),
    //          },
    //          m_elementSeed = {1,2,3,4,5}, // default array type no attibutes
    //
    //          m_networkedProperty =
    //          {
    //                default = 10,                // Configure the property like you would any other element
    //
    //
    //                netSynched = {                // Add in this special table called netSynched, and the value will be now be synchornized across the network
    //                    Enabled = true,           // Control flag for turning the network synching on and off without removing the entire table. Defaults to true if missing
    //                    OnNewValue = function,    // OnNewValue will be called whenever the the property has a new value
    //                    ForceIndex = [1..32]      // Profiling helper tool to force a property to use a particular DataSet to help avoid ambiguity within the profiler's Replica Usage.
    //                }
    //          }
    //      }
    //
    //      // NetRPCs are a table of RPC calls to be made across the network.
    //      NetRPCs =
    //      {
    //          // Construct a table inside of here, the name of which will be the RPC
    //          SampleRPC =
    //          {
    //              // Two callbacks can be registered to the NetRPC
    //              // A function to be invoked on the main server - OnServer
    //              // and a function to be invoked on the Proxy - OnProxy
    //              //
    //              // Every NetRPC needs to  have a valid OnServer function, while OnProxy is optional.
    //              OnServer = function()
    //                  Debug.Log("Function to be invoked on the server.");
    //              end
    //
    //              OnProxy = function()
    //                  Debug.Log("Function to be invoked on the Proxy.");
    //              end
    //          }
    //
    //      }
    // }
    //
    // function MyScriptComponent:Activate()
    //      self.NetRPCs.SampleRPC("Invoking an RPC");
    // end
    //
    // function MyScriptComponent:Deactivate()
    //
    // end

    //  simple: numbers, strings, booleans, arrays, entity
    //  complex: vectors, reflected classes

    namespace Internal
    {

        AZStd::string PrintLuaValue(lua_State* lua, int stackIdx, int depth = 0)
        {
            constexpr int MaxDepth = 4;
            if (depth > MaxDepth)
            {
                return "";
            }

            const int elementType = lua_type(lua, stackIdx);

            switch (elementType)
            {
            case LUA_TSTRING:
                return lua_tostring(lua, stackIdx);
            case LUA_TBOOLEAN:
                return lua_toboolean(lua, stackIdx) ? "true" : "false";
            case LUA_TNUMBER:
                return AZStd::to_string(lua_tonumber(lua, stackIdx));
            case LUA_TTABLE:
            {
                AZStd::string tableStr = "{";
                AZStd::string keyValuePairs;
                int keyCount = 0;

                // check if this lua table contains a meta-table
                if(lua_getmetatable(lua, stackIdx))
                {
                    keyValuePairs += "Meta";
                    keyValuePairs += PrintLuaValue(lua, lua_gettop(lua), depth+1);
                    lua_pop(lua, 1);
                    keyValuePairs += " ";
                    ++keyCount;
                }

                if (depth < MaxDepth)
                {
                    lua_pushnil(lua);
                    bool tableKeyExists = lua_next(lua, stackIdx);
                    while(tableKeyExists)
                    {
                        const int valueIndex = lua_gettop(lua);
                        const int keyIndex = valueIndex-1;

                        const AZStd::string key = PrintLuaValue(lua, keyIndex, depth+1);
                        const AZStd::string value = PrintLuaValue(lua, valueIndex, depth+1);
                        keyValuePairs += key + AZStd::string("=")+value;
                        ++keyCount;

                        lua_pop(lua, 1);  // removes 'value'; keeps 'key' for next iteration
                        tableKeyExists = lua_next(lua, stackIdx);
                        if(tableKeyExists)
                        {
                            keyValuePairs += " ";
                        }
                    }
                }

                tableStr += keyValuePairs.length() < 1024 ? keyValuePairs : AZStd::string::format("too many keys (%i)!", keyCount);
                tableStr += "}";

                return tableStr;
            }
            default:
                return lua_typename(lua, elementType);
            }
        }

        //=========================================================================
        // DebugPrintStack
        // Prints the Lua stack starting from the bottom.
        //=========================================================================
        // DO NOT DELETE StackDump is useful to debug the lua stack.
        //static void DebugPrintStack(lua_State* lua, const AZStd::string& prefix = "")
        //{
        //    AZStd::string dump = prefix;
        //    const int stackSize = lua_gettop(lua);
        //    for (int stackIdx = 1; stackIdx <= stackSize; ++stackIdx)
        //    {
        //        dump += PrintLuaValue(lua, stackIdx);
        //        dump += " "; // add separator
        //    }
        //
        //    AZ_Warning("ScriptComponent", false, "Stack Dump: '%s'", dump.c_str());
        //}

        //=========================================================================
        // Properties__IndexFindSubtable
        //=========================================================================
        static bool Properties__IndexFindSubtable(lua_State* lua, int lookupTable, int entityProperties, int scriptProperties)
        {
            LSV_BEGIN(lua, 0);

            lua_pushnil(lua);
            while (lua_next(lua, entityProperties))
            {
                if (lua_istable(lua, -1))
                {
                    lua_pushvalue(lua, -2); // copy the key
                    //const char* key = lua_tostring(lua, -1);
                    //(void)key;
                    lua_rawget(lua, scriptProperties); // load the table from the script properties
                    if (lua_istable(lua, -1))
                    {
                        if (lua_topointer(lua, -2) == lua_topointer(lua, lookupTable))
                        {
                            return true;
                        }
                        else
                        {
                            if (Properties__IndexFindSubtable(lua, lookupTable, lua_gettop(lua) - 1, lua_gettop(lua)))
                            {
                                return true;
                            }
                        }
                    }
                    lua_pop(lua, 1); // pop the table result
                }
                lua_pop(lua, 1); // pop the value
                // leave the key for next iteration
            }
            return false;
        }
        //=========================================================================
        // Properties__Index
        //=========================================================================
        static int Properties__Index(lua_State* lua)
        {
            LSV_BEGIN(lua, 1);

            AZ::ScriptContext::FromNativeContext(lua)->Error(AZ::ScriptContext::ErrorType::Warning, true,
                "Property %s not found in entity table. Please push this property to your slice to avoid decrease in performance.", lua_tostring(lua, -1));
            int lookupKey = lua_gettop(lua);

            int lookupTable = lookupKey - 1;
            // This is a slow function and it's made slow so we don't cache any extra data.
            // This is done because this function will be called only the exported components
            // and script are not in sync and we added new properties.
            lua_getmetatable(lua, -2); // get the metatable which will be the top property table
            int entityProperties = lua_gettop(lua);
            if (lua_getmetatable(lua, -1) == 0) // get the metatable of the property which will be the original table
            {
                // we are looking at top level properties
                lua_pushvalue(lua, -2); // copy the key
                lua_rawget(lua, -2); // read the value
            }
            else
            {
                // we are looking into the sub table, so do a slow traversal
                int scriptProperties = lua_gettop(lua);
                if (!Properties__IndexFindSubtable(lua, lookupTable, entityProperties, scriptProperties))
                {
                    lua_pushnil(lua);
                    return 1; // we did not find the table
                }
                else
                {
                    lua_pushvalue(lua, lookupKey);
                    lua_rawget(lua, -2);
                }
            }

            if (lua_istable(lua, -1))
            {
                // if we are here the target table is on the top if the stack
                lua_pushstring(lua, ScriptComponent::DefaultFieldName);
                lua_rawget(lua, -2);
                if (lua_isnil(lua, -1))
                {
                    // parent table is a group, pop the value and return the table
                    lua_pop(lua, 1);
                }
            }

            // Duplicate the value, so once the storage is done its on top of the stack, and returned
            lua_pushvalue(lua, -1);

            // Push key, and then move it below the value
            lua_pushvalue(lua, lookupKey);
            lua_insert(lua, -2);

            // Cache the value so that subsequent accesses to this property don't result in warnings
            lua_rawset(lua, lookupTable);

            return 1;
        }
        //=========================================================================
        // Properties__NewIndex
        //=========================================================================
        static int Properties__NewIndex(lua_State* lua)
        {
            LSV_BEGIN_VARIABLE(lua);

            // We want to raw set the value to avoid coming back in here.
            lua_rawset(lua, 1);
            LSV_END_VARIABLE(-2);
            return 0;
        }
    } // namespace Internal

    // The code will create a table with uniqueEntityName which has
    // MyScriptComponent as a metatable. A full new copy of the property table will be declared will the
    // appropriate variables from editor. Every other table variable should be replicated.
    //=========================================================================
    // ScriptComponent
    // [8/9/2013]
    //=========================================================================

    const char* ScriptComponent::DefaultFieldName = "default";

    ScriptComponent::ScriptComponent()
        : m_context(nullptr)
        , m_contextId(AZ::ScriptContextIds::DefaultScriptContextId)
        , m_script(AZ::Data::AssetLoadBehavior::PreLoad)
        , m_table(LUA_NOREF)
    {
        m_properties.m_name = "Properties";
    }

    //=========================================================================
    // ~PlacementComponent
    // [8/9/2013]
    //=========================================================================
    ScriptComponent::~ScriptComponent()
    {
        m_properties.Clear();
    }

    //=========================================================================
    // SetScriptContext
    // [8/9/2013]
    //=========================================================================
    void ScriptComponent::SetScriptContext(AZ::ScriptContext* context)
    {
        AZ_Assert(m_entity == nullptr || m_entity->GetState() != AZ::Entity::State::Active, "You can't change the context while the entity is active");
        m_context = context;
        m_contextId = context->GetId();
    }

    //=========================================================================
    // SetScriptName
    // [8/9/2013]
    //=========================================================================
    void ScriptComponent::SetScript(const AZ::Data::Asset<AZ::ScriptAsset>& script)
    {
        AZ_Assert(m_entity == nullptr || m_entity->GetState() != AZ::Entity::State::Active, "You can't change the script while the entity is active");

        m_script = script;
        m_script.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
    }

    AZ::ScriptProperty* ScriptComponent::GetScriptProperty(const char* propertyName)
    {
        return m_properties.GetProperty(propertyName);
    }

    void ScriptComponent::Init()
    {
        // Grab the script context
        AZ::ScriptSystemRequestBus::BroadcastResult(m_context, &AZ::ScriptSystemRequestBus::Events::GetContext, m_contextId);
        AZ_Assert(m_context, "We must have a valid script context!");
    }

    //=========================================================================
    // Activate
    // [8/12/2013]
    //=========================================================================
    void ScriptComponent::Activate()
    {
        AZ_PROFILE_SCOPE(Script, "Load: %s", m_script.GetHint().c_str());
        AZ_Error("LuaComponent", m_script.GetAutoLoadBehavior() == AZ::Data::AssetLoadBehavior::PreLoad, "Runtime LuaComponent script asset not set to Preload");
        AZ_Error("LuaComponent", m_script.Get(), "Runtime LuaComponent script asset not preloaded and ready");

        // Load the script, find the base table...
        if (LoadInContext())
        {
#if defined(_RELEASE)
            bool isHotReloadEnabled = false;
#else
            bool isHotReloadEnabled = true;
#endif
            if (auto* registry = AZ::SettingsRegistry::Get())
            {
                registry->Get(isHotReloadEnabled, ScriptComponentHotReloadPath);
            }

            if (isHotReloadEnabled)
            {
                AZ::Data::AssetBus::Handler::BusConnect(m_script.GetId());
            }

            // ...create the entity table, find the Activate/Deactivate functions in the script and call them
            CreateEntityTable();
        }
    }

    void ScriptComponent::DestroyEntityTable()
    {
        AZ_PROFILE_SCOPE(Script, "Unload: %s", m_script.GetHint().c_str());

        if (m_table != LUA_NOREF)
        {
            lua_State* lua = m_context->NativeContext();
            LSV_BEGIN(lua, 0);

            // call OnDeactivate
            // load table
            lua_rawgeti(lua, LUA_REGISTRYINDEX, m_table);
            if (lua_getmetatable(lua, -1) == 0) // load the base table
            {
                AZ_Assert(false, "This should not happen, all entity table should have the base table as a metatable!");
            }
            // cache
            lua_pushliteral(lua, "OnDeactivate");
            lua_rawget(lua, -2); // ScriptTable[OnDeactivte]
            if (lua_isfunction(lua, -1))
            {
                AZ_PROFILE_SCOPE(Script, "OnDeactivate");

                lua_pushvalue(lua, -3); // push the entity table as the only argument
                AZ::Internal::LuaSafeCall(lua, 1, 0); // Call OnDeactivate
            }
            else
            {
                lua_pop(lua, 1); // remove the OnDeactivate result
            }
            lua_pop(lua, 2); // remove the base table and the entity table

            // release table reference
            luaL_unref(lua, LUA_REGISTRYINDEX, m_table);
            m_table = LUA_NOREF;
        }
    }

    void ScriptComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();

        DestroyEntityTable();
    }

    void ScriptComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        DestroyEntityTable();

        m_script = asset;

        // Load the updated asset on the next tick so we're not holding on to the old script
        AZ::TickBus::Handler::BusConnect();
    }

    void ScriptComponent::OnTick([[maybe_unused]]float deltaTime, [[maybe_unused]]AZ::ScriptTimePoint time)
    {
        AZ::TickBus::Handler::BusDisconnect();

        if (LoadInContext())
        {
            CreateEntityTable();
        }
    }

    bool ScriptComponent::LoadInContext()
    {
        LSV_BEGIN(m_context->NativeContext(), 1);

        SCRIPT_SYSTEM_SCRIPT_STATUS("ScriptSystemComponent"
            , "ScriptComponent::LoadInContext requesting load with its own script value: %s-%s"
            , m_script.GetHint().c_str()
            , m_script.GetId().m_guid.ToString<AZStd::fixed_string<AZ::Uuid::MaxStringBuffer>>().c_str());

        // Set the metamethods as we will use the script table as a metatable for entity tables
        bool success = false;
        AZ::ScriptSystemRequestBus::BroadcastResult(
            success, &AZ::ScriptSystemRequestBus::Events::Load, m_script, AZ::k_scriptLoadBinary, m_contextId);
        if (!success)
        {
            return false;
        }

        lua_State* lua = m_context->NativeContext();
        if (!lua_istable(lua, -1))
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, m_script.GetId());

            AZStd::string tableName = assetInfo.m_relativePath;
            AzFramework::StringFunc::Path::GetFileName(tableName.c_str(), tableName);
            AZStd::to_lower(tableName.begin(), tableName.end());
            m_context->Error(AZ::ScriptContext::ErrorType::Error, false, "Script %s is setup incorrectly. Scripts must now return a table (for example, you may need to add \"return %s\" to the end of the file).", assetInfo.m_relativePath.c_str(), tableName.c_str());
            lua_pop(lua, 1);
            return false;
        }

        // Point the __index of the Script table to itself
        // because it will be used as a metatable
        // Stack = ScriptRootTable
        lua_pushliteral(lua, "__index");  // Stack = ScriptRootTable __index
        lua_pushvalue(lua, -2);  // Stack = ScriptRootTable __index CopyOfScriptRootTable

        // raw set: t[k] = v, where t is the value at the given index, v is the value at the top of the stack, and k is the value just below the top.  Both key and value are popped off the stack.
        // ie: ScriptRootTable[__index] = CopyOfScriptRootTable
        // Since __index value is a table and not a function, the final result is the result of indexing this table with key. However, this indexing is regular, not raw, and therefore can trigger another metamethod.
        lua_rawset(lua, -3);  // Stack = ScriptRootTable

        // load Property table name
        lua_pushlstring(lua, m_properties.m_name.c_str(), m_properties.m_name.length());  // Stack = ScriptRootTable "Properties"
        lua_rawget(lua, -2);  // Stack = ScriptRootTable ThisScriptPropertiesTable
        if (lua_istable(lua, -1))
        {
            // This property table will be used a metatable from all instances
            // set the __index so we can read values in case we change the script
            // after we export the component
            lua_pushliteral(lua, "__index");
            lua_pushcclosure(lua, &Internal::Properties__Index, 0);
            lua_rawset(lua, -3);

            lua_pushliteral(lua, "__newindex");
            lua_pushcclosure(lua, &Internal::Properties__NewIndex, 0);
            lua_rawset(lua, -3);
        }
        lua_pop(lua, 1); // pop the properties table (or the nil value)
        // Leave the script table on the stack for CreateEntityTable().

        return true;
    }

    //=========================================================================
    // CopyEntityTables
    // Mirror source entity's subtables (excluding property table, which is handled \ref CreatePropertyGroup)
    // The goal is to ensure that if we modify any table, modifications are isolated to the instance (not the global/shared entity table)
    // In addition set to entity tables as a metatable so cloned tables for read-only operations.
    //=========================================================================
    void CopyAndHookEntityTables(lua_State* lua, int sourceTable, int targetTable, const char* propertyTableName)
    {
        LSV_BEGIN(lua, 0);

        lua_pushnil(lua); // first key
        while (lua_next(lua, sourceTable))
        {
            if (lua_istable(lua, -1))
            {
                int keyType = lua_type(lua, -2);
                switch (keyType)
                {
                    case LUA_TSTRING:
                    {
                        const char* tableName = lua_tolstring(lua, -2, nullptr);
                        if (strncmp(tableName, "__", 2) == 0 || // skip metatables
                            strcmp(tableName, propertyTableName) == 0) // Skip the Properties table
                        {
                            break;
                        }
                        // else fall through
                    }

                    case LUA_TNUMBER:
                    {
                        // stack: source subtable, source key

                        // Create the target subtable.
                        lua_createtable(lua, 0, 0);
                        lua_pushvalue(lua, -3);             // push source subtable key
                        lua_pushvalue(lua, -2);             // push new subtable instance
                        lua_rawset(lua, targetTable);       // targetTable.subtableKey = newSubTable

                        // The source subtable will be used as a metatable, so set its __index to itself.
                        lua_pushliteral(lua, "__index");    // push __index key
                        lua_pushvalue(lua, -3);             // push the source subtable
                        lua_rawset(lua, -1);                // sourceSubtable.__index = sourceSubtable;

                        // Set the target subtable's metatable to the source subtable.
                        lua_pushvalue(lua, -2);             // push the source subtable
                        lua_setmetatable(lua, -2);          // setmetatable(targetSubtable, sourceSubtable)

                        int newTarget = lua_gettop(lua);
                        int newSource = newTarget - 1;
                        CopyAndHookEntityTables(lua, newSource, newTarget, propertyTableName);

                        lua_pop(lua, 1); // pop the new table
                    } break;

                    default:
                    {
                        AZ_Warning("Script", false, "Tables associated with non string/number keys are not copied, thus will be lost in new instances.");
                    } break;
                }
            }

            lua_pop(lua, 1); // pop the value
            // leave the key for next iteration
        }
    }

    //=========================================================================
    // CreateEntityTable
    // [3/3/2014]
    //=========================================================================
    void ScriptComponent::CreateEntityTable()
    {
        lua_State* lua = m_context->NativeContext();
        LSV_BEGIN(lua, -1);

        AZ_Error("Script", lua_istable(lua, -1), "%s", "Script did not return a table!");
        int baseStackIndex = lua_gettop(lua);

        // Stack: ScriptRootTable
        lua_pushlstring(lua, m_properties.m_name.c_str(), m_properties.m_name.length());  // Stack: ScriptRootTable "Properties"
        lua_rawget(lua, baseStackIndex);  // Stack: ScriptRootTable PropertiesTable
        AZ_Error("Script", lua_istable(lua, -1) || lua_isnil(lua, -1), "We should have the %s table for properties!", m_properties.m_name.c_str());
        int basePropertyTable = -1;
        if (lua_istable(lua, -1))
        {
            basePropertyTable = lua_gettop(lua);
        }

        lua_createtable(lua, 0, 1); // Create entity table;
        [[maybe_unused]] int entityStackIndex = lua_gettop(lua);

        // Stack: ScriptRootTable PropertiesTable EntityTable

        if (basePropertyTable > -1) // if property table exists
        {
            CreatePropertyGroup(m_properties, basePropertyTable, lua_gettop(lua), basePropertyTable, true);
            // Stack: ScriptRootTable PropertiesTable EntityTable{ PropertiesTable{__index __newIndex Meta{CopyOfPropertiesTable}} }
        }

        // replicate other tables to make sure we have table per instance.
        CopyAndHookEntityTables(lua, baseStackIndex, lua_gettop(lua), m_properties.m_name.c_str());

        // set my entity id
        lua_pushliteral(lua, "entityId"); // Stack: ScriptRootTable PropertiesTable EntityTable{ PropertiesTable{__index __newIndex Meta{CopyOfPropertiesTable}} } "entityId"
        AZ::ScriptValue<AZ::EntityId>::StackPush(lua, GetEntityId());  // Stack: ScriptRootTable PropertiesTable EntityTable{ PropertiesTable{__index __newIndex Meta{CopyOfPropertiesTable}} } "entityId" userdata
        lua_rawset(lua, -3);  // Stack: ScriptRootTable PropertiesTable EntityTable{ PropertiesTable{__index __newIndex Meta{CopyOfPropertiesTable}} entityId }

        // set the base metatable
        lua_pushvalue(lua, baseStackIndex); // copy the "Base table"
        lua_setmetatable(lua, -2); // set the scriptTable as a metatable for the entity table

        // Keep the entity table in the registry
        m_table = luaL_ref(lua, LUA_REGISTRYINDEX);

        // call OnActivate
        lua_pushliteral(lua, "OnActivate");
        lua_rawget(lua, baseStackIndex); // ScriptTable[OnActivate]
        if (lua_isfunction(lua, -1))
        {
            AZ_PROFILE_SCOPE(Script, "OnActivate");
            lua_rawgeti(lua, LUA_REGISTRYINDEX, m_table); // push the entity table as the only argument
            AZ::Internal::LuaSafeCall(lua, 1, 0); // Call OnActivate
        }
        else
        {
            lua_pop(lua, 1); // remove the OnActivate result
        }

        lua_pop(lua, 2); // remove the base property table and base script table
    }

    //=========================================================================
    // CreatePropertyGroup
    // [3/3/2014]
    //=========================================================================
    void ScriptComponent::CreatePropertyGroup(const ScriptPropertyGroup& group, int propertyGroupTableIndex, int parentIndex, int metatableIndex, bool isRoot)
    {
        lua_State* lua = m_context->NativeContext();
        LSV_BEGIN(lua, 0);

        lua_pushlstring(lua, group.m_name.c_str(), group.m_name.length()); // push table key
        lua_createtable(lua, 0, static_cast<int>(group.m_properties.size() + group.m_groups.size())); // create new table

        if (isRoot)
        {
            // Stack: ScriptRootTable PropertiesTable EntityTable "Properties" {}
            // This is the root table (properties) it will be used as properties for all sub tables
            // ScriptComponents can share the same lua script asset, but each instance's Properties table needs to be unique.
            // This way the script can change a property at runtime and not affect the other ScriptComponents which are using the same script.
            // For normal properties we will create new variable instances, but NetSynched variables aren't stored in Lua, and instead
            // are retrieved using the __index and __newIndex metamethods.
            // Ensure that this instance of Properties table has the proper __index and __newIndex metamethods.
            lua_newtable(lua);  // This new table will become the Properties instance metatable.  Stack: ScriptRootTable PropertiesTable EntityTable "Properties" {} {}
            lua_pushliteral(lua, "__index");  // Stack: ScriptRootTable PropertiesTable EntityTable "Properties" {} {} __index
            lua_pushcclosure(lua, &Internal::Properties__Index, 0);  // Stack: ScriptRootTable PropertiesTable EntityTable "Properties" {} {} __index function
            lua_rawset(lua, -3);  // Stack: ScriptRootTable PropertiesTable EntityTable "Properties" {} {__index=Internal::Properties__Index}

            lua_pushliteral(lua, "__newindex");
            lua_pushcclosure(lua, &Internal::Properties__NewIndex, 0);
            lua_rawset(lua, -3);  // Stack: ScriptRootTable PropertiesTable EntityTable "Properties" {} {__index=Internal::Properties__Index __newindex=Internal::Properties__NewIndex}
            lua_setmetatable(lua, -2);  // Stack: ScriptRootTable PropertiesTable EntityTable "Properties" {Meta{__index=Internal::Properties__Index __newindex=Internal::Properties__NewIndex} }

            metatableIndex = lua_gettop(lua);  // This will be the metatable for all subtables
        }
        else
        {
            lua_pushvalue(lua, metatableIndex);
            lua_setmetatable(lua, -2);
        }

        for (size_t i = 0; i < group.m_properties.size(); ++i)
        {
            AZ::ScriptProperty* prop = group.m_properties[i];

            lua_pushlstring(lua, prop->m_name.c_str(), prop->m_name.length());
            if (prop->Write(*m_context))
            {
                lua_rawset(lua, -3);
            }
            else
            {
                AZ_Warning("Script", false, "No data written for property %s!", prop->m_name.c_str());
                lua_pop(lua, 1); // pop the property name and skip it
            }
        }

        for (size_t i = 0; i < group.m_groups.size(); ++i)
        {
            int parentTableIndex = lua_gettop(lua);

            const ScriptPropertyGroup& subGroup = group.m_groups[i];

            lua_pushstring(lua, subGroup.m_name.c_str());
            lua_rawget(lua, propertyGroupTableIndex);

            int childPropertyGroupIndex = lua_gettop(lua);
            AZ_Assert(lua_istable(lua, childPropertyGroupIndex), "Invalid Child Property Table");

            CreatePropertyGroup(subGroup, childPropertyGroupIndex, parentTableIndex, metatableIndex, false);

            lua_remove(lua, childPropertyGroupIndex);
        }

        lua_rawset(lua, parentIndex); // Stack: ScriptRootTable PropertiesTable EntityTable{ PropertiesTable{Meta{__index __newIndex}} }
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void ScriptComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            // we may have been reflected by ScriptEditorComponent already, so check first
            if (serializeContext->FindClassData(AZ::Uuid("{8D1BC97E-C55D-4D34-A460-E63C57CD0D4B}")) == nullptr)
            {
                auto converter = [](AZ::SerializeContext&, AZ::SerializeContext::DataElementNode& node)
                {
                    // If version 2, remove SourceScriptName
                    if (node.GetVersion() == 2)
                    {
                        int index = node.FindElement(AZ_CRC_CE("SourceScriptName"));
                        if (index != -1)
                        {
                            node.RemoveElement(index);
                        }
                    }

                    return true;
                };

                serializeContext->Class<ScriptComponent, AZ::Component>()
                    ->Version(4, converter)
                    ->Field("ContextID", &ScriptComponent::m_contextId)
                    ->Field("Properties", &ScriptComponent::m_properties)
                    ->Field("Script", &ScriptComponent::m_script)
                    ;

                serializeContext->Class<ScriptPropertyGroup>()
                    ->Field("Name", &ScriptPropertyGroup::m_name)
                    ->Field("Properties", &ScriptPropertyGroup::m_properties)
                    ->Field("Groups", &ScriptPropertyGroup::m_groups)
                    ;

                // reflect all script properties
                AZ::ScriptProperties::Reflect(reflection);
            }
        }
    }

    //=========================================================================
    // GetGroup
    //=========================================================================
    ScriptPropertyGroup* ScriptPropertyGroup::GetGroup(const char* groupName)
    {
        for (ScriptPropertyGroup& subGroup : m_groups)
        {
            if (subGroup.m_name == groupName)
            {
                return &subGroup;
            }
        }
        return nullptr;
    }

    //=========================================================================
    // GetProperty
    //=========================================================================
    AZ::ScriptProperty* ScriptPropertyGroup::GetProperty(const char* propertyName)
    {
        for (AZ::ScriptProperty* property : m_properties)
        {
            if (property->m_name == propertyName)
            {
                return property;
            }
        }
        return nullptr;
    }

    //=========================================================================
    // Clear
    //=========================================================================
    void ScriptPropertyGroup::Clear()
    {
        for (AZ::ScriptProperty* property : m_properties)
        {
            delete property;
        }
        m_properties.clear();
        m_groups.clear();
    }

    //=========================================================================
    // ~ScriptPropertyGroup
    //=========================================================================
    ScriptPropertyGroup::~ScriptPropertyGroup()
    {
        Clear();
    }

    //=========================================================================
    // operator=
    //=========================================================================
    ScriptPropertyGroup& ScriptPropertyGroup::operator=(ScriptPropertyGroup&& rhs)
    {
        m_name.swap(rhs.m_name);
        m_properties.swap(rhs.m_properties);
        m_groups.swap(rhs.m_groups);

        return *this;
    }
}   // namespace AzFramework

#endif // #if !defined(AZCORE_EXCLUDE_LUA)
