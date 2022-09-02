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
        {
        }

        template<typename T>
        bool TryBuildNumericEditor(AdapterBuilder& builder, ConsoleFunctorBase* functor)
        {
            if (functor->GetTypeId() == azrtti_typeid<T>())
            {
                T buffer;
                functor->GetValue(buffer);
                builder.BeginPropertyEditor<Nodes::SpinBox<T>>(Dom::Value(buffer));
                builder.OnEditorChanged(
                    [this, functor](const Dom::Path& path, const Dom::Value& value, Nodes::ValueChangeType)
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
                        (*functor)({ ConsoleTypeHelpers::ToString(buffer) });
                        m_adapter->OnContentsChanged(path, value);
                    });

                using ValueType = typename Nodes::NumericEditor<T>::StorageType;
                builder.Attribute(Nodes::SpinBox<T>::Min, aznumeric_cast<ValueType>(AZStd::numeric_limits<T>::min()));
                builder.Attribute(Nodes::SpinBox<T>::Max, aznumeric_cast<ValueType>(AZStd::numeric_limits<T>::max()));
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
            if (functor->GetTypeId() == azrtti_typeid<CVarFixedString>())
            {
                CVarFixedString buffer;
                functor->GetValue(buffer);
                builder.BeginPropertyEditor<Nodes::LineEdit>(Dom::Value(buffer, true));
                builder.Attribute(Nodes::PropertyEditor::ValueType, AZ::Dom::Utils::TypeIdToDomValue(azrtti_typeid<AZStd::string>()));
                builder.OnEditorChanged(
                    [this, functor](const Dom::Path& path, const Dom::Value& value, Nodes::ValueChangeType)
                    {
                        (*functor)({ value.GetString() });
                        m_adapter->OnContentsChanged(path, value);
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
                builder.OnEditorChanged(
                    [this, functor](const Dom::Path& path, const Dom::Value& value, Nodes::ValueChangeType)
                    {
                        VectorType newContainer = Dom::Utils::ValueToType<VectorType>(value).value_or(VectorType());
                        // ConsoleCommandContainer holds string_views, so ensure we allocate our parameters here
                        AZStd::fixed_vector<CVarFixedString, ElementCount> newValue;
                        for (int i = 0; i < ElementCount; ++i)
                        {
                            newValue.push_back(ConsoleTypeHelpers::ToString(newContainer.GetElement(i)));
                        }
                        (*functor)(ConsoleCommandContainer(AZStd::from_range, newValue));
                        m_adapter->OnContentsChanged(path, value);
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
                builder.OnEditorChanged(
                    [this, functor](const Dom::Path& path, const Dom::Value& value, Nodes::ValueChangeType)
                    {
                        (*functor)({ ConsoleTypeHelpers::ToString(value.GetBool()) });
                        m_adapter->OnContentsChanged(path, value);
                    });
                builder.EndPropertyEditor();
                return true;
            }
            return false;
        }

        bool BuildEditorForCvar(AdapterBuilder& builder, ConsoleFunctorBase* functor)
        {
            // Check all possible type IDs for our supported types and try to create a property editor
            return TryBuildNumericEditor<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double>(
                       builder, functor) ||
                TryBuildStringEditor(builder, functor) || TryBuildMathVectorEditor<AZ::Vector2, 2, Nodes::Vector2>(builder, functor) ||
                TryBuildMathVectorEditor<AZ::Vector3, 3, Nodes::Vector3>(builder, functor) ||
                TryBuildMathVectorEditor<AZ::Vector4, 4, Nodes::Vector4>(builder, functor) ||
                TryBuildMathVectorEditor<AZ::Quaternion, 4, Nodes::Quaternion>(builder, functor) ||
                TryBuildMathVectorEditor<AZ::Color, 4, Nodes::Color>(builder, functor) || TryBuildCheckBoxEditor(builder, functor);
        }

        CvarAdapter* m_adapter;
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
