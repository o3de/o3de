#pragma once

#include <AzCore/std/limits.h>
#include <AzCore/std/optional.h>

namespace AzToolsFramework
{
    struct DeferredTextboxLikeEdit_default_traits
    {
        template<class C>
        using value_type = decltype(AZStd::declval<const C&>().value());

        template<class C>
        static auto get(const C* obj)
        {
            return obj->value();
        }

        template<class C, class T>
        static void set(C* obj, const T& val)
        {
            obj->setValue(val);
        }

        template<class C>
        static bool isUserEditing(const C* obj)
        {
            return obj->hasFocus();
        }

        template<class C>
        static void emitEditingFinished(C* obj)
        {
            emit obj->editingFinished();
        }
    };

    //! A small helper for deferring `setValue' events while a user is editing a textbox-like property.
    //! In these cases, any external `setValue' events shouldn't overwrite what the user enters.
    template<class Derived, class T, class Traits = DeferredTextboxLikeEdit_default_traits>
    class DeferredTextboxLikeEdit
    {
    public:
        //! Set the value of the property, but if the user is editing the textbox-like widget,
        //! don't overwrite their edits. Instead, save the given value for later use; when the
        //! user complets editing, if they didn't actually change anything, then the latest
        //! value passed to this method will be set on the textbox-like widget.
        void setValueFromSystem(const T&);

    protected:
        //! When the textbox-like property editing finishes (typically from the user pressing enter
        //! or the textbox-like widget losing focus), call this method to apply any deferred set-value
        //! actions.
        //! This method also signals the editing finished slot. All arguments passed to this method are
        //! forwarded to that slot. To pass arguments here, you must override the traits to allow
        //! `emitEditingFinished' to accept arguments.
        template<class... Args>
        void onTextBoxLikeEditingFinished(Args&&... args);

        //! Call to check if you should really set the new value. This might record that value for later use.
        //! Typically you should use `setValueFromSystem' instead of this method.
        //! @param prevValue if the value were to be changed now, the previous value of the property.
        //! @param newValue the value which
        //! @return true iff you should now set the new value
        bool shouldReallySetValue(const T& prevValue, const T& newValue, bool isUserEditing);

        //! Call when the user is done editing the textbox-like property.
        //! If this returns a value, you should set the property widget to have that value.
        //! Typically you should use `onTextBoxLikeEditingFinished' instead of this method.
        AZStd::optional<T> getDeferredValuedToSet(const T& value, bool isUserEditing);
        AZStd::optional<T> getDeferredValuedToSet();

    private:
        const Derived* GetDerived() const { return static_cast<const Derived*>(this); }
              Derived* GetDerived()       { return static_cast<      Derived*>(this); }

        struct DeferredSetValue
        {
            T prevValue;
            T value;
        };

        //! If a value is editing, but not by the user, and the user is currently editing the value,
        //! avoid overwriting their work, until they finish editing
        AZStd::optional<DeferredSetValue> m_deferredExternalValue;
    };

    template<class Derived, class T, class Traits>
    void DeferredTextboxLikeEdit<Derived, T, Traits>::setValueFromSystem(const T& newValue)
    {
        if (shouldReallySetValue(Traits::get(GetDerived()), newValue, Traits::isUserEditing(GetDerived())))
        {
            Traits::set(GetDerived(), newValue);
        }
    }

    template<class Derived, class T, class Traits>
    template<class... Args>
    void DeferredTextboxLikeEdit<Derived, T, Traits>::onTextBoxLikeEditingFinished(Args&&... args)
    {
        if (auto newValue = getDeferredValuedToSet())
        {
            Traits::set(GetDerived(), *newValue);
        };

        Traits::emitEditingFinished(GetDerived(), AZStd::forward<Args>(args)...);
    }

    template<class Derived, class T, class Traits>
    bool DeferredTextboxLikeEdit<Derived, T, Traits>::shouldReallySetValue(const T& prevValue, const T& newValue, bool isUserEditing)
    {
        if constexpr (AZStd::is_floating_point_v<T>)
        {
            // Nothing to do if the value is not actually changed
            if (AZ::IsCloseMag(prevValue, newValue, AZStd::numeric_limits<T>::epsilon()))
            {
                return false;
            }
        }

        // If the spin box currently has focus, the user is editing it, so we should not
        // change the value from non-user input while they're in the middle of editing
        if (isUserEditing)
        {
            auto& deferredValue = m_deferredExternalValue.emplace();
            deferredValue.value = newValue;
            deferredValue.prevValue = prevValue;
            return false;
        }

        return true;
    }

    template<class Derived, class T, class Traits>
    AZStd::optional<T> DeferredTextboxLikeEdit<Derived, T, Traits>::getDeferredValuedToSet(const T& value, [[maybe_unused]] bool isUserEditing)
    {
        if (m_deferredExternalValue)
        {
            DeferredSetValue deferredValue = *m_deferredExternalValue;
            m_deferredExternalValue.reset();

            if (value == deferredValue.prevValue)
            {
                return deferredValue.value;
            }
        }

        return {};
    }

    template<class Derived, class T, class Traits>
    AZStd::optional<T> DeferredTextboxLikeEdit<Derived, T, Traits>::getDeferredValuedToSet()
    {
        return getDeferredValuedToSet(Traits::get(GetDerived()), Traits::isUserEditing(GetDerived()));
    }


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING

}
