/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/Dependency/Version.h>

namespace AZ 
{
    /**
     * Specifies a particular object instance by UUID and version.
     */
    template <size_t N>
    struct Specifier
    {
        AZ::Uuid m_id;
        Version<N> m_version;

        Specifier(const AZ::Uuid& id, const Version<N>& version);
        virtual ~Specifier() = default;
    };

    /**
     * Defines a dependency upon another versioned object.
     */
    template <size_t N>
    class Dependency
    {
    public:
        class Bound
        {
        public:
            friend class Dependency<N>;

            enum class Comparison : AZ::u8
            {
                // Don't compare against this version
                None = 0,
                // Traditional operators
                GreaterThan = 1 << 0,
                LessThan = 1 << 1,
                EqualTo = 1 << 2,
                // Special operators ~> and ~=
                TwiddleWakka = 1 << 3
            };

            /**
            * Get a formatted string output of the Dependency's bounds.
            *
            * \returns                 The formatted output string.
            */
            AZStd::string ToString() const;

            /**
            * Check if the bound is satisfied by the version input.
            *
            * \params[in] version      The version to check against.
            *
            * \returns                 Returns true if the bound is satisfied.
            */
            bool MatchesVersion(const Version<N>& version) const;

            /**
            * Set the version of this bound that will be used to check against
            *
            * \params[in] version      The version to set this bound.
            */
            void SetVersion(const Version<N>& version);

            /**
            * Get the version of this bound that will be used to check against
            *
            * \returns                 The version used by this bound.
            */
            const Version<N>& GetVersion() const;

            /**
            * Set the comparison of this bound that will be used to check against
            *
            * \params[in] comp         The comparison to set this bound.
            */
            void SetComparison(Comparison comp);

            /**
            * Get the comparison operator of this bound that will be used to check against
            *
            * \returns                 The comparison used by this bound.
            */
            Comparison GetComparison() const;

        private:
            AZStd::string m_parsedString;
            Version<N> m_version;
            Comparison m_comparison = Comparison::None;
            AZ::u8 m_parseDepth;
        };

        Dependency();
        Dependency(const Dependency& dep);

        ~Dependency() = default;

        /**
         * Gets the ID of the object depended on.
         *
         * \returns             The ID of the object depended on.
         */
        const AZ::Uuid& GetID() const;

        /**
         * Set the ID of the object depended on.
         *
         * \params[in] id       The ID of the dependency
         */
        void SetID(const AZ::Uuid& id);

        /**
         * Gets the name of the object depended on.
         *
         * \returns             The name of the object depended on.
         */
        const AZStd::string& GetName() const;

        /**
         * Set the name of the object depended on.
         *
         * \params[in] name       The name of the dependency
         */
        void SetName(const AZStd::string& name);

        /**
         * Gets the bounds that the dependence's version must fulfill.
         *
         * \returns             The list of bounds.
         */
        const AZStd::vector<Bound>& GetBounds() const;

        /**
         * Checks if a specifier matches a dependency.
         *
         * Checks that the specifier's ID is the one depended on,
         * and that the version matches the bounds (which can be retrieved by GetBounds()).
         *
         * \params[in] spec     The specifier to test.
         *
         * \returns             Whether or not the Specifier<N> fits the dependency.
         */
        bool IsFullfilledBy(const Specifier<N>& spec) const;

        /**
         * Parses version bounds from a list of strings.
         *
         * Each string should fit the pattern [OPERATOR][VERSION],
         * where [OPERATOR] is >, >=, <, <=, ==, ~> or ~=,
         * and [VERSION] is a valid version string, parsable by AZ::Version<N>.
         *
         * \params[in] deps     The list of bound strings to parse.
         *
         * \returns             True on success, false on failure.
         */
        AZ::Outcome<void, AZStd::string> ParseVersions(const AZStd::vector<AZStd::string>& deps);

        AZ::Uuid m_id = AZ::Uuid::CreateNull();
        AZStd::string m_name;
        AZStd::vector<Bound> m_bounds;

    private:

        AZ::Outcome<AZ::u8> ParseVersion(AZStd::string str, Version<N>& ver);

        AZStd::regex m_dependencyRegex;
        AZStd::regex m_namedDependencyRegex;
        AZStd::regex m_versionRegex;
    };

#define BITMASK_OPS(Ty)                                                                             \
    inline Ty&operator&=(Ty & left, Ty right)   { left = (Ty)((int)left & (int)right); return (left); } \
    inline Ty& operator|=(Ty& left, Ty right)   { left = (Ty)((int)left | (int)right); return (left); } \
    inline Ty& operator^=(Ty& left, Ty right)   { left = (Ty)((int)left ^ (int)right); return (left); } \
    inline Ty operator&(Ty left, Ty right)      { return ((Ty)((int)left & (int)right)); }              \
    inline Ty operator|(Ty left, Ty right)      { return ((Ty)((int)left | (int)right)); }              \
    inline Ty operator^(Ty left, Ty right)      { return ((Ty)((int)left ^ (int)right)); }              \
    inline Ty operator~(Ty left)                { return ((Ty) ~(int)left); }

    BITMASK_OPS(Dependency< Version<4>::parts_count>::Bound::Comparison)
    BITMASK_OPS(Dependency<SemanticVersion::parts_count>::Bound::Comparison)

#undef BITMASK_OPS


} // namespace AzFramework

#include <AzCore/Dependency/Dependency.inl>
