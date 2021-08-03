"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from argparse import ArgumentParser
from operator import attrgetter
from xml.dom import minidom
from xml.etree import ElementTree
import os
import sys


__version__ = '0.1.0'
__copyright__ = 'Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution..'


all_events = set()
all_parameters = set()
all_preloads = set()
all_auxbusses = set()
all_switches = dict()
all_states = dict()

SWITCH_TYPE = 0
STATE_TYPE = 1


def get_options():
    parser = ArgumentParser(description='Process soundbank metadata and generate ATL xml data for Lumberyard.\n'
                            'Path arguments can be relative to the working directory of this script.')
    parser.add_argument('--bankPath', required=True,
                        help='Root path where to look for banks, e.g. <Project>\\Sounds\\wwise')
    parser.add_argument('--atlPath', required=True,
                        help='Output path for the ATL controls, e.g. <Project>\\libs\\gameaudio\\wwise')
    parser.add_argument('--atlName', required=True,
                        help='Name of the output xml file, e.g. generated_controls.xml')
    parser.add_argument('--autoLoad', required=False, action='store_true',
                        help='Whether "AutoLoad" setting should be applied to new SoundBanks.  Consecutive runs of '
                             'this script on the same output file will preserve any "AutoLoad" settings that were '
                             'there before.  This flag will apply "AutoLoad" to any SoundBanks that appear to be new.')
    parser.add_argument('--printDebug', required=False, action='store_true',
                        help='Print out the parsed Wwise data to stdout')
    options = parser.parse_args()

    if os.path.exists(options.atlPath):
        print('ATL File Path: {}'.format(os.path.join(options.atlPath, options.atlName)))
    else:
        sys.exit('--atlPath {}: Path does not exist!'.format(options.atlPath))

    if os.path.exists(options.bankPath):
        print('Bank Root Path: {}'.format(options.bankPath))
    else:
        sys.exit('--bankPath {}: Path does not exist!'.format(options.bankPath))

    print()
    return options


class ATLType:
    def __init__(self, name, path):
        def fix_path(_path):
            """
            Modifies the format of a path that came from a Wwise .txt file
            1) Converts all backslashes to forwardslashes
            2) Cuts off the last part of the path, which is equal to name
            3) Trims the slash from the beginning of the path
            :param _path: Input path string
            :return: Modified path string
            """
            _path = _path.replace('\\', '/')
            # HACK:
            if _path.endswith(name):
                _path = os.path.dirname(_path)

            if len(_path) > 0 and _path[0] == '/':
                return _path[1:]
            else:
                return _path

        self.name = name
        self.path = fix_path(path)
        self.name_attr = 'atl_name'
        self.path_attr = 'path'
        self.element = __class__.__name__

    def __str__(self):
        return '{}/{}'.format(self.path, self.name)

    def __eq__(self, other):
        return self.name == other.name and self.path == other.path

    def __hash__(self):
        return hash((self.name, self.path))

    def attach_xml(self, node):
        return ElementTree.SubElement(node,
                                      self.element,
                                      attrib={self.name_attr: self.name,
                                              self.path_attr: self.path})


class ATLEnvironment(ATLType):
    def __init__(self, name, path):
        super().__init__(name, path)
        self.element = __class__.__name__
        self.auxbus = WwiseAuxBus(self.name)

    def attach_xml(self, node):
        atl_env = super().attach_xml(node)
        self.auxbus.attach_xml(atl_env)


class ATLTrigger(ATLType):
    def __init__(self, name, path):
        super().__init__(name, path)
        self.element = __class__.__name__
        self.event = WwiseEvent(self.name)

    def attach_xml(self, node):
        atl_trigger = super().attach_xml(node)
        self.event.attach_xml(atl_trigger)


class ATLRtpc(ATLType):
    def __init__(self, name, path):
        super().__init__(name, path)
        self.element = __class__.__name__
        self.param = WwiseRtpc(self.name)

    def attach_xml(self, node):
        atl_rtpc = super().attach_xml(node)
        self.param.attach_xml(atl_rtpc)


class ATLSwitch(ATLType):
    def __init__(self, name, path):
        super().__init__(name, path)
        self.element = __class__.__name__
        self.states = dict()

    def attach_xml(self, node):
        atl_switch = super().attach_xml(node)
        for switch_state in sorted(self.states.values(), key=attrgetter('name')):
            switch_state.attach_xml(atl_switch)


class ATLSwitchState(ATLType):
    def __init__(self, name, switch_name, wwise_type):
        super().__init__(name, switch_name)
        # Here the 'path' will denote the parent ATLSwitch name.
        # The ATLSwitch has it's own 'path' member so that data is still reachable.
        self.wwise_type = wwise_type
        self.element = __class__.__name__
        if self.wwise_type == SWITCH_TYPE:
            self.state = WwiseSwitch(self.name, switch_name)
        elif self.wwise_type == STATE_TYPE:
            self.state = WwiseState(self.name, switch_name)

    def __eq__(self, other):
        return self.name == other.name and self.path == other.path and self.wwise_type == other.wwise_type

    def attach_xml(self, node):
        atl_switch_state = ElementTree.SubElement(node,
                                                  self.element,
                                                  attrib={self.name_attr: self.name})
        if self.state:
            self.state.attach_xml(atl_switch_state)


class ATLPreloadRequest(ATLType):
    def __init__(self, name, path, is_loc, is_autoload):
        super().__init__(name, path)
        self.element = __class__.__name__
        self.autoload_attr = 'atl_type'
        self.autoload = is_autoload
        self.param = WwiseFile(self.name + '.bnk', is_loc)

    def attach_xml(self, node):
        atl_preload = super().attach_xml(node)
        if self.autoload:
            atl_preload.set(self.autoload_attr, 'AutoLoad')
        self.param.attach_xml(atl_preload)


class WwiseType:
    def __init__(self, name):
        self.name = name
        self.name_attr = 'wwise_name'
        self.element = __class__.__name__

    def attach_xml(self, node):
        return ElementTree.SubElement(node,
                                      self.element,
                                      attrib={self.name_attr: self.name})


class WwiseAuxBus(WwiseType):
    def __init__(self, name):
        super().__init__(name)
        self.element = __class__.__name__


class WwiseEvent(WwiseType):
    def __init__(self, name):
        super().__init__(name)
        self.element = __class__.__name__


class WwiseRtpc(WwiseType):
    def __init__(self, name):
        super().__init__(name)
        self.element = __class__.__name__


class WwiseValue(WwiseType):
    def __init__(self, name):
        super().__init__(name)
        self.element = __class__.__name__


class WwiseSwitch(WwiseType):
    def __init__(self, name, path):
        super().__init__(path)      # This inits with 'path', the WwiseValue uses the 'name'
        self.element = __class__.__name__
        self.value = WwiseValue(name)

    def attach_xml(self, node):
        switch = super().attach_xml(node)
        self.value.attach_xml(switch)


class WwiseState(WwiseType):
    def __init__(self, name, path):
        super().__init__(path)      # This inits with 'path', the WwiseValue uses the 'name'
        self.element = __class__.__name__
        self.value = WwiseValue(name)

    def attach_xml(self, node):
        state = super().attach_xml(node)
        self.value.attach_xml(state)


class WwiseFile(WwiseType):
    def __init__(self, name, is_loc):
        super().__init__(name)
        self.element = __class__.__name__
        self.localized = is_loc
        self.loc_attr = 'wwise_localized'

    def attach_xml(self, node):
        wwise_file_node = super().attach_xml(node)
        if self.localized:
            wwise_file_node.set(self.loc_attr, 'true')


def get_load_types_of_existing_preloads(atl_file):
    """
    Pre-parses the ATL controls file if it exists and returns lists
     of preloads that were marked as auto-load and ones that weren't.
    :param atl_file: ATL Controls xml file
    :return: Lists of preload names that are manual-load and auto-load.
    """
    manual = []
    auto = []
    if os.path.exists(atl_file):
        try:
            atl_doc = ElementTree.parse(atl_file)
        except ElementTree.ParseError as e:
            sys.exit('Error parsing {} at line {}, column {}'.format(atl_file, e.position[0], e.position[1]))

        root = atl_doc.getroot()
        audio_preloads_node = root.find('./AudioPreloads')
        if audio_preloads_node is not None:
            autoloads = audio_preloads_node.findall('./ATLPreloadRequest')
            for node in autoloads:
                preload_name = node.get('atl_name')
                if node.get('atl_type') == 'AutoLoad':
                    auto.append(preload_name)
                else:
                    manual.append(preload_name)

    return manual, auto


def scan_for_localization_folders(folder):
    """
    Full scan of root folder to determine what paths are 'localized' bank paths.
    If a folder contains an 'init.bnk' file, then that's considered a main bank folder.
    Any subfolders of a main bank folder, with the exception of one named 'external',
    is considered localized.
    :param folder: Starting root folder for the scan
    :return: List of localized bank paths
    """
    loc_bank_paths = []
    for root, dirs, files in os.walk(folder):
        if 'init.bnk' in map(str.lower, files):
            path_gen = (d for d in dirs if d.lower() != 'external')
            for d in path_gen:
                loc_bank_paths.append(os.path.join(root, d))

    return loc_bank_paths


def get_bnktxt_files(folder):
    """
    Recursive generator that yields files in a folder that...
    1) Have a .txt extension
    2) Have a matching .bnk file next to it
    :param folder: Starting folder for the search
    :return: Yields a filepath that conforms with conditions 1 and 2 above
    """
    for f in os.listdir(folder):
        filepath = os.path.join(folder, f)
        if os.path.isdir(filepath):
            for file in get_bnktxt_files(filepath):
                yield file
        elif os.path.isfile(filepath) and filepath.endswith('.txt'):
            bnk_file = filepath.replace('.txt', '.bnk')
            if os.path.exists(bnk_file):
                yield filepath


def process_simple_types(file, func):
    line = file.readline().rstrip('\n')
    while line:
        columns = line.split('\t')
        assert len(columns) == 7, 'Column count should be 7 after tokenizing'
        func(columns[2], columns[5])

        line = file.readline().rstrip('\n')


def process_switches(file, group_type):
    global all_switches
    global all_states

    line = file.readline().rstrip('\n')
    while line:
        columns = line.split('\t')
        assert len(columns) == 7, 'Column count should be 7 after tokenizing'
        group = columns[2]
        path = columns[5]
        if group_type == SWITCH_TYPE:
            if group not in all_switches:
                all_switches[group] = ATLSwitch(group, path)
        elif group_type == STATE_TYPE:
            if group not in all_states:
                all_states[group] = ATLSwitch(group, path)

        line = file.readline().rstrip('\n')


def process_switch_states(file, group_type):
    global all_switches
    global all_states

    line = file.readline().rstrip('\n')
    while line:
        columns = line.split('\t')
        assert len(columns) == 7, 'Column count should be 7 after tokenizing'
        group = columns[3]
        child = columns[2]
        if group_type == SWITCH_TYPE:
            if group in all_switches:
                if child not in all_switches[group].states:
                    all_switches[group].states[child] = ATLSwitchState(child, group, group_type)
        elif group_type == STATE_TYPE:
            if group in all_states:
                if child not in all_states[group].states:
                    all_states[group].states[child] = ATLSwitchState(child, group, group_type)

        line = file.readline().rstrip('\n')


def parse_bnktxt_file(file):
    print('Processing {}'.format(file))
    global all_auxbusses
    global all_events
    global all_parameters

    with open(file) as f:
        line = f.readline()
        # readline() returns empty string when reaching end of file, so strip the newline inside this while loop.
        while line:
            line = line.rstrip('\n')
            columns = line.split('\t')
            if columns[0] == 'Auxiliary Bus':
                process_simple_types(f, lambda name, path: all_auxbusses.add(ATLEnvironment(name, path)))
            elif columns[0] == 'Event':
                process_simple_types(f, lambda name, path: all_events.add(ATLTrigger(name, path)))
            elif columns[0] == 'Game Parameter':
                process_simple_types(f, lambda name, path: all_parameters.add(ATLRtpc(name, path)))
            elif columns[0] == 'State Group':
                process_switches(f, STATE_TYPE)
            elif columns[0] == 'Switch Group':
                process_switches(f, SWITCH_TYPE)
            elif columns[0] == 'State':
                process_switch_states(f, STATE_TYPE)
            elif columns[0] == 'Switch':
                process_switch_states(f, SWITCH_TYPE)

            line = f.readline()


def write_xml_output(filepath):
    filename = os.path.basename(filepath)
    filename = os.path.splitext(filename)[0]
    root = ElementTree.Element('ATLConfig', atl_name=filename)
    sort_key = attrgetter('name', 'path')

    envs = ElementTree.SubElement(root, 'AudioEnvironments')
    for auxbus in sorted(all_auxbusses, key=sort_key):
        auxbus.attach_xml(envs)

    rtpcs = ElementTree.SubElement(root, 'AudioRTPCs')
    for rtpc in sorted(all_parameters, key=sort_key):
        rtpc.attach_xml(rtpcs)

    switches = ElementTree.SubElement(root, 'AudioSwitches')
    for state in sorted(all_states.values(), key=sort_key):
        state.attach_xml(switches)

    for switch in sorted(all_switches.values(), key=sort_key):
        switch.attach_xml(switches)

    trigs = ElementTree.SubElement(root, 'AudioTriggers')
    for trigger in sorted(all_events, key=sort_key):
        trigger.attach_xml(trigs)

    preloads = ElementTree.SubElement(root, 'AudioPreloads')
    for preload in sorted(all_preloads, key=sort_key):
        preload.attach_xml(preloads)

    # Prettify the XML document before writing
    # Unfortunately xml.etree does not contain easy pretty printing so transfer the doc to minidom.
    raw_string = ElementTree.tostring(root)
    xml_dom = minidom.parseString(raw_string)
    pretty_string = xml_dom.toprettyxml(indent='  ', encoding='utf-8').decode('UTF-8')
    try:
        with open(filepath, mode="w") as f:
            f.write(pretty_string)
    except OSError as e:
        sys.exit('Error: {}'.format(e.strerror))


def run_wwise_atl_gen_tool():
    print('Wwise ATL Generator Tool v{}'.format(__version__))
    print(__copyright__)
    print()

    options = get_options()

    output_file = os.path.join(options.atlPath, options.atlName)

    # Pre-parse the output file (if it exists) and split out the preloads into 'manual' and 'auto' load types...
    manual_preloads, auto_preloads = get_load_types_of_existing_preloads(output_file)

    localized_bank_paths = scan_for_localization_folders(options.bankPath)

    # Parse Wwise Bank TXT files for info...
    for f in get_bnktxt_files(options.bankPath):
        path, file = os.path.split(f)
        preload = os.path.splitext(file)[0].lower()
        is_localized = (path in localized_bank_paths)
        is_autoload = (preload in auto_preloads) or (options.autoLoad and preload not in manual_preloads)
        # Skip the ATL Preload for init.bnk, it's automatically loaded at runtime so it doesn't need to be written...
        if preload != 'init':
            all_preloads.add(ATLPreloadRequest(preload, 'SoundBanks', is_localized, is_autoload))
        parse_bnktxt_file(f)

    # Debug printing...
    if options.printDebug:
        sort_key = attrgetter('name', 'path')
        print('\nEvents:')
        for event in sorted(all_events, key=sort_key):
            print(event)

        print('\nParameters:')
        for parameter in sorted(all_parameters, key=sort_key):
            print(parameter)

        print('\nAuxBusses:')
        for auxbus in sorted(all_auxbusses, key=sort_key):
            print(auxbus)

        print('\nSwitches:')
        for switch in sorted(all_switches.values(), key=sort_key):
            for state in sorted(switch.states.values(), key=sort_key):
                print(state)

        print('\nStates:')
        for switch in sorted(all_states.values(), key=sort_key):
            for state in sorted(switch.states.values(), key=sort_key):
                print(state)

        print('\nSoundBanks:')
        for bank in sorted(all_preloads, key=sort_key):
            print(bank)

    # Output to XML...
    write_xml_output(output_file)
    print('Done!')


if __name__ == '__main__':
    run_wwise_atl_gen_tool()
