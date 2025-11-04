/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Dumper.h> // Moved to the top because AssetSerializer requires include for the SerializeContext
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonSerializationSettings.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings/TextParser.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <Application.h>
#include <Utilities.h>


namespace AZ::SerializeContextTools
{
    inline namespace Streams
    {
        // Using an inline namespace for the Function Object Stream

        /*
        * Implementation of the GenericStream interface
        * that uses a function object for writing
        */
        class FunctorStream
            : public AZ::IO::GenericStream
        {
        public:
            using WriteCallback = AZStd::function<AZ::IO::SizeType(AZStd::span<AZStd::byte const>)>;
            FunctorStream() = default;
            FunctorStream(WriteCallback writeCallback);

            bool IsOpen() const override;
            bool CanSeek() const override;
            bool CanRead() const override;
            bool CanWrite() const override;
            void Seek(AZ::IO::OffsetType, SeekMode) override;
            AZ::IO::SizeType Read(AZ::IO::SizeType, void*) override;
            AZ::IO::SizeType Write(AZ::IO::SizeType bytes, const void* iBuffer) override;
            AZ::IO::SizeType GetCurPos() const override;
            AZ::IO::SizeType GetLength() const override;
            AZ::IO::OpenMode GetModeFlags() const override;
            const char* GetFilename() const override;
        private:
            WriteCallback m_streamWriter;
        };

        FunctorStream::FunctorStream(WriteCallback writeCallback)
            : m_streamWriter { AZStd::move(writeCallback)}
        {}

        bool FunctorStream::IsOpen() const
        {
            return static_cast<bool>(m_streamWriter);
        }
        bool FunctorStream::CanSeek() const
        {
            return false;
        }
        bool FunctorStream::CanRead() const
        {
            return false;
        }

        bool FunctorStream::CanWrite() const
        {
            return true;
        }

        void FunctorStream::Seek(AZ::IO::OffsetType, SeekMode)
        {
            AZ_Assert(false, "Cannot seek in stdout stream");
        }

        AZ::IO::SizeType FunctorStream::Read(AZ::IO::SizeType, void*)
        {
            AZ_Assert(false, "The stdout file handle cannot be read from");
            return 0;
        }

        AZ::IO::SizeType FunctorStream::Write(AZ::IO::SizeType bytes, const void* iBuffer)
        {
            if (m_streamWriter)
            {
                return m_streamWriter(AZStd::span(reinterpret_cast<const AZStd::byte*>(iBuffer), bytes));
            }

            return 0;
        }

        AZ::IO::SizeType FunctorStream::GetCurPos() const
        {
            return 0;
        }

        AZ::IO::SizeType FunctorStream::GetLength() const
        {
            return 0;
        }

        AZ::IO::OpenMode FunctorStream::GetModeFlags() const
        {
            return AZ::IO::OpenMode::ModeWrite;
        }

        const char* FunctorStream::GetFilename() const
        {
            return "<function object>";
        }
    }
} // namespace AZ::SerializeContextTools::inline Stream


namespace AZ::SerializeContextTools
{
    static auto GetWriteBypassStdoutCapturerFunctor(Application& application)
    {
        return [&application](AZStd::span<AZStd::byte const> outputBytes)
        {
            // If the application is currently capturing stdout, use stdout capturer to write
            // directly to the file descriptor of stdout
            if (AZ::IO::FileDescriptorCapturer* stdoutCapturer = application.GetStdoutCapturer();
                stdoutCapturer != nullptr)
            {
                return stdoutCapturer->WriteBypassingCapture(outputBytes.data(), aznumeric_caster(outputBytes.size()));
            }
            else
            {
                constexpr int StdoutDescriptor = 1;
                return AZ::IO::PosixInternal::Write(StdoutDescriptor, outputBytes.data(), aznumeric_caster(outputBytes.size()));
            }
        };
    }

    bool Dumper::DumpFiles(Application& application)
    {
        SerializeContext* sc = application.GetSerializeContext();
        if (!sc)
        {
            AZ_Error("SerializeContextTools", false, "No serialize context found.");
            return false;
        }

        AZStd::string outputFolder = Utilities::ReadOutputTargetFromCommandLine(application);

        AZ::IO::Path sourceGameFolder;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(sourceGameFolder.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
        }
        bool result = true;

        AZStd::vector<AZStd::string> fileList = Utilities::ReadFileListFromCommandLine(application, "files");
        for (const AZStd::string& filePath : fileList)
        {
            AZ_Printf("DumpFiles", "Dumping file '%.*s'\n", aznumeric_cast<int>(filePath.size()), filePath.data());

            AZ::IO::FixedMaxPath outputPath{ AZStd::string_view{ outputFolder }};

            outputPath /= AZ::IO::FixedMaxPath(filePath).LexicallyRelative(sourceGameFolder);
            outputPath.Native() += ".dump.txt";

            IO::SystemFile outputFile;
            if (!outputFile.Open(outputPath.c_str(),
                IO::SystemFile::OpenMode::SF_OPEN_CREATE |
                IO::SystemFile::OpenMode::SF_OPEN_CREATE_PATH |
                IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
            {
                AZ_Error("SerializeContextTools", false, "Unable to open file '%s' for writing.", outputPath.c_str());
                result = false;
                continue;
            }

            AZStd::string content;
            content.reserve(1 * 1024 * 1024); // Reserve 1mb to avoid frequently resizing the string.

            auto callback = [&content, &result](void* classPtr, const Uuid& classId, SerializeContext* context)
            {
                result = DumpClassContent(content, classPtr, classId, context) && result;

                const SerializeContext::ClassData* classData = context->FindClassData(classId);
                if (classData && classData->m_factory)
                {
                    classData->m_factory->Destroy(classPtr);
                }
                else
                {
                    AZ_Error("SerializeContextTools", false, "Missing class factory, so data will leak.");
                    result = false;
                }
            };
            if (!AZ::Utils::InspectSerializedFile(filePath.c_str(), sc, callback))
            {
                result = false;
                continue;
            }

            outputFile.Write(content.data(), content.length());
        }

        return result;
    }

    bool Dumper::DumpSerializeContext(Application& application)
    {
        AZStd::string outputPath = Utilities::ReadOutputTargetFromCommandLine(application, "SerializeContext.json");
        AZ_Printf("dumpsc", "Writing Serialize Context at '%s'.\n", outputPath.c_str());

        IO::SystemFile outputFile;
        if (!outputFile.Open(outputPath.c_str(),
            IO::SystemFile::OpenMode::SF_OPEN_CREATE |
            IO::SystemFile::OpenMode::SF_OPEN_CREATE_PATH |
            IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
        {
            AZ_Error("SerializeContextTools", false, "Unable to open output file '%s'.", outputPath.c_str());
            return false;
        }

        SerializeContext* context = application.GetSerializeContext();

        AZStd::vector<Uuid> systemComponents = Utilities::GetSystemComponents(application);
        AZStd::sort(systemComponents.begin(), systemComponents.end());

        rapidjson::Document doc;
        rapidjson::Value& root = doc.SetObject();
        rapidjson::Value scObject;
        scObject.SetObject();

        AZStd::string temp;
        temp.reserve(256 * 1024); // Reserve 256kb of memory to avoid the string constantly resizing.

        bool result = true;
        auto callback = [context, &doc, &scObject, &temp, &systemComponents, &result](const SerializeContext::ClassData* classData, const Uuid& /*typeId*/) -> bool
        {
            if (!DumpClassContent(classData, scObject, doc, systemComponents, context, temp))
            {
                result = false;
            }
            return true;
        };
        context->EnumerateAll(callback, true);
        root.AddMember("SerializeContext", AZStd::move(scObject), doc.GetAllocator());

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        outputFile.Write(buffer.GetString(), buffer.GetSize());
        outputFile.Close();
        return result;
    }

    bool Dumper::DumpTypes(Application& application)
    {
        // outputStream defaults to writing to stdout
        AZStd::variant<FunctorStream, AZ::IO::SystemFileStream> outputStream(AZStd::in_place_type<FunctorStream>,
            GetWriteBypassStdoutCapturerFunctor(application));

        AZ::CommandLine& commandLine = *application.GetAzCommandLine();
        // If the output-file parameter has been supplied open the file path using FileIOStream
        if (size_t optionCount = commandLine.GetNumSwitchValues("output-file"); optionCount > 0)
        {
            AZ::IO::PathView outputPathView(commandLine.GetSwitchValue("output-file", optionCount - 1));
            // If the output file name is a single dash, use the default output stream value which writes to stdout
            if (outputPathView != "-")
            {
                AZ::IO::FixedMaxPath outputPath;
                if (outputPathView.IsRelative())
                {
                    AZ::Utils::ConvertToAbsolutePath(outputPath, outputPathView.Native());
                }
                else
                {
                    outputPath = outputPathView.LexicallyNormal();
                }

                constexpr AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath;

                if (auto& fileStream = outputStream.emplace<AZ::IO::SystemFileStream>(outputPath.c_str(), openMode);
                    !fileStream.IsOpen())
                {
                    AZ_Printf(
                        "dumptypes",
                        R"(Unable to open specified output-file "%s". Object will not be dumped)"
                        "\n",
                        outputPath.c_str());
                    return false;
                }
            }
        }

        AZ::SerializeContext* context = application.GetSerializeContext();

        struct TypeNameIdPair
        {
            AZStd::string m_name;
            AZ::TypeId m_id;

            bool operator==(const TypeNameIdPair& other) const
            {
                return m_name == other.m_name && m_id == other.m_id;
            }
            bool operator!=(const TypeNameIdPair& other) const
            {
                return !operator==(other);
            }

            struct Hash
            {
                size_t operator()(const TypeNameIdPair& key) const
                {
                    size_t hashValue{};
                    AZStd::hash_combine(hashValue, key.m_name);
                    AZStd::hash_combine(hashValue, key.m_id);
                    return hashValue;
                }
            };
        };


        // Append the Type names and type ids to a unordered_set to filter out duplicates
        AZStd::unordered_set<TypeNameIdPair, TypeNameIdPair::Hash> typeNameIdPairsSet;
        auto AppendTypeInfo = [&typeNameIdPairsSet](const SerializeContext::ClassData* classData, const Uuid&) -> bool
        {
            typeNameIdPairsSet.emplace(TypeNameIdPair{ classData->m_name, classData->m_typeId });
            return true;
        };
        context->EnumerateAll(AppendTypeInfo, true);

        // Move over the unordered set over to an array for later
        // The array is potentially sorted depending on the sort option
        AZStd::vector<TypeNameIdPair> typeNameIdPairs(
            AZStd::make_move_iterator(typeNameIdPairsSet.begin()),
            AZStd::make_move_iterator(typeNameIdPairsSet.end())
        );
        // Clear out the Unordered set container
        typeNameIdPairsSet = {};

        // Sort the TypeNameIdPair based on the --sort option value or by type name if not supplied
        enum class SortOptions
        {
            Name,
            TypeId,
            None
        };
        SortOptions sortOption{ SortOptions::Name };

        if (size_t sortOptionCount = commandLine.GetNumSwitchValues("sort"); sortOptionCount > 0)
        {
            AZStd::string sortOptionString = commandLine.GetSwitchValue("sort", sortOptionCount - 1);
            if (sortOptionString == "name")
            {
                sortOption = SortOptions::Name;
            }
            if (sortOptionString == "typeid")
            {
                sortOption = SortOptions::TypeId;
            }
            else if (sortOptionString == "none")
            {
                sortOption = SortOptions::None;
            }
            else
            {
                AZ_Error(
                    "dumptypes", false,
                    R"(Invalid --sort option supplied "%s".)"
                    " Sorting by type name will be used. See --help for valid values)",
                    sortOptionString.c_str());
            }
        }

        switch (sortOption)
        {
        case SortOptions::Name:
        {
            auto SortByName = [](const TypeNameIdPair& lhs, const TypeNameIdPair& rhs)
            {
                return azstricmp(lhs.m_name.c_str(), rhs.m_name.c_str()) < 0;
            };
            AZStd::sort(typeNameIdPairs.begin(), typeNameIdPairs.end(), SortByName);
            break;
        }
        case SortOptions::TypeId:
        {
            auto SortByTypeId = [](const TypeNameIdPair& lhs, const TypeNameIdPair& rhs)
            {
                return lhs.m_id < rhs.m_id;
            };
            AZStd::sort(typeNameIdPairs.begin(), typeNameIdPairs.end(), SortByTypeId);
            break;
        }
        case SortOptions::None:
            [[fallthrough]];
        default:
            break;
        }

        auto GetOutputPathString = [](auto&& stream) -> const char*
        {
            return stream.GetFilename();
        };

        AZ_Printf(
            "dumptypes",
            R"(Writing reflected types to "%s".)"
            "\n",
            AZStd::visit(GetOutputPathString, outputStream));

        auto WriteReflectedTypes = [&typeNameIdPairs](auto&& stream) -> bool
        {
            AZStd::string typeListContents;
            for (auto&& [typeName, typeId] : typeNameIdPairs)
            {
                typeListContents += AZStd::string::format("%s %s\n", typeName.c_str(), typeId.ToFixedString().c_str());
            }

            stream.Write(typeListContents.size(), typeListContents.data());
            stream.Close();
            return true;
        };
        const bool result = AZStd::visit(WriteReflectedTypes, outputStream);

        return result;
    }

    bool Dumper::CreateType(Application& application)
    {
        // outputStream defaults to writing to stdout
        AZStd::variant<FunctorStream, AZ::IO::SystemFileStream> outputStream(AZStd::in_place_type<FunctorStream>,
            GetWriteBypassStdoutCapturerFunctor(application));

        AZ::CommandLine& commandLine = *application.GetAzCommandLine();
        // If the output-file parameter has been supplied open the file path using FileIOStream
        if (size_t optionCount = commandLine.GetNumSwitchValues("output-file"); optionCount > 0)
        {
            AZ::IO::PathView outputPathView(commandLine.GetSwitchValue("output-file", optionCount - 1));
            // If the output file name is a single dash, use the default output stream value which writes to stdout
            if (outputPathView != "-")
            {
                AZ::IO::FixedMaxPath outputPath;
                if (outputPathView.IsRelative())
                {
                    AZ::Utils::ConvertToAbsolutePath(outputPath, outputPathView.Native());
                }
                else
                {
                    outputPath = outputPathView.LexicallyNormal();
                }

                constexpr AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath;

                if (auto& fileStream = outputStream.emplace<AZ::IO::SystemFileStream>(outputPath.c_str(), openMode);
                    !fileStream.IsOpen())
                {
                    AZ_Printf(
                        "createtype",
                        R"(Unable to open specified output-file "%s". Object will not be dumped)"
                        "\n",
                        outputPath.c_str());
                    return false;
                }
            }
        }

        size_t typeIdOptionCount = commandLine.GetNumSwitchValues("type-id");
        size_t typeNameOptionCount = commandLine.GetNumSwitchValues("type-name");
        if (typeIdOptionCount == 0 && typeNameOptionCount == 0)
        {
            AZ_Error("createtype", false, "One of the following options must be supplied: --type-id or --type-name");
            return false;
        }
        if (typeIdOptionCount > 0 && typeNameOptionCount > 0)
        {
            AZ_Error("createtype", false, "The --type-id and --type-name options are mutally exclusive. Only one can be specified");
            return false;
        }

        AZ::SerializeContext* context = application.GetSerializeContext();
        const AZ::SerializeContext::ClassData* classData = nullptr;
        if (typeIdOptionCount > 0)
        {
            AZStd::string typeIdValue = commandLine.GetSwitchValue("type-id", typeIdOptionCount - 1);
            classData = context->FindClassData(AZ::TypeId(typeIdValue.c_str(), typeIdValue.size()));
            if (classData == nullptr)
            {
                AZ_Error("createtype", false, "Type with ID %s is not registered with the SerializeContext", typeIdValue.c_str());
                return false;
            }
        }
        else
        {
            AZStd::string typeNameValue = commandLine.GetSwitchValue("type-name", typeNameOptionCount - 1);
            AZStd::vector<AZ::TypeId> classIds = context->FindClassId(AZ::Crc32{ AZStd::string_view{ typeNameValue } });
            if (classIds.size() != 1)
            {
                if (classIds.empty())
                {
                    AZ_Error("createtype", false, "Type with name %s is not registered with the SerializeContext", typeNameValue.c_str());
                }
                else
                {
                    const char* prependComma = "";
                    AZStd::string classIdString;
                    for (const AZ::TypeId& classId : classIds)
                    {
                        classIdString += prependComma + classId.ToString<AZStd::string>();
                        prependComma = ", ";
                    }
                    AZ_Error(
                        "createtype", classIds.size() < 2,
                        "Multiple types with name %s have been registered with the SerializeContext,\n"
                        "In order to disambiguate which type to use, the --type-id argument must be supplied with one of the following "
                        "Uuids:\n",
                        "%s", typeNameValue.c_str(), classIdString.c_str());
                }
                return false;
            }

            // Only one class with this typename has been registered with the serialize context, so look up its ClassData
            classData = context->FindClassData(classIds.front());
        }

        // Create a rapidjson document to store the default constructed object
        const AZStd::any typeInst = context->CreateAny(classData->m_typeId);
        rapidjson::Document document;
        rapidjson::Value& root = document.SetObject();

        AZ::JsonSerializerSettings serializerSettings;
        serializerSettings.m_serializeContext = context;
        serializerSettings.m_registrationContext = application.GetJsonRegistrationContext();
        serializerSettings.m_keepDefaults = true;

        using JsonResultCode = AZ::JsonSerializationResult::ResultCode;
        const JsonResultCode parseResult = AZ::JsonSerialization::Store(
            root, document.GetAllocator(), AZStd::any_cast<void>(&typeInst), nullptr, typeInst.type(), serializerSettings);
        if (parseResult.GetProcessing() == AZ::JsonSerializationResult::Processing::Halted)
        {
            AZ_Printf("createtype", "  Failed to store type %s in JSON format.\n", classData->m_name);
            return false;
        }

        auto GetOutputPathString = [](auto&& stream)
        {
            using StreamType = AZStd::remove_cvref_t<decltype(stream)>;
            if constexpr (AZStd::is_same_v<StreamType, AZ::IO::StdoutStream>)
            {
                return AZ::IO::FixedMaxPath{ "<stdout>" };
            }
            else if (AZStd::is_same_v<StreamType, AZ::IO::FileIOStream>)
            {
                return AZ::IO::FixedMaxPath{ stream.GetFilename() };
            }
            else
            {
                AZ_Assert(false, "OutputStream has invalid stream type. It must be StdoutStream or FileIOStream");
                return AZ::IO::FixedMaxPath{};
            }
        };

        AZ_Printf(
            "createtype",
            R"(Writing Type "%s" to "%s" using Json Serialization.)"
            "\n",
            classData->m_name, AZStd::visit(GetOutputPathString, outputStream).c_str());

        AZStd::string jsonDocumentRootPrefix;
        if (commandLine.HasSwitch("json-prefix"))
        {
            jsonDocumentRootPrefix = commandLine.GetSwitchValue("json-prefix", 0);
        }

        auto VisitStream = [&document, &jsonDocumentRootPrefix](auto&& stream) -> bool
        {
            if (WriteDocumentToStream(stream, document, jsonDocumentRootPrefix))
            {
                // Write out a newline to the end of the stream
                constexpr AZStd::string_view newline = "\n";
                stream.Write(newline.size(), newline.data());
                return true;
            }

            return false;
        };
        const bool result = AZStd::visit(VisitStream, outputStream);

        return result;
    }

    bool Dumper::CreateUuid(Application& application)
    {
        // outputStream defaults to writing to stdout
        AZStd::variant<FunctorStream, AZ::IO::SystemFileStream> outputStream(AZStd::in_place_type<FunctorStream>,
            GetWriteBypassStdoutCapturerFunctor(application));

        AZ::CommandLine& commandLine = *application.GetAzCommandLine();
        // If the output-file parameter has been supplied open the file path using FileIOStream
        if (size_t optionCount = commandLine.GetNumSwitchValues("output-file"); optionCount > 0)
        {
            AZ::IO::PathView outputPathView(commandLine.GetSwitchValue("output-file", optionCount - 1));
            // If the output file name is a single dash, use the default output stream value which writes to stdout
            if (outputPathView != "-")
            {
                AZ::IO::FixedMaxPath outputPath;
                if (outputPathView.IsRelative())
                {
                    AZ::Utils::ConvertToAbsolutePath(outputPath, outputPathView.Native());
                }
                else
                {
                    outputPath = outputPathView.LexicallyNormal();
                }

                constexpr AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath;

                if (auto& fileStream = outputStream.emplace<AZ::IO::SystemFileStream>(outputPath.c_str(), openMode);
                    !fileStream.IsOpen())
                {
                    AZ_Printf(
                        "createuuid",
                        R"(Unable to open specified output-file "%s". Uuid will not be output to stream)"
                        "\n",
                        outputPath.c_str());
                    return false;
                }
            }
        }

        size_t valuesOptionCount = commandLine.GetNumSwitchValues("values");
        size_t valuesFileOptionCount = commandLine.GetNumSwitchValues("values-file");
        if (valuesOptionCount == 0 && valuesFileOptionCount == 0)
        {
            AZ_Error("createuuid", false, "One of following options must be supplied: --values or --values-file");
            return false;
        }

        bool withCurlyBraces = true;
        if (size_t withCurlyBracesOptionCount = commandLine.GetNumSwitchValues("with-curly-braces");
            withCurlyBracesOptionCount > 0)
        {
            withCurlyBraces = AZ::StringFunc::ToBool(commandLine.GetSwitchValue("with-curly-braces", withCurlyBracesOptionCount - 1).c_str());
        }

        bool withDashes = true;
        if (size_t withDashesOptionCount = commandLine.GetNumSwitchValues("with-dashes");
            withDashesOptionCount > 0)
        {
            withDashes = AZ::StringFunc::ToBool(commandLine.GetSwitchValue("with-dashes", withDashesOptionCount - 1).c_str());
        }

        const bool quietOutput = commandLine.HasSwitch("q") || commandLine.HasSwitch("quiet");

        bool result = true;

        struct UuidStringPair
        {
            AZ::Uuid m_uuid;
            AZStd::string m_value;
        };
        AZStd::vector<UuidStringPair> uuidsToWrite;
        for (size_t i = 0; i < valuesOptionCount; ++i)
        {
            AZStd::string value = commandLine.GetSwitchValue("values", i);
            auto uuidFromName = AZ::Uuid::CreateName(value);
            uuidsToWrite.push_back({ AZStd::move(uuidFromName), AZStd::move(value) });
        }

        // Read string values from each --values-file argument
        for (size_t i = 0; i < valuesFileOptionCount; ++i)
        {
            AZ::IO::FixedMaxPath inputValuePath(AZ::IO::PathView(commandLine.GetSwitchValue("values-file", i)));
            AZ::IO::SystemFileStream valuesFileStream;
            if (inputValuePath == "-")
            {
                // If the input file is dash read from stdin
                valuesFileStream = AZ::IO::SystemFileStream(AZ::IO::SystemFile::GetStdin());
            }
            else
            {
                // Open the path from the values-file option
                constexpr AZ::IO::OpenMode openMode = AZ::IO::OpenMode::ModeRead;
                valuesFileStream.Open(inputValuePath.c_str(), openMode);
            }

            if (valuesFileStream.IsOpen())
            {
                // Use the text parser to parse plain text lines
                AZ::Settings::TextParserSettings textParserSettings;
                textParserSettings.m_parseTextEntryFunc = [&uuidsToWrite](AZStd::string_view token)
                {
                    // Remove leading and surrounding spaces and carriage returns
                    token = AZ::StringFunc::StripEnds(token, " \r");
                    auto uuidFromName = AZ::Uuid::CreateName(token);
                    uuidsToWrite.push_back({ AZStd::move(uuidFromName), token });
                    return true;
                };

                AZ::Settings::ParseTextFile(valuesFileStream, textParserSettings);
            }
        }

        for (const UuidStringPair& uuidStringPair : uuidsToWrite)
        {
            auto VisitStream = [&uuidToWrite = uuidStringPair.m_uuid, &value = uuidStringPair.m_value,
                withCurlyBraces, withDashes, quietOutput](auto&& stream) -> bool
            {
                AZStd::fixed_string<256> uuidString;
                if (quietOutput)
                {
                    uuidString = AZStd::fixed_string<256>::format("%s\n",
                        uuidToWrite.ToFixedString(withCurlyBraces, withDashes).c_str());
                }
                else
                {
                    uuidString = AZStd::fixed_string<256>::format(R"(%s %s)" "\n",
                        uuidToWrite.ToFixedString(withCurlyBraces, withDashes).c_str(),
                        value.c_str());
                }

                size_t bytesWritten = stream.Write(uuidString.size(), uuidString.c_str());
                return bytesWritten == uuidString.size();
            };
            result = AZStd::visit(VisitStream, outputStream) && result;
        }

        return result;
    }

    AZStd::vector<Uuid> Dumper::CreateFilterListByNames(SerializeContext* context, AZStd::string_view name)
    {
        AZStd::vector<AZStd::string_view> names;
        auto AppendNames = [&names](AZStd::string_view filename)
        {
            names.emplace_back(filename);
        };
        AZ::StringFunc::TokenizeVisitor(name, AppendNames, ';');
        AZStd::vector<Uuid> filterIds;
        filterIds.reserve(names.size());

        for (const AZStd::string_view& singleName : names)
        {
            AZStd::vector<Uuid> foundFilters = context->FindClassId(Crc32(singleName.data(), singleName.length(), true));
            filterIds.insert(filterIds.end(), foundFilters.begin(), foundFilters.end());
        }

        return filterIds;
    }

    AZStd::string_view Dumper::ExtractNamespace(const AZStd::string& name)
    {
        size_t offset = 0;
        const char* startChar = name.data();
        const char* currentChar = name.data();
        while (*currentChar != 0 && *currentChar != '<')
        {
            if (*currentChar != ':')
            {
                ++currentChar;
            }
            else
            {
                ++currentChar;
                if (*currentChar == ':')
                {
                    AZ_Assert(currentChar - startChar >= 1, "Offset out of bounds while trying to extract namespace from name '%s'.", name.c_str());
                    offset = currentChar - startChar - 1; // -1 to exclude the last "::"
                }
            }
        }

        return AZStd::string_view(startChar, offset);
    }

    rapidjson::Value Dumper::WriteToJsonValue(const Uuid& uuid, rapidjson::Document& document)
    {
        char buffer[Uuid::MaxStringBuffer];
        int writtenCount = uuid.ToString(buffer, AZ_ARRAY_SIZE(buffer));
        if (writtenCount > 0)
        {
            return rapidjson::Value(buffer, writtenCount - 1, document.GetAllocator()); //-1 as the null character shouldn't be written.
        }
        else
        {
            return rapidjson::Value(rapidjson::StringRef("{uuid conversion failed}"));
        }
    }

    bool Dumper::DumpClassContent(const SerializeContext::ClassData* classData, rapidjson::Value& parent, rapidjson::Document& document,
        const AZStd::vector<Uuid>& systemComponents, SerializeContext* context, AZStd::string& scratchStringBuffer)
    {
        AZ_Assert(scratchStringBuffer.empty(), "Provided scratch string buffer wasn't empty.");

        rapidjson::Value classNode(rapidjson::kObjectType);
        DumpClassName(classNode, context, classData, document, scratchStringBuffer);

        Edit::ClassData* editData = classData->m_editData;
        GenericClassInfo* genericClassInfo = context->FindGenericClassInfo(classData->m_typeId);

        if (editData && editData->m_description)
        {
            AZStd::string_view description = editData->m_description;
            // Skipping if there's only one character as there are several cases where a blank description is given.
            if (description.size() > 1)
            {
                classNode.AddMember("Description", rapidjson::Value(description.data(), document.GetAllocator()), document.GetAllocator());
            }
        }

        classNode.AddMember("Id", rapidjson::StringRef(classData->m_name), document.GetAllocator());
        classNode.AddMember("Version", classData->IsDeprecated() ?
            rapidjson::Value(rapidjson::StringRef("Deprecated")) : rapidjson::Value(classData->m_version), document.GetAllocator());

        auto systemComponentIt = AZStd::lower_bound(systemComponents.begin(), systemComponents.end(), classData->m_typeId);
        const bool isSystemComponent = systemComponentIt != systemComponents.end() && *systemComponentIt == classData->m_typeId;
        classNode.AddMember("IsSystemComponent", isSystemComponent, document.GetAllocator());
        const bool isComponent = isSystemComponent || (classData->m_azRtti != nullptr && classData->m_azRtti->IsTypeOf<AZ::Component>());
        classNode.AddMember("IsComponent", isComponent, document.GetAllocator());
        classNode.AddMember("IsPrimitive", Utilities::IsSerializationPrimitive(genericClassInfo ? genericClassInfo->GetGenericTypeId() : classData->m_typeId), document.GetAllocator());
        classNode.AddMember("IsContainer", classData->m_container != nullptr, document.GetAllocator());
        if (genericClassInfo)
        {
            classNode.AddMember("GenericUuid", WriteToJsonValue(genericClassInfo->GetGenericTypeId(), document), document.GetAllocator());
            classNode.AddMember("Generics", DumpGenericStructure(genericClassInfo, context, document, scratchStringBuffer), document.GetAllocator());
        }

        if (!classData->m_elements.empty())
        {
            rapidjson::Value fields(rapidjson::kArrayType);
            rapidjson::Value bases(rapidjson::kArrayType);

            for (const SerializeContext::ClassElement& element : classData->m_elements)
            {
                DumpElementInfo(element, classData, context, fields, bases, document, scratchStringBuffer);
            }

            if (!bases.Empty())
            {
                classNode.AddMember("Bases", AZStd::move(bases), document.GetAllocator());
            }
            if (!fields.Empty())
            {
                classNode.AddMember("Fields", AZStd::move(fields), document.GetAllocator());
            }
        }

        parent.AddMember(WriteToJsonValue(classData->m_typeId, document), AZStd::move(classNode), document.GetAllocator());

        return true;
    }

    bool Dumper::DumpClassContent(AZStd::string& output, void* classPtr, const Uuid& classId, SerializeContext* context)
    {
        const SerializeContext::ClassData* classData = context->FindClassData(classId);
        if (!classData)
        {
            AZ_Printf("", "  Class data for '%s' is missing.\n", classId.ToString<AZStd::string>().c_str());
            return false;
        }

        size_t indention = 0;
        auto begin = [context, &output, &indention](void* /*instance*/, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement) -> bool
        {
            for (size_t i = 0; i < indention; ++i)
            {
                output += ' ';
            }

            if (classData)
            {
                output += classData->m_name;
            }
            DumpElementInfo(output, classElement, context);
            DumpPrimitiveTag(output, classData, classElement);

            output += '\n';
            indention += 2;
            return true;
        };
        auto end = [&indention]() -> bool
        {
            indention = indention > 0 ? indention - 2 : 0;
            return true;
        };

        SerializeContext::EnumerateInstanceCallContext callContext(begin, end, context, SerializeContext::ENUM_ACCESS_FOR_WRITE, nullptr);
        context->EnumerateInstance(&callContext, classPtr, classId, classData, nullptr);
        return true;
    }

    void Dumper::DumpElementInfo(const SerializeContext::ClassElement& element, const SerializeContext::ClassData* classData, SerializeContext* context,
        rapidjson::Value& fields, rapidjson::Value& bases, rapidjson::Document& document, AZStd::string& scratchStringBuffer)
    {
        AZ_Assert(fields.IsArray(), "Expected 'fields' to be an array.");
        AZ_Assert(bases.IsArray(), "Expected 'bases' to be an array.");
        AZ_Assert(scratchStringBuffer.empty(), "Provided scratch string buffer wasn't empty.");

        const SerializeContext::ClassData* elementClass = context->FindClassData(element.m_typeId, classData);

        AppendTypeName(scratchStringBuffer, elementClass, element.m_typeId);
        Uuid elementTypeId = element.m_typeId;
        if (element.m_genericClassInfo)
        {
            DumpGenericStructure(scratchStringBuffer, element.m_genericClassInfo, context);
            elementTypeId = element.m_genericClassInfo->GetSpecializedTypeId();
        }
        if ((element.m_flags & SerializeContext::ClassElement::FLG_POINTER) != 0)
        {
            scratchStringBuffer += '*';
        }
        rapidjson::Value elementTypeString(scratchStringBuffer.c_str(), document.GetAllocator());
        scratchStringBuffer.clear();

        if ((element.m_flags & SerializeContext::ClassElement::FLG_BASE_CLASS) != 0)
        {
            rapidjson::Value baseNode(rapidjson::kObjectType);
            baseNode.AddMember("Type", AZStd::move(elementTypeString), document.GetAllocator());
            baseNode.AddMember("Uuid", WriteToJsonValue(elementTypeId, document), document.GetAllocator());

            bases.PushBack(AZStd::move(baseNode), document.GetAllocator());
        }
        else
        {
            rapidjson::Value elementNode(rapidjson::kObjectType);
            elementNode.AddMember("Name", rapidjson::StringRef(element.m_name), document.GetAllocator());
            elementNode.AddMember("Type", AZStd::move(elementTypeString), document.GetAllocator());
            elementNode.AddMember("Uuid", WriteToJsonValue(elementTypeId, document), document.GetAllocator());

            elementNode.AddMember("HasDefault", (element.m_flags & SerializeContext::ClassElement::FLG_NO_DEFAULT_VALUE) == 0, document.GetAllocator());
            elementNode.AddMember("IsDynamic", (element.m_flags & SerializeContext::ClassElement::FLG_DYNAMIC_FIELD) != 0, document.GetAllocator());
            elementNode.AddMember("IsPointer", (element.m_flags & SerializeContext::ClassElement::FLG_POINTER) != 0, document.GetAllocator());
            elementNode.AddMember("IsUiElement", (element.m_flags & SerializeContext::ClassElement::FLG_UI_ELEMENT) != 0, document.GetAllocator());
            elementNode.AddMember("DataSize", static_cast<uint64_t>(element.m_dataSize), document.GetAllocator());
            elementNode.AddMember("Offset", static_cast<uint64_t>(element.m_offset), document.GetAllocator());

            Edit::ElementData* elementEditData = element.m_editData;
            if (elementEditData)
            {
                elementNode.AddMember("Description", rapidjson::StringRef(elementEditData->m_description), document.GetAllocator());
            }

            if (element.m_genericClassInfo)
            {
                rapidjson::Value genericArray(rapidjson::kArrayType);
                rapidjson::Value classObject(rapidjson::kObjectType);

                const SerializeContext::ClassData* genericClassData = element.m_genericClassInfo->GetClassData();
                classObject.AddMember("Type", rapidjson::StringRef(genericClassData->m_name), document.GetAllocator());
                classObject.AddMember("GenericUuid", WriteToJsonValue(element.m_genericClassInfo->GetGenericTypeId(), document), document.GetAllocator());
                classObject.AddMember("SpecializedUuid", WriteToJsonValue(element.m_genericClassInfo->GetSpecializedTypeId(), document), document.GetAllocator());
                classObject.AddMember("Generics", DumpGenericStructure(element.m_genericClassInfo, context, document, scratchStringBuffer), document.GetAllocator());

                genericArray.PushBack(AZStd::move(classObject), document.GetAllocator());
                elementNode.AddMember("Generics", AZStd::move(genericArray), document.GetAllocator());
            }

            fields.PushBack(AZStd::move(elementNode), document.GetAllocator());
        }
    }

    void Dumper::DumpElementInfo(AZStd::string& output, const SerializeContext::ClassElement* classElement, SerializeContext* context)
    {
        if (classElement)
        {
            if (classElement->m_genericClassInfo)
            {
                DumpGenericStructure(output, classElement->m_genericClassInfo, context);
            }
            if ((classElement->m_flags & SerializeContext::ClassElement::FLG_POINTER) != 0)
            {
                output += '*';
            }
            output += ' ';
            output += classElement->m_name;
            if ((classElement->m_flags & SerializeContext::ClassElement::FLG_BASE_CLASS) != 0)
            {
                output += " [Base]";
            }
        }
    }

    void Dumper::DumpGenericStructure(AZStd::string& output, GenericClassInfo* genericClassInfo, SerializeContext* context)
    {
        output += '<';

        const SerializeContext::ClassData* classData = genericClassInfo->GetClassData();
        if (classData && classData->m_container)
        {
            bool firstArgument = true;
            auto callback = [&output, context, &firstArgument](const Uuid& elementClassId, const SerializeContext::ClassElement* genericClassElement) -> bool
            {
                if (!firstArgument)
                {
                    output += ',';
                }
                else
                {
                    firstArgument = false;
                }

                const SerializeContext::ClassData* argClassData = context->FindClassData(elementClassId);
                AppendTypeName(output, argClassData, elementClassId);
                if (genericClassElement->m_genericClassInfo)
                {
                    DumpGenericStructure(output, genericClassElement->m_genericClassInfo, context);
                }
                if ((genericClassElement->m_flags & SerializeContext::ClassElement::FLG_POINTER) != 0)
                {
                    output += '*';
                }
                return true;
            };
            classData->m_container->EnumTypes(callback);
        }
        else
        {
            // No container information available, so as much as possible through other means, although
            // this might not be complete information.
            size_t numArgs = genericClassInfo->GetNumTemplatedArguments();
            for (size_t i = 0; i < numArgs; ++i)
            {
                if (i != 0)
                {
                    output += ',';
                }
                const Uuid& argClassId = genericClassInfo->GetTemplatedTypeId(i);
                const SerializeContext::ClassData* argClass = context->FindClassData(argClassId);
                AppendTypeName(output, argClass, argClassId);
            }
        }
        output += '>';
    }

    rapidjson::Value Dumper::DumpGenericStructure(GenericClassInfo* genericClassInfo, SerializeContext* context,
        rapidjson::Document& parentDoc, AZStd::string& scratchStringBuffer)
    {
        AZ_Assert(scratchStringBuffer.empty(), "Provided scratch string buffer still contains data.");

        rapidjson::Value result(rapidjson::kArrayType);

        const SerializeContext::ClassData* classData = genericClassInfo->GetClassData();
        if (classData && classData->m_container)
        {
            auto callback = [&result, context, &parentDoc, &scratchStringBuffer](const Uuid& elementClassId,
                const SerializeContext::ClassElement* genericClassElement) -> bool
            {
                rapidjson::Value classObject(rapidjson::kObjectType);

                const SerializeContext::ClassData* argClassData = context->FindClassData(elementClassId);
                AppendTypeName(scratchStringBuffer, argClassData, elementClassId);
                classObject.AddMember("Type", rapidjson::Value(scratchStringBuffer.c_str(), parentDoc.GetAllocator()), parentDoc.GetAllocator());
                scratchStringBuffer.clear();

                classObject.AddMember("IsPointer", (genericClassElement->m_flags & SerializeContext::ClassElement::FLG_POINTER) != 0, parentDoc.GetAllocator());
                if (genericClassElement->m_genericClassInfo)
                {
                    GenericClassInfo* genericClassInfo = genericClassElement->m_genericClassInfo;
                    classObject.AddMember("GenericUuid", WriteToJsonValue(genericClassInfo->GetGenericTypeId(), parentDoc), parentDoc.GetAllocator());
                    classObject.AddMember("SpecializedUuid", WriteToJsonValue(genericClassInfo->GetSpecializedTypeId(), parentDoc), parentDoc.GetAllocator());
                    classObject.AddMember("Generics", DumpGenericStructure(genericClassInfo, context, parentDoc, scratchStringBuffer), parentDoc.GetAllocator());
                }
                else
                {
                    classObject.AddMember("GenericUuid", WriteToJsonValue(elementClassId, parentDoc), parentDoc.GetAllocator());
                    classObject.AddMember("SpecializedUuid", WriteToJsonValue(elementClassId, parentDoc), parentDoc.GetAllocator());
                }

                result.PushBack(AZStd::move(classObject), parentDoc.GetAllocator());
                return true;
            };
            classData->m_container->EnumTypes(callback);
        }
        else
        {
            // No container information available, so as much as possible through other means, although
            // this might not be complete information.
            size_t numArgs = genericClassInfo->GetNumTemplatedArguments();
            for (size_t i = 0; i < numArgs; ++i)
            {
                const Uuid& elementClassId = genericClassInfo->GetTemplatedTypeId(i);

                rapidjson::Value classObject(rapidjson::kObjectType);

                const SerializeContext::ClassData* argClassData = context->FindClassData(elementClassId);
                AppendTypeName(scratchStringBuffer, argClassData, elementClassId);
                classObject.AddMember("Type", rapidjson::Value(scratchStringBuffer.c_str(), parentDoc.GetAllocator()), parentDoc.GetAllocator());
                scratchStringBuffer.clear();

                classObject.AddMember("GenericUuid",
                    WriteToJsonValue(argClassData ? argClassData->m_typeId : elementClassId, parentDoc), parentDoc.GetAllocator());
                classObject.AddMember("SpecializedUuid", WriteToJsonValue(elementClassId, parentDoc), parentDoc.GetAllocator());
                classObject.AddMember("IsPointer", false, parentDoc.GetAllocator());

                result.PushBack(AZStd::move(classObject), parentDoc.GetAllocator());
            }
        }

        return result;
    }

    void Dumper::DumpPrimitiveTag(AZStd::string& output, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement)
    {
        if (classData)
        {
            Uuid classId = classData->m_typeId;
            if (classElement && classElement->m_genericClassInfo)
            {
                classId = classElement->m_genericClassInfo->GetGenericTypeId();
            }
            if (Utilities::IsSerializationPrimitive(classId))
            {
                output += " [Primitive]";
            }
        }
    }

    void Dumper::DumpClassName(rapidjson::Value& parent, SerializeContext* context, const SerializeContext::ClassData* classData,
        rapidjson::Document& parentDoc, AZStd::string& scratchStringBuffer)
    {
        AZ_Assert(scratchStringBuffer.empty(), "Scratch string buffer is not empty.");

        Edit::ClassData* editData = classData->m_editData;

        GenericClassInfo* genericClassInfo = context->FindGenericClassInfo(classData->m_typeId);
        if (genericClassInfo)
        {
            // If the type itself is a generic, dump it's information.
            scratchStringBuffer = classData->m_name;
            DumpGenericStructure(scratchStringBuffer, genericClassInfo, context);
        }
        else
        {
            bool hasEditName = editData && editData->m_name && strlen(editData->m_name) > 0;
            scratchStringBuffer = hasEditName ? editData->m_name : classData->m_name;
        }

        AZStd::string_view namespacePortion = ExtractNamespace(scratchStringBuffer);
        if (!namespacePortion.empty())
        {
            parent.AddMember("Namespace",
                rapidjson::Value(namespacePortion.data(), azlossy_caster(namespacePortion.length()), parentDoc.GetAllocator()),
                parentDoc.GetAllocator());
            parent.AddMember("Name", rapidjson::Value(scratchStringBuffer.c_str() + namespacePortion.length() + 2, parentDoc.GetAllocator()), parentDoc.GetAllocator());
        }
        else
        {
            parent.AddMember("Name", rapidjson::Value(scratchStringBuffer.c_str(), parentDoc.GetAllocator()), parentDoc.GetAllocator());
        }
        scratchStringBuffer.clear();
    }

    void Dumper::AppendTypeName(AZStd::string& output, const SerializeContext::ClassData* classData, const Uuid& classId)
    {
        if (classData)
        {
            output += classData->m_name;
        }
        else if (classId == GetAssetClassId())
        {
            output += "Asset";
        }
        else
        {
            output += classId.ToString<AZStd::string>();
        }
    }

    bool Dumper::WriteDocumentToStream(AZ::IO::GenericStream& outputStream, const rapidjson::Document& document,
        AZStd::string_view pointerRoot)
    {
        rapidjson::StringBuffer scratchBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(scratchBuffer);

        // rapidjson::Pointer constructor attempts to dereference the const char* index 0 even if the size is 0
        // so make sure the string_view isn't referencing a nullptr
        rapidjson::Pointer jsonPointerAnchor(pointerRoot.data() ? pointerRoot.data() : "", pointerRoot.size());

        // Anchor the content in the Json Document under the Json Pointer root path
        rapidjson::Document rootDocument;
        rapidjson::SetValueByPointer(rootDocument, jsonPointerAnchor, document);
        rootDocument.Accept(writer);

        outputStream.Write(scratchBuffer.GetSize(), scratchBuffer.GetString());

        scratchBuffer.Clear();
        return true;
    }
    // namespace AZ::SerializeContextTools
}
