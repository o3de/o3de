/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/Translation/TranslationBus.h>

class QAction;
class QWidget;
class QMenu;

namespace ScriptCanvasDeveloperEditor
{
    namespace TranslationGenerator
    {
        void GenerateTranslationDatabase();

        //! Utility structures for generating the JSON files used to
        //! configure the names of elements in Script Canvas
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

        using VerificationSet = AZStd::unordered_set<GraphCanvas::TranslationKey>;

        //! Utility function that determines if a given BehaviorClass should not be considered for translation
        bool ShouldSkipClass(const AZ::BehaviorClass* behaviorClass);

        //! The Qt action that will start the database generation
        QAction* TranslationDatabaseFileAction(QMenu* mainMenu, QWidget* mainWindow);

        AZStd::string GetContextName(const AZ::SerializeContext::ClassData& classData);

        AZStd::string GetLibraryCategory(const AZ::SerializeContext& serializeContext, const AZStd::string& nodeName);
    };
}
