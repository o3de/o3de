/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CLI/CLI.hpp>

#include "AzslcReflection.h"
#include "AzslcEmitter.h"
#include "AzslcHomonymVisitor.h"
#include "AzslcPlatformEmitter.h"
#include "Texture2DMSto2DCodeMutator.h"
#include "SubpassInputToTexture2DCodeMutator.h"

#include <cstddef>
#include <filesystem>
namespace StdFs = std::filesystem;

// versioning
// Correspond to the supported version of the AZSL language.
#define AZSLC_MAJOR "1"
// For large features or milestones. Minor version allows for breaking changes. Existing tests can change.
#define AZSLC_MINOR "8"   // last change: introduction of class inheritance
// For small features or bug fixes. They cannot introduce breaking changes. Existing tests shouldn't change.
#define AZSLC_REVISION "20"  // last change: update antlrv4 to 4.13.2

namespace AZ::ShaderCompiler
{
    DiagnosticStream verboseCout;
    DiagnosticStream warningCout(std::cerr);
    Endl azEndl;

    using MapOfStringViewToSetOfString = map<string_view, set<string>>;

    template <typename TypeClassFilterPredicate = std::nullptr_t>
    void VisitTokens(const antlr4::Recognizer* recognizer,
                     MapOfStringViewToSetOfString& acceptedToken, set<string>& notTypes1,  // out
                     TypeClassFilterPredicate tcFilter = nullptr)
    {
        // loop over all keywords
        const auto& vocabulary = recognizer->getVocabulary();
        size_t maxToken = vocabulary.getMaxTokenType();
        for (size_t ii = 0; ii < maxToken; ++ii)
        {
            string_view token = vocabulary.getLiteralName(ii);
            token = Trim(token, "\"'"); // because AntlR gives us e.g "'float'"
            if (!token.empty())         // empty when there is a complex rule (not a straight unconditional keyword)
            {
                TypeClass tc = AnalyzeTypeClass(TentativeName{string{token}});
                bool accept = true;
                if constexpr (!is_same_v<std::nullptr_t, std::remove_reference_t<decltype(tcFilter)>>)
                {
                    accept = tcFilter(tc);
                }

                if (tc == TypeClass::IsNotType)
                {
                    notTypes1.insert(string{token});
                }

                if (accept)
                {
                    acceptedToken[TypeClass::ToStr(tc)].emplace(std::move(token));
                }
            }
        }
    }

    // out argument: classifiedTokens
    // filter: is a predicate for condition to check to pass registration
    template <typename FilterFunction>
    void ClassifyAllTokens(const azslLexer* lexer, MapOfStringViewToSetOfString& classifiedTokens, FilterFunction filter)
    {
        set<string> notTypes1;
        VisitTokens(lexer, classifiedTokens, notTypes1, filter);
        // now. because of names such as StructuredBuffer or matrix, need to have a generic appendix to mean something,
        // they will be classified as IsNotType. So we need to re-attempt analysis by appending something parseable.

        // get a scalar typename from the scalar class:
        string someScalar = *classifiedTokens[TypeClass::ToStr(TypeClass::Scalar)].begin();

        // now we'll use it to construct parseable generic type expressions
        enum class RetryStateMachine
        {
            OneTypeGenericParameter,
            OneTypeAndDimensionGenericParameters,
            OneTypeAndTwoDimensionsGenericParameters,
            End,
        };

        constexpr auto isNotTypeKey = TypeClass::ToStr(TypeClass::IsNotType);
        // completely delete all IsNotType from the classification, and redo it all over.
        set<string> notTypes = std::move(classifiedTokens[isNotTypeKey]);
        classifiedTokens.erase(isNotTypeKey);
        SetMerge(notTypes, notTypes1);

        for (const string& token : notTypes)
        {
            RetryStateMachine state = RetryStateMachine::OneTypeGenericParameter;
            TypeClass tc;
            do
            {
                string attemptedTypeName = token;
                switch (state)
                {
                case RetryStateMachine::OneTypeGenericParameter:
                    attemptedTypeName += "<" + someScalar + ">";
                    state = RetryStateMachine::OneTypeAndDimensionGenericParameters;
                    break;
                case RetryStateMachine::OneTypeAndDimensionGenericParameters:
                    attemptedTypeName += "<" + someScalar + ",1>";
                    state = RetryStateMachine::OneTypeAndTwoDimensionsGenericParameters;
                    break;
                case RetryStateMachine::OneTypeAndTwoDimensionsGenericParameters:
                    attemptedTypeName += "<" + someScalar + "1,1>";
                    state = RetryStateMachine::End;
                    break;
                case RetryStateMachine::End:
                default:
                    // save as is
                    break;
                }
                tc = AnalyzeTypeClass(TentativeName{attemptedTypeName});
            } while (tc == TypeClass::IsNotType && state != RetryStateMachine::End);

            // re-register at the correct place (if passes filter):
            bool accept = true;
            if constexpr (!is_same_v<std::nullptr_t, std::remove_reference_t<decltype(filter)>>)
            {
                accept = filter(tc);
            }

            if (accept)
            {
                classifiedTokens[TypeClass::ToStr(tc)].insert(token);
            }
        } // end for
    }

    bool IsKeyword(const antlr4::Recognizer* r, antlr4::Token* token)
    {
        MapOfStringViewToSetOfString byTypeClass;
        set<string> notTypes;
        VisitTokens(r, byTypeClass, notTypes);
        bool notType = notTypes.find(token->getText()) != notTypes.end();
        bool notIdentifier = r->getVocabulary().getSymbolicName(token->getType()) != "Identifier";
        bool firstIsalpha(isalpha(token->getText()[0]));
        return notType && notIdentifier && firstIsalpha;
    }

    void DumpClassifiedTokensToYaml(const MapOfStringViewToSetOfString& classifiedTokens)
    {
        for (const auto& [category, tokens] : classifiedTokens)
        {
            std::cout << category << ":\n";
            for (const string& tokenName : tokens)
            {
                std::cout << "  - \"" << tokenName << "\"\n";
            }
        }
    }

    // outputs information about language keywords related to predefined types
    void DumpPredefinedVocabulary(const azslLexer* lexer)
    {
        MapOfStringViewToSetOfString classifiedTokens;
        ClassifyAllTokens(lexer, classifiedTokens /*out*/, [](TypeClass tc) { return IsPredefinedType(tc); });
        DumpClassifiedTokensToYaml(classifiedTokens);
    }

    //! iterates on tokens and build the line number mapping (from preprocessor line directives)
    void ConstructLineMap(vector<std::unique_ptr<Token>>* allTokens, PreprocessorLineDirectiveFinder* lineFinder)
    {
        string lastNonEmptyFileName = lineFinder->m_physicalSourceFileName;
        for (auto& token : *allTokens) // auto& because each element is a unique_ptr we can't copy
        {
            if (token->getType() == azslLexer::LineDirective)
            {
                LineDirectiveInfo directiveInfo{ 0, 0 };
                const auto lineText = token->getText();
                //                    the sharp
                //                        |  any whitespaces
                //                        | /   optional line token
                //                        | |     |       decimal
                // custom raw string      | |     |          |  optional filename between quotes
                //         delimiter --+  | |     |          |        |
                static const std::regex lineRegex(R"__(#\s*(line\s+)?\s*(\d+)\s*("(.*)")?)__");
                auto matchBegin = std::sregex_iterator(lineText.begin(), lineText.end(), lineRegex);
                // there can be only 1 match, and it HAS to match since AntlR lexer already matched.
                auto& groups = *matchBegin; // 4 groups: [0] is the whole line. [1] is the first parenthesized group, [2] the 2nd etc
                directiveInfo.m_physicalTokenLine = token->getLine();
                directiveInfo.m_forcedLineNumber = std::atoi(groups[2].str().c_str());
                directiveInfo.m_containingFilename = groups[4];
                if (directiveInfo.m_containingFilename.empty())
                {
                    // if we don't have a filename specified on the line, it means the last seen filename is still active.
                    // storing it this way simplifies the lookup algorithm using this data.
                    directiveInfo.m_containingFilename = lastNonEmptyFileName;
                }
                else
                {
                    lastNonEmptyFileName = directiveInfo.m_containingFilename;
                }
                lineFinder->PushLineDirective(directiveInfo);
            }
        }
        if (lineFinder->m_lineMap.find(1) == lineFinder->m_lineMap.end())
        {
            // if we have no line directives on line 1, add one before the file's first line (at 0), that can always be found by Infimum
            lineFinder->PushLineDirective({0, 1, lineFinder->m_physicalSourceFileName});
        }
    }
}

namespace AZ::ShaderCompiler::Main
{
    using namespace AZ::ShaderCompiler;

    /// This function will support the --ast option. It uses an AntlR facility and prettifies it.
    void PrintAst(tree::ParseTree* tree, azslParser& parser)
    {
        std::string s = tree->toStringTree(&parser);
        // hopefully easy to read indentator
        std::string curindent = "";
        for (char c : s)
        {
            if (c == '(')
            {
                std::cout << "\n";
                curindent += "  ";
                std::cout << curindent << c;
            }
            else if (c == ')')
            {
                std::cout << c << "\n";
                curindent = curindent.substr(0, std::max(2_sz, curindent.size()) - 2);
                std::cout << curindent;
            }
            else
            {
                std::cout << c;
            }
        }
        std::cout << std::endl; // flush
    }

    /// this function supports the --visitsym option
    // @symbolMqn   starting point of symbol homonyms graph discovery. Mqn: mangled qualified name
    void PrintVisitSymbol(IntermediateRepresentation& ir, string_view symbolMqn, RelationshipExtentFlag visitOptions)
    {
        IdAndKind* symbol = ir.GetIdAndKindInfo(QualifiedNameView{symbolMqn});
        if (!symbol)
        {
            std::cerr << "Error: symbol " << symbolMqn.data() << " not found. To list all symbols use --dumpsym option.\n";
            return;
        }
        std::cout << "Symbol found. kind: " << Kind::ToStr(ir.GetKind(symbol->first)) << ". Homonyms list:\n";
        HomonymVisitor hv{[&ir](QualifiedNameView qnv) { return ir.GetKindInfo({{qnv}}); }};
        hv(symbol->first,
           [](const Seenat &at, RelationshipExtent category)
             {
                 std::cout << "- {categ: " << RelationshipExtent::ToStr(category)
                           << ", id: " << Decorate("'", at.m_referredDefinition.GetName())
                           << ", at: ':" << at.m_where.m_line << ":" << at.m_where.m_charPos + 1 << "'"
                           << ", token#: " << at.m_where.m_focusedTokenId << "}\n";
             },
            visitOptions);
    }

    void ParseWarningLevel(const std::array<bool, Warn::EndEnumeratorSentinel_>& args, DiagnosticStream& warningConfig)
    {
        for (auto level : Warn::Enumerate{})
        {
            bool active = args[level];
            if (active)
            {
                if (level >= Warn::Wx)
                {
                    warningConfig.SetAsErrorLevel(level);
                }
                else
                {
                    warningConfig.SetRevealedWarningLevel(level);
                }
            }
        }
    }
}

namespace AZ
{
    string_view GetFileLeafName(string_view path)
    {
        return Slice(path, path.find_last_of("/\\") + 1, -1);
    }

    inline void DoAsserts()
    {
#ifndef NDEBUG
        using namespace AZ::Tests;

        DoAsserts2();
        DoAsserts4();
        DoAsserts5();
        DoAsserts6();
#endif
    }
}

int main(int argc, const char* argv[])
{
    using namespace AZ;
    using namespace AZ::ShaderCompiler::Main;

    CLI::App cli{ "Amazon Shader Language Compiler" };

    bool printVersion = false;
    cli.add_flag("--version", printVersion, "Prints version information");

    std::string inputFile;
    cli.add_option("FILE", inputFile, "Input file (pass - to read from stdin).");

    std::string output;
    cli.add_option("-o", output, "Output file (writes to stdout if omitted).");

    bool uniqueIdx = false;
    cli.add_flag("--unique-idx", uniqueIdx, "Use unique indices for all registers. e.g. b0, t0, u0, s0 becomes b0, t1, u2, s3. Use on platforms that don't differentiate registers by resource type.");
    bool cbBody = false;
    cli.add_flag("--cb-body", cbBody, "Emit ConstantBuffer body rather than using <T>.");

    bool rootSig = false;
    cli.add_flag("--root-sig", rootSig, "Emit RootSignature for parameter binding in the shader. --namespace must also be used to select a specific API.");

    int rootConst = 0;
    auto rootConstOpt = cli.add_option("--root-const", rootConst, "Maximum size in bytes of the root constants buffer.");

    bool padRootConst = false;
    cli.add_flag("--pad-root-const", padRootConst, "Automatically append padding data to the root constant CB to keep it aligned to a 16-byte boundary.");

    bool Zpc = false;
    cli.add_flag("--Zpc", Zpc, "Pack matrices in column-major order (default). Cannot be specified together with -Zpr.");

    bool Zpr = false;
    cli.add_flag("--Zpr", Zpr, "Pack matrices in row-major order. Cannot be specified together with -Zpc.");

    bool packDx12 = false;
    cli.add_flag("--pack-dx12", packDx12, "Pack buffers using strict DX12 packing rules. If not specified AZSLc will use relaxed packing rules.");

    bool packVulkan = false;
    cli.add_flag("--pack-vulkan", packVulkan, "Pack buffers using strict Vulkan packing rules (Vector-relaxed std140 for uniforms and std430 for storage buffers).");

    bool packOpenGL = false;
    cli.add_flag("--pack-opengl", packOpenGL, "Pack buffers using strict OpenGL packing rules (Vector-strict std140 for uniforms and std430 for storage buffers).");

    std::vector<std::string> namespaces;
    cli.add_option("--namespace", namespaces,
        "Activate an attribute namespace. May be used multiple times to activate multiple namespaces. "
        "Activating a namespace may also activate corresponding API-specific features, like dx for DirectX 12, vk for Vulkan, and mt for Metal.");

    bool ia = false;
    cli.add_flag("--ia", ia, "Output a list of vs entries with their Input Assembler layouts *and* a list of CS entries and their numthreads.");

    bool om = false;
    cli.add_flag("--om", om, "Output the Output Merger layout instead of the shader code.");

    bool srg = false;
    cli.add_flag("--srg", srg, "Output the Shader Resource Group layout instead of the shader code.");

    bool options = false;
    cli.add_flag("--options", options, "Output the list of available shader options for this shader.");

    bool dumpsym = false;
    cli.add_flag("--dumpsym", dumpsym, "Dump symbols.");

    bool syntax = false;
    cli.add_flag("--syntax", syntax, "Check syntax (no output means no complaints).");

    bool semantic = false;
    cli.add_flag("--semantic", semantic, "Check semantics (no output means no complaints).");

    bool ast = false;
    cli.add_flag("--ast", ast, "Output the abstract syntax tree.");

    bool bindingdep = false;
    cli.add_flag("--bindingdep", bindingdep, "Output binding dependencies (what entry points access what external resources).");

    std::string visitName;
    cli.add_option("--visitsym", visitName, "Output the locations of all relationships of the supplied symbol name.");

    bool full = false;
    cli.add_flag("--full", full, "Output the shader code, IA layout, OM layout, SRG layout, the list of available shader options, and the binding dependencies.");

    bool stripUnusedSrgs = false;
    cli.add_flag("--strip-unused-srgs", stripUnusedSrgs, "Strips unused SRGs.");

    bool noMS = false;
    cli.add_flag("--no-ms", noMS, "Transforms usage of Texture2DMS/Texture2DMSArray and related functions and semantics into plain Texture2D/Texture2DArray "
        "equivalents. This is useful for allowing shader authors to easily write AZSL code that can be compiled into alternatives to work with both a "
        "multisample render pipeline and a non-MS render pipeline.");

    bool noAlignmentValidation = false;
    cli.add_flag("--no-alignment-validation", noAlignmentValidation, "Skips checking for potential alignment issues related to differences between dxil and spirv."
        "By default, potential alignment discrepancies will fail compilation.");

    bool visitDirectReferences = false;
    cli.add_flag("-d", visitDirectReferences, "(Option of --visitsym) Visit direct references.");

    bool visitOverloadSet = false;
    cli.add_flag("-v", visitOverloadSet, "(Option of --visitsym) Visit overload-set.");

    bool visitFamily = false;
    cli.add_flag("-f", visitFamily, "(Option of --visitsym) Visit family.");

    bool visitRecursively = false;
    cli.add_flag("-r", visitRecursively, "(Option of --visitsym) Visit recursively.");

    bool listPredefined = false;
    cli.add_flag("--listpredefined", listPredefined, "Output a list of all predefined types in AZSLang.");

    int maxSpaces = std::numeric_limits<int>::max();
    auto maxSpacesOpt = cli.add_option("--max-spaces", maxSpaces, "Will choose register spaces that do not extend past this limit.");

    bool useSpecializationConstants = false;
    cli.add_flag("--sc-options", useSpecializationConstants, "Use specialization constants for shader options.");

    bool noSubpassInput = false;
    cli.add_flag("--no-subpass-input", noSubpassInput, "Transform usage of SubpassInput/SubpassInputMS into Texture2D/Texture2DMS");

    int32_t subpassInputOffset = 0;
    cli.add_option("--subpass-input-offset", subpassInputOffset, "Offset to apply to the subpass index attribute.");

    std::array<bool, Warn::EndEnumeratorSentinel_> warningOpts;
    for (const auto e : Warn::Enumerate{})
    {
        warningOpts[e] = false;
    }
    cli.add_flag("--W0", warningOpts[Warn::EnumType::W0], "Suppresses all warnings.");
    cli.add_flag("--W1", warningOpts[Warn::EnumType::W1], "Activate severe warnings (default).");
    cli.add_flag("--W2", warningOpts[Warn::EnumType::W2], "Activate warnings that may be significant.");
    cli.add_flag("--W3", warningOpts[Warn::EnumType::W3], "Activate low-confidence diagnostic warnings.");
    cli.add_flag("--Wx", warningOpts[Warn::EnumType::Wx], "Treat activated warnings as errors.");
    cli.add_flag("--Wx1", warningOpts[Warn::EnumType::Wx1], "Treat level-1 warnings as errors.");
    cli.add_flag("--Wx2", warningOpts[Warn::EnumType::Wx2], "Treat level-2 and below warnings as errors.");
    cli.add_flag("--Wx3", warningOpts[Warn::EnumType::Wx3], "Treat level-3 and below warnings as errors.");

    std::vector<int> minDescriptors;
    auto minDescriptorsOpt = cli.add_option("--min-descriptors", minDescriptors, "Comma-separated list of limits corresponding to "
        "<set,space,sampler,texture,buffer> descriptors. Emits a warning if a count overshoots a limit. Use -1 to specify \"no limit\".");
    minDescriptorsOpt->delimiter(',')->expected(5);
    bool verbose = false;
    cli.add_flag("--verbose", verbose);

    DoAsserts();

    int processReturnCode = 0;
    try
    {
        CLI11_PARSE(cli, argc, argv);

        // Major.Minor.Revision
        auto versionString = string{"AZSL Compiler " AZSLC_MAJOR "." AZSLC_MINOR "." AZSLC_REVISION " "} + GetCurrentOsName().data();

        if (printVersion)
        {
            std::cout << versionString << std::endl;
            return 0;
        }

        if (listPredefined)
        {
            verboseCout.m_on = false;
            ANTLRInputStream is;
            azslLexer lexer{&is};
            DumpPredefinedVocabulary(&lexer);
            return 0;
        }

        verboseCout.m_on = verbose;

        bool useStdin = inputFile == '-';
        // we need to scope a stream object here, to be able to bind a polymorphic reference to it
        std::ifstream ifs;
        if (!useStdin)
        {
            ifs = std::ifstream{ inputFile }; // try to open as file
        }

        std::istream& in{useStdin ? std::cin : ifs};
        if (!in.good())
        {
            throw std::runtime_error("input file could not be opened");
        }

        if (rootSig && namespaces.empty())
        {
            throw std::runtime_error("--root-sig requested but no API was selected. Use a --namespace option as well.");
        }

        const string inputFileName = useStdin ? "" : inputFile;
        PreprocessorLineDirectiveFinder lineFinder;
        lineFinder.m_physicalSourceFileName = useStdin ? "stdin" : inputFile;
        // setup the line finder address on the exception system so that errors are canonically mutated to "virtual line space"
        AzslcException::s_lineFinder = &lineFinder;

        bool useOutputFile = !output.empty();
        const string outputFileName = output;

        ANTLRInputStream input(in);
        azslLexer lexer(&input);
        CommonTokenStream tokens(&lexer);
        IntermediateRepresentation ir(&lexer);
        auto allTokens = lexer.getAllTokens();
        if (lexer.getNumberOfSyntaxErrors() > 0)
        {
            throw std::runtime_error("syntax errors present");
        }
        ConstructLineMap(&allTokens, &lineFinder);
        lexer.reset();
        AzslParserEventListener azslParserEventListener;
        azslParser parser(&tokens);
        parser.removeErrorListeners();
        azslParserEventListener.m_isKeywordPredicate = IsKeyword;
        parser.addErrorListener(&azslParserEventListener);
        tree::ParseTree *tree = parser.compilationUnit();

        if (ast)
        {
            PrintAst(tree, parser);
            syntax = true; // ast print is a syntax only build.
        }

        if (parser.getNumberOfSyntaxErrors() > 0)
        {
            throw std::runtime_error("grammatic errors present");
        }

        if (syntax)
        { // if we are here with no exception then the syntax pass is valid.
        }
        else // continue with semantic, and later emission
        {
            if (!useStdin)
            {
                StdFs::path inSource{ inputFile };
                ir.m_metaData.m_insource = StdFs::absolute(inSource).lexically_normal().generic_string();
            }

            // Enable attribute namespaces
            std::for_each(namespaces.begin(), namespaces.end(),
                [&](const string& space) { ir.AddAttributeNamespaceFilter(space); });

            SubpassInputSupportFlag subpassInputSupport = Backend::GetPlatformEmitter(&ir).GetSubpassInputSupport();
            if (noSubpassInput)
            {
                subpassInputSupport = SubpassInputSupportFlag::None;
            }

            tree::ParseTreeWalker walker;
            Texture2DMSto2DCodeMutator texture2DMSto2DCodeMutator(&ir, &tokens);
            SubpassInputToTexture2DCodeMutator subpassInputToTexture2DCodeMutator(&ir, &tokens, subpassInputSupport);
            SemaCheckListener semanticListener{&ir};
            warningCout.m_onErrorCallback = [](string_view message) {
                throw AzslcException{WX_WARNINGS_AS_ERRORS, "as-error", string{message}};
            };
            ParseWarningLevel(warningOpts, warningCout);
            bool nonValidativeOptions[] = {full, ia, om, srg, options, dumpsym, ast, bindingdep, !visitName.empty(), stripUnusedSrgs};
            bool anyNonValidativeOption = std::any_of(std::begin(nonValidativeOptions), std::end(nonValidativeOptions), [](bool opt) { return opt; });
            semanticListener.m_silentPrintExtensions = !semantic || verbose; // print-extensions are useful for interested parties; but not normal operation.
            if (noMS)
            {
                semanticListener.m_functionCallMutators.push_back(&texture2DMSto2DCodeMutator);
            }
            semanticListener.m_functionCallMutators.push_back(&subpassInputToTexture2DCodeMutator);
            warningCout.m_on = !anyNonValidativeOption; // warnings are interesting for emission, and explicit semantic check modes.

            UnboundedArraysValidator::Options unboundedArraysValidationOptions;
            unboundedArraysValidationOptions.m_useUniqueIndicesEnabled = uniqueIdx;
            if (*maxSpacesOpt)
            {
                unboundedArraysValidationOptions.m_maxSpaces = maxSpaces;
            }
            ir.m_sema.m_unboundedArraysValidator.SetOptions(unboundedArraysValidationOptions);

            // semantic logic and validation
            walker.walk(&semanticListener, tree);

            Options emitOptions;
            emitOptions.m_useUniqueIndices = uniqueIdx;
            emitOptions.m_emitConstantBufferBody = cbBody;
            emitOptions.m_emitRootSig = rootSig;
            emitOptions.m_padRootConstantCB = padRootConst;
            emitOptions.m_skipAlignmentValidation = noAlignmentValidation;
            emitOptions.m_useSpecializationConstantsForOptions = useSpecializationConstants;
            emitOptions.m_subpassInputsOffset = subpassInputOffset;

            if (*rootConstOpt)
            {
                emitOptions.m_rootConstantsMaxSize = rootConst;
            }

            if (*minDescriptorsOpt)
            {
                emitOptions.m_minAvailableDescriptors = {
                    minDescriptors[0], minDescriptors[1], minDescriptors[2], minDescriptors[3], minDescriptors[4]};
            }

            if (*maxSpacesOpt)
            {
                emitOptions.m_maxSpaces = maxSpaces;
            }

            if (Zpc && Zpr)
            {
                throw std::runtime_error("Cannot specify --Zpr and --Zpc together, use --help to get usage information");
            }
            else if (Zpr)
            {
                emitOptions.m_forceMatrixRowMajor = true;
                emitOptions.m_forceEmitMajor = true;
            }
            else if (Zpc)
            {
                emitOptions.m_forceMatrixRowMajor = false; // Default
                emitOptions.m_forceEmitMajor = true;
            }

            if (packDx12)
            {
                emitOptions.m_packConstantBuffers = AZ::ShaderCompiler::Packing::Layout::DirectXPacking;
                emitOptions.m_packDataBuffers = AZ::ShaderCompiler::Packing::Layout::DirectXStoragePacking;
            }

            if (packVulkan)
            {
                emitOptions.m_packConstantBuffers = AZ::ShaderCompiler::Packing::Layout::RelaxedStd140Packing;
                emitOptions.m_packDataBuffers = AZ::ShaderCompiler::Packing::Layout::RelaxedStd430Packing;
            }

            if (packOpenGL)
            {
                emitOptions.m_packConstantBuffers = AZ::ShaderCompiler::Packing::Layout::StrictStd140Packing;
                emitOptions.m_packDataBuffers = AZ::ShaderCompiler::Packing::Layout::StrictStd430Packing;
            }

            // middle end logic
            MiddleEndConfiguration middleEndConfigration{emitOptions.m_rootConstantsMaxSize,
                                                         emitOptions.m_packConstantBuffers,
                                                         emitOptions.m_packDataBuffers,
                                                         emitOptions.m_forceMatrixRowMajor,
                                                         emitOptions.m_padRootConstantCB,
                                                         emitOptions.m_skipAlignmentValidation};
            ir.MiddleEnd(middleEndConfigration, &lineFinder);
            if (noMS)
            {
                texture2DMSto2DCodeMutator.RunMiddleEndMutations();
            }
            bool hasSubpassInputMutations = subpassInputToTexture2DCodeMutator.RunMiddleEndMutations();

            // If ir fails to find any root members in the source, overwrite the m_numOfRootConstants to 0
            if (ir.m_rootConstantStructUID.m_name == "")
            {
                emitOptions.m_rootConstantsMaxSize = 0;
            }
            // intermediate state validation
            ir.Validate();

            if (stripUnusedSrgs)
            {
                ir.RemoveUnusedSrgs();
            }

            if (dumpsym)
            {
                DumpSymbols(ir);
            }
            else if (!visitName.empty())
            {
                using RE = RelationshipExtent;
                RelationshipExtentFlag visitOptions{RE::Self}; // at least self.  + optional things as listed here-under
                array<pair<bool, RE::EnumType>, 4> optToRelation = {{{visitDirectReferences, RE::Reference},
                    {visitFamily, RE::Family},
                    {visitOverloadSet, RE::OverloadSet},
                    {visitRecursively, RE::Recursive}}};
                for (auto &&possibleOption : optToRelation)
                {
                    visitOptions |= possibleOption.first ? possibleOption.second : RE::EnumType(0);
                }
                PrintVisitSymbol(ir, visitName, visitOptions);
            }
            else if (!semantic)  // do emission
            {
                verboseCout << "--Emission/Reflection--\n";
                std::ofstream mainOutFile;

                if (useOutputFile)
                {
                    mainOutFile = std::ofstream(outputFileName);
                    if (!mainOutFile.good())
                    {
                        throw std::runtime_error("output file could not be opened");
                    }
                }

                std::ostream& out{useOutputFile ? mainOutFile : std::cout};

                CodeReflection reflecter{&ir, &tokens, out};

                // Lambda to create an output stream and perform an output action
                auto prepareOutputAndCall = [&](const string &suffix, std::function<void(CodeReflection&)> action)
                {
                    string outputName;
                    if (useOutputFile)
                    {
                        outputName = string(GetFileNameWithoutExtension(outputFileName));
                    }
                    else
                    {
                        outputName = string(GetFileNameWithoutExtension(inputFileName));
                    }
                    outputName = outputName + "." + suffix + ".json";

                    std::ofstream outFile = std::ofstream(outputName);
                    if (!outFile.good())
                    {
                        throw std::runtime_error("output file '" + outputName + "' could not be opened");
                    }

                    std::ostream& out{outFile};
                    CodeReflection reflecter{&ir, &tokens, out};

                    action(reflecter);
                };

                if (full)
                { // Combine the default emission and the ia, om, srg, options, bindingdep commands
                    CodeEmitter emitter{&ir, &tokens, out, &lineFinder};
                    if (noMS)
                    {
                        emitter.AddCodeMutator(&texture2DMSto2DCodeMutator);
                    }
                    if (hasSubpassInputMutations)
                    {
                        emitter.AddCodeMutator(&subpassInputToTexture2DCodeMutator);
                    }
                    emitter << "// HLSL emission by " << versionString << "\n";
                    emitter.Run(emitOptions);

                    prepareOutputAndCall("ia", [&](CodeReflection& r) { r.DumpShaderEntries(); });
                    prepareOutputAndCall("om", [&](CodeReflection& r) { r.DumpOutputMergerLayout(); });
                    prepareOutputAndCall("srg", [&](CodeReflection& r) { r.DumpSRGLayout(emitOptions, &lineFinder); });
                    prepareOutputAndCall("options", [&](CodeReflection& r) { r.DumpVariantList(emitOptions); });
                    prepareOutputAndCall("bindingdep", [&](CodeReflection& r) { r.DumpResourceBindingDependencies(emitOptions); });
                }
                else if (ia)
                { // Reflect the Input Assembler layout and the Compute shader entries
                    reflecter.DumpShaderEntries();
                }
                else if (om)
                { // Reflect the Input Assembler layout
                    reflecter.DumpOutputMergerLayout();
                }
                else if (srg)
                { // Reflect the Shader Resource Groups layout
                    reflecter.DumpSRGLayout(emitOptions, &lineFinder);
                }
                else if (options)
                { // Reflect the list of available variant options for this shader
                    reflecter.DumpVariantList(emitOptions);
                }
                else if (bindingdep)
                {
                    reflecter.DumpResourceBindingDependencies(emitOptions);
                }
                else
                { // Emit the shader source code
                    CodeEmitter emitter{&ir, &tokens, out, &lineFinder};
                    if (noMS)
                    {
                        emitter.AddCodeMutator(&texture2DMSto2DCodeMutator);
                    }
                    if (hasSubpassInputMutations)
                    {
                        emitter.AddCodeMutator(&subpassInputToTexture2DCodeMutator);
                    }
                    emitter << "// HLSL emission by " << versionString << "\n";
                    emitter.Run(emitOptions);
                }
            }
        }
    }
    catch (const exception& e)
    {
        OutputNestedAndException(e);
        processReturnCode = 1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception" << std::endl;
        processReturnCode = 1;
    }

    return processReturnCode;
}
