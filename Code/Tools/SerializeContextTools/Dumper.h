/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>


namespace AZ::IO
{
    class GenericStream;
}
namespace AZ::SerializeContextTools
{
    class Application;

    class Dumper
    {
    public:
        static bool DumpFiles(Application& application);
        static bool DumpSerializeContext(Application& application);

        static bool DumpTypes(Application& application);
        static bool CreateType(Application& application);
        static bool CreateUuid(Application& application);

    private:
        static AZStd::vector<Uuid> CreateFilterListByNames(SerializeContext* context, AZStd::string_view name);

        static AZStd::string_view ExtractNamespace(const AZStd::string& name);

        static rapidjson::Value WriteToJsonValue(const Uuid& uuid, rapidjson::Document& document);

        static bool DumpClassContent(const SerializeContext::ClassData* classData, rapidjson::Value& parent, rapidjson::Document& document,
            const AZStd::vector<Uuid>& systemComponents, SerializeContext* context, AZStd::string& scratchStringBuffer);
        static bool DumpClassContent(AZStd::string& output, void* classPtr, const Uuid& classId, SerializeContext* context);

        static void DumpElementInfo(const SerializeContext::ClassElement& element, const SerializeContext::ClassData* classData, SerializeContext* context,
            rapidjson::Value& fields, rapidjson::Value& bases, rapidjson::Document& document, AZStd::string& scratchStringBuffer);
        static void DumpElementInfo(AZStd::string& output, const SerializeContext::ClassElement* classElement, SerializeContext* context);

        static void DumpGenericStructure(AZStd::string& output, GenericClassInfo* genericClassInfo, SerializeContext* context);
        static rapidjson::Value DumpGenericStructure(GenericClassInfo* genericClassInfo, SerializeContext* context,
            rapidjson::Document& parentDoc, AZStd::string& scratchStringBuffer);

        static void DumpPrimitiveTag(AZStd::string& output, const SerializeContext::ClassData* classData,
            const SerializeContext::ClassElement* classElement);
        static void DumpClassName(rapidjson::Value& parent, SerializeContext* context, const SerializeContext::ClassData* classData,
            rapidjson::Document& parentDoc, AZStd::string& scratchStringBuffer);

        static void AppendTypeName(AZStd::string& output, const SerializeContext::ClassData* classData, const Uuid& classId);

        static bool WriteDocumentToStream(AZ::IO::GenericStream& outputstream, const rapidjson::Document& document,
            AZStd::string_view pointerRoot);
    };
} // namespace AZ::SerializeContextTools
