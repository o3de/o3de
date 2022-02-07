"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import argparse
import os
import tempfile
import json
import xml.etree.ElementTree as ET
import subprocess
from threading import Thread
import time
import re

def main():
    args = parse_args()
    (ui_copy_file_path, inserted_variables, inserted_qss) = make_copy(args)
    monitor_thread = start_monitoring_for_changes(ui_copy_file_path, args, inserted_variables, inserted_qss)
    run_designer(args.designer_file_path, ui_copy_file_path)
    stop_monitor_for_changes(monitor_thread)
    delete_copy(ui_copy_file_path)

def parse_args():

    script_directory_path = os.path.dirname(os.path.realpath(__file__))
    dev_directory_path = os.path.join(script_directory_path, '../..')
    editor_styles_directory_path = os.path.join(dev_directory_path, 'Editor/Styles')
    
    third_party_directory_path = os.path.join(dev_directory_path, '../3rdParty')
    if not os.path.isdir(third_party_directory_path):
        third_party_directory_path = os.path.join(dev_directory_path, '../../3rdParty')
        if not os.path.isdir(third_party_directory_path):
            third_party_directory_path = os.path.join(dev_directory_path, '../../../3rdParty')
            if not os.path.isdir(third_party_directory_path):
                raise RuntimeError('Could not find 3rdParty directory.')

    default_qss_file_path = os.path.normpath(os.path.join(editor_styles_directory_path, 'EditorStylesheet.qss'))
    default_variables_file_path = os.path.normpath(os.path.join(editor_styles_directory_path, 'EditorStylesheetVariables_Dark.json'))
    default_designer_file_path = os.path.normpath(os.path.join(third_party_directory_path, 'Qt/5.3.2/msvc2013_64/bin/designer.exe'))

    parser = argparse.ArgumentParser(
        prog='syleui', 
        description='Inserts styles from an qss into an ui file for use in QT Designer.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
This program creates a temporary copy of an ui file and starts QT Designer 
on that copy.

The ui file provided to QT Designer will have the contents of the qss file 
inserted into the root widget's stylesheet (right click on the root widget 
and select "Change styleSheet..." to view the stylesheet).

Whenever the ui file is saved in QT Designer, the inserted qss content is
removed and the original ui file is updated.

The qss file may contain variable references of the form @VARIABLE_NAME. 
Values for these variable are taken from a json format file with the following
structure:

    {
        "StylesheetVariables" : {
            "VARIABLE_NAME_1" : "VARIABLE_VALUE_1",
            "VARIABLE_NAME_2" : "VARIABLE_VALUE_2",
            ...
            "VARIABLE_NAME_N" : "VARIABLE_VALUE_N"
        }
    }
    
Variable references in the qss file will be replaced with a comment containing
the variable name and the variable value (not in a comment). For example, @Foo
is replaced with /*@Foo*/VALUE/*@*/. The trailing /*@*/ marks the end of the
inserted value (it is used when restoring the variable names as described 
below).

The variables file content is saved as a comment in the stylesheet. For 
example:

    /*** START INSERTED VARIABLES    
    {
        "StylesheetVariables" : {
            "WindowBackgroundColor" : "#393a3c",
            "PanelBackgroundColor" : "#303030",
            ...
        }
    }
    END INSERTED VARIABLES ***/
    
You can edit this content while in QT Designer. Changes you make will be saved
back to the original variables file.

The qss file content is saved between two comments in the stylesheet. For 
example:

    /*** START INSERTED STYLES ***/
    Foo { color: #000000 }
    /*** END INSERTED STYLES ***/

You can edit this content while in QT Designer. Changtes you make will be 
saved back to the original qss file, with the variable replacements changed 
back to variable references. YOU MUST CHANNGE VARIABLE VALUES IN THE VARIABLES
SECTION FOR THOSE CHANGES TO BE SAVED. You can change the name of a variable 
in comment to cause the qss to reference a different variable. If you add a 
new variable reference, be sure to include the /*@VARIABLE_NAME*/ and /*@*/ 
comments before and after the variable's temporary value, respectively.

Be sure not to modify the START and END comments in the stylesheet or the 
changes you make may not be saved property.
    ''')
    
    parser.add_argument('ui_file_path', metavar='UI_FILE_PATH', help='path and name of ui file')
    parser.add_argument('--qss', default=default_qss_file_path, dest='qss_file_path', help='Path and name of qss file. Default is: ' + default_qss_file_path)
    parser.add_argument('--variables', default=default_variables_file_path, dest='variables_file_path', help='Path and name of json file containing variable definitions. Default is: ' + default_variables_file_path)
    parser.add_argument('--designer', default=default_designer_file_path, dest='designer_file_path', help='Path and name of QT Designer executable file. Default is: ' + default_designer_file_path)

    args = parser.parse_args()
    
    return args

def make_copy(args):
    qss_content = read_qss_file(args.qss_file_path)
    variables_content = read_variables_file(args.variables_file_path)
    qss_content = replace_variables(qss_content, variables_content)
    ui_content = read_ui_file(args.ui_file_path)
    (inserted_variables, inserted_qss) = insert_styles_into_ui(qss_content, variables_content, ui_content)
    ui_copy_file_path = write_ui_copy(ui_content)
    print 'copied', args.ui_file_path, 'to', ui_copy_file_path, 'with styles from', args.qss_file_path
    return (ui_copy_file_path, inserted_variables, inserted_qss)

def read_qss_file(qss_file_path):
    with open(qss_file_path, 'r') as qss_file:
        qss_content = qss_file.read()
        #print 'qss_content', qss_content
        return qss_content

def read_variables_file(variables_file_path):
    with open(variables_file_path, 'r') as variables_file:
        variables_content = json.load(variables_file)
        #print 'variables_content', variables_content
        return variables_content
        
def replace_variables(qss_content, variables_content):
    for name, value in variables_content.get('StylesheetVariables', {}).iteritems():
        qss_content = qss_content.replace('@' + name, '/*@' + name + '*/' + value + '/*@*/')
    #print 'replace_variables', qss_content
    return qss_content
    
def read_ui_file(ui_file_path):
    ui_content = ET.parse(ui_file_path)
    #print 'ui_content', ui_content
    return ui_content
    
def insert_styles_into_ui(qss_content, variables_content, ui_content):
    property_value_element = ui_content.find("./widget/property[@name='styleSheet']/string")
    if property_value_element is None:
        property_element = ET.SubElement(ui_content.find("./widget"), 'property')
        property_element.set('name', 'styleSheet')
        property_value_element = ET.SubElement(property_element, 'string')
        property_value_element.set('notr', 'true')
        property_value_element.text = ''
    value = property_value_element.text
    (value, removed_variables) = remove_variables_from_property_value(value)
    (value, removed_qss) = remove_qss_from_property_value(value)
    (value, inserted_variables) = insert_variables_into_property_value(value, variables_content)
    (value, inserted_qss) = insert_qss_into_property_value(value, qss_content)
    property_value_element.text = value
    return (inserted_variables, inserted_qss)
    
START_VARIABLES_MARKER = '\n/*** START INSERTED VARIABLES\n'
END_VARIABLES_MARKER = '\nEND INSERTED VARIABLES ***/\n'

def remove_variables_from_property_value(property_value):
    removed_variables = None
    start_index = property_value.find(START_VARIABLES_MARKER)
    if start_index != -1:
        end_index = property_value.find(END_VARIABLES_MARKER, start_index + len(START_VARIABLES_MARKER))
        if end_index != -1:
            removed_variables = property_value[start_index + len(START_VARIABLES_MARKER):end_index]
            property_value = property_value[:start_index] + property_value[end_index + len(END_VARIABLES_MARKER):]
            #print 'removed', property_value
    return (property_value, removed_variables)

def insert_variables_into_property_value(property_value, variables_content):    
    inserted_variables = json.dumps(variables_content, sort_keys=True, indent=4)
    property_value = property_value + START_VARIABLES_MARKER + inserted_variables + END_VARIABLES_MARKER
    #print 'inserted', property_value
    return (property_value, inserted_variables)
    
START_QSS_MARKER = '\n/*** START INSERTED STYLES ***/\n'
END_QSS_MARKER = '\n/*** END INSERTED STYLES ***/\n'

def remove_qss_from_property_value(property_value):
    removed_qss = None
    start_index = property_value.find(START_QSS_MARKER)
    if start_index != -1:
        end_index = property_value.find(END_QSS_MARKER, start_index + len(START_QSS_MARKER))
        if end_index != -1:
            removed_qss = property_value[start_index + len(START_QSS_MARKER):end_index]
            property_value = property_value[:start_index] + property_value[end_index + len(END_QSS_MARKER):]
            #print 'removed', property_value
    return (property_value, removed_qss)

def insert_qss_into_property_value(property_value, qss_content):    
    property_value = property_value + START_QSS_MARKER + qss_content + END_QSS_MARKER
    #print 'inserted', property_value
    return (property_value, qss_content)
    
def write_ui_copy(ui_content):
    (ui_copy_file, ui_copy_file_path) = tempfile.mkstemp(suffix='.ui', text=True)
    #print 'ui_copy_file_path', ui_copy_file_path
    ui_content.write(os.fdopen(ui_copy_file, 'w'))
    #os.close(ui_copy_file) ElementTree.write must be closing... fails if called
    return ui_copy_file_path

def start_monitoring_for_changes(ui_copy_file_path, args, inserted_variables, inserted_qss):
    print 'monitoring', ui_copy_file_path, 'for changes'
    monitor_thread = Thread(target = monitor_for_changes, args=(ui_copy_file_path, args, inserted_variables, inserted_qss))
    monitor_thread.start()
    return monitor_thread
    
continue_monitoring = True

def monitor_for_changes(ui_copy_file_path, args, inserted_variables, inserted_qss):
    # lets see if simple polling works ok... otherwise maybe use https://pythonhosted.org/watchdog/
    last_mtime = os.path.getmtime(ui_copy_file_path)
    global continue_monitoring
    while continue_monitoring:
        time.sleep(2) # seconds
        current_mtime = os.path.getmtime(ui_copy_file_path)
        if(last_mtime != current_mtime):
            last_mtime = current_mtime
            update_files(ui_copy_file_path, args, inserted_variables, inserted_qss)

def update_files(ui_copy_file_path, args, inserted_variables, inserted_qss):
    ui_copy_content = read_ui_file(ui_copy_file_path)
    (removed_variables, removed_qss) = remove_styles_from_ui(ui_copy_content)
    update_ui(ui_copy_content, args.ui_file_path)
    if inserted_variables != removed_variables:
        update_variables(removed_variables, args.variables_file_path)
    if inserted_qss != removed_qss:
        update_qss(removed_qss, args.qss_file_path)
    
def remove_styles_from_ui(ui_content):
    removed_variables = None
    removed_qss = None
    property_value_element = ui_content.find("./widget/property[@name='styleSheet']/string")
    if property_value_element is not None:
        value = property_value_element.text
        (value, removed_variables) = remove_variables_from_property_value(value)
        (value, removed_qss) = remove_qss_from_property_value(value)
        property_value_element.text = value
    return (removed_variables, removed_qss)
    
def update_ui(ui_content, ui_file_path):
    print 'updating', ui_file_path, 'with changes'
    try:
        ui_content.write(ui_file_path)
    except Exception as e:
        print '\n*** WRITE FAILED', e
        parts = os.path.splitext(ui_file_path)
        temp_path = parts[0] + '_BACKUP' + parts[1]
        print '*** saving to', temp_path, 'instead\n'
        ui_content.write(temp_path)
    
def update_variables(removed_variables, variables_file_path):
    print 'updating', variables_file_path, 'with changes'
    try:
        with open(variables_file_path, "w") as variables_file:
            variables_file.write(removed_variables)
    except Exception as e:
        print '\n*** WRITE FAILED', e
        parts = os.path.splitext(variables_file_path)
        temp_path = parts[0] + '_BACKUP' + parts[1]
        print '*** saving to', temp_path, 'instead\n'
        with open(temp_path, "w") as variables_file:
            variables_file.write(removed_variables)
    
def update_qss(removed_qss, qss_file_path):
    print 'updating', qss_file_path, 'with changes'
    removed_qss = re.sub(r'/\*@(\w+)\*/.*/\*@\*/', '@\g<1>', removed_qss)
    try:
        with open(qss_file_path, "w") as qss_file:
            qss_file.write(removed_qss)
    except Exception as e:
        print '\n*** WRITE FAILED', e
        parts = os.path.splitext(qss_file_path)
        temp_path = parts[0] + '_BACKUP' + parts[1]
        print '*** saving to', temp_path, 'instead\n'
        with open(temp_path, "w") as qss_file:
            qss_file.write(removed_qss)
    
def stop_monitor_for_changes(monitor_thread):
    print 'stopping change monitor'
    global continue_monitoring
    continue_monitoring = False
    monitor_thread.join()
    
def run_designer(designer_file_path, ui_copy_file_path):
    print 'starting designer with', ui_copy_file_path
    subprocess.call([designer_file_path, ui_copy_file_path])
    print 'designer exited'
    
def delete_copy(ui_copy_file_path):
    print 'deleting', ui_copy_file_path
    os.remove(ui_copy_file_path)
    
main()

