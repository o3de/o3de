/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcBackend.h"

namespace AZ::ShaderCompiler
{
    using MapOfBeginToSpanAndUid = map<ssize_t, pair< misc::Interval, IdentifierUID> >;
    using MapOfIntervalToUid = map<Interval<ssize_t>, IdentifierUID>;

    struct CodeReflection : Backend
    {
        CodeReflection(IntermediateRepresentation* ir, TokenStream* tokens, std::ostream& out)
            : Backend(ir, tokens),
              m_out(out)
        {}

        // bring all non code emitting features here.

        //! Reflect information about vertex and compute stage inputs
        Json::Value GetShaderEntries(const char * const sEntry = nullptr) const;

        //! Reflect information about fragment stage outputs
        Json::Value GetOutputMergerLayout(const char * const psEntry = nullptr) const;

        //! Dumps reflected information about fragment stage outputs
        void DumpOutputMergerLayout(const char * const psEntry = nullptr) const;

        //! Dumps reflected information about vertex and compute shader entries
        void DumpShaderEntries() const;

        //! Reflect resource groups layout
        //! @param options  user configuration parsed from command line
        void DumpSRGLayout(const Options& options, PreprocessorLineDirectiveFinder* lineFinder) const;

        //! Reflect shader options
        //! @param options  user configuration parsed from command line
        void DumpVariantList(const Options& options) const;

        //! Reflect extended resource binding information.
        //! Such as which stage function is client of a resource; and what are the registers of the descriptors.
        //! @param options  user configuration parsed from command line
        void DumpResourceBindingDependencies(const Options& options) const;

        //! Determine a heurisitcal global order between options in the program, using "impacted code size" static analysis.
        void AnalyzeOptionRanks() const;

    private:

        //! Builds member variable packing information and adds it to the membersContainer
        uint32_t BuildMemberLayout(Json::Value& membersContainer, string_view namePrefix, const IdentifierUID& memberId, const bool isArrayItr, const Options& options, const AZ::ShaderCompiler::Packing::Layout layoutPacking, uint32_t& offset) const;

        //! Gets the stride for a resource view based on its generic type
        uint32_t GetViewStride(const IdentifierUID& memberId, const AZ::ShaderCompiler::Packing::Layout& layoutPacking, const Options& options) const;

        //! Gets the stride for a user defined type containing member variables
        [[nodiscard]]
        uint32_t BuildUserDefinedMemberLayout(Json::Value& membersContainer, const IdentifierUID& exportedTypeId, const Options& options, const AZ::ShaderCompiler::Packing::Layout layoutPacking, uint32_t& startAt, string_view namePrefix) const;

        bool BuildIAElement(Json::Value& jsonVal, const IdentifierUID& uid, bool allowStruct) const;

        bool BuildOMElement(Json::Value& jsonVal, const ExtendedTypeInfo& returnTypeRef, string_view semanticOverride, int& semanticIndex, bool systemValue) const;

        bool BuildOMStruct(const ExtendedTypeInfo& returnTypeRef, string_view semanticOverride, Json::Value& jsonVal, int& semanticIndex) const;

        //! Populate a list of functions where a symbol appear as potentially used
        //! @param uid      The symbol to start the dependency analysis on
        //! @param output   Any dependency symbol will be appended to this set
        //! @scopes         Code reflecting structure storing a map of intervals of function scopes
        //!                 The last parameter is for internal recursion tracking.
        void DiscoverTopLevelFunctionDependencies(const IdentifierUID& uid, set<IdentifierUID>& output, const MapOfBeginToSpanAndUid& scopes, set<IdentifierUID>&&) const;

        bool IsNonOverloaded(const IdentifierUID& uid) const;

        bool IsPotentialEntryPoint(const IdentifierUID& uid) const;

        // Estimate a score proportional to how much code is "child" to the AST node at `location`
        int AnalyzeImpact(TokensLocation const& location) const;

        // Recursive internal detail version
        void AnalyzeImpact(ParserRuleContext* astNode, int& scoreAccumulator) const;

        // Function-call specific
        void AnalyzeImpact(azslParser::FunctionCallExpressionContext* callNode, int& scoreAccumulator) const;

        //! Useful for static analysis on dependencies or option ranks
        void GenerateTokenScopeIntervalToUidReverseMap() const;
        mutable MapOfBeginToSpanAndUid m_functionIntervals;  //< only functions because they are guaranteed to be disjointed (largely simplifies queries)
        mutable IntervalCollection<ssize_t> m_intervals;  //< augmented version with anonymous blocks (slower query)
        mutable MapOfIntervalToUid m_intervalToUid;  //< side by side data since we don't want to weight the interval struct with a payload

        std::ostream& m_out;
    };
}
