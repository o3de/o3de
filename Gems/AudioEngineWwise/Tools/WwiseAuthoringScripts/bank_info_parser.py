"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from argparse import ArgumentParser
import glob
import json
import os
import sys
from xml.etree import ElementTree


__version__ = '0.1.0'
__copyright__ = 'Copyright (c) Contributors to the Open 3D Engine Project.\nFor complete copyright and license terms please see the LICENSE at the root of this distribution.'

metadata_file_extension = '.bankdeps'
metadata_version = '1.0'
init_bank_path = 'Init.bnk'
no_info_xml_error = 'SoundBanksInfo.xml does not exist, and there is more than ' \
                    'one bank (aside from Init.bnk), so complete dependency info of banks cannot be ' \
                    'determined. Please ensure "Project > Project Settings > SoundBanks > Generate ' \
                    'Metadata File" is enabled in your Wwise project to generate complete ' \
                    'dependencies. Limited dependency info will be generated.'
no_init_bank_error = 'There is no Init.bnk that exists at path {}. Init.bnk is ' \
                     'necessary for other soundbanks to work properly. Please regenerate soundbanks ' \
                     'from the Wwise project.'


class Bank:
    def __init__(self, name):
        self.path = name
        self.embedded_media = []    # set of short ids for embedded media
        self.streamed_media = []    # set of short ids for media being streamed
        self.excluded_media = []    # set of short ids for media that is embedded in other banks
        self.included_events = []   # set of names of events that are included in the bank.
        self.metadata_object = {}   # object that will be serialized to JSON for this bank
        self.metadata_path = ""     # path to serialize the metadata object to


def parse_args():
    parser = ArgumentParser(description='Generate metadata files for all banks referenced in a SoundBankInfo.xml')
    parser.add_argument('soundbank_info_path', help='Path to a SoundBankInfo.xml to parse')
    parser.add_argument('output_path', help='Path that the banks have been output to, where these metadata files will live as well.')
    options = parser.parse_args()

    if not os.path.exists(options.output_path):
        sys.exit('Output path {} does not exist'.format(options.output_path))

    return options


def parse_bank_info_xml(root):
    sound_banks_element = root.find('SoundBanks')
    banks = []
    for bank_element in sound_banks_element.iter('SoundBank'):
        bank_path = bank_element.find('Path').text
        # If this is the init bank, then skip as it doesn't need an entry, as the init bank does not need metadata
        if bank_path == init_bank_path:
            continue

        bank = Bank(bank_path)
        for embedded_file in bank_element.findall('IncludedMemoryFiles/File'):
            bank.embedded_media.append(embedded_file.get('Id'))
        for streamed_file in bank_element.findall('ReferencedStreamedFiles/File'):
            bank.streamed_media.append(streamed_file.get('Id'))
        for excluded_file in bank_element.findall('ExcludedMemoryFiles/File'):
            bank.excluded_media.append(excluded_file.get('Id'))
        for event_name in bank_element.findall('IncludedEvents/Event'):
            bank.included_events.append(event_name.get('Name'))

            for embedded_file in event_name.findall('IncludedMemoryFiles/File'):
                bank.embedded_media.append(embedded_file.get('Id'))

            for streamed_file in event_name.findall('ReferencedStreamedFiles/File'):
                bank.streamed_media.append(streamed_file.get('Id'))

            for excluded_file in event_name.findall('ExcludedMemoryFiles/File'):
                bank.excluded_media.append(excluded_file.get('Id'))

        banks.append(bank)
    return banks


def make_banks_from_file_paths(bank_paths):
    return [Bank(bank) for bank in bank_paths]


def build_media_to_bank_dictionary(banks):
    media_to_banks = {}
    for bank in banks:
        for short_id in bank.embedded_media:
            media_to_banks[short_id] = bank

    return media_to_banks


def serialize_metadata_list(banks):
    for bank in banks:
        # generate metadata json file
        with open(bank.metadata_path, 'w') as metadata_file:
            json.dump(bank.metadata_object, metadata_file, indent=4)


def generate_default_metadata_path_and_object(bank_path, output_path):
    metadata_file_path = os.path.join(output_path, bank_path)
    metadata_file_path = os.path.splitext(metadata_file_path)[0] + metadata_file_extension

    metadata = dict()
    metadata['version'] = metadata_version
    metadata['bankName'] = bank_path
    return metadata_file_path, metadata


def generate_bank_metadata(banks, media_dictionary, output_path):
    for bank in banks:
        # Determine path for metadata file.
        metadata_file_path, metadata = generate_default_metadata_path_and_object(bank.path, output_path)

        # Determine paths for each of the streamed files.
        dependencies = set()
        for short_id in bank.streamed_media:
            dependencies.add(str.format("{}.wem", short_id))

        # Any media that has been excluded from this bank and embedded in another, add that bank as a dependency
        for short_id in bank.excluded_media:
            dependencies.add(media_dictionary[short_id].path)

        # Force a dependency on the init bank.
        dependencies.add(init_bank_path)
        metadata['dependencies'] = list(dependencies)

        metadata['includedEvents'] = bank.included_events

        # Push the data generated bank into the bank to be used later (by tests or by serializer).
        bank.metadata_object = metadata
        bank.metadata_path = metadata_file_path

    return banks


def register_wems_as_streamed_file_dependencies(bank, output_path):
    for wem_file in glob.glob1(output_path, '*.wem'):
        bank.streamed_media.append(os.path.splitext(wem_file)[0])


def generate_metadata(soundbank_info_path, output_path):
    bank_paths = glob.glob1(output_path, '*.bnk')
    soundbank_xml_exists = os.path.exists(soundbank_info_path)
    error_code = 0
    banks_with_metadata = dict()
    init_bank_exists = init_bank_path in bank_paths

    if init_bank_exists:
        bank_paths.remove(init_bank_path)
    else:
        print(str.format(no_init_bank_error, output_path))
        error_code = max(error_code, 1)

    # Check to see if the soundbankinfo file exists. If it doesn't then there are no streamed files.
    if soundbank_xml_exists:
        root = ElementTree.parse(soundbank_info_path).getroot()
        banks = parse_bank_info_xml(root)
        media_dictionary = build_media_to_bank_dictionary(banks)
        banks_with_metadata = generate_bank_metadata(banks, media_dictionary, output_path)

    # If there are more than two content banks in the directory, then there is
    #   no way to generate dependencies properly without the XML.
    #   Just generate the dependency on the init bank and generate a warning.
    elif len(bank_paths) > 1:
        print(no_info_xml_error)
        error_code = max(error_code, 1)
        banks = make_banks_from_file_paths(bank_paths)
        media_dictionary = build_media_to_bank_dictionary(banks)
        banks_with_metadata = generate_bank_metadata(banks, media_dictionary, output_path)

    # There is one content bank, so this bank depends on the init bank and all wem files in the output path
    elif len(bank_paths) == 1:
        banks = make_banks_from_file_paths(bank_paths)
        # populate bank streamed file dependencies with all the wems in the folder.
        register_wems_as_streamed_file_dependencies(banks[0], output_path)
        media_dictionary = build_media_to_bank_dictionary(banks)
        banks_with_metadata = generate_bank_metadata(banks, media_dictionary, output_path)

    # There were no banks in the directory, and no metadata xml, then we can't generate any dependencies
    elif not init_bank_exists:
        print(str.format(no_init_bank_error, output_path))
        error_code = max(error_code, 2)

    return banks_with_metadata, error_code


def main():
    print('Wwise Bank Info Parser v{}'.format(__version__))
    print(__copyright__)
    print()

    args = parse_args()
    banks, error_code = generate_metadata(args.soundbank_info_path, args.output_path)

    if banks is not None:
        serialize_metadata_list(banks)

    return error_code


if __name__ == '__main__':
    main()
