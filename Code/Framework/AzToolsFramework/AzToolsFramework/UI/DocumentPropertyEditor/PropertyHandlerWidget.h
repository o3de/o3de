/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>
#include <AzToolsFramework/AzToolsFrameworkAPI.h>

class QWidget;
class QString;

namespace AZ
{
    struct Uuid;
    using TypeId = Uuid;
    namespace Dom
    {
        class Value;
    }
}

namespace AzToolsFramework
{
    class DocumentPropertyEditor;
    //! Base class for all property editor widgets in the DocumentPropertyEditor.
    //! Property handler widgets are registered to the PropertyEditorToolsSystemInterface and
    //! instantiated as part of the DocumentPropertyEditor, with one handler instance being constructed
    //! for each property editor.
    class AZTF_API PropertyHandlerWidgetInterface
    {
    public:
        friend class DocumentPropertyEditor;
        friend class DPERowWidget;

        virtual ~PropertyHandlerWidgetInterface() = default;

        //! Gets the widget that should be added to the DocumentPropertyEditor.
        virtual QWidget* GetWidget() = 0;
        //! Sets up the widget provided by GetWidget to reflect the values provided by a given DOM node.
        //! This should consume both the property value (if applicable) and any attributes, including OnChange.
        virtual void SetValueFromDom(const AZ::Dom::Value& node) = 0;

        //! The above SetValueFromDom function could be called repeatedly during updates, to set little pieces
        //! of the node data.  In order to avoid repeatedly updating the widget, avoid setting any widget UI values
        //! in SetValueFromDom, and instead wait for this function to be called, which will be called after all DOM updates are complete.
        virtual void RefreshUI() = 0;

        //! Attempts to reset the widget handler to default, typically for recycling. Returns true if successful
        virtual bool ResetToDefaults()
        {
            return false;
        }

        //! Allow the widget to lint its matching text to outline the current search
        virtual void SetFilter([[maybe_unused]] const QString& filter) {}; // optional

        //! Returns the first widget in the tab order for this property editor, i.e. the widget that should be selected
        //! when the user hits tab on the widget immediately prior to this.
        //! By default, this returns GetWidget, a single widget tab order.
        virtual QWidget* GetFirstInTabOrder();
        //! Returns the last widget in the tab order for this property editor, i.e. the widget that should select the
        //! widget immediately subsequent to this property editor when the user presses tab.
        //! By default, this returns GetWidget, a single widget tab order.
        virtual QWidget* GetLastInTabOrder();

        //! Returns true if this handler should be used for this type (assuming its type already matches GetHandlerName())
        //! This may be reimplemented in subclasses to allow handler specialization, for example a handler that handles
        //! integral types and a separate handler for handling floating point types.
        static bool ShouldHandleType([[maybe_unused]] const AZ::TypeId& typeId)
        {
            return true;
        }

        //! This must be implemented in subclasses registered with PropertyEditorToolsSystemInterface::RegisterHandler.
        //! Returns the handler name, used to look up the type of handler to use from a PropertyEditor node.
        static constexpr const AZStd::string_view GetHandlerName()
        {
            return "<undefined handler name>";
        }

        //! If overridden, this can be used to indicate that this handler should be added to the "default" pool of
        //! property handlers. Default property handlers will have ShouldHandleType queried for PropertyEditor nodes
        //! that don't have an explicit Type set.
        //! For example, a numeric spin box might be registered as a default handler, in which case a node like:
        //! <PropertyEditor Value=5 ValueType="int" />
        //! would resolve to a spin box, while
        //! <PropertyEditor Type="Slider" Value=5 ValueType="int" />
        //! would still resolve to a slider.
        static constexpr bool IsDefaultHandler()
        {
            return false;
        }

    protected:
        // The internal code will call this function, which will call the overridable SetValueFromDOM function.
        // This allows the internal code to perform any necessary setup and teardown and makes it so that implementors
        // do not have to remember to call the base class.
        void SetValueFromDom_Internal(const AZ::Dom::Value& node, AzToolsFramework::DocumentPropertyEditor* owningDPE);
    };

    //! Helper class, provides a PropertyHandlerWidgetInterface implementation in which
    //! the handler itself is-a QWidget of a given type.
    //! BaseWidget can be any QWidget subclass.
    template<typename BaseWidget>
    class PropertyHandlerWidget
        : public BaseWidget
        , public PropertyHandlerWidgetInterface
    {
    public:
        PropertyHandlerWidget(QWidget* parent = nullptr)
            : BaseWidget(parent)
        {
        }

        QWidget* GetWidget() override
        {
            return this;
        }
    };
} // namespace AzToolsFramework
