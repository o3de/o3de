/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: ScriptAdapterComponent.
 * Create: 2021-06-28
 */
#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Script/ScriptAdapterBus.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/IO/Path/Path.h>

namespace AZ
{
    typedef AZ::u32 LuaDataTypeId;
    enum LuaDataType : LuaDataTypeId
    {
        GLOBAL = 0,
        CLASS = 1,
        EBUS = 2,
    };

    class ScriptAdapterComponent
        : public Component
        , public AdapterBus::Handler
    {
    public:
        AZ_COMPONENT(ScriptAdapterComponent, "{DA7F4D69-BA4D-4D1C-AEAE-5DE51F5B9592}", Component);
        static void Reflect(ReflectContext* context);

    protected:
        // AdapterBus overrides
        void Activate() override;
        void Deactivate() override;
        void SaveLuaFile() override;
        void DeleteLuaFile() override;

    private:
        // Get the lua dir absolute path.
        AZStd::string GetProjectPath();
        AZStd::string GetAbsoluteLuaDirPath();
        AZStd::string GetGenDirPath();
        // Get the lua file path based on the type.
        AZStd::string GetLuaFilePathByType(LuaDataTypeId id);
        // Remove the ebusname special characters.
        void FormatEbusName(AZStd::string& name);
        // Format properties and methods in the stack.
        AZStd::string GetFunctionString(const AZStd::string& funcName, LuaDataTypeId id);
        AZStd::string GetPropertyString(const AZStd::string& proName, LuaDataTypeId id);
        // Write global/class/ebus to lua file.
        void WriteStringToFileByType(const AZStd::string& fileStr, LuaDataTypeId id);
        AZStd::string GetSingleEBusString(AZ::BehaviorEBus* ebus);
        void SaveLuaGlobalsToFile(ScriptContext* context);
        void SaveLuaClassesToFile(ScriptContext* context);
        void SaveLuaEBusesToFile(ScriptContext* context);
        bool SetGenDirWritable(bool isWritable);
        static void DeleteFolderRecursive(const AZ::IO::PathView& path);
        static void SetGenDirWritableRecursive(const IO::PathView& path, bool isWritable);

        // Path and file name.
        static const char* RELATIVE_LUA_PATH;
        static const char* GEN_DIR_NAME;
        static const char* GLOBAL_LUA_FILE_NAME;
        static const char* CLASS_LUA_FILE_NAME;
        static const char* EBUS_LUA_FILE_NAME;
        static const int NAME_BUFFER_SIZE;
    };

}