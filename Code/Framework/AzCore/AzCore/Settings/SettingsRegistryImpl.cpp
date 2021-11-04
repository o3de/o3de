/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <cctype>
#include <cerrno>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/FileReader.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/parallel/scoped_lock.h>

namespace AZ::SettingsRegistryImplInternal
{
    AZ::SettingsRegistryInterface::Type RapidjsonToSettingsRegistryType(const rapidjson::Value& value)
    {
        using Type = AZ::SettingsRegistryInterface::Type;
        switch (value.GetType())
        {
        case rapidjson::Type::kNullType:
            return Type::Null;
        case rapidjson::Type::kFalseType:
            return Type::Boolean;
        case rapidjson::Type::kTrueType:
            return Type::Boolean;
        case rapidjson::Type::kObjectType:
            return Type::Object;
        case rapidjson::Type::kArrayType:
            return Type::Array;
        case rapidjson::Type::kStringType:
            return Type::String;
        case rapidjson::Type::kNumberType:
            return value.IsDouble() ? Type::FloatingPoint :
                Type::Integer;
        }

        return Type::NoType;
    }
}

namespace AZ
{
    template<typename T>
    bool SettingsRegistryImpl::SetValueInternal(AZStd::string_view path, T value)
    {
        if (path.empty())
        {
            // rapidjson::Pointer asserts that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }
        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            if constexpr (AZStd::is_same_v<T, bool> || AZStd::is_same_v<T, double>)
            {
                pointer.Set(m_settings, value);
            }
            else if constexpr (AZStd::is_same_v<T, s64>)
            {
                rapidjson::Value& setting = pointer.Create(m_settings, m_settings.GetAllocator());
                setting.SetInt64(value);
            }
            else if constexpr (AZStd::is_same_v<T, u64>)
            {
                rapidjson::Value& setting = pointer.Create(m_settings, m_settings.GetAllocator());
                setting.SetUint64(value);
            }
            else if constexpr (AZStd::is_same_v<T, AZStd::string_view>)
            {
                rapidjson::Value& setting = pointer.Create(m_settings, m_settings.GetAllocator());
                setting.SetString(value.data(), aznumeric_caster(value.length()), m_settings.GetAllocator());
            }
            else
            {
                static_assert(!AZStd::is_same_v<T, T>, "SettingsRegistryImpl::SetValueInternal called with unsupported type.");
            }

            return true;
        }
        return false;
    }

    template<typename T>
    bool SettingsRegistryImpl::GetValueInternal(T& result, AZStd::string_view path) const
    {
        if (path.empty())
        {
            // rapidjson::Pointer asserts that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }
        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            const rapidjson::Value* value = pointer.Get(m_settings);
            if constexpr (AZStd::is_same_v<T, bool>)
            {
                if (value && value->IsBool())
                {
                    result = value->GetBool();
                    return true;
                }
            }
            else if constexpr (AZStd::is_same_v<T, s64>)
            {
                if (value && value->IsInt64())
                {
                    result = value->GetInt64();
                    return true;
                }
            }
            else if constexpr (AZStd::is_same_v<T, u64>)
            {
                if (value && value->IsUint64())
                {
                    result = value->GetUint64();
                    return true;
                }
            }
            else if constexpr (AZStd::is_same_v<T, double>)
            {
                if (value && value->IsDouble())
                {
                    result = value->GetDouble();
                    return true;
                }
            }
            else if constexpr (AZStd::is_same_v<T, AZStd::string> || AZStd::is_same_v<T, SettingsRegistryInterface::FixedValueString>)
            {
                if (value && value->IsString())
                {
                    result.append(value->GetString(), value->GetStringLength());
                    return true;
                }
            }
            else
            {
                static_assert(!AZStd::is_same_v<T,T>, "SettingsRegistryImpl::GetValueInternal called with unsupported type.");
            }
        }
        return false;
    }

    SettingsRegistryImpl::SettingsRegistryImpl()
    {
        m_serializationSettings.m_keepDefaults = true;

        rapidjson::Pointer pointer(AZ_SETTINGS_REGISTRY_HISTORY_KEY);
        pointer.Create(m_settings, m_settings.GetAllocator()).SetArray();
    }

    SettingsRegistryImpl::SettingsRegistryImpl(bool useFileIo)
        : SettingsRegistryImpl()
    {
        m_useFileIo = useFileIo;
    }

    void SettingsRegistryImpl::SetContext(SerializeContext* context)
    {
        AZStd::scoped_lock lock(m_settingMutex);

        m_serializationSettings.m_serializeContext = context;
        m_deserializationSettings.m_serializeContext = context;
    }

    void SettingsRegistryImpl::SetContext(JsonRegistrationContext* context)
    {
        AZStd::scoped_lock lock(m_settingMutex);

        m_serializationSettings.m_registrationContext = context;
        m_deserializationSettings.m_registrationContext = context;
    }

    bool SettingsRegistryImpl::Visit(Visitor& visitor, AZStd::string_view path) const
    {
        if (path.empty())
        {
            // rapidjson::Pointer asserts that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }

        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            AZStd::scoped_lock lock(m_settingMutex);
            const rapidjson::Value* value = pointer.Get(m_settings);
            if (value)
            {
                StackedString jsonPath(StackedString::Format::JsonPointer);
                if (!path.empty())
                {
                    path.remove_prefix(1); // Remove the leading slash as the StackedString will add this back in.
                    jsonPath.Push(path);
                }
                // Extract the last token of the JSON pointer to use as the valueName
                AZStd::string_view valueName;
                size_t pointerTokenCount = pointer.GetTokenCount();
                if (pointerTokenCount > 0)
                {
                    const rapidjson::Pointer::Token& lastToken = pointer.GetTokens()[pointerTokenCount - 1];
                    valueName = AZStd::string_view(lastToken.name, lastToken.length);
                }
                Visit(visitor, jsonPath, valueName, *value);
                return true;
            }
        }
        return false;
    }

    bool SettingsRegistryImpl::Visit(const VisitorCallback& callback, AZStd::string_view path) const
    {
        struct CallbackVisitor : public Visitor
        {
            explicit CallbackVisitor(const VisitorCallback& callback) : m_callback(callback) {};

            VisitResponse Traverse(AZStd::string_view path, AZStd::string_view valueName, VisitAction action, Type type) override
            {
                return m_callback(path, valueName, action, type);
            }

            const VisitorCallback& m_callback;
        };
        CallbackVisitor visitor(callback);
        return Visit(visitor, path);
    }

    auto SettingsRegistryImpl::RegisterNotifier(NotifyCallback callback) -> NotifyEventHandler
    {
        NotifyEventHandler notifyHandler{ AZStd::move(callback) };
        {
            AZStd::scoped_lock lock(m_notifierMutex);
            notifyHandler.Connect(m_notifiers);
        }
        return notifyHandler;
    }

    auto SettingsRegistryImpl::RegisterNotifier(NotifyEventHandler& notifyHandler) -> void
    {
        AZStd::scoped_lock lock(m_notifierMutex);
        notifyHandler.Connect(m_notifiers);
    }

    void SettingsRegistryImpl::ClearNotifiers()
    {
        AZStd::scoped_lock lock(m_notifierMutex);
        m_notifiers.DisconnectAllHandlers();
    }

    auto SettingsRegistryImpl::RegisterPreMergeEvent(PreMergeEventCallback callback) -> PreMergeEventHandler
    {
        PreMergeEventHandler preMergeHandler{ AZStd::move(callback) };
        {
            AZStd::scoped_lock lock(m_settingMutex);
            preMergeHandler.Connect(m_preMergeEvent);
        }
        return preMergeHandler;
    }

    auto SettingsRegistryImpl::RegisterPreMergeEvent(PreMergeEventHandler& preMergeHandler) -> void
    {
        AZStd::scoped_lock lock(m_settingMutex);
        preMergeHandler.Connect(m_preMergeEvent);
    }

    auto SettingsRegistryImpl::RegisterPostMergeEvent(PostMergeEventCallback callback) -> PostMergeEventHandler
    {
        PostMergeEventHandler postMergeHandler{ AZStd::move(callback) };
        {
            AZStd::scoped_lock lock(m_settingMutex);
            postMergeHandler.Connect(m_postMergeEvent);
        }
        return postMergeHandler;
    }

    auto SettingsRegistryImpl::RegisterPostMergeEvent(PostMergeEventHandler& postMergeHandler) -> void
    {
        AZStd::scoped_lock lock(m_settingMutex);
        postMergeHandler.Connect(m_postMergeEvent);
    }

    void SettingsRegistryImpl::ClearMergeEvents()
    {
        AZStd::scoped_lock lock(m_settingMutex);
        m_preMergeEvent.DisconnectAllHandlers();
        m_postMergeEvent.DisconnectAllHandlers();
    }

    void SettingsRegistryImpl::SignalNotifier(AZStd::string_view jsonPath, Type type)
    {
        // Move the Notifier AZ::Event to a local AZ::Event in order to allow
        // the notifier handlers to be signaled outside of the notifier mutex
        // This allows other threads to register notifiers while this thread
        // is invoking the handlers
        decltype(m_notifiers) localNotifierEvent;
        {
            AZStd::scoped_lock lock(m_notifierMutex);
            localNotifierEvent = AZStd::move(m_notifiers);
        }

        // Signal the NotifyEvent for each queued argument
        decltype(m_signalNotifierQueue) localNotifierQueue;
        {
            AZStd::scoped_lock signalLock(m_signalMutex);
            m_signalNotifierQueue.push_back({ FixedValueString{jsonPath}, type });
            // If the signal count was 0, then a dispatch is in progress
            if (m_signalCount++ == 0)
            {
                AZStd::swap(localNotifierQueue, m_signalNotifierQueue);
            }
        }

        while (!localNotifierQueue.empty())
        {
            for (SignalNotifierArgs notifierArgs : localNotifierQueue)
            {
                localNotifierEvent.Signal(notifierArgs.m_jsonPath, notifierArgs.m_type);
            }
            // Clear the local notifier queue and check if more notifiers have been added
            localNotifierQueue = {};
            {
                AZStd::scoped_lock signalLock(m_signalMutex);
                AZStd::swap(localNotifierQueue, m_signalNotifierQueue);
            }
        }

        {
            AZStd::scoped_lock signalLock(m_signalMutex);
            --m_signalCount;
        }

        {
            // Swap the local handlers with the current m_notifiers which
            // will contain any handlers added during the signaling of the
            // local event
            AZStd::scoped_lock lock(m_notifierMutex);
            AZStd::swap(m_notifiers, localNotifierEvent);
            // Append any added handlers to the m_notifier structure
            m_notifiers.ClaimHandlers(AZStd::move(localNotifierEvent));
        }
    }

    SettingsRegistryInterface::Type SettingsRegistryImpl::GetType(AZStd::string_view path) const
    {
        if (path.empty())
        {
            //rapidjson::Pointer asserts that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }

        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            AZStd::scoped_lock lock(m_settingMutex);
            if (const rapidjson::Value* value = pointer.Get(m_settings); value != nullptr)
            {
                return SettingsRegistryImplInternal::RapidjsonToSettingsRegistryType(*value);
            }
        }
        return Type::NoType;
    }

    bool SettingsRegistryImpl::Get(bool& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(s64& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(u64& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(double& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(AZStd::string& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(FixedValueString& result, AZStd::string_view path) const
    {
        AZStd::scoped_lock lock(m_settingMutex);
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::GetObject(void* result, Uuid resultTypeID, AZStd::string_view path) const
    {
        if (path.empty())
        {
            // rapidjson::Pointer asserts that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }

        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            AZStd::scoped_lock lock(m_settingMutex);
            const rapidjson::Value* value = pointer.Get(m_settings);
            if (value)
            {
                JsonSerializationResult::ResultCode jsonResult = JsonSerialization::Load(result, resultTypeID, *value, m_deserializationSettings);
                return jsonResult.GetProcessing() != JsonSerializationResult::Processing::Halted;
            }
        }
        return false;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, bool value)
    {
        if (AZStd::scoped_lock lock(m_settingMutex); !SetValueInternal(path, value))
        {
            return false;
        }
        SignalNotifier(path, Type::Boolean);
        return true;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, s64 value)
    {
        if (AZStd::scoped_lock lock(m_settingMutex); !SetValueInternal(path, value))
        {
            return false;
        }
        SignalNotifier(path, Type::Integer);
        return true;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, u64 value)
    {
        if (AZStd::scoped_lock lock(m_settingMutex); !SetValueInternal(path, value))
        {
            return false;
        }
        SignalNotifier(path, Type::Integer);
        return true;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, double value)
    {
        if (AZStd::scoped_lock lock(m_settingMutex); !SetValueInternal(path, value))
        {
            return false;
        }
        SignalNotifier(path, Type::FloatingPoint);
        return true;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, AZStd::string_view value)
    {
        if (AZStd::scoped_lock lock(m_settingMutex); !SetValueInternal(path, value))
        {
            return false;
        }
        SignalNotifier(path, Type::String);
        return true;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, const char* value)
    {
        return Set(path, AZStd::string_view{ value });
    }

    bool SettingsRegistryImpl::SetObject(AZStd::string_view path, const void* value, Uuid valueTypeID)
    {
        if (path.empty())
        {
            // rapidjson::Pointer asserts that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }

        rapidjson::Pointer pointer(path.data(), path.length());
        if (pointer.IsValid())
        {
            rapidjson::Value store;
            JsonSerializationResult::ResultCode jsonResult = JsonSerialization::Store(store, m_settings.GetAllocator(), 
                value, nullptr, valueTypeID, m_serializationSettings);
            if (jsonResult.GetProcessing() != JsonSerializationResult::Processing::Halted)
            {
                auto anchorType = Type::NoType;
                {
                    AZStd::scoped_lock lock(m_settingMutex);
                    rapidjson::Value& setting = pointer.Create(m_settings, m_settings.GetAllocator());
                    setting = AZStd::move(store);
                    anchorType = SettingsRegistryImplInternal::RapidjsonToSettingsRegistryType(setting);
                }
                SignalNotifier(path, anchorType);
                return true;
            }
        }
        return false;
    }

    bool SettingsRegistryImpl::Remove(AZStd::string_view path)
    {
        if (path.empty())
        {
            // rapidjson::Pointer asserts that the supplied string
            // is not nullptr even if the supplied size is 0
            // Setting to empty string to prevent assert
            path = "";
        }
        rapidjson::Pointer pointerPath(path.data(), path.size());
        if (!pointerPath.IsValid())
        {
            return false;
        }

        AZStd::scoped_lock lock(m_settingMutex);
        return pointerPath.Erase(m_settings);
    }

    bool SettingsRegistryImpl::MergeCommandLineArgument(AZStd::string_view argument, AZStd::string_view rootKey,
        const CommandLineArgumentSettings& commandLineSettings)
    {
        if (!commandLineSettings.m_delimiterFunc)
        {
            AZ_Error("SettingsRegistry", false,
                R"(No delimiter function, therefore there splitting of the argument "%.*s" into a key value pair cannot be done)",
                aznumeric_cast<int>(argument.size()), argument.data());
            return false;
        }

        auto [key, value] = commandLineSettings.m_delimiterFunc(argument);
        if (key.empty())
        {
            // They key where to set the JSON value cannot be empty
            // The value of the JSON can be though
            // This is so that a key can be set to empty string using "/KeyPath="
            return false;
        }

        // Prepend the rootKey as an anchor to the argument key
        SettingsRegistryInterface::FixedValueString keyPath{ rootKey.ends_with('/')
            ? rootKey.substr(0, rootKey.size() - 1)
            : rootKey };
        // Append the JSON reference token prefix of '/' to the keyPath
        if (!key.starts_with('/'))
        {
            keyPath.push_back('/');
        }
        if ((key.size() + keyPath.size()) > keyPath.max_size())
        {
            // The key portion is longer than the FixedValueString max size that can be stored
            // This limitation is arbitrary, if an AZStd::string is used or if the C++17 std::to_chars
            // function is used, there wouldn't need to be a limitation
            return false;
        }
        keyPath += key;
        key = keyPath;

        if (value.empty())
        {
            return Set(key, value);
        }

        if (value == "true")
        {
            return Set(key, true);
        }
        if (value == "false")
        {
            return Set(key, false);
        }

        SettingsRegistryInterface::FixedValueString valueString;
        if (value.size() > valueString.max_size())
        {
            // The value portion is longer than the FixedValueString max size that can be stored
            // This limitation is arbitrary, if an AZStd::string is used or if the C++17 std::to_chars
            // function is used, there wouldn't need to be a limitation
            return false;
        }
        valueString = value;
        const char* valueStringEnd = valueString.c_str() + valueString.size();

        errno = 0;
        char* convertEnd = nullptr;
        s64 intValue = strtoll(valueString.c_str(), &convertEnd, 0);
        if (errno != ERANGE && convertEnd == valueStringEnd)
        {
            return Set(key, intValue);
        }
        errno = 0;
        convertEnd = nullptr;
        u64 uintValue = strtoull(valueString.c_str(), &convertEnd, 0);
        if (errno != ERANGE && convertEnd == valueStringEnd)
        {
            return Set(key, uintValue);
        }
        errno = 0;
        convertEnd = nullptr;
        double floatingPointValue = strtod(valueString.c_str(), &convertEnd);
        if (errno != ERANGE && convertEnd == valueStringEnd)
        {
            return Set(key, floatingPointValue);
        }

        // If the type isn't a boolean, integer or floating point then treat the value as a string.
        return Set(key, value);
    }

    bool SettingsRegistryImpl::MergeSettings(AZStd::string_view data, Format format, AZStd::string_view anchorKey)
    {
        rapidjson::Document jsonPatch;
        constexpr int flags = rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag;
        jsonPatch.Parse<flags>(data.data(), data.length());
        if (jsonPatch.HasParseError())
        {
            AZ_Error("Settings Registry", false, R"(Unable to parse data due to json error "%s" at offset %llu.)",
                GetParseError_En(jsonPatch.GetParseError()), jsonPatch.GetErrorOffset());
            return false;
        }

        JsonMergeApproach mergeApproach;
        switch (format)
        {
        case Format::JsonPatch:
            mergeApproach = JsonMergeApproach::JsonPatch;
            break;
        case Format::JsonMergePatch:
            mergeApproach = JsonMergeApproach::JsonMergePatch;
            break;
        default:
            AZ_Assert(false, "Provided format for merging settings into the Setting Registry is unsupported.");
            return false;
        }

        rapidjson::Pointer anchorPath;
        if (!anchorKey.empty())
        {
            anchorPath = rapidjson::Pointer(anchorKey.data(), anchorKey.size());
            if (!anchorPath.IsValid())
            {
                rapidjson::Pointer pointer(AZ_SETTINGS_REGISTRY_HISTORY_KEY "/-");
                AZ_Error("Settings Registry", false, R"(Anchor path "%.*s" is invalid.)", AZ_STRING_ARG(anchorKey));
                AZStd::scoped_lock lock(m_settingMutex);
                pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                    .AddMember(rapidjson::StringRef("Error"), rapidjson::StringRef("Invalid anchor key."), m_settings.GetAllocator())
                    .AddMember(rapidjson::StringRef("Path"),
                    rapidjson::Value(anchorKey.data(), aznumeric_caster(anchorKey.size()), m_settings.GetAllocator()),
                    m_settings.GetAllocator());
                return false;
            }
        }

        auto anchorType = AZ::SettingsRegistryInterface::Type::NoType;
        {
            AZStd::scoped_lock lock(m_settingMutex);
            rapidjson::Value& anchorRoot = anchorPath.IsValid() ? anchorPath.Create(m_settings, m_settings.GetAllocator())
                : m_settings;

            JsonSerializationResult::ResultCode mergeResult =
                JsonSerialization::ApplyPatch(anchorRoot, m_settings.GetAllocator(), jsonPatch, mergeApproach);
            if (mergeResult.GetProcessing() != JsonSerializationResult::Processing::Completed)
            {
                AZ_Error("Settings Registry", false, "Failed to fully merge data into registry.");
                return false;
            }

            // The settings have been successfully merged, query the type at the anchor key
            anchorType = SettingsRegistryImplInternal::RapidjsonToSettingsRegistryType(anchorRoot);
        }

        SignalNotifier(anchorKey, anchorType);

        return true;
    }

    bool SettingsRegistryImpl::MergeSettingsFile(AZStd::string_view path, Format format, AZStd::string_view rootKey,
        AZStd::vector<char>* scratchBuffer)
    {
        using namespace rapidjson;

        if (path.empty())
        {
            AZ_Error("Settings Registry", false, "Path provided for MergeSettingsFile is empty.");
            return false;
        }

        AZStd::vector<char> buffer;
        if (!scratchBuffer)
        {
            scratchBuffer = &buffer;
        }

        bool result = false;
        if (path[path.length()] == 0)
        {
            result = MergeSettingsFileInternal(path.data(), format, rootKey, *scratchBuffer);
        }
        else
        {
            if (AZ::IO::MaxPathLength < path.length() + 1)
            {
                AZ_Error("Settings Registry", false,
                    R"(Path "%.*s" is too long. Either make sure that the provided path is terminated or use a shorter path.)",
                    static_cast<int>(path.length()), path.data());
                Pointer pointer(AZ_SETTINGS_REGISTRY_HISTORY_KEY "/-");

                AZStd::scoped_lock lock(m_settingMutex);
                Value pathValue(path.data(), aznumeric_caster(path.length()), m_settings.GetAllocator());
                pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                    .AddMember(StringRef("Error"), StringRef("Unable to read registry file."), m_settings.GetAllocator())
                    .AddMember(StringRef("Path"), AZStd::move(pathValue), m_settings.GetAllocator());
                return false;
            }
            AZ::IO::FixedMaxPathString filePath(path);
            result = MergeSettingsFileInternal(filePath.c_str(), format, rootKey, *scratchBuffer);
        }

        scratchBuffer->clear();
        return result;
    }

    bool SettingsRegistryImpl::MergeSettingsFolder(AZStd::string_view path, const Specializations& specializations,
        AZStd::string_view platform, AZStd::string_view rootKey, AZStd::vector<char>* scratchBuffer)
    {
        using namespace AZ::IO;
        using namespace rapidjson;

        if (path.empty())
        {
            AZ_Error("Settings Registry", false, "Path provided for MergeSettingsFolder is empty.");
            return false;
        }


        AZStd::vector<char> buffer;
        if (!scratchBuffer)
        {
            scratchBuffer = &buffer;
        }

        Pointer pointer(AZ_SETTINGS_REGISTRY_HISTORY_KEY "/-");

        size_t additionalSpaceRequired = 3; // 3 is for the '/', '*' and 0
        if (!platform.empty())
        {
            additionalSpaceRequired += AZ_ARRAY_SIZE(PlatformFolder) + platform.length() + 2; // +2 for the two slashes.
        }

        if (path.length() + additionalSpaceRequired > AZ::IO::MaxPathLength)
        {
            AZ_Error("Settings Registry", false, "Folder path for the Setting Registry is too long: %.*s",
                static_cast<int>(path.size()), path.data());
            AZStd::scoped_lock lock(m_settingMutex);
            pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                .AddMember(StringRef("Error"), StringRef("Folder path for the Setting Registry is too long."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path.data(), aznumeric_caster(path.length()), m_settings.GetAllocator()), m_settings.GetAllocator());
            return false;
        }

        RegistryFileList fileList;
        scratchBuffer->clear();

        AZ::IO::FixedMaxPath folderPath{ path };

        const size_t platformKeyOffset = folderPath.Native().size();
        folderPath /= '*';

        Value specialzationArray(kArrayType);
        size_t specializationCount = specializations.GetCount();
        for (size_t i = 0; i < specializationCount; ++i)
        {
            AZStd::string_view name = specializations.GetSpecialization(i);
            specialzationArray.PushBack(Value(name.data(), aznumeric_caster(name.length()), m_settings.GetAllocator()), m_settings.GetAllocator());
        }
        pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
            .AddMember(StringRef("Folder"), Value(folderPath.c_str(), aznumeric_caster(folderPath.Native().size()), m_settings.GetAllocator()), m_settings.GetAllocator())
            .AddMember(StringRef("Specializations"), AZStd::move(specialzationArray), m_settings.GetAllocator());


        auto CreateSettingsFindCallback = [this, &fileList, &specializations, &pointer, &folderPath](bool isPlatformFile)
        {
            return [this, &fileList, &specializations, &pointer, &folderPath, isPlatformFile](AZStd::string_view filename, bool isFile) -> bool
            {
                if (isFile)
                {
                    if (fileList.size() >= MaxRegistryFolderEntries)
                    {
                        AZ_Error("Settings Registry", false, "Too many files in registry folder.");
                        AZStd::scoped_lock lock(m_settingMutex);
                        pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                            .AddMember(StringRef("Error"), StringRef("Too many files in registry folder."), m_settings.GetAllocator())
                            .AddMember(StringRef("Path"), Value(folderPath.c_str(), aznumeric_caster(folderPath.Native().size()), m_settings.GetAllocator()), m_settings.GetAllocator())
                            .AddMember(StringRef("File"), Value(filename.data(), aznumeric_caster(filename.size()), m_settings.GetAllocator()), m_settings.GetAllocator());
                        return false;
                    }

                    fileList.push_back();
                    RegistryFile& registryFile = fileList.back();
                    if (ExtractFileDescription(registryFile, filename, specializations))
                    {
                        registryFile.m_isPlatformFile = isPlatformFile;
                    }
                    else
                    {
                        fileList.pop_back();
                    }
                }
                return true;
            };
        };

        struct FindFilesPayload
        {
            bool m_isPlatformFile{};
            AZStd::fixed_vector<AZStd::string_view, 2> m_pathSegmentsToAppend;
        };

        AZStd::fixed_vector<FindFilesPayload, 2> findFilesPayloads{ {false} };
        if (!platform.empty())
        {
            findFilesPayloads.push_back(FindFilesPayload{ true, { PlatformFolder, platform } });
        }

        for (const FindFilesPayload& findFilesPayload : findFilesPayloads)
        {
            // Erase back to initial path
            folderPath.Native().erase(platformKeyOffset);
            for (AZStd::string_view pathSegmentToAppend : findFilesPayload.m_pathSegmentsToAppend)
            {
                folderPath /= pathSegmentToAppend;
            }

            auto findFilesCallback = CreateSettingsFindCallback(findFilesPayload.m_isPlatformFile);
            if (AZ::IO::FileIOBase* fileIo = m_useFileIo ? AZ::IO::FileIOBase::GetInstance() : nullptr; fileIo != nullptr)
            {
                auto FileIoToSystemFileFindFiles = [findFilesCallback = AZStd::move(findFilesCallback), fileIo](const char* filePath) -> bool
                {
                    return findFilesCallback(AZ::IO::PathView(filePath).Filename().Native(), !fileIo->IsDirectory(filePath));
                };
                fileIo->FindFiles(folderPath.c_str(), "*", FileIoToSystemFileFindFiles);
            }
            else
            {
                SystemFile::FindFiles((folderPath / "*").c_str(), findFilesCallback);
            }
        }

        if (!fileList.empty())
        {
            // Sort the registry files in the order
            bool collisionFound = false;
            auto sorter = [this, &collisionFound, &specializations, &pointer, path](
                const RegistryFile& lhs, const RegistryFile& rhs) -> bool
            {
                return IsLessThan(collisionFound, lhs, rhs, specializations, pointer, path);
            };
            AZStd::sort(fileList.begin(), fileList.end(), sorter);

            if (collisionFound)
            {
                return false;
            }

            // Load the registry files in the sorted order.
            for (RegistryFile& registryFile : fileList)
            {
                folderPath.Native().erase(platformKeyOffset); // Erase all characters after the platformKeyOffset
                if (registryFile.m_isPlatformFile)
                {
                    folderPath /= PlatformFolder;
                    folderPath /= platform;
                }

                folderPath /= registryFile.m_relativePath;

                if (!registryFile.m_isPatch)
                {
                    MergeSettingsFileInternal(folderPath.c_str(), Format::JsonMergePatch, rootKey, *scratchBuffer);
                }
                else
                {
                    MergeSettingsFileInternal(folderPath.c_str(), Format::JsonPatch, rootKey, *scratchBuffer);
                }
                scratchBuffer->clear();
            }
        }
        return true;
    }

    SettingsRegistryInterface::VisitResponse SettingsRegistryImpl::Visit(Visitor& visitor, StackedString& path, AZStd::string_view valueName,
        const rapidjson::Value& value) const
    {
        VisitResponse result;
        switch (value.GetType())
        {
        case rapidjson::Type::kNullType:
            result = visitor.Traverse(path, valueName, VisitAction::Value, Type::Null);
            break;
        case rapidjson::Type::kFalseType:
            result = visitor.Traverse(path, valueName, VisitAction::Value, Type::Boolean);
            if (result == VisitResponse::Continue)
            {
                visitor.Visit(path, valueName, Type::Boolean, false);
            }
            break;
        case rapidjson::Type::kTrueType:
            result = visitor.Traverse(path, valueName, VisitAction::Value, Type::Boolean);
            if (result == VisitResponse::Continue)
            {
                visitor.Visit(path, valueName, Type::Boolean, true);
            }
            break;
        case rapidjson::Type::kObjectType:
            result = visitor.Traverse(path, valueName, VisitAction::Begin, Type::Object);
            if (result == VisitResponse::Continue)
            {
                for (const auto& member : value.GetObject())
                {
                    AZStd::string_view fieldName(member.name.GetString(), member.name.GetStringLength());
                    path.Push(fieldName);
                    if (Visit(visitor, path, fieldName, member.value) == VisitResponse::Done)
                    {
                        return VisitResponse::Done;
                    }
                    path.Pop();
                }
                if (visitor.Traverse(path, valueName, VisitAction::End, Type::Object) == VisitResponse::Done)
                {
                    return VisitResponse::Done;
                }
            }
            break;
        case rapidjson::Type::kArrayType:
            result = visitor.Traverse(path, valueName, VisitAction::Begin, Type::Array);
            if (result == VisitResponse::Continue)
            {
                size_t counter = 0;
                for (const rapidjson::Value& entry : value.GetArray())
                {
                    size_t endIndex = path.Get().size();
                    path.Push(counter);
                    // Push can reallocate the internal AZ::OSString on the StackedString, invalidating existing iterators
                    // Therefore StackedString::Get() is called again to retrieve a fresh string_view
                    AZStd::string_view entryName(path.Get());
                    entryName.remove_prefix(endIndex + 1);
                    if (Visit(visitor, path, entryName, entry) == VisitResponse::Done)
                    {
                        return VisitResponse::Done;
                    }
                    counter++;
                    path.Pop();
                }
                if (visitor.Traverse(path, valueName, VisitAction::End, Type::Array) == VisitResponse::Done)
                {
                    return VisitResponse::Done;
                }
            }
            break;
        case rapidjson::Type::kStringType:
            result = visitor.Traverse(path, valueName, VisitAction::Value, Type::String);
            if (result == VisitResponse::Continue)
            {
                visitor.Visit(path, valueName, Type::String, AZStd::string_view(value.GetString(), value.GetStringLength()));
            }
            break;
        case rapidjson::Type::kNumberType:
            if (value.IsDouble())
            {
                result = visitor.Traverse(path, valueName, VisitAction::Value, Type::FloatingPoint);
                if (result == VisitResponse::Continue)
                {
                    visitor.Visit(path, valueName, Type::FloatingPoint, value.GetDouble());
                }
            }
            else
            {
                result = visitor.Traverse(path, valueName, VisitAction::Value, Type::Integer);
                if (result == VisitResponse::Continue)
                {
                    if (value.IsInt64())
                    {
                        s64 integerValue = value.GetInt64();
                        visitor.Visit(path, valueName, Type::Integer, integerValue);
                    }
                    else
                    {
                        u64 integerValue = value.GetUint64();
                        visitor.Visit(path, valueName, Type::Integer, integerValue);
                    }
                }
            }
            break;
        default:
            AZ_Assert(false, "Unsupported RapidJSON type: %i.", aznumeric_cast<int>(value.GetType()));
            result = VisitResponse::Done;
        }
        return result;
    }

    bool SettingsRegistryImpl::IsLessThan(bool& collisionFound, const RegistryFile& lhs, const RegistryFile& rhs,
        const Specializations& specializations, const rapidjson::Pointer& historyPointer, AZStd::string_view folderPath)
    {
        using namespace rapidjson;
        
        if (&lhs == &rhs)
        {
            // Early return to avoid setting the collisionFound reference to true
            // std::sort is allowed to pass in the same memory address for the left and right elements
            return false;
        }

        AZ_Assert(!lhs.m_tags.empty(), "Comparing a settings file without at least a name tag.");
        AZ_Assert(!rhs.m_tags.empty(), "Comparing a settings file without at least a name tag.");

        // Sort by the name first so the registry file gets applied with all its specializations.
        if (lhs.m_tags[0] != rhs.m_tags[0])
        {
            return lhs.m_relativePath < rhs.m_relativePath;
        }

        // Then sort by size first so the files with the fewest specializations get applied first.
        const size_t lhsTagsSize = lhs.m_tags.size();
        const size_t rhsTagsSize = rhs.m_tags.size();
        if (lhsTagsSize != rhsTagsSize)
        {
            return lhsTagsSize < rhsTagsSize;
        }

        // If registry files have the same number of tags then sort them by the order they appear
        // in the specialization list.
        const size_t numTags = lhsTagsSize; // Pick either lhs or rhs as they're the same length.
        for (size_t i = 1; i < numTags; ++i)
        {
            const size_t left = specializations.GetPriority(lhs.m_tags[i]);
            const size_t right = specializations.GetPriority(rhs.m_tags[i]);
            if (left != right)
            {
                return left < right;
            }
        }

        if (lhs.m_isPlatformFile != rhs.m_isPlatformFile)
        {
            return !lhs.m_isPlatformFile;
        }

        collisionFound = true;
        AZ_Error("Settings Registry", false, R"(Two registry files in "%.*s" point to the same specialization: "%s" and "%s")",
            AZ_STRING_ARG(folderPath), lhs.m_relativePath.c_str(), rhs.m_relativePath.c_str());

        AZStd::scoped_lock lock(m_settingMutex);
        historyPointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
            .AddMember(StringRef("Error"), StringRef("Too many files in registry folder."), m_settings.GetAllocator())
            .AddMember(StringRef("Path"),
                Value(folderPath.data(), aznumeric_caster(folderPath.length()), m_settings.GetAllocator()), m_settings.GetAllocator())
            .AddMember(StringRef("File1"), Value(lhs.m_relativePath.c_str(), m_settings.GetAllocator()), m_settings.GetAllocator())
            .AddMember(StringRef("File2"), Value(rhs.m_relativePath.c_str(), m_settings.GetAllocator()), m_settings.GetAllocator());
        return false;
    }

    bool SettingsRegistryImpl::ExtractFileDescription(RegistryFile& output, AZStd::string_view filename, const Specializations& specializations)
    {
        static constexpr auto PatchExtensionWithDot = AZStd::fixed_string<32>(".") + PatchExtension;
        static constexpr auto ExtensionWithDot = AZStd::fixed_string<32>(".") + Extension;
        static constexpr AZ::IO::PathView PatchExtensionView(PatchExtensionWithDot);
        static constexpr AZ::IO::PathView ExtensionView(ExtensionWithDot);

        if (filename.empty())
        {
            AZ_Error("Settings Registry", false, "Settings file without name found");
            return false;
        }

        AZ::IO::PathView filePath{ filename };
        const size_t filePathSize = filePath.Native().size();

        // The filePath.empty() check makes sure that the file extension after the final <dot> isn't added to the output.m_tags
        auto AppendSpecTags = [&output](AZStd::string_view pathTag)
        {
            output.m_tags.push_back(Specializations::Hash(pathTag));
        };
        AZ::StringFunc::TokenizeVisitor(filePath.Stem().Native(), AppendSpecTags, '.');

        // If token is invalid, then the filename has no <dot> characters and therefore no extension
        if (AZ::IO::PathView fileExtension = filePath.Extension(); !fileExtension.empty())
        {
            if (fileExtension == PatchExtensionView)
            {
                output.m_isPatch = true;
            }
            else if (fileExtension != ExtensionView)
            {
                return false;
            }
        }
        else
        {
            AZ_Error("Settings Registry", false, R"(Settings file without extension found: "%.*s")", AZ_STRING_ARG(filename));
            return false;
        }

        if (output.m_tags.size() > 1)
        {
            // Remove files with duplicate tags. This is considered an error.
            AZStd::sort(AZStd::next(output.m_tags.begin()), output.m_tags.end());
            TagList::iterator currentIt = output.m_tags.begin();
            TagList::iterator endIt = output.m_tags.end();
            ++currentIt;
            while (currentIt != endIt)
            {
                if (*currentIt == *(currentIt - 1))
                {
                    AZ_Error("Settings Registry", false, R"(One or more tags are duplicated in registry file "%.*s")", AZ_STRING_ARG(filename));
                    return false;
                }
                ++currentIt;
            }

            // Remove registry file if it won't be applied because there's a specialization that's not been selected. Note that
            // the first tag is actually the hash of the filename for later use so will be skipped.
            size_t tagCount = output.m_tags.size();
            for (size_t i = 1; i < tagCount; ++i)
            {
                if (!specializations.Contains(output.m_tags[i]))
                {
                    return false;
                }
            }
        }

        // Sort the specialization tags so they're in a consistent order for quicker compares later on. Skip the first one as
        // thats the name tag.
        AZStd::sort(AZStd::next(output.m_tags.begin()), output.m_tags.end());

        if (filePathSize < output.m_relativePath.max_size())
        {
            output.m_relativePath = filename;
            return true;
        }
        else
        {
            AZ_Error("Settings Registry", false, R"(Found relative path to settings file "%.*s" is too long.)", AZ_STRING_ARG(filename));
            return false;
        }
    }

    bool SettingsRegistryImpl::MergeSettingsFileInternal(const char* path, Format format, AZStd::string_view rootKey,
        AZStd::vector<char>& scratchBuffer)
    {
        using namespace AZ::IO;
        using namespace rapidjson;

        Pointer pointer(AZ_SETTINGS_REGISTRY_HISTORY_KEY "/-");

        FileReader fileReader(m_useFileIo ? AZ::IO::FileIOBase::GetInstance(): nullptr, path);
        if (!fileReader.IsOpen())
        {
            AZ_Error("Settings Registry", false, R"(Unable to open registry file "%s".)", path);
            pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                .AddMember(StringRef("Error"), StringRef("Unable to open registry file."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
            return false;
        }

        u64 fileSize = fileReader.Length();
        if (fileSize == 0)
        {
            AZ_Warning("Settings Registry", false, R"(Registry file "%s" is 0 bytes in length. There is no nothing to merge)", path);
            pointer.Create(m_settings, m_settings.GetAllocator())
                .SetObject()
                .AddMember(StringRef("Error"), StringRef("registry file is 0 bytes."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
            return false;
        }

        scratchBuffer.clear();
        scratchBuffer.resize_no_construct(fileSize + 1);
        if (fileReader.Read(fileSize, scratchBuffer.data()) != fileSize)
        {
            AZ_Error("Settings Registry", false, R"(Unable to read registry file "%s".)", path);
            pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                .AddMember(StringRef("Error"), StringRef("Unable to read registry file."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
            return false;
        }
        scratchBuffer[fileSize] = 0;

        rapidjson::Document jsonPatch;
        constexpr int flags = rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag;
        jsonPatch.ParseInsitu<flags>(scratchBuffer.data());
        if (jsonPatch.HasParseError())
        {
            auto nativeUI = AZ::Interface<NativeUI::NativeUIRequests>::Get();
            if (jsonPatch.GetParseError() == rapidjson::kParseErrorDocumentEmpty)
            {
                AZ_Warning("Settings Registry", false, R"(Unable to parse registry file "%s" due to json error "%s" at offset %zu.)",
                    path, GetParseError_En(jsonPatch.GetParseError()), jsonPatch.GetErrorOffset());
            }
            else
            {
                using ErrorString = AZStd::fixed_string<4096>;
                auto jsonError = ErrorString::format(R"(Unable to parse registry file "%s" due to json error "%s" at offset %zu.)", path,
                    GetParseError_En(jsonPatch.GetParseError()), jsonPatch.GetErrorOffset());
                AZ_Error("Settings Registry", false, "%s", jsonError.c_str());

                if (nativeUI)
                {
                    nativeUI->DisplayOkDialog("Setreg(Patch) Merge Issue", AZStd::string_view(jsonError), false);
                }
            }
            
            AZStd::scoped_lock lock(m_settingMutex);
            pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                .AddMember(StringRef("Error"), StringRef("Unable to parse registry file due to invalid json."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator())
                .AddMember(StringRef("Message"), StringRef(GetParseError_En(jsonPatch.GetParseError())), m_settings.GetAllocator())
                .AddMember(StringRef("Offset"), aznumeric_cast<uint64_t>(jsonPatch.GetErrorOffset()), m_settings.GetAllocator());
            return false;
        }

        JsonMergeApproach mergeApproach;
        switch (format)
        {
        case Format::JsonPatch:
            mergeApproach = JsonMergeApproach::JsonPatch;
            break;
        case Format::JsonMergePatch:
            mergeApproach = JsonMergeApproach::JsonMergePatch;
            if (!jsonPatch.IsObject())
            {
                AZ_Error("Settings Registry", false, R"(Attempting to merge the settings registry file "%s" where the root element is a)"
                    R"( non-JSON Object using the JSON MergePatch approach. The JSON MergePatch algorithm would therefore)"
                    R"( overwrite all settings at the supplied root-key path and therefore merging has been)"
                    R"( disallowed to prevent field destruction.)" "\n"
                    R"(To merge the supplied settings registry file, the settings within it must be placed within a JSON Object '{}')"
                    R"( in order to allow moving of its fields using the root-key as an anchor.)", path);

                AZStd::scoped_lock lock(m_settingMutex);
                pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                    .AddMember(StringRef("Error"), StringRef("Cannot merge registry file with a root which is not a JSON Object,"
                        " an empty root key and a merge approach of JsonMergePatch. Otherwise the Settings Registry would be overridden."
                        " See RFC 7386 for more information"), m_settings.GetAllocator())
                    .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
                return false;
            }
            break;
        default:
            AZ_Assert(false, "Provided format for merging settings into the Setting Registry is unsupported.");
            return false;
        }

        ScopedMergeEvent scopedMergeEvent(m_preMergeEvent, m_postMergeEvent, path, rootKey);

        JsonSerializationResult::ResultCode mergeResult(JsonSerializationResult::Tasks::Merge);
        auto anchorType = Type::NoType;
        if (rootKey.empty())
        {
            AZStd::scoped_lock lock(m_settingMutex);
            mergeResult = JsonSerialization::ApplyPatch(m_settings, m_settings.GetAllocator(), jsonPatch, mergeApproach, m_applyPatchSettings);
            anchorType = SettingsRegistryImplInternal::RapidjsonToSettingsRegistryType(m_settings);
        }
        else
        {
            Pointer root(rootKey.data(), rootKey.length());
            if (root.IsValid())
            {
                AZStd::scoped_lock lock(m_settingMutex);
                Value& rootValue = root.Create(m_settings, m_settings.GetAllocator());
                mergeResult = JsonSerialization::ApplyPatch(rootValue, m_settings.GetAllocator(), jsonPatch, mergeApproach, m_applyPatchSettings);
                anchorType = SettingsRegistryImplInternal::RapidjsonToSettingsRegistryType(rootValue);
            }
            else
            {
                AZ_Error("Settings Registry", false, R"(Failed to root path "%.*s" is invalid.)",
                    aznumeric_cast<int>(rootKey.length()), rootKey.data());
                AZStd::scoped_lock lock(m_settingMutex);
                pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                    .AddMember(StringRef("Error"), StringRef("Invalid root key."), m_settings.GetAllocator())
                    .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
                return false;
            }
        }
        if (mergeResult.GetProcessing() != JsonSerializationResult::Processing::Completed)
        {
            AZ_Error("Settings Registry", false, R"(Failed to fully merge registry file "%s".)", path);
            AZStd::scoped_lock lock(m_settingMutex);
            pointer.Create(m_settings, m_settings.GetAllocator()).SetObject()
                .AddMember(StringRef("Error"), StringRef("Failed to fully merge registry file."), m_settings.GetAllocator())
                .AddMember(StringRef("Path"), Value(path, m_settings.GetAllocator()), m_settings.GetAllocator());
            return false;
        }

        {
            AZStd::scoped_lock lock(m_settingMutex);
            pointer.Create(m_settings, m_settings.GetAllocator()).SetString(path, m_settings.GetAllocator());
        }

        SignalNotifier(rootKey, anchorType);

        return true;
    }

    void SettingsRegistryImpl::SetApplyPatchSettings(const AZ::JsonApplyPatchSettings& applyPatchSettings)
    {
        m_applyPatchSettings = applyPatchSettings;
    }
    void SettingsRegistryImpl::GetApplyPatchSettings(AZ::JsonApplyPatchSettings& applyPatchSettings)
    {
        applyPatchSettings = m_applyPatchSettings;
    }

    void SettingsRegistryImpl::SetUseFileIO(bool useFileIo)
    {
        m_useFileIo = useFileIo;
    }
} // namespace AZ
