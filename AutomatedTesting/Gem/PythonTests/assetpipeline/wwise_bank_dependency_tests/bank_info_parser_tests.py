"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import os
import pytest
import sys

soundbanks_xml_filename = 'SoundbanksInfo.xml'


@pytest.fixture
def soundbank_metadata_generator_setup_fixture(workspace):

    resources = dict()
    resources['tests_dir'] = os.path.dirname(os.path.realpath(__file__))
    return resources


def success_case_test(test_folder, expected_dependencies_dict, bank_info, expected_result_code=0):
    """
    Test Steps:
    1. Make sure the return code is what was expected, and that the expected number of banks were returned.
    2. Validate bank is in the expected dependencies dictionary.
    3. Validate the path to output the metadata file to was assembled correctly.
    4. Validate metadata object for this bank is set, and that it has an object assigned to its dependencies field
    and its includedEvents field
    5. Validate metadata object has the correct number of dependencies, and validated that every expected dependency
    exists in the dependencies list of the metadata object.
    6. Validate metadata object has the correct number of events, and validate that every expected event exists in the
    events of the metadata object.
    """
    expected_bank_count = len(expected_dependencies_dict)

    banks, result_code = bank_info.generate_metadata(
        os.path.join(test_folder, soundbanks_xml_filename),
        test_folder)

    # Make sure the return code is what was expected, and that the expected number of banks were returned.
    assert result_code is expected_result_code
    assert len(banks) is expected_bank_count

    for bank_index in range(expected_bank_count):
        bank = banks[bank_index]
        # Find a bank of this name in the expected dependencies dictionary.
        assert bank.path in expected_dependencies_dict

        # Make sure the path to output the metadata file to was assembled correctly.
        expected_metadata_filepath = os.path.splitext(os.path.join(test_folder, bank.path))[0] + \
            bank_info.metadata_file_extension
        assert bank.metadata_path == expected_metadata_filepath

        # Make sure the metadata object for this bank is set, and that it has an object assigned to
        #   its dependencies field and its includedEvents field
        assert bank.metadata_object
        assert bank.metadata_object['dependencies'] is not None
        assert bank.metadata_object['includedEvents'] is not None

        # Make sure the generated metadata object has the correct number of dependencies, and validated that every
        #   expected dependency exists in the dependencies list of the metadata object.
        assert len(bank.metadata_object['dependencies']) is len(expected_dependencies_dict[bank.path]['dependencies'])
        for dependency in expected_dependencies_dict[bank.path]['dependencies']:
            assert dependency in bank.metadata_object['dependencies']

        # Make sure the generated metadata object has the correct number of events, and validate that every expected
        #   event exists in the events list of the metadata object.
        assert len(bank.metadata_object['includedEvents']) is len(expected_dependencies_dict[bank.path]['events'])
        for event in expected_dependencies_dict[bank.path]['events']:
            assert event in bank.metadata_object['includedEvents']


def get_bank_info(workspace):
    sys.path.append(
        os.path.join(workspace.paths.engine_root(), 'Gems', 'AudioEngineWwise', 'Tools'))

    from WwiseAuthoringScripts import bank_info_parser as bank_info_module
    return bank_info_module


@pytest.mark.usefixtures("workspace")
@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestSoundBankMetadataGenerator:


    def test_NoMetadataTooFewBanks_ReturnCodeIsError(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Trying to generate metadata for banks in a folder with one or fewer banks and no metadata is not possible
        and should fail.

        Test Steps:
        1. Setup testing environment with only 1 bank file
        2. Get Sound Bank Info
        3. Attempt to generate sound bank metadata
        4. Verify that proper error code is returned
        """
        #
        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_NoMetadataTooFewBanks_ReturnCodeIsError')
        if not os.path.isdir(test_assets_folder):
            os.makedirs(test_assets_folder)

        bank_info = get_bank_info(workspace)

        banks, error_code = bank_info.generate_metadata(
            os.path.join(test_assets_folder, soundbanks_xml_filename),
            test_assets_folder)
        os.rmdir(test_assets_folder)

        assert error_code is 2, 'Metadata was generated when there were fewer than two banks in the target directory.'

    def test_NoMetadataNoContentBank_NoMetadataGenerated(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Test Steps:
        1. Setup testing environment
        2. No expected dependencies
        3. Call success case test
        """
        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_NoMetadataNoContentBank_NoMetadataGenerated')
        expected_dependencies = dict()
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))

    def test_NoMetadataOneContentBank_NoStreamedFiles_OneDependency(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        When no Wwise metadata is present, and there is only one content bank in the target directory with no wem
        files, then only the content bank should have metadata associated with it. The generated metadata should
        only describe a dependency on the init bank.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_NoMetadataOneContentBank_NoStreamedFiles_OneDependency')


        bank_info = get_bank_info(workspace)
        expected_dependencies = {'Content.bnk': {'dependencies': [bank_info.init_bank_path], 'events': []},}
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))
        
    def test_NoMetadataOneContentBank_StreamedFiles_MultipleDependencies(self, workspace, 
                                                                         soundbank_metadata_generator_setup_fixture):
        """
        When no Wwise metadata is present, and there is only one content bank in the target directory with wem files
        present, then only the content bank should have metadata associated with it. The generated metadata should
        describe a dependency on the init bank and all wem files in the folder.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_NoMetadataOneContentBank_StreamedFiles_MultipleDependencies')

        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'Content.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    '590205561.wem',
                    '791740036.wem'
                ],
                'events': []
            }
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))

    def test_NoMetadataMultipleBanks_OneDependency_ReturnCodeIsWarning(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        When no Wwise metadata is present, and there are multiple content banks in the target directory with wem files
        present, there is no way to tell which bank requires which wem files. A warning should be emitted,
        stating that the full dependency graph could not be created, and only dependencies on the init bank are
        described in the generated metadata files.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_NoMetadataMultipleBanks_OneDependency_ReturnCodeIsWarning')
        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'test_bank1.bnk': {'dependencies': [bank_info.init_bank_path], 'events': []},
            'test_bank2.bnk': {'dependencies': [bank_info.init_bank_path], 'events': []}
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace), expected_result_code=1)

    def test_OneContentBank_NoStreamedFiles_OneDependency(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Wwise metadata describes one content bank that contains all media needed by its events. Generated metadata
        describes a dependency only on the init bank.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_OneContentBank_NoStreamedFiles_OneDependency')

        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'test_bank1.bnk': {
                'dependencies': [bank_info.init_bank_path],
                'events': ['test_event_1_bank1_embedded_target']
            }
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))

    def test_OneContentBank_StreamedFiles_MultipleDependencies(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Wwise metadata describes one content bank that references streamed media files needed by its events. Generated
        metadata describes dependencies on the init bank and wems named by the IDs of referenced streamed media.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_OneContentBank_StreamedFiles_MultipleDependencies')

        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'test_bank1.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    '590205561.wem',
                    '791740036.wem'
                ],
                'events': [
                    'test_event_1_bank1_embedded_target',
                    'test_event_2_bank1_streamed_target'
                ]
            }
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))

    def test_MultipleContentBanks_NoStreamedFiles_OneDependency(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Wwise metadata describes multiple content banks. Each bank contains all media needed by its events. Generated
        metadata describes each bank having a dependency only on the init bank.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_MultipleContentBanks_NoStreamedFiles_OneDependency')

        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'test_bank1.bnk': {
                'dependencies': [bank_info.init_bank_path],
                'events': ['test_event_1_bank1_embedded_target']
            },
            'test_bank2.bnk': {
                'dependencies': [bank_info.init_bank_path],
                'events': ['test_event_3_bank2_embedded_target', 'test_event_4_bank2_streamed_target']
            }
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))

    def test_MultipleContentBanks_Bank1StreamedFiles(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Wwise metadata describes multiple content banks. Bank 1 references streamed media files needed by its events,
        while bank 2 contains all media need by its events.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_MultipleContentBanks_Bank1StreamedFiles')

        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'test_bank1.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    '590205561.wem'
                ],
                'events': ['test_event_1_bank1_embedded_target', 'test_event_2_bank1_streamed_target']
            },
            'test_bank2.bnk': {
                'dependencies': [bank_info.init_bank_path],
                'events': ['test_event_3_bank2_embedded_target', 'test_event_4_bank2_streamed_target']
            }
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))

    def test_MultipleContentBanks_SplitBanks_OnlyBankDependenices(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Wwise metadata describes multiple content banks. Bank 3 events require media that is contained in bank 4.
        Generated metadata describes each bank having a dependency on the init bank, while bank 3 has an additional
        dependency on bank 4.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_MultipleContentBanks_SplitBanks_OnlyBankDependenices')

        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'test_bank3.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    'test_bank4.bnk'
                ],
                'events': ['test_event_5_bank3_embedded_target_bank4']
            },
            'test_bank4.bnk': {'dependencies': [bank_info.init_bank_path], 'events': []}
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))

    def test_MultipleContentBanks_ReferencedEvent_MediaEmbeddedInBank(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Wwise metadata describes multiple content banks. Bank 1 contains all media required by its events, while bank
        5 contains a reference to an event in bank 1, but no media for that event. Generated metadata describes both
        banks having a dependency on the init bank, while bank 5 has an additional dependency on bank 1.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_MultipleContentBanks_ReferencedEvent_MediaEmbeddedInBank')

        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'test_bank1.bnk': {
                'dependencies': [bank_info.init_bank_path],
                'events': ['test_event_1_bank1_embedded_target']
            },
            'test_bank5.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    'test_bank1.bnk'
                ],
                'events': ['test_event_1_bank1_embedded_target', 'test_event_7_bank5_referenced_event_bank1_embedded']
            }
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))

    def test_MultipleContentBanks_ReferencedEvent_MediaStreamed(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Wwise metadata describes multiple content banks. Bank 1 references streamed media files needed by its events,
        while bank 5 contains a reference to an event in bank 1. This causes bank 5 to also describe a reference to
        the streamed media file referenced by the event from bank 1. Generated metadata describes both banks having
        dependencies on the init bank, as well as the wem named by the ID of referenced streamed media.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_MultipleContentBanks_ReferencedEvent_MediaStreamed')

        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'test_bank1.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    '590205561.wem'
                ],
                'events': ['test_event_2_bank1_streamed_target']
            },
            'test_bank5.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    '590205561.wem'
                ],
                'events': ['test_event_2_bank1_streamed_target', 'test_event_8_bank5_referenced_event_bank1_streamed']
            }
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))

    def test_MultipleContentBanks_ReferencedEvent_MixedSources(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Wwise metadata describes multiple content banks. Bank 1 references a streamed media files needed by one of its
        events, and contains all media needed for its other events, while bank 5 contains a reference to two events
        in bank 1: one that requires streamed media, and one that requires media embedded in bank 1. Generated
        metadata describes both banks having dependencies on the init bank and the wem named by the ID of referenced
        streamed media, while bank 5 has an additional dependency on bank 1.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_MultipleContentBanks_ReferencedEvent_MixedSources')

        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'test_bank1.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    '590205561.wem'
                ],
                'events': ['test_event_1_bank1_embedded_target', 'test_event_2_bank1_streamed_target']
            },
            'test_bank5.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    'test_bank1.bnk',
                    '590205561.wem'
                ],
                'events': [
                    'test_event_1_bank1_embedded_target',
                    'test_event_2_bank1_streamed_target',
                    'test_event_7_bank5_referenced_event_bank1_embedded',
                    'test_event_8_bank5_referenced_event_bank1_streamed'
                ]
            }
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))

    def test_MultipleContentBanks_VaryingDependencies_MixedSources(self, workspace, soundbank_metadata_generator_setup_fixture):
        """
        Wwise metadata describes multiple content banks that have varying dependencies on each other, and dependencies
        on streamed media files.

        Test Steps:
        1. Setup testing environment
        2. Get current bank info
        3. Build expected dependencies
        4. Call success case test
        """

        test_assets_folder = os.path.join(soundbank_metadata_generator_setup_fixture['tests_dir'], 'assets',
                                          'test_MultipleContentBanks_VaryingDependencies_MixedSources')

        bank_info = get_bank_info(workspace)
        expected_dependencies = {
            'test_bank1.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    '590205561.wem'
                ],
                'events': ['test_event_1_bank1_embedded_target', 'test_event_2_bank1_streamed_target']
            },
            'test_bank2.bnk': {
                'dependencies': [bank_info.init_bank_path],
                'events': ['test_event_3_bank2_embedded_target', 'test_event_4_bank2_streamed_target']
            },
            'test_bank3.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    '791740036.wem',
                    'test_bank4.bnk'
                ],
                'events': ['test_event_5_bank3_embedded_target_bank4', 'test_event_6_bank3_streamed_target_bank4']
            },
            'test_bank4.bnk': {'dependencies': [bank_info.init_bank_path], 'events': []},
            'test_bank5.bnk': {
                'dependencies': [
                    bank_info.init_bank_path,
                    'test_bank1.bnk',
                    '590205561.wem'
                ],
                'events': [
                    'test_event_1_bank1_embedded_target',
                    'test_event_2_bank1_streamed_target',
                    'test_event_7_bank5_referenced_event_bank1_embedded',
                    'test_event_8_bank5_referenced_event_bank1_streamed'
                ]
            },
            'test_bank6.bnk': {
                'dependencies': [bank_info.init_bank_path],
                'events': [
                    'test_event_3_bank2_embedded_target',
                    'test_event_4_bank2_streamed_target',
                    'test_event_9_bank6_referenced_event_bank2_embedded',
                    'test_event_10_bank6_referenced_event_bank2_streamed'
                ]
            },
        }
        success_case_test(test_assets_folder, expected_dependencies, get_bank_info(workspace))
