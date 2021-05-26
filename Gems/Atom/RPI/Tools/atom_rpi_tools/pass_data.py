"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

OS and devices are detected and set as constants when ly_test_tools.__init__() completes.
"""

import sys, os
import json
import shutil

class PassTemplate:
    # This class provide necessary functions for insert pass requests and update connections
    # which are common functions required for adding features. 
    # It doesn't include the remove/delete furnctions since that's not common case for merging render pipeline
    def __init__(self, filePath: str):
        self.initialized = False
        self.file_path: str = filePath
        #load the json file
        json_data = open(filePath, "r")
        self.file_data = json.load(json_data)
        
        if self.file_data['ClassName']!='PassAsset':
            # initialize failed if this is not a PassAsset data
            return
        self.passRequests = self.file_data['ClassData']['PassTemplate']['PassRequests']
        self.slots = self.file_data['ClassData']['PassTemplate']['Slots']
        self.initialized = True
        print('PassTemplate is loaded from ', filePath)

    def find_pass(self, passName):
        # return pass's index in PassRequests if a PassRequest with input passName exists
        index = 0
        for passRequest in self.passRequests:
            if passRequest['Name'] == passName:
                return index
            index += 1
        return -1

    def insert_pass_request_before_pass(self, passIndex, passRequest):
        # insert a passRequest before the specified pass location (passIndex)
        self.passRequests.insert(passIndex, passRequest)

    def insert_pass_request_after_pass(self, passIndex, passRequest):
        # insert a passRequest after the specified pass location (passIndex)
        self.passRequests.insert(passIndex+1, passRequest)

    def replace_references_after(self, startPassRequest, oldPass, oldSlot, newPass, newSlot):
        # from all pass requests after startPassRequest
        # replace all attachment references which uses oldPass and oldSlot
        # with newPass and newSlot
        started = False
        for request in self.passRequests:
            if started:
                for connection in request['Connections']:
                    if connection['AttachmentRef']['Pass'] == oldPass and connection['AttachmentRef']['Attachment'] == oldSlot:
                        connection['AttachmentRef']['Pass'] = newPass
                        connection['AttachmentRef']['Attachment'] = newSlot
            if request['Name'] == startPassRequest and not started:
                started = True
    
    def replace_references_for(self, passRequest, oldPass, oldSlot, newPass, newSlot):
        #replace pass reference for the specified passRequest 
        for request in self.passRequests:
            if request['Name'] == passRequest:
                for connection in request['Connections']:
                    if connection['AttachmentRef']['Pass'] == oldPass and connection['AttachmentRef']['Attachment'] == oldSlot:
                        connection['AttachmentRef']['Pass'] = newPass
                        connection['AttachmentRef']['Attachment'] = newSlot
                return #return when the specified pass request is updated. 

    def find_slot(self, slotName):
        # return slot's index in Slots if a PassRequest with input passName exists
        index = 0
        for slot in self.slots:
            if slot['Name'] == slotName:
                return index
            index += 1
        return -1

    def insert_slot(self, location, newSlotData):
        # insert a new slot at specified location
        self.slots.insert(location, newSlotData)
        
    def add_slot(self, newSlotData):
        # append a new slot to slots
        self.slots.append(newSlotData)

    def get_pass_request(self, passName):
        # Get the pass request from PassRequests with matching pass name
        for passRequest in self.passRequests:
            if passRequest['Name'] == passName:
                return passRequest

    def save(self):
        # backup the original file
        backupFilePath = self.file_path +'.backup'
        shutil.copyfile(self.file_path, backupFilePath)
        # save and overwrite file
        with open(self.file_path, 'w') as json_file:
            json.dump(self.file_data, json_file, indent = 4)            
            print('File [', self.file_path, '] is updated. Old version is saved in [', backupFilePath, ']')


class PassRequest:
    
    def __init__(self, passRequest: object):
        self.pass_request = passRequest

    def add_connection(self, newConnection):
        self.pass_request['Connections'].append(newConnection)