/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <TestImpactTestUtils.h>

#include <Artifact/Factory/TestImpactNativeTargetDescriptorFactory.h>
#include <Artifact/Static/TestImpactNativeTargetDescriptorCompiler.h>
#include <Dependency/TestImpactDynamicDependencyMap.h>

#include <unordered_map>

namespace UnitTest
{
    // ==============================
    // Build target dependency graphs
    // ==============================
    // 
    //       #--------#     #-------#   #------------#
    //       |Lib Core|     |Lib Aux|   | Lib Shared |*
    //       #--^--^--#     #--^-^--#   #------^-----#
    //          |  |           | |             |
    //    #-----#  #----# #----# #-----##------#
    //    |             | |            ||
    // #-----#        #-----#      #--------#
    // |Lib A|        |Lib B|      |Lib Misc|
    // #--^--#        #--^--#      #--------#
    //    |              |            
    //    #-----# #------#            
    //          | |                   
    //        #-----#             
    //        |Lib C|             
    //        #-----#             
    // 
    // =============================
    // Test target dependency graphs
    // =============================
    //                                            #--------#
    //                                            |Lib Core|
    //                                            #--^--^--#
    //                                               |  |
    //                                         #-----#  #----#
    //                                         |             |
    // #--------# #--------#      #-------# #-----#        #-----# #-------# #------------#
    // |Lib Core| |Lib Core|      |Lib Aux| |Lib A|        |Lib B| |Lib Aux| | Lib Shared |*
    // #----^---# #----^---#      #---^---# #--^--#        #-----# #---^---# #------^-----#
    //     |          |              |         |              |        |            |   
    //     |          #-----# #------#         #-----# #------#        #----##------#   
    //     |                | |                      | |                    ||
    //  #-----#           #-----#                  #-----#               #--------#  #----------#    #--------# #-------#
    //  |Lib A|           |Lib B|                  |Lib C|               |Lib Misc|  |Lib Shared|    |Lib Core| |Lib Aux|
    //  #--^--#           #--^--#                  #--^--#               #---^----#  #----^-----#    #---^----# #---^---#
    //     |                 |                        |                      |            |              |          |
    //     |                 |                        |                      |            |              |          |
    // #-------#         #-------#                #-------#              #---------# #-----------#  #---------# #--------#
    // |Test A |         |Test B |                |Test C |              |Test Misc| |Test Shared|* |Test Core| |Test Aux|
    // #-------#         #-------#                #-------#              #---------# #-----------#  #---------# #--------#
    //
    // =============
    // Test Coverage
    // ============= 
    // +=======================+===================================================+
    // | Production Source     | Tests Covered                                     |
    // +=======================+===================================================+
    // | LibA_1.cpp            | Test A                                            |
    // +-----------------------+---------------------------------------------------+
    // | LibA_2.cpp            | Test A, Test C                                    |
    // +-----------------------+---------------------------------------------------+
    // | LibB_1.cpp            | Test B, Test C                                    |
    // +-----------------------+---------------------------------------------------+
    // | LibB_2.cpp            | Test B                                            |
    // +-----------------------+---------------------------------------------------+
    // | LibB_3.cpp            | Test C                                            |
    // +-----------------------+---------------------------------------------------+
    // | LibB_AutogenInput.cpp | Test B, Test C                                    |
    // +-----------------------+---------------------------------------------------+
    // | LibC_1.cpp            | Test C                                            |
    // +-----------------------+---------------------------------------------------+
    // | LibC_2.cpp            | Test C                                            |
    // +-----------------------+---------------------------------------------------+
    // | LibC_3.cpp            | Test C                                            |
    // +-----------------------+---------------------------------------------------+
    // | LibMisc_1.cpp         | Test Misc                                         |
    // +-----------------------+---------------------------------------------------+
    // | LibMisc_2.cpp         | Test Misc                                         |
    // +-----------------------+---------------------------------------------------+
    // | LibCore_1.cpp         | Test Core, Test C                                 |
    // +-----------------------+---------------------------------------------------+
    // | LibCore_2.cpp         | Test Core, Test A, Test B, Test C                 |
    // +-----------------------+---------------------------------------------------+
    // | LibAux_1.cpp          | Test Aux, Test B, Test Misc                       |
    // +-----------------------+---------------------------------------------------+
    // | LibAux_2.cpp          | Test Aux, Test C, Test Misc, Test Shared*         |
    // +-----------------------+---------------------------------------------------+
    // | LibAux_3.cpp          | Test Aux, Test B, Test C, Test Misc               |
    // +-----------------------+---------------------------------------------------+
    // | LibShared.cpp*        | Test Aux, Test Misc, Test B, Test C, Test Shared* |
    // +-----------------------+---------------------------------------------------+
    // | ProdAndTest.cpp       | Test A (Shared by Lib A and Test Misc)            |
    // +-----------------------+---------------------------------------------------+
    //
    // +================+===============+
    // | Test Source    | Tests Covered |
    // +================+===============+
    // | TestA.cpp      | Test A        |
    // +----------------+---------------+
    // | TestB.cpp      | Test B        |
    // +----------------+---------------+
    // | TestC.cpp      | Test C        |
    // +----------------+---------------+
    // | TestMisc.cpp   | Test Misc     |
    // +----------------+---------------+
    // | TestCore.cpp   | Test Core     |
    // +----------------+---------------+
    // | TestAux.cpp    | Test Aux      |
    // +----------------+---------------+
    // | TestShared.cpp | Test Shared*  |
    // +----------------+---------------+
    //
    // +===================+===================================================+
    // | Production Target | Tests Covered                                     |
    // +===================+===================================================+
    // | Lib A             | Test A, Test C                                    |
    // +-------------------+---------------------------------------------------+
    // | Lib B             | Test B, Test C, Test Shared                       |
    // +-------------------+---------------------------------------------------+
    // | Lib C             | Test C                                            |
    // +-------------------+---------------------------------------------------+
    // | Lib Misc          | Test Misc                                         |
    // +-------------------+---------------------------------------------------+
    // | Lib Core          | Test Core, Test A, Test B, Test C                 |
    // +-------------------+---------------------------------------------------+
    // | Lib Aux           | Test Aux, Test Misc, Test Shared*, Test B, Test C |
    // +-------------------+---------------------------------------------------+
    // | Lib Shared*       | Test Shared*, Test Misc, Test Aux, Test B, Test C |
    // +-------------------+---------------------------------------------------+
    // 
    // * = Only when using WithSharedSources versions of create functions

    namespace MicroRepo
    {
        AZStd::vector<AZStd::unique_ptr<TestImpact::NativeProductionTargetDescriptor>> CreateProductionTargetDescriptors();

        AZStd::vector<AZStd::unique_ptr<TestImpact::NativeTestTargetDescriptor>> CreateTestTargetDescriptors();

        AZStd::vector<AZStd::unique_ptr<TestImpact::NativeProductionTargetDescriptor>> CreateProductionTargetDescriptorsWithSharedSources();

        AZStd::vector<AZStd::unique_ptr<TestImpact::NativeTestTargetDescriptor>> CreateTestTargetDescriptorsWithSharedSources();

        AZStd::vector<TestImpact::SourceCoveringTests> CreateSourceCoveringTestList();

        AZStd::vector<TestImpact::SourceCoveringTests> CreateSourceCoveringTestListWithSharedSources();

        template<typename TargetDescriptor>
        AZStd::unordered_set<AZStd::string> GetSources(const AZStd::vector<AZStd::unique_ptr<TargetDescriptor>>& targetDescriptors)
        {
            AZStd::unordered_set<AZStd::string> sources;
            for (const auto& targetDescriptor : targetDescriptors)
            {
                for (const auto& staticSource : targetDescriptor.m_sources.m_staticSources)
                {
                    sources.insert(staticSource);
                }

                for (const auto& autogenSource : targetDescriptor.m_sources.m_autogenSources)
                {
                    sources.insert(autogenSource.m_input);
                    for (const auto& outputSource : autogenSource.m_outputs)
                    {
                        sources.insert(outputSource);
                    }
                }
            }

            return sources;
        }

        template<typename TargetDescriptor>
        AZStd::vector<AZStd::unique_ptr<TargetDescriptor>> CreateTargetDescriptorWithoutSpecifiedSource(
            AZStd::vector<AZStd::unique_ptr<TargetDescriptor>> targetDescriptors, const TestImpact::RepoPath& sourceToRemove)
        {
            for (auto& targetDescriptor : targetDescriptors)
            {
                auto& staticSources = targetDescriptor.m_sources.m_staticSources;
                auto& autogenSources = targetDescriptor.m_sources.m_autogenSources;
                AZStd::erase_if(staticSources, [&sourceToRemove](const TestImpact::RepoPath& staticSource)
                {
                    return sourceToRemove == staticSource;
                });

                AZStd::erase_if(autogenSources, [&sourceToRemove, &staticSources](const TestImpact::AutogenPairs& pairs)
                {
                    const bool erase = (pairs.m_input == sourceToRemove)
                        || (AZStd::find(pairs.m_outputs.begin(), pairs.m_outputs.end(), sourceToRemove) != pairs.m_outputs.end());

                    if (erase)
                    {
                        for (const auto& output : pairs.m_outputs)
                        {
                            AZStd::erase_if(staticSources, [&output](const TestImpact::RepoPath& staticSource)
                            {
                                return output == staticSource;
                            });
                        }
                    }

                    return erase;
                });
            }

            return targetDescriptors;
        }

        AZStd::vector<TestImpact::SourceCoveringTests> CreateSourceCoverageTestsWithoutSpecifiedSource(
            AZStd::vector<TestImpact::SourceCoveringTests> sourceCoveringTestsList, const TestImpact::RepoPath& sourceToRemove);

        using SelectedTests = std::vector<std::string>;

        struct CRUDResult
        {
            SelectedTests m_createParentYesCoverageNo;
            SelectedTests m_updateParentYesCoverageNo;
            SelectedTests m_updateParentYesCoverageYes;
            SelectedTests m_updateParentNoCoverageYes;
            SelectedTests m_deleteParentNoCoverageYes;
        };

        using SourceMap = std::unordered_map<std::string, CRUDResult>;
        using SourceMapEntry = AZStd::pair<SourceMap::key_type, SourceMap::mapped_type>;
        using TargetMap = std::unordered_map<std::string, std::vector<std::string>>;

        enum Sources
        {
            Production = 1 << 0,
            AutogenInput = 1 << 1,
            Test = 1 << 2,
            Mixed = 1 << 3
        };

        SourceMap GenerateSourceMap(size_t sourcesToInclude);
    }
}
