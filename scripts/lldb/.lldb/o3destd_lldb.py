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

def read_string_data(process: lldb.SBProcess, string_type: lldb.SBType, string_address: lldb.SBValue, string_size: int):
    # If the string size is 0, then return an empty string
    if string_size == 0:
        return '""'

    # Get the string element type from the first tempalte argument
    element_type = string_type.GetTemplateArgumentType(0)
    # query the byte size of the character type. It should be 1 for `char`
    # 2 for wchar_t on Windows and 4 for wchar_t on Linux/MacOS
    element_size = element_type.GetByteSize()

    # Stores the encoding to use to decode the string byte data read in from memory
    # into a python unicode string
    encoding = ''

    # SBType.GetBasicType is strange, passing in the enum value of the type
    # doesn't query the actual type but returns a new SBType
    # instance that represents the basic type
    # https://lldb.llvm.org/python_api/lldb.SBType.html#lldb.SBType.GetBasicType
    if element_type == element_type.GetBasicType(lldb.eBasicTypeChar):
        encoding = 'utf-8'
    elif element_type == element_type.GetBasicType(lldb.eBasicTypeWChar):
        if element_size == 4:
            encoding = 'utf-32'
        elif element_size == 2:
            encoding = 'utf-16'
        else:
            return f'<error: wchar_t type size ({element_size}) is not 2 or 4 bytes. string data cannot be decoded>'
    else:
        # element type is not handled(not a char or wchar_t)
        return '<not handled>'

    try:
        # Try to read from the string_address, the size of the string into a bytearray
        # That byte array is then decoded to a python string
        error = lldb.SBError()
        byte_data = process.ReadMemory(string_address.GetValueAsUnsigned(), string_size * element_size, error)
        result_string = byte_data.decode(encoding)
        if error.Fail():
            return f'process read error: {error}'
    except Exception as e:
        return f'error: {e}'

    return f'"{result_string}"'

def string_summary(valobj: lldb.SBValue, internal_dict):
    logger = lldb.formatters.Logger.Logger()
    logger >> f"{valobj.GetType()} summary"

    compressed_pair_storage = valobj.GetValueForExpressionPath(".m_storage")
    # The first child of the compressed pair is the AZStd::basic_string::storage union
    storage_inst = compressed_pair_storage.GetChildAtIndex(0)
    storage_element = storage_inst.GetValueForExpressionPath(".m_element")
    short_string_storage = storage_element.GetValueForExpressionPath(".m_shortData")

    # Check if the Short String optimization is active
    is_sso_active_member = short_string_storage.GetValueForExpressionPath(".m_packed.m_ssoActive")
    is_sso_active = int(is_sso_active_member.GetValueAsUnsigned())
    if is_sso_active:
        # Read the string size and memory address
        string_size = int(short_string_storage.GetValueForExpressionPath(".m_packed.m_size").GetValueAsUnsigned())
        string_address = short_string_storage.GetValueForExpressionPath('.m_buffer').AddressOf()
    else:
        # Short String optimization is not active
        try:
            allocated_storage = storage_element.GetValueForExpressionPath(".m_allocatedData")
        except Exception as e:
            return f'error: {e}'

        string_size = allocated_storage.GetValueForExpressionPath(".m_size").GetValueAsUnsigned()
        string_address = allocated_storage.GetValueForExpressionPath(".m_data")


    # return result of reading in the string data as python string
    return read_string_data(valobj.GetProcess(), valobj.GetType(), string_address, string_size)


def fixed_string_summary(valobj: lldb.SBValue, internal_dict):
    logger = lldb.formatters.Logger.Logger()
    logger >> f"{valobj.GetType()} summary"

    string_size = valobj.GetValueForExpressionPath(".m_size").GetValueAsUnsigned()
    string_address = valobj.GetValueForExpressionPath(".m_buffer").AddressOf()

    return read_string_data(valobj.GetProcess(), valobj.GetType(), string_address, string_size)


def string_view_summary(valobj: lldb.SBValue, internal_dict):
    logger = lldb.formatters.Logger.Logger()
    logger >> f"{valobj.GetType()} summary"

    string_size = valobj.GetValueForExpressionPath(".m_size").GetValueAsUnsigned()
    string_address = valobj.GetValueForExpressionPath(".m_begin")

    return read_string_data(valobj.GetProcess(), valobj.GetType(), string_address, string_size)


class hash_table_synthetic_provider(lldb.SBSyntheticValueProvider):
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.bucket_count = None
        self.num_elements = None
        self.next_element = None
        self.element_cache = None


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
            node = self.next_element.CreateValueFromAddress("list_node", self.next_element.GetValueAsUnsigned(), self.node_type)
            self.element_cache.append(node.GetValueForExpressionPath(".m_value"))

            # Move to the next element in the list
            self.next_element = self.next_element.GetValueForExpressionPath(".m_next")
            if not self.next_element.GetValueAsUnsigned(0):
                self.next_element = None

        # The value has been found so return it
        value = self.element_cache[index]
        return self.valobj.CreateValueFromData(f'[{index}]', value.GetData(), value.GetType())


    def update(self):
        try:
            logger = lldb.formatters.Logger.Logger()
            # EvaluateExpression is more reliable when it comes to retrieving data
            # but it is slow ~50-100 ms per expression
            # https://llvm.org/devmtg/2021-11/slides/2021-Buildingafaster-expression-evaluatorforLLDB.pdf
            data_storage = self.valobj.EvaluateExpression("m_data")
            hash_table_list = data_storage.GetValueForExpressionPath(".m_list")
            list_type = hash_table_list.GetType()
            # Get the node type stored within the hash_table list type
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

            self.num_elements = hash_table_list.GetValueForExpressionPath(".m_numElements").GetValueAsUnsigned()
            logger >> f'Num elements = {self.num_elements}'

            self.max_load_factor = data_storage.GetValueForExpressionPath(".m_max_load_factor").GetValueAsUnsigned()

            hash_table_bucket_vector = data_storage.GetValueForExpressionPath(".m_vector")
            self.bucket_count = hash_table_bucket_vector.GetValueForExpressionPath(".m_last").GetValueAsUnsigned() \
                - hash_table_bucket_vector.GetValueForExpressionPath(".m_start").GetValueAsUnsigned()
            logger >> f'Bucket count = {self.bucket_count}'

            # Store off the pointer to the first element in the hash_table list of nodes
            self.next_element = hash_table_list.GetValueForExpressionPath(".m_head.m_next") if self.num_elements else None

            # Cache SB Value containing the map value type and the hash value
            self.element_cache = []
        except Exception as e:
            logger >> f'exception: {e}'


def hash_table_summary(valobj, dict):
    prov = hash_table_synthetic_provider(valobj, None)
    # Call update() to initialize the synthetic provider address and size members
    prov.update()
    return "size=" + str(prov.num_children())


class contiguous_range_synthetic_provider_base(lldb.SBSyntheticValueProvider):
    '''
    Base class which is used to implements the general methods for synthetic provider
    for the AZStd::vector and AZStd::fixed_vector classes
    '''
    def __init__(self, valobj, dict):
        self.valobj = valobj
        # Stores SBValue to the start of the contiguous range
        self.start = None
        # Stores either the SBValue to the pointer just past the last valid element in the contigous range
        # or the size value
        self.last_or_size = None


    def has_children(self):
        return self.num_children() != 0


    def get_child_index(self, name):
        try:
            return int(name.lstrip("[").rstrip("]"))
        except:
            return -1


class vector_synthetic_provider(contiguous_range_synthetic_provider_base):
    '''
    Provides a synthetic provider for the AZStd::vector class
    '''

    def __init__(self, valobj, dict):
        super(vector_synthetic_provider, self).__init__(valobj, dict)

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()

        if not self.start:
            logger >> "update() method needs to be called at least once to populate start and last address"
            return 0

        start_address = self.start.GetValueAsUnsigned(0)
        last_address = self.last_or_size.GetValueAsUnsigned(0)

        # Make sure start is before the last address
        if start_address >= last_address:
            return 0


        byte_size = last_address - start_address

        if not self.value_type:
            return 0

        value_type_size = self.value_type.GetByteSize()

        # Make sure the value type is at least 1 byte
        if value_type_size == 0:
            return 0

        # verify that the valid address range is divisble by the size of the value type
        if byte_size % value_type_size != 0:
            return 0

        num_children = byte_size // value_type_size
        return num_children


    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        logger >> "Retrieving child " + str(index)
        if index < 0 or index >= self.num_children():
            return None

        try:
            value_type_size = self.value_type.GetByteSize()
            offset = index * value_type_size
            return self.start.CreateChildAtOffset("[" + str(index) + "]", offset, self.value_type)
        except Exception as e:
            logger >> f'exception: {e}'


    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            self.start = self.valobj.GetValueForExpressionPath(".m_start")
            self.last_or_size = self.valobj.GetValueForExpressionPath(".m_last")
            # Store the value type of the vector
            self.value_type = self.start.GetType().GetPointeeType()
        except Exception as e:
            logger >> f'exception: {e}'


def vector_summary(valobj, dict):
    prov = vector_synthetic_provider(valobj, None)
    # Call update() to initialize the synthetic provider address and size members
    prov.update()
    return "size=" + str(prov.num_children())


class fixed_vector_synthetic_provider(contiguous_range_synthetic_provider_base):
    '''
    Provides a synthetic provider for the AZStd::fixed_vector class
    '''

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()

        if not self.start:
            logger >> "update() method needs to be called at least once to populate start address and size"

        start_address = self.start.GetValueAsUnsigned(0)
        size = self.last_or_size.GetValueAsUnsigned(0)

        return size


    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        logger >> "Retrieving child " + str(index)
        if index < 0 or index >= self.num_children():
            return None

        try:
            value_type_size = self.value_type.GetByteSize()
            offset = index * value_type_size
            return self.start.CreateChildAtOffset("[" + str(index) + "]", offset, self.value_type)
        except Exception as e:
            logger >> f'exception: {e}'


    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            self.capacity = self.valobj.GetTemplateArgumentType(1)
            if self.capacity.GetAsUnsigned(0) > 0:
                self.start = self.valobj.GetValueForExpressionPath(".m_data")
                self.last_or_size = self.valobj.GetValueForExpressionPath(".m_size")
            else:
                # if the fixed_vector capacity is 0, then there are no elements to display
                # and its size is 0
                self.start = 0
                self.last_or_size = 0

            self.value_type = self.valobj.GetTemplateArgumentType(0)

        except Exception as e:
            logger >> f'exception: {e}'


def fixed_vector_summary(valobj, dict):
    prov = fixed_vector_synthetic_provider(valobj, None)
    # Call update() to initialize the synthetic provider address and size members
    prov.update()
    return "size=" + str(prov.num_children())

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

    debugger.HandleCommand('type summary add -x "^AZStd::hash_table<.+>$" -F o3destd_lldb.hash_table_summary --category o3de')
    debugger.HandleCommand('type summary add -x "^AZStd::unordered_map<.+>$" -F o3destd_lldb.hash_table_summary --category o3de')
    debugger.HandleCommand('type summary add -x "^AZStd::unordered_set<.+>$" -F o3destd_lldb.hash_table_summary --category o3de')
    debugger.HandleCommand('type summary add -x "^AZStd::unordered_multimap<.+>$" -F o3destd_lldb.hash_table_summary --category o3de')
    debugger.HandleCommand('type summary add -x "^AZStd::unordered_multiset<.+>$" -F o3destd_lldb.hash_table_summary --category o3de')

    # Import the AZStd::vector summary
    debugger.HandleCommand('type synthetic add -x "^AZStd::vector<.+>$" --python-class o3destd_lldb.vector_synthetic_provider --category o3de')
    debugger.HandleCommand('type summary add -x "^AZStd::vector<.+>$" -F o3destd_lldb.vector_summary --category o3de')

    # Import the AZStd::fixed_vector summary
    debugger.HandleCommand('type synthetic add -x "^AZStd::fixed_vector<.+>$" --python-class o3destd_lldb.fixed_vector_synthetic_provider --category o3de')
    debugger.HandleCommand('type summary add -x "^AZStd::fixed_vector<.+>$" -F o3destd_lldb.fixed_vector_summary --category o3de')

    # Enable the o3de category for formatters
    debugger.HandleCommand('type category enable o3de')
