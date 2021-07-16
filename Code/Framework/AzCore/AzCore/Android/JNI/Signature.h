/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/std/utils.h>

#include <AzCore/Android/JNI/JNI.h>


namespace AZ
{
    namespace Android
    {
        namespace JNI
        {
            namespace Internal
            {
                //! \brief Templated interface for getting specific JNI type signatures.  This is intentionally left empty
                //!        to enforce usage to only known template specializations.
                //! \tparam Type The JNI type desired for the signature request
                //! \tparam StringType The type of string that should be returned.  Defaults to 'const char*'
                //! \return String containing the type specific JNI signature
                template<typename Type, typename StringType = const char*>
                StringType GetTypeSignature(Type);

                ///!@{
                //! \brief Known type specializations for \ref AZ::Android::JNI::Internal::GetTypeSignature.
                template<> inline const char* GetTypeSignature(jboolean) { return "Z"; }
                template<> inline const char* GetTypeSignature(bool) { return "Z"; }
                template<> inline const char* GetTypeSignature(jbooleanArray) { return "[Z"; }

                template<> inline const char* GetTypeSignature(jbyte) { return "B"; }
                template<> inline const char* GetTypeSignature(jbyteArray) { return "[B"; }

                template<> inline const char* GetTypeSignature(jchar) { return "C"; }
                template<> inline const char* GetTypeSignature(jcharArray) { return "[C"; }

                template<> inline const char* GetTypeSignature(jshort) { return "S"; }
                template<> inline const char* GetTypeSignature(jshortArray) { return "[S"; }

                template<> inline const char* GetTypeSignature(jint) { return "I"; }
                template<> inline const char* GetTypeSignature(jintArray) { return "[I"; }

                template<> inline const char* GetTypeSignature(jlong) { return "J"; }
                template<> inline const char* GetTypeSignature(jlongArray) { return "[J"; }

                template<> inline const char* GetTypeSignature(jfloat) { return "F"; }
                template<> inline const char* GetTypeSignature(jfloatArray) { return "[F"; }

                template<> inline const char* GetTypeSignature(jdouble) { return "D"; }
                template<> inline const char* GetTypeSignature(jdoubleArray) { return "[D"; }

                template<> inline const char* GetTypeSignature(jstring) { return "Ljava/lang/String;"; }

                template<> inline const char* GetTypeSignature(jclass) { return "Ljava/lang/Class;"; }

                template<typename StringType>
                StringType GetTypeSignature(jobject value);

                template<typename StringType>
                StringType GetTypeSignature(jobjectArray value);
                //!@}


                //! \brief Templated interface for comparing JNI type signatures.  This leverages
                //!         \ref AZ::Android::JNI::Internal::GetTypeSignature under the hood
                //!         to weed out unsupported types
                //! \tparam StringType The type of string that should used for base comparison
                //! \tparam Type The JNI type desired for the signature verification
                //! \param baseSignature The string representation of the expected type \p param should be
                //! \param param The raw JNI type to be validated
                //! \return True if \p param is an acceptable type for the type specified in \p baseSignature,
                //!         false otherwise
                template<typename StringType, typename Type>
                bool CompareTypeSignature(const StringType& baseSignature, Type param);

                ///!@{
                //! \brief Explicit definitions for jobject and jobjectArray need to be defined in order to support
                //!         subclass validation for Java classes
                template<typename StringType>
                bool CompareTypeSignature(const StringType& baseSignature, jobject param);

                template<typename StringType>
                bool CompareTypeSignature(const StringType& baseSignature, jobjectArray param);
                //!@}
            }


            //! \brief Utility for generating and validating JNI signatures
            //! \tparam StringType The type of string used internally for generation and validation.  Defaults to AZStd::string
            template<typename StringType = AZStd::string>
            class Signature
            {
            public:
                //! \brief Required for handling cases when an empty set of variadic arguments are forwarded from
                //!         \ref AZ::Android::JNI::GetSignature calls
                //! \return An empty string
                static StringType Generate()
                {
                    Signature sig;
                    return sig.m_signature;
                }

                //! \brief Gets the signature from n-number of parameters
                //! \param parameters Variables only used to forward their types on to \ref AZ::Android::JNI::Signature::GenerateImpl
                //! \return String containing a fully qualified Java signature
                template<typename... Args>
                static StringType Generate(Args&&... parameters)
                {
                    Signature sig;
                    sig.GenerateImpl(AZStd::forward<Args>(parameters)...);
                    return sig.m_signature;
                }


                //! \brief Required for handling cases when an empty set of variadic arguments are forwarded from
                //!         \ref AZ::Android::JNI::ValidateSignature calls
                //! \param baseSignature The string representation of the expected type signature.  Should be an empty string in
                //!         this case.
                //! \return True if the string is empty (e.g. nothing to validate), false otherwise
                static bool Validate(const StringType& baseSignature)
                {
                    Signature sig(baseSignature);
                    return sig.m_signature.empty();
                }

                //! \brief Validates n-number of parameters type signatures
                //! \param baseSignature The string representation of the expected JNI type signatures in \p parameters
                //! \param parameters The input arguments to be validated
                //! \return True if all arguments in \p parameters match the expected type signature in \p baseSignature, False otherwise
                template<typename... Args>
                static bool Validate(const StringType& baseSignature, Args&&... parameters)
                {
                    Signature sig(baseSignature);
                    return (sig.m_signature.empty() ?
                            false :
                            sig.ValidateImpl(AZStd::forward<Args>(parameters)...));
                }


            private:
                //! \brief Internal constructor of the signature util used for generation.  Pre-allocates some memory for the internal
                //!        cache to try and prevent the hammering of reallocations in most cases.
                Signature()
                    : m_signature()
                    , m_signatureLength(0)
                    , m_currentIndex(0)
                {
                    m_signature.reserve(8);
                }

                //! \brief Internal constructor of the signature util used for validation.
                //! \param baseSignature The signature to compare against
                explicit Signature(const StringType& baseSignature)
                    : m_signature(baseSignature)
                    , m_signatureLength(0)
                    , m_currentIndex(0)
                {
                    m_signatureLength = m_signature.length();
                }


                //! \brief Appends the desired type to the internal signature cache
                //! \param value Throwaway variable only used to forward the type on to \ref AZ::Android::JNI::Internal::GetTypeSignature
                template<typename Type>
                void GenerateImpl(Type value)
                {
                    m_signature.append(Internal::GetTypeSignature(value));
                }

                ///!@{
                //! \brief Explicit definitions for jobject and jobjectArray need to be defined in order to route their
                //!        calls to the correct version of \ref AZ::Android::JNI::Internal::GetTypeSignature which returns
                //!        a string instead of a c-string.
                void GenerateImpl(jobject value)
                {
                    m_signature.append(Internal::GetTypeSignature<StringType>(value));
                }

                void GenerateImpl(jobjectArray value)
                {
                    m_signature.append(Internal::GetTypeSignature<StringType>(value));
                }
                //!@}

                //! \brief Appends the desired type to the internal signature cache
                //! \param first Variable only used to forward the type on to \ref AZ::Android::JNI::Signature::GenerateImpl
                //! \param parameters Additional variables only used to forward their types into recursive calls
                template<typename Type, typename... Args>
                void GenerateImpl(Type first, Args&&... parameters)
                {
                    GenerateImpl(first);
                    GenerateImpl(AZStd::forward<Args>(parameters)...);
                }


                //! \brief Validates a single (or final) type against the remaining signature(s) in the internal signature cache
                //! \param param The type to be validated
                //! \return True if the type of \p param matches the remaining signature(s) in the internal signature cache,
                //!         False otherwise
                template<typename Type>
                bool ValidateImpl(Type param)
                {
                    int paramLength = m_signatureLength - m_currentIndex;
                    if (paramLength > 0)
                    {
                        StringType paramSignature = m_signature.substr(m_currentIndex, paramLength);
                        return Internal::CompareTypeSignature<StringType>(paramSignature, param);
                    }
                    return false;
                }

                //! \brief Validates n-number of parameters type signatures
                //! \param first The first type to be validated
                //! \param parameters The remaining types to be validated recursively
                //! \return True if all the types in \p parameters match the expected type signature stored internally,
                //!         False otherwise
                template<typename Type, typename... Args>
                bool ValidateImpl(Type first, Args&&... parameters);


                // ----

                StringType m_signature; //!< Internal cache for signature generation/validation

                int m_signatureLength; //!< Cache of the total length of the base signature for validation
                int m_currentIndex; //!< Current index in walking the base signature for validation
            };


            //! \brief Default Signature template (AZStd::string), primarily used in \ref AZ::Android::JNI::GetSignature
            //!         and \ref AZ::Android::JNI::ValidateSignature
            typedef Signature<AZStd::string> SignatureUtil;


            //! \brief Generates a fully qualified Java signature from n-number of parameters. This is the preferred implementation
            //!        for generating JNI signatures
            //! \param parameters Variables only used to forward their type info on to \ref AZ::Android::Signature::Generate
            //! \return String containing a fully qualified Java signature
            template<typename... Args>
            AZ_INLINE AZStd::string GetSignature(Args&&... parameters)
            {
                return SignatureUtil::Generate(AZStd::forward<Args>(parameters)...);
            }

            //! \brief Validates a JNI signature with n-number of parameters.  Will walk the signature validating
            //!         each parameter individually.  The validation will exit once an argument fails validation.
            //! \param baseSignature Base JNI signature to be comparing against
            //! \param parameters The input arguments to be validated
            //! \return True if all arguments match the signature, False otherwise
            template<typename... Args>
            AZ_INLINE bool ValidateSignature(const AZStd::string& baseSignature, Args&&... parameters)
            {
                return SignatureUtil::Validate(baseSignature, AZStd::forward<Args>(parameters)...);
            };
        } // namespace JNI
    } // namespace Android
} // namespace AZ

#include <AzCore/Android/JNI/Internal/Signature_impl.h>


