/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/ConsoleFunctor.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/CvarAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    struct CvarAdapter::Impl
    {
        Impl(CvarAdapter* adapter)
            : m_adapter(adapter)
            , m_commandInvokedHandler(
                  [adapter,
                   this](AZStd::string_view command, const AZ::ConsoleCommandContainer&, AZ::ConsoleFunctorFlags, AZ::ConsoleInvokedFrom)
                  {
                      // look up the command that was performed and rebuild its corresponding editor
                      if (!m_performingCommand && !command.empty())
                      {
                          auto commandFunctor = AZ::Interface<AZ::IConsole>::Get()->FindCommand(command);
                          if (commandFunctor)
                          {
                              // find the path to the existing editor
                              Dom::Path path;
                              const auto& existingContents = adapter->GetContents();

                              for (size_t rowIndex = 0, numRows = existingContents.ArraySize(); rowIndex < numRows; ++rowIndex)
                              {
                                  const auto& rowValue = existingContents.ArrayAt(rowIndex);
                                  auto labelString = AZ::Dpe::Nodes::Label::Value.ExtractFromDomNode(rowValue.ArrayAt(0)).value_or("");
                                  if (labelString == command)
                                  {
                                      // found the command at column 0, the editor must be at column 1
                                      path.Push(rowIndex);
                                      path.Push(1);
                                      break;
                                  }
                              }

                              if (!path.IsEmpty())
                              {
                                  // replace the existing editor with a new one that has the correct value
                                  AdapterBuilder builder;
                                  builder.SetCurrentPath(path);
                                  BuildEditorForCvar(builder, commandFunctor);
                                  auto newEditor = builder.FinishAndTakeResult();
                                  adapter->NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(path, newEditor) });
                              }
                          }
                      }
                  })
        {
            m_commandInvokedHandler.Connect(AZ::Interface<AZ::IConsole>::Get()->GetConsoleCommandInvokedEvent());
        }

        template<typename T>
        bool TryBuildNumericEditor(AdapterBuilder& builder, ConsoleFunctorBase* functor)
        {
            if (functor->GetTypeId() == azrtti_typeid<T>())
            {
                T buffer;
                functor->GetValue(buffer);
                builder.BeginPropertyEditor<Nodes::SpinBox<T>>(Dom::Value(buffer));
                builder.Attribute(Nodes::PropertyEditor::Description, functor->GetDesc());
                builder.OnEditorChanged(
                    [this, functor](const Dom::Path& path, const Dom::Value& value, Nodes::ValueChangeType changeType)
                    {
                        if (changeType == Nodes::ValueChangeType::FinishedEdit)
                        {
                            if (m_adapter->GetContents()[path] != value)
                            {
                                T buffer;
                                if constexpr (AZStd::is_integral_v<T> && AZStd::is_signed_v<T>)
                                {
                                    buffer = aznumeric_cast<T>(value.GetInt64());
                                }
                                else if constexpr (AZStd::is_integral_v<T> && AZStd::is_unsigned_v<T>)
                                {
                                    buffer = aznumeric_cast<T>(value.GetUint64());
                                }
                                else
                                {
                                    buffer = aznumeric_cast<T>(value.GetDouble());
                                }
                                PerformCommand(functor->GetName(), { ConsoleTypeHelpers::ToString(buffer) });
                                m_adapter->OnContentsChanged(path, value);
                            }
                        }
                    });
                builder.EndPropertyEditor();
                return true;
            }
            return false;
        }

        template<typename First, typename Second, typename... Rest>
        bool TryBuildNumericEditor(AdapterBuilder& builder, ConsoleFunctorBase* functor)
        {
            if (TryBuildNumericEditor<First>(builder, functor))
            {
                return true;
            }
            return TryBuildNumericEditor<Second, Rest...>(builder, functor);
        }

        bool TryBuildStringEditor(AdapterBuilder& builder, ConsoleFunctorBase* functor)
        {
            const auto typeId = functor->GetTypeId();
            if (typeId == azrtti_typeid<CVarFixedString>() || typeId == azrtti_typeid<AZStd::string>() ||
                typeId == azrtti_typeid<const char*>())
            {
                CVarFixedString buffer;
                functor->GetValue(buffer);
                builder.BeginPropertyEditor<Nodes::LineEdit>(Dom::Value(buffer, true));
                builder.Attribute(Nodes::PropertyEditor::Description, functor->GetDesc());
                builder.Attribute(Nodes::PropertyEditor::ValueType, AZ::Dom::Utils::TypeIdToDomValue(azrtti_typeid<AZStd::string>()));
                builder.OnEditorChanged(
                    [this, functor](const Dom::Path& path, const Dom::Value& value, Nodes::ValueChangeType changeType)
                    {
                        if (changeType == Nodes::ValueChangeType::FinishedEdit && m_adapter->GetContents()[path] != value)
                        {
                            PerformCommand(functor->GetName(), { value.GetString() });
                            m_adapter->OnContentsChanged(path, value);
                        }
                    });
                builder.EndPropertyEditor();
                return true;
            }
            return false;
        }

        template<typename VectorType, size_t ElementCount, typename NodeType>
        bool TryBuildMathVectorEditor(AdapterBuilder& builder, ConsoleFunctorBase* functor)
        {
            if (functor->GetTypeId() == azrtti_typeid<VectorType>())
            {
                VectorType container;
                functor->GetValue(container);
                Dom::Value contents = Dom::Utils::ValueFromType(container);
                builder.BeginPropertyEditor<NodeType>(AZStd::move(contents));
                builder.Attribute(Nodes::PropertyEditor::Description, functor->GetDesc());
                builder.OnEditorChanged(
                    [this, functor](const Dom::Path& path, const Dom::Value& value, Nodes::ValueChangeType changeType)
                    {
                        if (changeType == Nodes::ValueChangeType::FinishedEdit)
                        {
                            VectorType oldContainer =
                                Dom::Utils::ValueToType<VectorType>(m_adapter->GetContents()[path]).value_or(VectorType());
                            VectorType newContainer = Dom::Utils::ValueToType<VectorType>(value).value_or(VectorType());
                            if (newContainer != oldContainer)
                            {
                                // ConsoleCommandContainer holds string_views, so ensure we allocate our parameters here
                                AZStd::fixed_vector<CVarFixedString, ElementCount + 1> newValue;
                                for (int i = 0; i < ElementCount; ++i)
                                {
                                    newValue.push_back(ConsoleTypeHelpers::ToString(newContainer.GetElement(i)));
                                }
                                PerformCommand(functor->GetName(), ConsoleCommandContainer(AZStd::from_range, newValue));

                                m_adapter->OnContentsChanged(path, value);
                            }
                        }
                    });
                builder.EndPropertyEditor();
                return true;
            }
            return false;
        }

        bool TryBuildCheckBoxEditor(AdapterBuilder& builder, ConsoleFunctorBase* functor)
        {
            if (functor->GetTypeId() == azrtti_typeid<bool>())
            {
                bool value;
                functor->GetValue(value);
                builder.BeginPropertyEditor<Nodes::CheckBox>(Dom::Value(value));
                builder.Attribute(Nodes::PropertyEditor::Description, functor->GetDesc());
                builder.OnEditorChanged(
                    [this, functor](const Dom::Path& path, const Dom::Value& value, Nodes::ValueChangeType changeType)
                    {
                        if (changeType == Nodes::ValueChangeType::FinishedEdit && m_adapter->GetContents()[path] != value)
                        {
                            PerformCommand(functor->GetName(), { ConsoleTypeHelpers::ToString(value.GetBool()) });
                            m_adapter->OnContentsChanged(path, value);
                        }
                    });
                builder.EndPropertyEditor();
                return true;
            }
            return false;
        }

        bool BuildEditorForCvar(AdapterBuilder& builder, ConsoleFunctorBase* functor)
        {
            // Check all possible type IDs for our supported types and try to create a property editor
            return TryBuildNumericEditor<
                       AZ::s8,
                       AZ::u8,
                       AZ::s16,
                       AZ::u16,
                       AZ::s32,
                       AZ::u32,
                       AZ::s64,
                       AZ::u64,
                       long,
                       unsigned long,
                       float,
                       double>(builder, functor) ||
                TryBuildStringEditor(builder, functor) || TryBuildMathVectorEditor<AZ::Vector2, 2, Nodes::Vector2>(builder, functor) ||
                TryBuildMathVectorEditor<AZ::Vector3, 3, Nodes::Vector3>(builder, functor) ||
                TryBuildMathVectorEditor<AZ::Vector4, 4, Nodes::Vector4>(builder, functor) ||
                TryBuildMathVectorEditor<AZ::Quaternion, 4, Nodes::Quaternion>(builder, functor) ||
                TryBuildMathVectorEditor<AZ::Color, 4, Nodes::Color>(builder, functor) || TryBuildCheckBoxEditor(builder, functor);
        }

        void PerformCommand(AZStd::string_view command, const AZ::ConsoleCommandContainer& commandContainer)
        {
            m_performingCommand = true;
            AZ::Interface<AZ::IConsole>::Get()->PerformCommand(command, commandContainer);
            m_performingCommand = false;
        }

        CvarAdapter* m_adapter;
        AZ::ConsoleCommandInvokedEvent::Handler m_commandInvokedHandler;
        bool m_performingCommand = false;
    };

    CvarAdapter::CvarAdapter()
        : m_impl(AZStd::make_unique<Impl>(this))
    {
    }

    CvarAdapter::~CvarAdapter()
    {
    }

    Dom::Value CvarAdapter::GenerateContents()
    {
        AdapterBuilder builder;
        builder.BeginAdapter();

        auto consoleInterface = AZ::Interface<AZ::IConsole>::Get();
        if (consoleInterface != nullptr)
        {
            // Enumerate all functors, try to wire up a proper editor for each CVar
            consoleInterface->VisitRegisteredFunctors(
                [&](ConsoleFunctorBase* functor)
                {
                    // Skip console functors with no registered type ID - CVars all have a type ID set
                    if (functor->GetTypeId().IsNull())
                    {
                        return;
                    }
                    builder.BeginRow();
                    builder.Label(functor->GetName());
                    if (!m_impl->BuildEditorForCvar(builder, functor))
                    {
                        builder.Label("Unknown CVAR type");
                    }
                    builder.EndRow();
                });
        }

        builder.EndAdapter();
        return builder.FinishAndTakeResult();
    }

    void CvarAdapter::OnContentsChanged(const Dom::Path& path, const Dom::Value& value)
    {
        // If one of our values has changed, forward a replace patch to the view
        NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(path, value) });
    }
} // namespace AZ::DocumentPropertyEditor
