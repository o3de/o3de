"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Unit tests for pass_data.py
"""
import os
import pytest
import shutil
import json
from atom_rpi_tools.pass_data import PassTemplate
from atom_rpi_tools.pass_data import PassRequest

good_pass_requests_file = os.path.join(os.path.dirname(__file__), 'testdata/pass_requests.json')
good_pass_slots_file = os.path.join(os.path.dirname(__file__), 'testdata/pass_slots.json')
bad_test_data_file = os.path.join(os.path.dirname(__file__), 'testdata/pass_test_bad.json')

@pytest.fixture
def pass_requests_template(tmpdir):
    filename = 'pass_requests.json'
    source_path = os.path.join(os.path.dirname(__file__), 'testdata/', filename)
    destFilePath = os.path.join(tmpdir, 'pass_requests.json')
    shutil.copyfile(source_path, destFilePath)
    return PassTemplate(destFilePath)

@pytest.fixture
def pass_slots_template(tmpdir):
    filename = 'pass_slots.json'
    source_path = os.path.join(os.path.dirname(__file__), 'testdata/', filename)
    destFilePath = os.path.join(tmpdir, 'pass_requests.json')
    shutil.copyfile(source_path, destFilePath)
    return PassTemplate(destFilePath)

def test_invalid_pass_tempalte():
    with pytest.raises(KeyError):
        PassTemplate(bad_test_data_file)

def test_find_pass(pass_requests_template):
    assert pass_requests_template.find_pass('OpaquePass') == 0
    assert pass_requests_template.find_pass('ImGuiPass') == 4
    assert pass_requests_template.find_pass('NotExistPass') == -1

def test_insert_pass_request(pass_requests_template):
    template = pass_requests_template
    pass_count = template.get_pass_count()

    # insert at the beginning
    pass_request1 = json.loads('{\"Name\": \"InsertPass1\",\"TemplateName\": \"InsertPassTemplate\"}')
    template.insert_pass_request(0, pass_request1)
    assert template.find_pass('InsertPass1') == 0
    assert template.find_pass('OpaquePass') ==  1
    pass_count += 1
    assert template.get_pass_count() == pass_count

    # insert at the end
    pass_request2 = json.loads('{\"Name\": \"InsertPass2\",\"TemplateName\": \"InsertPassTemplate\"}')
    template.insert_pass_request(pass_count, pass_request2)
    assert template.find_pass('InsertPass2') == pass_count
    pass_count += 1
    assert template.get_pass_count() == pass_count

    # insert a pass which is already existed
    with pytest.raises(ValueError):
        template.insert_pass_request(2, pass_request2)
    assert template.get_pass_count() == pass_count
                        
    # insert bad pass request
    bad_pass_request = json.loads('{\"name\":\"value\"}')
    with pytest.raises(KeyError):
        template.insert_pass_request(2, bad_pass_request)
    assert template.get_pass_count() == pass_count
                
    # insert out of the range which ends up append
    pass_request3 = json.loads('{\"Name\": \"InsertPass3\",\"TemplateName\": \"InsertPassTemplate\"}')
    template.insert_pass_request(pass_count+2, pass_request3)
    pass_count += 1
    assert template.get_pass_count() == pass_count

    # verify changes are saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.find_pass('InsertPass1') == 0
    assert saved_tamplate.find_pass('InsertPass3') == pass_count-1

def test_replace_references_after(pass_requests_template):
    # replace OpaquePass.DepthStencil with Parent.DepthStencil'
    refPass = 'OpaquePass'
    # there are 2 passes after OpaquePass which use OpaquePass.DepthStencil as attachment reference
    assert pass_requests_template.replace_references_after(refPass, 'OpaquePass', 'DepthStencil', 'Parent', 'DepthStencil') == 2
    # after the previous replacement, there it no OpaquePass.DepthStencil reference
    refPass = 'TransparentPass'
    assert pass_requests_template.replace_references_after(refPass, 'OpaquePass', 'DepthStencil', 'Parent', 'DepthStencil') == 0
        
    # verify changes are saved
    pass_requests_template.save()
    saved_tamplate = PassTemplate(pass_requests_template.file_path)
    assert saved_tamplate.replace_references_after('OpaquePass', 'OpaquePass', 'DepthStencil', 'Parent', 'DepthStencil') == 0

def test_replace_references_for(pass_requests_template):
    refPass = 'TransparentPass'
    assert pass_requests_template.replace_references_for(refPass, 'OpaquePass', 'DepthStencil', 'Parent', 'DepthStencil') == 1
    refPass = '2DPass'
    assert pass_requests_template.replace_references_for(refPass, 'OpaquePass', 'DepthStencil', 'Parent', 'DepthStencil') == 0

    # verify changes are saved
    pass_requests_template.save()
    saved_tamplate = PassTemplate(pass_requests_template.file_path)
    # no reference of OpaquePass.DepthStencil in TransparentPass
    assert saved_tamplate.replace_references_for('TransparentPass', 'OpaquePass', 'DepthStencil', 'Parent', 'DepthStencil') == 0

def test_find_slot(pass_slots_template):
    assert pass_slots_template.find_slot('Color') == -1
    assert pass_slots_template.find_slot('DepthStencil') == 0
    assert pass_slots_template.find_slot('ColorInputOutput') == 1
        
def test_insert_or_add_slot(pass_slots_template):
    template = pass_slots_template
    slot_count = template.get_slot_count()
               
    depth_stencil_slot = template.find_slot('DepthStencil')

    # insert at the beginning
    new_slot1 = json.loads('{\"Name\": \"New1\",\"SlotType\": \"Input\"}')
    template.insert_slot(0, new_slot1)
    assert template.find_slot(new_slot1['Name']) == 0
    assert template.find_slot('DepthStencil') == depth_stencil_slot+1
    slot_count += 1
    assert template.get_slot_count() == slot_count
                
    # insert at the end
    new_slot2 = json.loads('{\"Name\": \"New2\",\"SlotType\": \"Input\"}')
    template.insert_slot(slot_count, new_slot2)
    assert template.find_slot(new_slot2['Name']) == slot_count
    slot_count += 1
    assert template.get_slot_count() == slot_count

    # append
    new_slot3 = json.loads('{\"Name\": \"New3\",\"SlotType\": \"Input\"}')
    template.add_slot(new_slot3)
    assert template.find_slot(new_slot3['Name']) == slot_count
    slot_count += 1
    assert template.get_slot_count() == slot_count
                
    # insert/add duplicate slot: expect exception
    with pytest.raises(ValueError):
        template.insert_slot(0, new_slot3)
    assert template.get_slot_count() == slot_count
    with pytest.raises(ValueError):
        template.add_slot(new_slot3)
    assert template.get_slot_count() == slot_count

    # insert/add bad slot data
    bad_slot = json.loads('{\"slot\": \"xxx\"}')
    with pytest.raises(KeyError):
        template.insert_slot(0, bad_slot)
    with pytest.raises(KeyError):
        template.add_slot(bad_slot)
    assert template.get_slot_count() == slot_count
                
    # insert out of the range ends up append
    new_slot4 = json.loads('{\"Name\": \"New4\",\"SlotType\": \"Input\"}')
    template.insert_slot(slot_count+3, new_slot4)
    slot_count += 1
    assert template.get_slot_count() == slot_count

    # verify changes are saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.find_slot(new_slot1['Name']) == 0
    assert saved_tamplate.find_slot(new_slot2['Name']) == slot_count-3
    assert saved_tamplate.find_slot(new_slot3['Name']) == slot_count-2
    assert saved_tamplate.find_slot(new_slot4['Name']) == slot_count-1
                        
def test_pass_request(pass_requests_template):
    template = pass_requests_template
    # pass request with 2 connections already
    request = PassRequest(template.get_pass_request('OpaquePass'))
    assert request
    connection_count = request.get_connection_count()
    assert connection_count == 2
    # add a connection 
    new_connection = json.loads('{\"LocalSlot\": \"color\", \"AttachmentRef\": { \"Pass\": \"Parent\", \"Attachment\": \"DepthStencil\"}}')
    request.add_connection(new_connection)
    connection_count += 1
    assert request.get_connection_count() == connection_count

    # add a duplicate connect: fail
    with pytest.raises(ValueError):
        request.add_connection(new_connection)
            
    # add a bad connect: fail
    bad_connection = json.loads('{\"xxx\": \"xxx\"}')
    with pytest.raises(KeyError):
        request.add_connection(bad_connection)
        
    # pass request with no connection
    request = PassRequest(template.get_pass_request('ImGuiPass'))
    assert request
    request.add_connection(new_connection)
    assert request.get_connection_count() == 1

    # try to get a pass request which doesn't exist
    assert not template.get_pass_request('NotExistPass')
        
    # verify changes are saved
    pass_requests_template.save()
    saved_tamplate = PassTemplate(template.file_path)
    request = PassRequest(saved_tamplate.get_pass_request('OpaquePass'))
    assert request.get_connection_count() == connection_count
    request = PassRequest(saved_tamplate.get_pass_request('ImGuiPass'))
    assert request.get_connection_count() == 1

def test_insert_to_empty_slots(pass_requests_template):
    template = pass_requests_template
    # test insert slot function to pass template which doesn't have any slots
    slot_count = template.get_slot_count()
    assert slot_count == 0

    assert template.find_slot('New1')==-1

    # insert at the beginning
    new_slot1 = json.loads('{\"Name\": \"New1\",\"SlotType\": \"Input\"}')
    pass_requests_template.insert_slot(0, new_slot1)
    assert template.find_slot(new_slot1['Name']) == 0
    assert template.get_slot_count() == 1

    # verify changes are saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.get_slot_count() == 1
        
def test_insert_to_empty_requests(pass_slots_template):
    template = pass_slots_template
    # test insert pass function to pass template which doesn't have any pass requests
    pass_count = template.get_pass_count()
    assert pass_count == 0
    
    # insert at the beginning
    pass_request1 = json.loads('{\"Name\": \"InsertPass1\",\"TemplateName\": \"InsertPassTemplate\"}')
    template.insert_pass_request(0, pass_request1)
    assert template.find_pass('InsertPass1') == 0
    assert template.get_pass_count() == 1

    # verify changes are saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.get_pass_count() == 1
        
def test_save(pass_requests_template):
    pass_requests_template.save()
    saved_tamplate = PassTemplate(pass_requests_template.file_path)
    assert os.path.exists(pass_requests_template.file_path)
    assert os.path.exists(pass_requests_template.file_path +'.backup')
