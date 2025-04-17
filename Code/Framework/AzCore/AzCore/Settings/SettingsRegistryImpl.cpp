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
#include <AzCore/JSON/error/en.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/Serialization/Json/JsonImporter.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/Serialization/Locale.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/ranges/ranges_algorithm.h>
#include <AzCore/std/ranges/split_view.h>

namespace AZ::SettingsRegistryImplInternal
{
    [[nodiscard]] AZ::SettingsRegistryInterface::Type RapidjsonToSettingsRegistryType(const rapidjson::Value& value)
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
    SettingsRegistryImpl::ScopedMergeEvent::ScopedMergeEvent(SettingsRegistryImpl& settingsRegistry,
        MergeEventArgs mergeEventArgs)
        : m_settingsRegistry{ settingsRegistry }
        , m_mergeEventArgs{ AZStd::move(mergeEventArgs) }
    {
        {
            // Push the file to be merged under protection of the Settings Mutex
            AZStd::scoped_lock lock(m_settingsRegistry.LockForWriting());
            m_settingsRegistry.m_mergeFilePathStack.emplace(m_mergeEventArgs.m_mergeFilePath);
        }
        m_settingsRegistry.m_preMergeEvent.Signal(mergeEventArgs);
    }

    SettingsRegistryImpl::ScopedMergeEvent::~ScopedMergeEvent()
    {
        m_settingsRegistry.m_postMergeEvent.Signal(m_mergeEventArgs);

        {
            // Pop the file that finished merging under protection of the Settings Mutex
            AZStd::scoped_lock lock(m_settingsRegistry.LockForWriting());
            m_settingsRegistry.m_mergeFilePathStack.pop();
        }
    }

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
            AZStd::scoped_lock lock(LockForReading());

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
        // Initialize the setting registry with a empty json object '{}'
        m_settings.SetObject();
    }

    SettingsRegistryImpl::SettingsRegistryImpl(bool useFileIo)
        : SettingsRegistryImpl()
    {
        m_useFileIo = useFileIo;
    }

    SettingsRegistryImpl::~SettingsRegistryImpl() = default;

    void SettingsRegistryImpl::SetContext(SerializeContext* context)
    {
        AZStd::scoped_lock lock(LockForWriting());

        m_serializationSettings.m_serializeContext = context;
        m_deserializationSettings.m_serializeContext = context;
    }

    void SettingsRegistryImpl::SetContext(JsonRegistrationContext* context)
    {
        AZStd::scoped_lock lock(LockForWriting());

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
            // During GetValue and Visit, we are not about to mutate
            // the values in the registry, so do NOT call LockForWriting, just
            // lock the mutex.
            AZStd::scoped_lock lock(LockForReading());
            const rapidjson::Value* value = pointer.Get(m_settings);
            if (value)
            {
                StackedString jsonPath(StackedString::Format::JsonPointer);
                if (!path.empty())
                {
                    path.remove_prefix(1); // Remove the leading slash as the StackedString will add this back in.
                    // Push each JSON pointer reference token to avoid '/' being encoded
                    AZStd::ranges::for_each(path | AZStd::views::split(JsonPointerReferenceTokenPrefix),
                        [&jsonPath](AZStd::string_view refToken) { jsonPath.Push(refToken); });
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

            VisitResponse Traverse(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, VisitAction action) override
            {
                return m_callback(visitArgs, action);
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
            AZStd::scoped_lock lock(LockForWriting());
            preMergeHandler.Connect(m_preMergeEvent);
        }
        return preMergeHandler;
    }

    auto SettingsRegistryImpl::RegisterPreMergeEvent(PreMergeEventHandler& preMergeHandler) -> void
    {
        AZStd::scoped_lock lock(LockForWriting());
        preMergeHandler.Connect(m_preMergeEvent);
    }

    auto SettingsRegistryImpl::RegisterPostMergeEvent(PostMergeEventCallback callback) -> PostMergeEventHandler
    {
        PostMergeEventHandler postMergeHandler{ AZStd::move(callback) };
        {
            AZStd::scoped_lock lock(LockForWriting());
            postMergeHandler.Connect(m_postMergeEvent);
        }
        return postMergeHandler;
    }

    auto SettingsRegistryImpl::RegisterPostMergeEvent(PostMergeEventHandler& postMergeHandler) -> void
    {
        AZStd::scoped_lock lock(LockForWriting());
        postMergeHandler.Connect(m_postMergeEvent);
    }

    void SettingsRegistryImpl::ClearMergeEvents()
    {
        AZStd::scoped_lock lock(LockForWriting());
        m_preMergeEvent.DisconnectAllHandlers();
        m_postMergeEvent.DisconnectAllHandlers();
    }

    void SettingsRegistryImpl::SignalNotifier(AZStd::string_view jsonPath, SettingsType type)
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
            m_signalNotifierQueue.push_back({
                FixedValueString{jsonPath},
                type,
                !m_mergeFilePathStack.empty() ? m_mergeFilePathStack.top() : "<in-memory>"});
            // If the signal count was 0, then a dispatch is in progress
            if (m_signalCount++ == 0)
            {
                AZStd::swap(localNotifierQueue, m_signalNotifierQueue);
            }
        }

        while (!localNotifierQueue.empty())
        {
            for (const SignalNotifierArgs& notifierArgs : localNotifierQueue)
            {
                localNotifierEvent.Signal({ notifierArgs.m_jsonPath, notifierArgs.m_type, notifierArgs.m_mergeFilePath.Native()});
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

    [[nodiscard]] SettingsRegistryInterface::SettingsType SettingsRegistryImpl::GetType(AZStd::string_view path) const
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
            AZStd::scoped_lock lock(LockForReading());
            return GetTypeNoLock(path);
        }
        return SettingsType{};
    }

    [[nodiscard]] SettingsRegistryInterface::SettingsType SettingsRegistryImpl::GetTypeNoLock(AZStd::string_view path) const
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
            if (const rapidjson::Value* value = pointer.Get(m_settings); value != nullptr)
            {
                SettingsType type;
                type.m_type = SettingsRegistryImplInternal::RapidjsonToSettingsRegistryType(*value);
                if (value->IsInt64())
                {
                    type.m_signedness = Signedness::Signed;
                }
                else if (value->IsUint64())
                {
                    type.m_signedness = Signedness::Unsigned;
                }
                return type;
            }
        }
        return { Type::NoType, Signedness::None };
    }


    bool SettingsRegistryImpl::Get(bool& result, AZStd::string_view path) const
    {
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(s64& result, AZStd::string_view path) const
    {
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(u64& result, AZStd::string_view path) const
    {
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(double& result, AZStd::string_view path) const
    {
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(AZStd::string& result, AZStd::string_view path) const
    {
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::Get(FixedValueString& result, AZStd::string_view path) const
    {
        return GetValueInternal(result, path);
    }

    bool SettingsRegistryImpl::GetObject(void* result, AZ::Uuid resultTypeID, AZStd::string_view path) const
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
            AZStd::scoped_lock lock(LockForReading());
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
        if (AZStd::scoped_lock lock(LockForWriting()); !SetValueInternal(path, value))
        {
            return false;
        }
        SignalNotifier(path, { Type::Boolean });
        return true;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, s64 value)
    {
        if (AZStd::scoped_lock lock(LockForWriting()); !SetValueInternal(path, value))
        {
            return false;
        }
        SignalNotifier(path, { Type::Integer, Signedness::Signed });
        return true;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, u64 value)
    {
        if (AZStd::scoped_lock lock(LockForWriting()); !SetValueInternal(path, value))
        {
            return false;
        }
        SignalNotifier(path, { Type::Integer, Signedness::Unsigned });
        return true;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, double value)
    {
        if (AZStd::scoped_lock lock(LockForWriting()); !SetValueInternal(path, value))
        {
            return false;
        }
        SignalNotifier(path, { Type::FloatingPoint });
        return true;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, AZStd::string_view value)
    {
        if (AZStd::scoped_lock lock(LockForWriting()); !SetValueInternal(path, value))
        {
            return false;
        }
        SignalNotifier(path, SettingsType{ Type::String });
        return true;
    }

    bool SettingsRegistryImpl::Set(AZStd::string_view path, const char* value)
    {
        return Set(path, AZStd::string_view{ value });
    }

    bool SettingsRegistryImpl::SetObject(AZStd::string_view path, const void* value, AZ::Uuid valueTypeID)
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
                SettingsType anchorType;
                {
                    AZStd::scoped_lock lock(LockForWriting());
                    rapidjson::Value& setting = pointer.Create(m_settings, m_settings.GetAllocator());
                    setting = AZStd::move(store);
                    anchorType = GetTypeNoLock(path);
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

        bool removeSuccess;
        {
            AZStd::scoped_lock lock(LockForWriting());
            removeSuccess = pointerPath.Erase(m_settings);
        }

        // The removal type is Type::NoType
        constexpr SettingsType removeType;
        SignalNotifier(path, removeType);

        return removeSuccess;
    }

    bool SettingsRegistryImpl::MergeCommandLineArgument(AZStd::string_view argument, AZStd::string_view rootKey,
        const CommandLineArgumentSettings& commandLineSettings)
    {
        // Incoming data is always in "C" locale.
        AZ::Locale::ScopedSerializationLocale scopedLocale;

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

    auto SettingsRegistryImpl::MergeSettings(AZStd::string_view data, Format format, AZStd::string_view anchorKey)
        -> MergeSettingsResult
    {
        return MergeSettingsString(data, format, anchorKey, AZ::IO::PathView{});
    }

    auto SettingsRegistryImpl::MergeSettingsFile(AZStd::string_view path, Format format, AZStd::string_view rootKey,
        [[maybe_unused]] AZStd::vector<char>* scratchBuffer)
        -> MergeSettingsResult
    {
        using namespace rapidjson;

        if (path.empty())
        {
            MergeSettingsResult errorResult;
            errorResult.m_returnCode = MergeSettingsReturnCode::Failure;
            errorResult.m_operationMessages = "Path provided for MergeSettingsFile is empty.";
            return errorResult;
        }

        MergeSettingsResult result;
        result.m_returnCode = MergeSettingsReturnCode::Success;
        if (path[path.length()] == 0)
        {
            result = MergeSettingsFileInternal(path.data(), format, rootKey);
        }
        else
        {
            if (AZ::IO::MaxPathLength < path.size())
            {
                result.m_returnCode = MergeSettingsReturnCode::Failure;
                result.m_operationMessages += AZStd::string::format(
                    R"(Path "%.*s" is too long. Either make sure that the provided path is terminated or use a shorter path.)",
                    AZ_STRING_ARG(path));

                return result;
            }
            AZ::IO::FixedMaxPathString filePath(path);
            result = MergeSettingsFileInternal(filePath.c_str(), format, rootKey);
        }

        return result;
    }

    auto SettingsRegistryImpl::MergeSettingsFolder(AZStd::string_view path, const Specializations& specializations,
        AZStd::string_view platform, AZStd::string_view rootKey, [[maybe_unused]] AZStd::vector<char>* scratchBuffer)
        -> MergeSettingsResult
    {
        using namespace AZ::IO;
        using namespace rapidjson;

        if (path.empty())
        {
            MergeSettingsResult result;
            result.m_returnCode = MergeSettingsReturnCode::Failure;
            result.m_operationMessages += "Path provided for MergeSettingsFolder is empty.";
            return result;
        }

        size_t additionalSpaceRequired = 3; // 3 is for the '/', '*' and 0
        if (!platform.empty())
        {
            additionalSpaceRequired += AZ_ARRAY_SIZE(PlatformFolder) + platform.length() + 2; // +2 for the two slashes.
        }

        if (path.length() + additionalSpaceRequired > AZ::IO::MaxPathLength)
        {
            MergeSettingsResult result;
            result.m_returnCode = MergeSettingsReturnCode::Failure;
            result.m_operationMessages += AZStd::string::format(
                "Folder path for the Setting Registry is too long: %.*s\n",
                AZ_STRING_ARG(path));

            return result;
        }

        RegistryFileList fileList;

        AZ::IO::FixedMaxPath folderPath{ path };

        const size_t platformKeyOffset = folderPath.Native().size();
        folderPath /= '*';

        MergeSettingsResult multiFileResult;

        auto CreateSettingsFindCallback = [this, &fileList, &specializations, &folderPath,
            &multiFileResult](bool isPlatformFile)
        {
            return [this, &fileList, &specializations, &folderPath, isPlatformFile,
                &multiFileResult](AZStd::string_view filename, bool isFile) -> bool
            {
                if (isFile)
                {
                    if (fileList.size() >= MaxRegistryFolderEntries)
                    {
                        AZ_Error("Settings Registry", false, "Too many files in registry folder.");
                        AZStd::scoped_lock lock(LockForWriting());
                        multiFileResult.m_operationMessages += AZStd::string::format(R"(Too many files in registry folder "%s".)"
                            " The limit is %zu\n", folderPath.c_str(), MaxRegistryFolderEntries);
                        multiFileResult.Combine(MergeSettingsReturnCode::Failure);
                        return false;
                    }

                    RegistryFile& registryFile = fileList.emplace_back();
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
            MergeSettingsResult result;
            result.m_returnCode = MergeSettingsReturnCode::Success;
            auto sorter = [this, &result, &specializations, path](
                const RegistryFile& lhs, const RegistryFile& rhs) -> bool
            {
                return IsLessThan(result, lhs, rhs, specializations, path);
            };
            AZStd::sort(fileList.begin(), fileList.end(), sorter);

            if (!result)
            {
                return result;
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

                // Combine the MergeSettings result of each MergeSettingsFileInternal
                // operation
                if (!registryFile.m_isPatch)
                {
                    multiFileResult.Combine(MergeSettingsFileInternal(folderPath.c_str(), Format::JsonMergePatch, rootKey));
                }
                else
                {
                    multiFileResult.Combine(MergeSettingsFileInternal(folderPath.c_str(), Format::JsonPatch, rootKey));
                }
            }
        }

        return multiFileResult;
    }

    SettingsRegistryInterface::VisitResponse SettingsRegistryImpl::Visit(Visitor& visitor, StackedString& path, AZStd::string_view valueName,
        const rapidjson::Value& value) const
    {
        ++m_visitDepth;
        VisitResponse result;
        VisitArgs visitArgs(*this);
        switch (value.GetType())
        {
        case rapidjson::Type::kNullType:
            visitArgs.m_jsonKeyPath = path;
            visitArgs.m_fieldName = valueName;
            visitArgs.m_type = { Type::Null };
            result = visitor.Traverse(visitArgs, VisitAction::Value);
            break;
        case rapidjson::Type::kFalseType:
            visitArgs.m_jsonKeyPath = path;
            visitArgs.m_fieldName = valueName;
            visitArgs.m_type = { Type::Boolean };
            result = visitor.Traverse(visitArgs, VisitAction::Value);
            if (result == VisitResponse::Continue)
            {
                visitor.Visit(visitArgs, false);
            }
            break;
        case rapidjson::Type::kTrueType:
            visitArgs.m_jsonKeyPath = path;
            visitArgs.m_fieldName = valueName;
            visitArgs.m_type = { Type::Boolean };
            result = visitor.Traverse(visitArgs, VisitAction::Value);
            if (result == VisitResponse::Continue)
            {
                visitor.Visit(visitArgs, true);
            }
            break;
        case rapidjson::Type::kObjectType:
            visitArgs.m_jsonKeyPath = path;
            visitArgs.m_fieldName = valueName;
            visitArgs.m_type = { Type::Object };
            result = visitor.Traverse(visitArgs, VisitAction::Begin);
            if (result == VisitResponse::Continue)
            {
                for (const auto& member : value.GetObject())
                {
                    AZStd::string_view fieldName(member.name.GetString(), member.name.GetStringLength());
                    path.Push(fieldName);
                    if (Visit(visitor, path, fieldName, member.value) == VisitResponse::Done)
                    {
                        --m_visitDepth;
                        return VisitResponse::Done;
                    }
                    path.Pop();
                }

                // Must refresh the m_jsonKeyPath string view as the for loop would modify the StackedString
                // and therefore invalidate the string_view
                visitArgs.m_jsonKeyPath = path;
                visitArgs.m_fieldName = valueName;
                if (visitor.Traverse(visitArgs, VisitAction::End) == VisitResponse::Done)
                {
                    --m_visitDepth;
                    return VisitResponse::Done;
                }
            }
            break;
        case rapidjson::Type::kArrayType:
            visitArgs.m_jsonKeyPath = path;
            visitArgs.m_fieldName = valueName;
            visitArgs.m_type = { Type::Array };
            result = visitor.Traverse(visitArgs, VisitAction::Begin);
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
                        --m_visitDepth;
                        return VisitResponse::Done;
                    }
                    counter++;
                    path.Pop();
                }

                // Must refresh the m_jsonKeyPath string view as the for loop would modify the StackedString
                // and therefore invalidate the string_view
                visitArgs.m_jsonKeyPath = path;
                visitArgs.m_fieldName = valueName;
                if (visitor.Traverse(visitArgs, VisitAction::End) == VisitResponse::Done)
                {
                    --m_visitDepth;
                    return VisitResponse::Done;
                }
            }
            break;
        case rapidjson::Type::kStringType:
            visitArgs.m_jsonKeyPath = path;
            visitArgs.m_fieldName = valueName;
            visitArgs.m_type = { Type::String };
            result = visitor.Traverse(visitArgs, VisitAction::Value);
            if (result == VisitResponse::Continue)
            {
                visitor.Visit(visitArgs, AZStd::string_view(value.GetString(), value.GetStringLength()));
            }
            break;
        case rapidjson::Type::kNumberType:
            if (value.IsDouble())
            {
                visitArgs.m_jsonKeyPath = path;
                visitArgs.m_fieldName = valueName;
                visitArgs.m_type = { Type::FloatingPoint };
                result = visitor.Traverse(visitArgs, VisitAction::Value);
                if (result == VisitResponse::Continue)
                {
                    visitor.Visit(visitArgs, value.GetDouble());
                }
            }
            else
            {
                visitArgs.m_jsonKeyPath = path;
                visitArgs.m_fieldName = valueName;
                visitArgs.m_type = { Type::Integer, value.IsInt64() ? Signedness::Signed : Signedness::Unsigned };
                result = visitor.Traverse(visitArgs, VisitAction::Value);
                if (result == VisitResponse::Continue)
                {
                    if (visitArgs.m_type.m_signedness == Signedness::Signed)
                    {
                        s64 integerValue = value.GetInt64();
                        visitor.Visit(visitArgs, integerValue);
                    }
                    else
                    {
                        u64 integerValue = value.GetUint64();
                        visitor.Visit(visitArgs, integerValue);
                    }
                }
            }
            break;
        default:
            AZ_Assert(false, "Unsupported RapidJSON type: %i.", aznumeric_cast<int>(value.GetType()));
            result = VisitResponse::Done;
        }

        --m_visitDepth;
        return result;
    }

    bool SettingsRegistryImpl::IsLessThan(MergeSettingsResult& collisionFoundResult, const RegistryFile& lhs, const RegistryFile& rhs,
        const Specializations& specializations, AZStd::string_view folderPath)
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

        collisionFoundResult.m_returnCode = MergeSettingsReturnCode::Failure;
        // Append to the error message each pair of registry files with specialization conflicts
        collisionFoundResult.m_operationMessages += AZStd::string::format(R"(Two registry files in "%.*s" point to the same specialization: "%s" and "%s")",
            AZ_STRING_ARG(folderPath), lhs.m_relativePath.c_str(), rhs.m_relativePath.c_str());

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

    auto SettingsRegistryImpl::MergeSettingsFileInternal(const char* path, Format format, AZStd::string_view rootKey)
        -> MergeSettingsResult
    {
        AZStd::string jsonData;
        if (MergeSettingsResult loadFileResult = LoadJsonFileIntoString(jsonData, path);
            !loadFileResult)
        {
            // If the Json file failed to load, then return that result
            return loadFileResult;
        }

        return MergeSettingsString(AZStd::move(jsonData), format, rootKey, path);
    }

    auto SettingsRegistryImpl::LoadJsonFileIntoString(AZStd::string& jsonData, const char* filePath)
        -> MergeSettingsResult
    {
        AZ::IO::FileReader fileReader;
        // If the path of "-" is supplied, then use a FileReader
        // that reads from stdin
        if (AZ::IO::PathView pathView(filePath);
            pathView == "-")
        {
            fileReader = AZ::IO::FileReader::GetStdin();
        }
        else
        {
            fileReader = AZ::IO::FileReader(m_useFileIo ? AZ::IO::FileIOBase::GetInstance() : nullptr, filePath);
        }
        if (!fileReader.IsOpen())
        {
            MergeSettingsResult result;
            result.Combine(MergeSettingsReturnCode::Failure);
            result.m_operationMessages = AZStd::string::format(R"(Unable to open registry file "%s".)", filePath);
            return result;
        }

        AZ::u64 fileSize = fileReader.Length();
        if (fileSize == 0)
        {
            MergeSettingsResult result;
            result.Combine(MergeSettingsReturnCode::Failure);
            result.m_operationMessages = AZStd::string::format(R"(Registry file "%s" is 0 bytes in length. There is no nothing to merge)", filePath);
            return result;
        }

        auto ReadJsonIntoString = [&fileReader](char* buffer, size_t size)
        {
            // If the file cannot be complete read into memoryy
            // return 0 as the size for the string
            return fileReader.Read(size, buffer) == size ? size : 0;
        };
        jsonData.resize_and_overwrite(fileSize, ReadJsonIntoString);

        // If the string is empty then the file could not be read
        if (jsonData.empty())
        {
            MergeSettingsResult result;
            result.Combine(MergeSettingsReturnCode::Failure);
            result.m_operationMessages = AZStd::string::format(R"(Unable to read registry file "%s".)", filePath);
            return result;
        }

        // The file has been successfulyl merge therefore return a MergeSettingsResult of success
        MergeSettingsResult mergeResult;
        mergeResult.Combine(MergeSettingsReturnCode::Success);
        return mergeResult;
    }

    auto SettingsRegistryImpl::MergeSettingsString(AZStd::string jsonData, Format format, AZStd::string_view anchorKey,
        AZ::IO::PathView filePath)
        -> MergeSettingsResult
    {
        // There is no work to be done for an empty JSON string
        if (jsonData.empty())
        {
            MergeSettingsResult mergeResult;
            mergeResult.m_returnCode = MergeSettingsReturnCode::Failure;
            mergeResult.m_operationMessages = "JSON String is empty. No merging to be done";
            return mergeResult;
        }

        rapidjson::Document jsonPatch;
        constexpr int flags = rapidjson::kParseStopWhenDoneFlag | rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag;
        jsonPatch.ParseInsitu<flags>(jsonData.data());
        if (jsonPatch.HasParseError())
        {
            MergeSettingsResult mergeResult;
            mergeResult.m_returnCode = MergeSettingsReturnCode::Failure;
            mergeResult.m_operationMessages = "Unable to parse ";
            mergeResult.m_operationMessages +=
                filePath.empty() ? "data" : AZStd::string::format(R"(registry file "%.*s")", AZ_PATH_ARG(filePath));
            mergeResult.m_operationMessages += AZStd::string::format(
                " at offset %zu.\nError: %s", jsonPatch.GetErrorOffset(), rapidjson::GetParseError_En(jsonPatch.GetParseError()));
            return mergeResult;
        }

        // Delegate to the MergeSettingsJsonDocument function to merge the settings to registry
        return MergeSettingsJsonDocument(jsonPatch, format, anchorKey, filePath);
    }

    auto SettingsRegistryImpl::MergeSettingsJsonDocument(const rapidjson::Document& jsonPatch, Format format,
        AZStd::string_view anchorKey, AZ::IO::PathView filePath)
            -> MergeSettingsResult
    {
        MergeSettingsResult mergeResult;

        JsonMergeApproach mergeApproach;
        switch (format)
        {
        case Format::JsonPatch:
            mergeApproach = JsonMergeApproach::JsonPatch;
            break;
        case Format::JsonMergePatch:
            mergeApproach = JsonMergeApproach::JsonMergePatch;
            if (anchorKey.empty() && !jsonPatch.IsObject())
            {
                mergeResult.Combine(MergeSettingsReturnCode::Failure);
                mergeResult.m_operationMessages = AZStd::string::format(
                    R"(Cannot merge settings registry JSON data due to root element field with a non-JSON Object)"
                    R"( using the JSON Merge Patch approach. The JSON MergePatch algorithm would)"
                    R"( overwrite all settings at the supplied root-key path and therefore merging has been)"
                    R"( disallowed to prevent field destruction.)" "\n"
                    R"(To merge the supplied settings registry file, the settings within it must be placed within a JSON Object '{}')"
                    R"( in order to allow moving of its fields using the root-key as an anchor.)");
                return mergeResult;
            }
            break;
        default:
            mergeResult.Combine(MergeSettingsReturnCode::Failure);
            mergeResult.m_operationMessages = AZStd::string::format(
                R"(Provided format for merging settings into the Setting Registry is unsupported. Format enum value is %d)",
                static_cast<int>(format));
            return mergeResult;
        }

        rapidjson::Pointer anchorPath;
        if (!anchorKey.empty())
        {
            anchorPath = rapidjson::Pointer(anchorKey.data(), anchorKey.size());
            if (!anchorPath.IsValid())
            {
                mergeResult.Combine(MergeSettingsReturnCode::Failure);
                mergeResult.m_operationMessages = AZStd::string::format(R"(Anchor path "%.*s" is invalid.)",
                    AZ_STRING_ARG(anchorKey));

                return mergeResult;
            }
        }

        // Add a reporting callback to capture JSON patch operations while merging if the merge operations notify
        // option is enabled
        JsonApplyPatchSettings applyPatchSettings;
         // Stores unique list of settings keys that are being merged in the JsonSerialization::ApplyPatch calls below
        AZStd::vector<AZStd::string> mergedSettingsKeys;
        if (m_mergeOperationNotify)
        {
            applyPatchSettings.m_reporting = [&mergedSettingsKeys, scratchBuffer = AZStd::string{}]
            (AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view jsonPath) mutable
                -> AZ::JsonSerializationResult::ResultCode
            {
                if (result.GetTask() == JsonSerializationResult::Tasks::Merge
                    && result.GetProcessing() == JsonSerializationResult::Processing::Completed)
                {
                    auto FindSettingsKey = [&jsonPath](AZStd::string_view settingKeyPath)
                    {
                        return jsonPath == settingKeyPath;
                    };

                    // If the settings key entry is not part of the merged Settings Key set append it to it
                    if (AZStd::ranges::find_if(mergedSettingsKeys, FindSettingsKey) == AZStd::ranges::end(mergedSettingsKeys))
                    {
                        mergedSettingsKeys.push_back(jsonPath);
                    }
                }

                // This is the default issue reporting, that logs using the warning category
                if (result.GetProcessing() != JsonSerializationResult::Processing::Completed)
                {
                    scratchBuffer.append(message.begin(), message.end());
                    scratchBuffer.append("\n    Reason: ");
                    result.AppendToString(scratchBuffer, jsonPath);
                    scratchBuffer.append(".");
                    AZ_Warning("JSON Serialization", false, "%s", scratchBuffer.c_str());

                    scratchBuffer.clear();
                }
                return result;
            };
        }

        // First resolve any $import directives in the source JSON blob in place, while maintaining
        // the order of fields
        // Any fields that are after $import directives override the imported JSON data, while fields before
        // are overridden by the $import directive
        AZ::StackedString importedFieldKey(AZ::StackedString::Format::JsonPointer);
        JsonImportResolver::ImportPathStack importPathStack;
        importPathStack.push_back(filePath);

        // JSON Importer lifetime needs to be larger than the import settings
        AZ::BaseJsonImporter jsonImporter;
        AZ::JsonImportSettings importSettings;
        importSettings.m_reporting = [&mergeResult](AZStd::string_view message,
            AZ::JsonSerializationResult::ResultCode result, AZStd::string_view)
        {
            // Store any JSON Importer messages as part of the merge result
            mergeResult.m_operationMessages += message;
            mergeResult.m_operationMessages += '\n';
            return result;
        };
        importSettings.m_importer = &jsonImporter;
        // Set the resolve flags to ImportTracking::Import to get all the metadata information
        // (import directive json pointer path, import directive relative path value, import directive resolved path value)
        importSettings.m_resolveFlags = ImportTracking::Imports;
        // Use the supplied file path as the path for for the current JSON document @jsonPatch
        importSettings.m_loadedJsonPath = filePath;

        rapidjson::Value jsonPatchPostImport;
        if (AZ::JsonSerializationResult::ResultCode result = AZ::JsonImportResolver::ResolveImportsInOrder(jsonPatchPostImport,
            m_settings.GetAllocator(), jsonPatch,
            importPathStack, importSettings, importedFieldKey);
            result.GetProcessing() != JsonSerializationResult::Processing::Completed)
        {
            // Mark merge operation as having failed, if the JSON Importer has not completed
            mergeResult.Combine(MergeSettingsReturnCode::Failure);
        }

        // Create a scoped merge event to trigger the PreMerge notification on construction
        // and PostMerge event on destruction
        ScopedMergeEvent scopedMergeEvent(*this, { filePath.Native(), anchorKey });
        SettingsType anchorType;
        {
            AZStd::scoped_lock lock(LockForWriting());

            rapidjson::Value& anchorRoot = anchorPath.IsValid() ? anchorPath.Create(m_settings, m_settings.GetAllocator())
                : m_settings;

            // Merge the @jsonPatchPostImport object after the imports have been resolved into the Settings Registry
            JsonSerializationResult::ResultCode patchResult =
                JsonSerialization::ApplyPatch(anchorRoot, m_settings.GetAllocator(), jsonPatchPostImport, mergeApproach, applyPatchSettings);
            if (patchResult.GetProcessing() != JsonSerializationResult::Processing::Completed)
            {
                mergeResult.Combine(MergeSettingsReturnCode::Failure);
                mergeResult.m_operationMessages = AZStd::string::format(R"(Failed to fully merge data into registry.)"
                    " Merge failed with resultCode %s.",
                    patchResult.ToString(anchorKey).c_str());
                return mergeResult;
            }

            // The settings have been successfully merged, query the type at the anchor key
            anchorType = GetTypeNoLock(anchorKey);
        }

        // For each merged settings key, signal the notifier event
        for (AZStd::string_view mergedSettingsKey : mergedSettingsKeys)
        {
            SignalNotifier(mergedSettingsKey, GetType(mergedSettingsKey));
        }

        SignalNotifier(anchorKey, anchorType);

        return mergeResult;
    }

    void SettingsRegistryImpl::SetNotifyForMergeOperations(bool notify)
    {
        m_mergeOperationNotify = notify;
    }
    bool SettingsRegistryImpl::GetNotifyForMergeOperations() const
    {
        return m_mergeOperationNotify;
    }

    void SettingsRegistryImpl::SetUseFileIO(bool useFileIo)
    {
        m_useFileIo = useFileIo;
    }

    AZStd::scoped_lock<AZStd::recursive_mutex> SettingsRegistryImpl::LockForWriting() const
    {
        // ensure that we aren't actively iterating over this data that is about to be
        // invalid.
        AZ_Assert(m_visitDepth == 0, "Attempt to mutate the Settings Registry while visiting, "
            "this may invalidate visitor iterators and cause crashes.  Visit depth is %i", m_visitDepth);
        return AZStd::scoped_lock(m_settingMutex);
    }

    AZStd::scoped_lock<AZStd::recursive_mutex> SettingsRegistryImpl::LockForReading() const
    {
        return AZStd::scoped_lock(m_settingMutex);
    }
} // namespace AZ
