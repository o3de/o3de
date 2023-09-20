#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# See the following pages for information on how to write formatters for lldb
# https://lldb.llvm.org/use/variable.html#finding-formatters-101
# https://lldb.llvm.org/python_api/lldb.SBValue.html#lldb.SBValue
# https://github.com/llvm/llvm-project/blob/main/lldb/examples/synthetic/libcxx.py
import enum
import lldb
import lldb.formatters.Logger

# Uncomment for debug output
# lldb.formatters.Logger._lldb_formatters_debug_level = 3

def string_summary(valobj: lldb.SBValue, internal_dict):
    logger = lldb.formatters.Logger.Logger()
    logger >> f"{valobj.GetType()} summary"


    def element_type_string_summary(valobj: lldb.SBValue):
        # Grab the SBProcess object which is needed to read a c-string from memory
        process = valobj.GetProcess()

        compressed_pair_storage = valobj.GetChildMemberWithName("m_storage")
        # The first child of the compressed pair is the AZStd::basic_string::storage union
        storage_inst = compressed_pair_storage.GetChildAtIndex(0)
        storage_element = storage_inst.GetChildMemberWithName("m_element")
        short_string_storage = storage_element.GetChildMemberWithName("m_shortData")

        # Check if the Short String optimization is active
        is_sso_active_member = short_string_storage.GetChildMemberWithName("m_packed").GetChildMemberWithName("m_ssoActive")
        is_sso_active = int(is_sso_active_member.GetValueAsUnsigned())
        if is_sso_active:
            # Read the string size and memory address
            string_size = int(short_string_storage.GetChildMemberWithName("m_packed").GetChildMemberWithName("m_size").GetValueAsUnsigned())
            string_address = short_string_storage.GetChildMemberWithName('m_buffer').AddressOf()
        else:
            # Short String optimization is not active
            try:
                allocated_storage = storage_element.EvaluateExpression("m_allocatedData")
            except Exception as e:
                return f'error: {e}'

            string_size = allocated_storage.GetChildMemberWithName("m_size").GetValueAsUnsigned()
            string_address = allocated_storage.GetChildMemberWithName("m_data")

        # If the string size is 0, then return an empty string
        if string_size == 0:
            return '""'

        # Stores the result of summary string
        result_string = ''
        try:
            string_type = valobj.GetType()
            element_type = string_type.GetTemplateArgumentType(0)
            error = lldb.SBError()

            # SBType.GetBasicType is strange, passing in the enum value of the type
            # doesn't query the actual type but returns a new SBType
            # instance represents the basic type
            # https://lldb.llvm.org/python_api/lldb.SBType.html#lldb.SBType.GetBasicType
            if element_type == element_type.GetBasicType(lldb.eBasicTypeChar):
            # Get the byte size of the character type being stored
                element_size = element_type.GetByteSize()
                byte_data = process.ReadMemory(string_address.GetValueAsUnsigned(), string_size * element_size, error)
                result_string = byte_data.decode('utf-8')
            elif element_type == element_type.GetBasicType(lldb.eBasicTypeWChar):
                element_size = element_type.GetByteSize()
                if element_size == 4:
                    byte_data = process.ReadMemory(string_address.GetValueAsUnsigned(), string_size * element_size, error)
                    result_string = byte_data.decode('utf-32')
                if element_size == 2:
                    byte_data = process.ReadMemory(string_address.GetValueAsUnsigned(), string_size * element_size, error)
                    result_string = byte_data.decode('utf-16')
            else:
                # element type is not handled(not a char or wchar_t)
                return '"<not handled>"'

            if error.Fail():
                return f'process read error: {error}'
        except Exception as e:
            return f'error: {e}'

        return f'"{result_string}"'

    # Invoke the inner function that iterates through the AZStd::basic_string structure
    return element_type_string_summary(valobj)

def fixed_string_summary(valobj: lldb.SBValue, internal_dict):
    logger = lldb.formatters.Logger.Logger()
    logger >> f"{valobj.GetType()} summary"

    # Grab the SBProcess object which is needed to read a c-string from memory
    process = valobj.GetProcess()

    string_size = valobj.GetChildMemberWithName("m_size").GetValueAsUnsigned()
    string_address = valobj.GetChildMemberWithName("m_buffer").AddressOf()

    # If the string size is 0, then return an empty string
    if string_size == 0:
        return '""'

    # Stores the result of summary string
    result_string = ''
    try:
        string_type = valobj.GetType()
        element_type = string_type.GetTemplateArgumentType(0)
        error = lldb.SBError()

        # SBType.GetBasicType is strange, passing in the enum value of the type
        # doesn't query the actual type but returns a new SBType
        # instance represents the basic type
        # https://lldb.llvm.org/python_api/lldb.SBType.html#lldb.SBType.GetBasicType
        if element_type == element_type.GetBasicType(lldb.eBasicTypeChar):
        # Get the byte size of the character type being stored
            element_size = element_type.GetByteSize()
            byte_data = process.ReadMemory(string_address.GetValueAsUnsigned(), string_size * element_size, error)
            result_string = byte_data.decode('utf-8')
        elif element_type == element_type.GetBasicType(lldb.eBasicTypeWChar):
            element_size = element_type.GetByteSize()
            if element_size == 4:
                byte_data = process.ReadMemory(string_address.GetValueAsUnsigned(), string_size * element_size, error)
                result_string = byte_data.decode('utf-32')
            if element_size == 2:
                byte_data = process.ReadMemory(string_address.GetValueAsUnsigned(), string_size * element_size, error)
                result_string = byte_data.decode('utf-16')
        else:
            # element type is not handled(not a char or wchar_t)
            return '"<not handled>"'

        if error.Fail():
            return f'process read error: {error}'
    except Exception as e:
        return f'error: {e}'

    return f'"{result_string}"'

def string_view_summary(valobj: lldb.SBValue, internal_dict):
    logger = lldb.formatters.Logger.Logger()
    logger >> f"{valobj.GetType()} summary"

    # Grab the SBProcess object which is needed to read a c-string from memory
    process = valobj.GetProcess()

    string_size = valobj.GetChildMemberWithName("m_size").GetValueAsUnsigned()
    string_address = valobj.GetChildMemberWithName("m_begin")

    # If the string size is 0, then return an empty string
    if string_size == 0:
        return '""'

    # Stores the result of summary string
    result_string = ''
    try:
        string_type = valobj.GetType()
        element_type = string_type.GetTemplateArgumentType(0)
        error = lldb.SBError()

        # SBType.GetBasicType is strange, passing in the enum value of the type
        # doesn't query the actual type but returns a new SBType
        # instance represents the basic type
        # https://lldb.llvm.org/python_api/lldb.SBType.html#lldb.SBType.GetBasicType
        if element_type == element_type.GetBasicType(lldb.eBasicTypeChar):
        # Get the byte size of the character type being stored
            element_size = element_type.GetByteSize()
            byte_data = process.ReadMemory(string_address.GetValueAsUnsigned(), string_size * element_size, error)
            result_string = byte_data.decode('utf-8')
        elif element_type == element_type.GetBasicType(lldb.eBasicTypeWChar):
            element_size = element_type.GetByteSize()
            if element_size == 4:
                byte_data = process.ReadMemory(string_address.GetValueAsUnsigned(), string_size * element_size, error)
                result_string = byte_data.decode('utf-32')
            if element_size == 2:
                byte_data = process.ReadMemory(string_address.GetValueAsUnsigned(), string_size * element_size, error)
                result_string = byte_data.decode('utf-16')
        else:
            # element type is not handled(not a char or wchar_t)
            return '"<not handled>"'

        if error.Fail():
            return f'process read error: {error}'
    except Exception as e:
        return f'error: {e}'

    return f'"{result_string}"'


class hash_table_synthetic_provider(lldb.SBSyntheticValueProvider):
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.bucket_count = None
        self.num_elements = None
        self.next_element = None
        self.element_cache = None

        logger = lldb.formatters.Logger.Logger()

        # Get the node type stored within the hash_table list type
        data_storage = self.valobj.GetChildMemberWithName("m_data")
        hash_table_list = data_storage.GetChildMemberWithName("m_list")
        list_type = hash_table_list.GetType()
        list_type_template_args = list_type.template_arg_array()
        if len(list_type_template_args) > 0:
            # The first template argument is the type T of AZStd::list<T>
            list_element_type = list_type_template_args[0]

            # Grab the SBTarget object in order to query the list node type
            target = self.valobj.GetTarget()

            # Substitute in type T into the AZStd::Internal::list_node<T> to get list node type
            # This will be used to cast the AZStd::Internal::list_base_node to actual value type
            list_node_type_str = f'AZStd::Internal::list_node<{list_element_type.GetName()} >'
            self.node_type = target.FindFirstType(list_node_type_str)
            logger >> f'Hash table list node_type is: "{self.node_type.GetName()}"'

    def num_children(self) -> int:
        return self.num_elements if self.num_elements else 0

    def has_children(self) -> bool:
        return self.num_children() != 0

    def get_child_index(self, name):
        try:
            return int(name.lstrip("[").rstrip("]"))
        except:
            return -1

    def get_child_at_index(self, index):
        if index < 0 or index >= self.num_children():
            return None

        if not self.node_type:
            logger >> f'Cannot query Hash table element without node_type'
            return None

        # populate the cache all the previous elements up to the current index
        while index >= len(self.element_cache):
            if not self.next_element:
                logger >> f'Reached the end of the hash_table list. Element at index {index} cannot be found'
                return None

            # First dereference the AZStd::Internal::list_base_node*
            # Next cast the AZStd::Internal::list_base_node -> AZtd::Internal::list_node<T>
            node = self.next_element.CreateValueFromData("list_node", self.next_element.GetData(), self.node_type)
            self.element_cache.append(node.GetChildMemberWithName("m_value"))

            # Move to the next element in the list
            self.next_element = self.next_element.GetChildMemberWithName("m_next")
            if not self.next_element.GetValueAsUnsigned(0):
                self.next_element = None


        # The value has been found so return it
        value = self.element_cache[index]
        return self.valobj.CreateValueFromData(f'[{index}]', value.GetData(), value.GetType())


    def update(self):
        try:
            logger = lldb.formatters.Logger.Logger()
            data_storage = self.valobj.GetChildMemberWithName("m_data")
            hash_table_list = data_storage.GetChildMemberWithName("m_list")
            self.num_elements = hash_table_list.GetChildMemberWithName("m_numElements").GetValueAsUnsigned()
            logger >> f'Num elements = {self.num_elements}'

            self.max_load_factor = data_storage.GetChildMemberWithName("m_max_load_factor").GetValueAsUnsigned()

            hash_table_bucket_vector = data_storage.GetChildMemberWithName("m_vector")
            self.bucket_count = hash_table_bucket_vector.GetChildMemberWithName("m_last").GetValueAsUnsigned() \
                - hash_table_bucket_vector.GetChildMemberWithName("m_start").GetValueAsUnsigned()
            logger >> f'Bucket count = {self.bucket_count}'

            # Store off the pointer to the first element in the hash_table list of nodes
            self.next_element = hash_table_list.GetChildMemberWithName("m_head").GetChildMemberWithName("m_next") \
                if self.num_elements else None

            # Cache SB Value containing the map value type and the hash value
            self.element_cache = []
        except Exception as e:
            logger >> f'exception: {e}'


def __lldb_init_module(debugger, dict):
    # Import the AZStd::basic_string summary
    debugger.HandleCommand('type summary add -n "AZStd::string" -F o3destd_lldb.string_summary --category o3de')
    debugger.HandleCommand('type summary add -x "^AZStd::basic_string<.+>$" -F o3destd_lldb.string_summary --category o3de')

    # Import the AZStd::basic_fixed_string summary
    debugger.HandleCommand('type summary add -x "^AZStd::basic_fixed_string<.+>$" -F o3destd_lldb.fixed_string_summary --category o3de')

    # Import the AZStd::basic_string_view summary
    debugger.HandleCommand('type summary add -x "^AZStd::basic_string_view<.+>$" -F o3destd_lldb.string_view_summary --category o3de')

    # Import the AZStd::hash_table synthetic children providers (unordered_map and unordered_set)
    debugger.HandleCommand('type synthetic add -x "^AZStd::hash_table<.+>$" --python-class o3destd_lldb.hash_table_synthetic_provider --category o3de')
    debugger.HandleCommand('type synthetic add -x "^AZStd::unordered_map<.+>$" --python-class o3destd_lldb.hash_table_synthetic_provider --category o3de')
    debugger.HandleCommand('type synthetic add -x "^AZStd::unordered_set<.+>$" --python-class o3destd_lldb.hash_table_synthetic_provider --category o3de')
    debugger.HandleCommand('type synthetic add -x "^AZStd::unordered_multimap<.+>$" --python-class o3destd_lldb.hash_table_synthetic_provider --category o3de')
    debugger.HandleCommand('type synthetic add -x "^AZStd::unordered_multiset<.+>$" --python-class o3destd_lldb.hash_table_synthetic_provider --category o3de')

    # Enable the o3de category for formatters
    debugger.HandleCommand('type category enable o3de')
