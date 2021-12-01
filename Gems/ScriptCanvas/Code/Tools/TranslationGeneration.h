/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>

namespace AZ
{
    class BehaviorClass;
    class BehaviorContext;
    class BehaviorEBus;
    class BehaviorMethod;
    class BehaviorProperty;
    class Entity;
    class SerializeContext;
}

namespace ScriptCanvasEditorTools
{
    //! Utility structures for generating the JSON files used for names of elements in Script Canvas
    struct EntryDetails
    {
        AZStd::string m_name;
        AZStd::string m_tooltip;
        AZStd::string m_category;
        AZStd::string m_subtitle;
    };
    using EntryDetailsList = AZStd::vector<EntryDetails>;

    //! Utility structure that represents a method's argument
    struct Argument
    {
        AZStd::string m_typeId;
        EntryDetails m_details;
    };

    //! Utility structure that represents a method
    struct Method
    {
        AZStd::string m_key;
        AZStd::string m_context;

        EntryDetails m_details;

        EntryDetails m_entry;
        EntryDetails m_exit;

        AZStd::vector<Argument> m_arguments;
        AZStd::vector<Argument> m_results;
    };

    //! Utility structure that represents a Script Canvas slot
    struct Slot
    {
        AZStd::string m_key;

        EntryDetails m_details;

        Argument m_data;
    };

    //! Utility structure that represents an reflected element
    struct Entry
    {
        AZStd::string m_key;
        AZStd::string m_context;
        AZStd::string m_variant;

        EntryDetails m_details;

        AZStd::vector<Method> m_methods;
        AZStd::vector<Slot> m_slots;
    };

    // The root level JSON object
    struct TranslationFormat
    {
        AZStd::vector<Entry> m_entries;
    };


    //! Class the wraps all the generation of translation data for all scripting types.
    class TranslationGeneration
    {
    public:

        TranslationGeneration();

        //! Generate the translation data for a given BehaviorClass
        bool TranslateBehaviorClass(const AZ::BehaviorClass* behaviorClass);

        //! Generate the translation data for all Behavior Context classes
        void TranslateBehaviorClasses();

        //! Generate the translation data for Behavior Ebus, handles both Handlers and Senders
        void TranslateEBus(const AZ::BehaviorEBus* behaviorEBus);

        //! Generate the translation data for a specific AZ::Event
        AZ::Entity* TranslateAZEvent(const AZ::BehaviorMethod& method);

        //! Generate the translation data for AZ::Events
        void TranslateAZEvents();

        //! Generate the translation data for all ScriptCanvas::Node types
        void TranslateNodes();

        //! Generate the translation data for the specified TypeId (must inherit from ScriptCanvas::Node)
        void TranslateNode(const AZ::TypeId& nodeTypeId);

        //! Generate the translation data for on-demand reflected types
        void TranslateOnDemandReflectedTypes(TranslationFormat& translationRoot);

        //! Generates the translation data for all global properties and methods in the BehaviorContext
        void TranslateBehaviorGlobals();

        //! Generates the translation data for the specified property in the BehaviorContext (global, by name)
        void TranslateBehaviorProperty(const AZStd::string& propertyName);

        //! Generates the translation data for the specified property in the BehaviorContext
        void TranslateBehaviorProperty(const AZ::BehaviorProperty* behaviorProperty, const AZStd::string& className, const AZStd::string& context, Entry* entry = nullptr);

        //! Generates a type map from reflected types that are suitable for BehaviorContext objects used by ScriptCanvas
        void TranslateDataTypes();

    private:

        //! Utility to populate a BehaviorMethod's translation data
        void TranslateMethod(AZ::BehaviorMethod* behaviorMethod, Method& methodEntry);

        //! Generates the translation data for a BehaviorEBus that has an BehaviorEBusHandler
        bool TranslateEBusHandler(const AZ::BehaviorEBus* behaviorEbus, TranslationFormat& translationRoot);

        //! Utility function that saves a TranslationFormat object in the desired JSON format
        void SaveJSONData(const AZStd::string& filename, TranslationFormat& translationRoot);

        //! Utility function that splits camel-case syntax string into separate words
        void SplitCamelCase(AZStd::string&);

        //! Evaluates if the specified object has exclusion flags and should be skipped from generation
        template <typename T>
        bool ShouldSkip(const T* object) const
        {
            using namespace AZ::Script::Attributes;

            // Check for "ignore" attribute for ScriptCanvas
            const auto& excludeClassAttributeData = azdynamic_cast<AZ::Edit::AttributeData<ExcludeFlags>*>(AZ::FindAttribute(ExcludeFrom, object->m_attributes));
            const bool excludeClass = excludeClassAttributeData && (static_cast<AZ::u64>(excludeClassAttributeData->Get(nullptr)) & static_cast<AZ::u64>(ExcludeFlags::List | ExcludeFlags::Documentation));

            if (excludeClass)
            {
                return true; // skip this class
            }

            return false;
        }

        AZ::SerializeContext* m_serializeContext;
        AZ::BehaviorContext* m_behaviorContext;
    };

    namespace Helpers
    {
        //! Generic function that fetches from a valid type that has attributes a string attribute
        template <typename T>
        AZStd::string GetStringAttribute(const T* source, const AZ::Crc32& attribute)
        {
            AZStd::string attributeValue = "";
            if (auto attributeItem = azrtti_cast<AZ::AttributeData<AZStd::string>*>(AZ::FindAttribute(attribute, source->m_attributes)))
            {
                attributeValue = attributeItem->Get(nullptr);
            }
            return attributeValue;
        }

        //! Utility function that fetches from an AttributeArray a string attribute whether it's an AZStd::string or a const char*
        AZStd::string ReadStringAttribute(const AZ::AttributeArray& attributes, const AZ::Crc32& attribute);

        //! Utility function to verify if a BehaviorMethod has the specified attribute
        bool MethodHasAttribute(const AZ::BehaviorMethod* method, AZ::Crc32 attribute);

        //! Utility function to find a valid name from the ClassData/EditContext
        void GetTypeNameAndDescription(AZ::TypeId typeId, AZStd::string& outName, AZStd::string& outDescription);

        //! Utility function to get the path to the specified gem
        AZStd::string GetGemPath(const AZStd::string& gemName);

        //! Get the category attribute for a given ClassData
        AZStd::string GetCategory(const AZ::SerializeContext::ClassData* classData);

        //! Get the category for a ScriptCanvas node library
        AZStd::string GetLibraryCategory(const AZ::SerializeContext& serializeContext, const AZStd::string& nodeName);
    }

}
