/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/NativeUI/NativeUISystemComponent.h>

#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/JNI/scoped_ref.h>
#include <AzCore/Android/AndroidEnv.h>
#include <AzCore/Android/Utils.h>

namespace AZ
{
    namespace NativeUI
    {
        AZStd::string NativeUISystem::DisplayBlockingDialog(const AZStd::string& title, const AZStd::string& message, const AZStd::vector<AZStd::string>& options) const
        {
            if (m_mode == NativeUI::Mode::DISABLED)
            {
                return {};
            }

            AZ::Android::JNI::Object object("com/amazon/lumberyard/NativeUI/LumberyardNativeUI");
            object.RegisterStaticMethod("DisplayDialog", "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V");
            object.RegisterStaticMethod("GetUserSelection", "()Ljava/lang/String;");

            JNIEnv* env = AZ::Android::JNI::GetEnv();
            AZ::Android::JNI::scoped_ref<jobjectArray> joptions(static_cast<jobjectArray>(env->NewObjectArray(options.size(), env->FindClass("java/lang/String"), env->NewStringUTF(""))));
            for (int i = 0; i < options.size(); i++)
            {
                env->SetObjectArrayElement(joptions.get(), i, env->NewStringUTF(options[i].c_str()));
            }

            AZ::Android::JNI::scoped_ref<jstring> jtitle(AZ::Android::JNI::ConvertStringToJstring(title));
            AZ::Android::JNI::scoped_ref<jstring> jmessage(AZ::Android::JNI::ConvertStringToJstring(message));

            object.InvokeStaticVoidMethod("DisplayDialog", AZ::Android::Utils::GetActivityRef(), jtitle.get(), jmessage.get(), joptions.get());

            AZStd::string userSelection = "";
            while (userSelection.empty())
            {
                userSelection = object.InvokeStaticStringMethod("GetUserSelection");
            }

            for (int i = 0; i < options.size(); i++)
            {
                jstring str = static_cast<jstring>(env->GetObjectArrayElement(joptions.get(), i));
                env->DeleteLocalRef(str);
            }

            return userSelection;
        }
    }
}
