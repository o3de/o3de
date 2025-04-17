/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QObject>
#include <QDialog>
#include <QLineEdit>
#include <QTableView>
#include <QSortFilterProxyModel>
#include <QAbstractTableModel>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Console/Console.h>
#endif

namespace Ui
{
    class CommandLine;
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class Command
        {
        public:
            using Functor = AZStd::function<void(AZStd::vector<AZStd::string>)>;

            Command(const AZStd::string& name, const AZStd::string& description, Functor functor)
                : m_name(name)
                , m_description(description)
                , m_functor(functor)
            {}

            void operator()(const AZStd::vector<AZStd::string>& args)
            {
                m_functor(args);
            }

            const AZStd::string& GetName() const { return m_name; }
            const AZStd::string& GetDescription() const { return m_description; }

        private:
            AZStd::string m_name;
            AZStd::string m_description;
            Functor m_functor;
        };

        using CommandRegistry = AZStd::unordered_map<AZStd::string, AZStd::unique_ptr<Command>>;

        struct ScriptCanvasCommandLineRequests : public AZ::EBusTraits
        {
            virtual void AddCommand(const AZStd::string commandName, const AZStd::string description, Command::Functor) = 0;
            virtual void Invoke(const char* commandName) = 0;
            virtual void InvokeWithArguments(const char* commandName, const AZStd::vector<AZStd::string>&) = 0;

            using CommandNameList = AZStd::list<AZStd::pair<AZStd::string, AZStd::string>>;
            virtual CommandNameList GetCommands() = 0;
        };
        using ScriptCanvasCommandLineRequestBus = AZ::EBus<ScriptCanvasCommandLineRequests>;

        // TODO #lsempe: this deserves its own file
        // CommandListDataModel
        /////////////////////////////////////////////////////////////////////////////////////////////
        class CommandListDataModel : public QAbstractTableModel
            , ScriptCanvasCommandLineRequestBus::Handler
        {
            Q_OBJECT

        public:

            AZ_CLASS_ALLOCATOR(CommandListDataModel, AZ::SystemAllocator);

            enum ColumnIndex
            {
                CommandIndex,
                DescriptionIndex,
                TrailIndex,
                Count
            };

            enum CustomRole
            {
                Node = Qt::UserRole,
                Types,
                EBusSender,
                EBusHandler,
                Commands
            };

            CommandListDataModel(QWidget* parent = nullptr);
            ~CommandListDataModel() override;

            QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
            QModelIndex parent(const QModelIndex &child) const override;
            int rowCount(const QModelIndex &parent = QModelIndex()) const override;
            int columnCount(const QModelIndex &parent = QModelIndex()) const override;
            QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

            Qt::ItemFlags flags(const QModelIndex &index) const override;

            bool HasMatches(const AZStd::string& input);

            struct Entry
            {
                AZ::Uuid m_type;
                AZStd::string m_command;

                Entry()
                {
                    m_type = AZ::Uuid::CreateNull();
                }
            };

        protected:

            AZStd::vector<Entry> m_entries;

            static CommandRegistry m_commands;

            void AddCommand(const AZStd::string commandName, const AZStd::string description, Command::Functor f) override
            {
                if (m_commands.find(commandName) == m_commands.end())
                {
                    m_commands[commandName] = AZStd::make_unique<Command>(commandName, description, f);
                    Entry entry;
                    entry.m_command = commandName;
                    entry.m_type = AZ::Uuid::CreateNull();
                    m_entries.emplace_back(entry);
                }
            }

            void Invoke(const char* commandName) override
            {
                auto command = m_commands.find(commandName);
                if (command != m_commands.end())
                {
                    command->second->operator()({});
                }
            }

            void InvokeWithArguments(const char* commandName, const AZStd::vector<AZStd::string>& args) override
            {
                auto command = m_commands.find(commandName);
                if (command != m_commands.end())
                {
                    command->second->operator()(args);
                }
            }

            ScriptCanvasCommandLineRequests::CommandNameList GetCommands() override
            {
                ScriptCanvasCommandLineRequests::CommandNameList commands;
                for (auto& command : m_commands)
                {
                    commands.push_back(AZStd::make_pair(command.second->GetName(), command.second->GetDescription()));
                }
                return commands;
            }
        };

        class CommandListDataProxyModel : public QSortFilterProxyModel
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CommandListDataProxyModel, AZ::SystemAllocator);

            CommandListDataProxyModel(CommandListDataModel* commandListData, QObject* parent = nullptr);

            bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

            void SetInput(const AZStd::string& input)
            {
                m_input = input;
                invalidate();
            }

        protected:
            QCompleter* m_completer;
            AZStd::string m_input;

        };


        // CommandLineEdit
        /////////////////////////////////////////////////////////////////////////////////////////////
        class CommandLineEdit : public QLineEdit
        {
            Q_OBJECT
        public:
            CommandLineEdit(QWidget* parent = nullptr);
            void ResetState();

        Q_SIGNALS:
            void onFocusChange(bool focused);
            void onKeyReleased(QKeyEvent*);

        protected:
            void focusInEvent(QFocusEvent*) override;
            void focusOutEvent(QFocusEvent*) override;
            void keyPressEvent(QKeyEvent *) override;
            void keyReleaseEvent(QKeyEvent*) override;

            void onTextChanged(const QString&);
            void onTextEdited(const QString&);
            void onReturnPressed();

            bool m_empty;
            const QString m_defaultText;
        };

        // CommandLineList
        /////////////////////////////////////////////////////////////////////////////////////////////
        class CommandLineList : public QTableView
        {
            Q_OBJECT
        public:
            CommandLineList(QWidget* parent = nullptr);

        Q_SIGNALS:
            void onKeyReleased(QKeyEvent*);

        protected:
            void keyReleaseEvent(QKeyEvent* event) override
            {
                Q_EMIT(onKeyReleased(event));
            }
        };

        // CommandLine
        /////////////////////////////////////////////////////////////////////////////////////////////
        class CommandLine 
            : public QWidget
        {
            Q_OBJECT
        public:

            CommandLine(QWidget* object = nullptr);

            void showEvent(QShowEvent *event) override;
            void onTextChanged(const QString&);
            void onEditKeyReleaseEvent(QKeyEvent*);
            void onListKeyReleaseEvent(QKeyEvent*);

            AZStd::unique_ptr<Ui::CommandLine> ui;
        };
    }
}
