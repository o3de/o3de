"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

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

@pytest.fixture
def new_pass_request():
    pass_request = json.loads('{\"Name\": \"InsertPass\",\"TemplateName\": \"InsertPassTemplate\"}')
    return pass_request

@pytest.fixture
def new_slot():
    slot = json.loads('{\"Name\": \"NewSlot\",\"SlotType\": \"Input\"}')
    return slot

@pytest.fixture
def new_connection():
    connection = json.loads('{\"LocalSlot\": \"color\", \"AttachmentRef\": { \"Pass\": \"Parent\", \"Attachment\": \"DepthStencil\"}}')
    return connection

def test_PassTemplate_Initialize_BadPassTemplateData_ExceptionThrown():
    with pytest.raises(KeyError):
        PassTemplate(bad_test_data_file)

def test_PassTemplate_FindPass_Success(pass_requests_template):
    assert pass_requests_template.find_pass('OpaquePass') == 0
    assert pass_requests_template.find_pass('ImGuiPass') == 4
    assert pass_requests_template.find_pass('NotExistPass') == -1

def test_PassTemplate_InsertPassRequest_AtBegining_Success(pass_requests_template, new_pass_request):
    template = pass_requests_template
    pass_count = template.get_pass_count()
    template.insert_pass_request(0, new_pass_request)
    assert template.find_pass(new_pass_request['Name']) == 0
    assert template.get_pass_count() == pass_count+1
    
    # verify the change is saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.find_pass(new_pass_request['Name'])== 0
    assert saved_tamplate.get_pass_count() == pass_count+1

def test_PassTemplate_InsertPassRequest_AtEnd_Success(pass_requests_template, new_pass_request):
    template = pass_requests_template
    pass_count = template.get_pass_count()
    template.insert_pass_request(pass_count, new_pass_request)
    assert template.find_pass(new_pass_request['Name']) == pass_count
    assert template.get_pass_count() == pass_count+1
    
    # verify the change is saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.find_pass(new_pass_request['Name']) == pass_count
    assert saved_tamplate.get_pass_count() == pass_count+1
        
def test_PassTemplate_InsertPassRequest_WithDuplicatedName_ExceptionThrown(pass_requests_template, new_pass_request):
    template = pass_requests_template
    # insert new pass request
    template.insert_pass_request(0, new_pass_request)
    pass_count = template.get_pass_count()
    # exception when insert the same pass again
    with pytest.raises(ValueError):
        template.insert_pass_request(2, new_pass_request)
    # pass count doesn't change
    assert template.get_pass_count() == pass_count

def test_PassTemplate_InsertPassRequest_WithBadData_ExceptionThrown(pass_requests_template):
    template = pass_requests_template
    pass_count = template.get_pass_count()
    bad_pass_request = json.loads('{\"name\":\"value\"}')
    with pytest.raises(KeyError):
        template.insert_pass_request(2, bad_pass_request)
    assert template.get_pass_count() == pass_count

def test_PassTemplate_InsertPassRequest_AtOutOfRange_AppendSuccess(pass_requests_template, new_pass_request):
    template = pass_requests_template
    pass_count = template.get_pass_count()
    template.insert_pass_request(pass_count+2, new_pass_request)
    assert template.find_pass(new_pass_request['Name']) == pass_count
    assert template.get_pass_count() == pass_count+1

def test_PassTemplate_ReplaceReferencesAfter_Success(pass_requests_template):
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

def test_PassTemplate_ReplaceReferencesFor_Success(pass_requests_template):
    refPass = 'TransparentPass'
    assert pass_requests_template.replace_references_for(refPass, 'OpaquePass', 'DepthStencil', 'Parent', 'DepthStencil') == 1
    refPass = '2DPass'
    assert pass_requests_template.replace_references_for(refPass, 'OpaquePass', 'DepthStencil', 'Parent', 'DepthStencil') == 0

    # verify changes are saved
    pass_requests_template.save()
    saved_tamplate = PassTemplate(pass_requests_template.file_path)
    # no reference of OpaquePass.DepthStencil in TransparentPass
    assert saved_tamplate.replace_references_for('TransparentPass', 'OpaquePass', 'DepthStencil', 'Parent', 'DepthStencil') == 0

def test_PassTemplate_FindSlot_Success(pass_slots_template):
    assert pass_slots_template.find_slot('Color') == -1
    assert pass_slots_template.find_slot('DepthStencil') == 0
    assert pass_slots_template.find_slot('ColorInputOutput') == 1
            
def test_PassTemplate_InsertSlot_AtBegining_Success(pass_slots_template, new_slot):
    template = pass_slots_template
    slot_count = template.get_slot_count()
    depth_stencil_slot = template.find_slot('DepthStencil')
    template.insert_slot(0, new_slot)
    assert template.find_slot(new_slot['Name']) == 0
    assert template.find_slot('DepthStencil') == depth_stencil_slot+1 # DepthStencil moved back by 1
    assert template.get_slot_count() == slot_count+1
        
    # verify the change is saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.find_slot(new_slot['Name']) == 0
    assert saved_tamplate.get_slot_count() == slot_count+1
    
def test_PassTemplate_InsertSlot_AtEnd_Success(pass_slots_template, new_slot):
    template = pass_slots_template
    slot_count = template.get_slot_count()
    depth_stencil_slot = template.find_slot('DepthStencil')
    template.insert_slot(slot_count, new_slot)
    assert template.find_slot(new_slot['Name']) == slot_count
    assert template.find_slot('DepthStencil') == depth_stencil_slot
    assert template.get_slot_count() == slot_count+1
        
    # verify the change is saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.find_slot(new_slot['Name']) == slot_count
    assert saved_tamplate.get_slot_count() == slot_count+1
    
def test_PassTemplate_AddSlot_GoodSlotData_Success(pass_slots_template, new_slot):
    template = pass_slots_template
    slot_count = template.get_slot_count()
    depth_stencil_slot = template.find_slot('DepthStencil')
    template.add_slot(new_slot)
    assert template.find_slot(new_slot['Name']) == slot_count
    assert template.find_slot('DepthStencil') == depth_stencil_slot
    assert template.get_slot_count() == slot_count+1
        
    # verify the change is saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.find_slot(new_slot['Name']) == slot_count
    assert saved_tamplate.get_slot_count() == slot_count+1
    
def test_PassTemplate_InsertSlot_OutOfRange_AppendSuccess(pass_slots_template, new_slot):
    template = pass_slots_template
    slot_count = template.get_slot_count()
    template.insert_slot(slot_count+3, new_slot)
    assert template.find_slot(new_slot['Name']) == slot_count
    assert template.get_slot_count() == slot_count+1
        
def test_PassTemplate_AddDuplicateSlot_ExceptionThrown(pass_slots_template, new_slot):
    template = pass_slots_template
    slot_count = template.get_slot_count()
    template.add_slot(new_slot)
    
    with pytest.raises(ValueError):
        template.insert_slot(0, new_slot)
    with pytest.raises(ValueError):
        template.add_slot(new_slot)
        
def test_PassTemplate_InsertOrAddSlot_WithBadSlotData_ExceptionThrown(pass_slots_template):
    template = pass_slots_template
    slot_count = template.get_slot_count()
    bad_slot = json.loads('{\"slot\": \"xxx\"}')
    
    with pytest.raises(KeyError):
        template.insert_slot(0, bad_slot)
    with pytest.raises(KeyError):
        template.add_slot(bad_slot)
        
def test_PassReqeuest_Initialize_WithExistPassReqeuestFromPassTemplate_Success(pass_requests_template):
    template = pass_requests_template
    request = PassRequest(template.get_pass_request('OpaquePass'))
    connection_count = request.get_connection_count()
    assert connection_count == 2
        
def test_PassTemplate_GetPassRequest_NotExist_ReturnNull(pass_requests_template):
    assert not pass_requests_template.get_pass_request('NotExistPass')
            
def test_PassReqeuest_AddConnection_WithExistingConnections_Success(pass_requests_template, new_connection):
    template = pass_requests_template
    request = PassRequest(template.get_pass_request('OpaquePass'))
    connection_count = request.get_connection_count()
    request.add_connection(new_connection)
    connection_count += 1
    assert request.get_connection_count() == connection_count
    
    # verify changes are saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    saved_request = PassRequest(saved_tamplate.get_pass_request('OpaquePass'))
    assert saved_request.get_connection_count() == connection_count

def test_PassReqeuest_AddConnection_WithNoExistingConnections_Success(pass_requests_template, new_connection):
    template = pass_requests_template
    request = PassRequest(template.get_pass_request('ImGuiPass'))
    assert request.get_connection_count() == 0
    request.add_connection(new_connection)
    assert request.get_connection_count() == 1
    
    # verify changes are saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    saved_request = PassRequest(saved_tamplate.get_pass_request('ImGuiPass'))
    assert saved_request.get_connection_count() == 1
    
def test_PassReqeuest_AddConnection_WithDuplicatedName_ExceptionThrown(pass_requests_template, new_connection):
    template = pass_requests_template
    request = PassRequest(template.get_pass_request('OpaquePass'))
    request.add_connection(new_connection)
    with pytest.raises(ValueError):
        request.add_connection(new_connection)

def test_PassReqeuest_AddConnect_BadConnectionData_ExceptionThrown(pass_requests_template, new_connection):
    template = pass_requests_template
    request = PassRequest(template.get_pass_request('OpaquePass'))
    bad_connection = json.loads('{\"xxx\": \"xxx\"}')
    with pytest.raises(KeyError):
        request.add_connection(bad_connection)
                        
def test_PassTemplate_InsertSlot_ToEmptyList_Success(pass_requests_template, new_slot):
    template = pass_requests_template
    # test insert slot function to pass template which doesn't have any slots
    slot_count = template.get_slot_count()
    assert slot_count == 0

    assert template.find_slot(new_slot['Name'])==-1
    pass_requests_template.insert_slot(0, new_slot)
    assert template.find_slot(new_slot['Name']) == 0
    assert template.get_slot_count() == 1

    # verify changes are saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.get_slot_count() == 1
        
def test_PassTempalte_InsertPassRequest_ToEmptyList_Success(pass_slots_template, new_pass_request):
    template = pass_slots_template
    # test insert pass function to pass template which doesn't have any pass requests
    pass_count = template.get_pass_count()
    assert pass_count == 0
    template.insert_pass_request(0, new_pass_request)
    assert template.find_pass(new_pass_request['Name']) == 0
    assert template.get_pass_count() == 1

    # verify changes are saved
    template.save()
    saved_tamplate = PassTemplate(template.file_path)
    assert saved_tamplate.get_pass_count() == 1
        
def test_PassTemplate_Save_Success(pass_requests_template):
    pass_requests_template.save()
    saved_tamplate = PassTemplate(pass_requests_template.file_path)
    assert os.path.exists(pass_requests_template.file_path)
    assert os.path.exists(pass_requests_template.file_path +'.backup')
