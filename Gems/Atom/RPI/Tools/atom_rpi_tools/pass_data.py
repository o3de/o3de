"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

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
        
        if 'ClassName' not in self.file_data or 'ClassData' not in self.file_data or self.file_data['ClassName']!='PassAsset' or 'PassTemplate' not in self.file_data['ClassData']:
            raise KeyError('the json file is not a PassAsset file')
            return
       
        if 'PassRequests' in self.file_data['ClassData']['PassTemplate']:
            self.passRequests = self.file_data['ClassData']['PassTemplate']['PassRequests']

        if 'Slots' in self.file_data['ClassData']['PassTemplate']:
            self.slots = self.file_data['ClassData']['PassTemplate']['Slots']

        self.initialized = True
        print('PassTemplate is loaded from ', filePath)

    def find_pass(self, passName):
        # return pass's index in PassRequests if a PassRequest with input passName exists
        if not hasattr(self, 'passRequests'):
            return -1
        index = 0
        for passRequest in self.passRequests:
            if passRequest['Name'] == passName:
                return index
            index += 1
        return -1

    def get_pass_count(self):
        if not hasattr(self, 'passRequests'):
            return 0
        return len(self.passRequests)
       
    def __validate_pass_request_data(self, passRequest):
        if ('Name' not in passRequest or 'TemplateName' not in passRequest):
            raise KeyError('invalid pass request data')

    def __ensure_pass_requests_key(self):
        if not hasattr(self, 'passRequests'):
            self.file_data['ClassData']['PassTemplate']['PassRequests'] = []
            self.passRequests = self.file_data['ClassData']['PassTemplate']['PassRequests']

    def __ensure_pass_slots_key(self):
        if not hasattr(self, 'slots'):
            self.file_data['ClassData']['PassTemplate']['Slots'] = []
            self.slots = self.file_data['ClassData']['PassTemplate']['Slots']

    def insert_pass_request(self, location, passRequest):
        self.__validate_pass_request_data(passRequest)

        if (self.find_pass(passRequest['Name']) >= 0):
            raise ValueError('pass request ', passRequest['Name'], ' is already exist')
        # insert a passRequest before the specified location
        self.__ensure_pass_requests_key()
        self.passRequests.insert(location, passRequest)

    def replace_references_after(self, startPassRequest, oldPass, oldSlot, newPass, newSlot):
        if not hasattr(self, 'passRequests'):
            return 0
        # from all pass requests after startPassRequest
        # replace all attachment references which uses oldPass and oldSlot
        # with newPass and newSlot
        started = False
        replaced_count = 0
        for request in self.passRequests:
            if started:
                if ('Connections' in request):
                    for connection in request['Connections']:
                        if connection['AttachmentRef']['Pass'] == oldPass and connection['AttachmentRef']['Attachment'] == oldSlot:
                            connection['AttachmentRef']['Pass'] = newPass
                            connection['AttachmentRef']['Attachment'] = newSlot
                            replaced_count += 1
            if request['Name'] == startPassRequest and not started:
                started = True
        return replaced_count
    
    def replace_references_for(self, passRequest, oldPass, oldSlot, newPass, newSlot):
        if not hasattr(self, 'passRequests'):
            return 0
        #replace pass reference for the specified passRequest 
        replaced_count = 0
        for request in self.passRequests:
            if request['Name'] == passRequest:
                if ('Connections' in request):
                    for connection in request['Connections']:
                        if connection['AttachmentRef']['Pass'] == oldPass and connection['AttachmentRef']['Attachment'] == oldSlot:
                            connection['AttachmentRef']['Pass'] = newPass
                            connection['AttachmentRef']['Attachment'] = newSlot
                            replaced_count += 1
                    return replaced_count #return when the specified pass request is updated. 
        return replaced_count

    def __validate_slot_data(self, slotData):
        if ('Name' not in slotData or 'SlotType' not in slotData):
            raise KeyError('invalid slot data')
                
    def get_slot_count(self):
        if not hasattr(self, 'slots'):
            return 0
        return len(self.slots)

    def find_slot(self, slotName):
        # return slot's index in Slots if a PassRequest with input passName exists
        if not hasattr(self, 'slots'):
            return -1
        index = 0
        for slot in self.slots:
            if slot['Name'] == slotName:
                return index
            index += 1
        return -1

    def insert_slot(self, location, newSlotData):
        # insert a new slot at specified location 
        self.__validate_slot_data(newSlotData)
        # check if the slot already exist
        if (self.find_slot(newSlotData['Name']) >= 0):
            raise ValueError('Slot ', newSlotData['Name'], ' is already exist')
        
        self.__ensure_pass_slots_key()
        self.slots.insert(location, newSlotData)
        
    def add_slot(self, newSlotData):
        # append a new slot to slots
        self.__validate_slot_data(newSlotData)
        # check if the slot already exist
        if (self.find_slot(newSlotData['Name']) >= 0):
            raise ValueError('Slot ', newSlotData['Name'], ' is already exist')
        
        self.__ensure_pass_slots_key()
        self.slots.append(newSlotData)

    def get_pass_request(self, passName):
        if not hasattr(self, 'passRequests'):
            return
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
        if 'Connections' in passRequest:
            self.connections = passRequest['Connections']
        
    def __validate_connection(self, connection):
        if ('LocalSlot' not in connection or 'AttachmentRef' not in connection):
            raise KeyError('invalid connection data')
        
    def __ensure_connections_key(self):
        if not hasattr(self, 'connections'):
            self.pass_request['Connections'] = []
            self.connections = self.pass_request['Connections']

    def get_connection_count(self):
        if not hasattr(self, 'connections'): 
            return 0
        return len(self.connections)

    def find_connection(self, localSlotName):
        if not hasattr(self, 'connections'): 
            return -1
        index = 0
        for connection in self.connections:
            if connection['LocalSlot'] == localSlotName:
                return index
            index += 1
        return -1

    def add_connection(self, newConnection):
        self.__validate_connection(newConnection)
        if self.find_connection(newConnection['LocalSlot']) >= 0:
            raise ValueError('connection ', newConnection['LocalSlot'], ' already exists')
        self.__ensure_connections_key()
        self.connections.append(newConnection)