
# See the following pages for information on how to write formatters for lldb
# https://lldb.llvm.org/use/variable.html#finding-formatters-101
# https://lldb.llvm.org/python_api/lldb.SBValue.html#lldb.SBValue
# https://github.com/llvm/llvm-project/blob/main/lldb/examples/synthetic/libcxx.py
import enum
import lldb
import lldb.formatters.Logger

def string_summary(valobj: lldb.SBValue, internal_dict):
    # Uncomment for debug output
    #lldb.formatters.Logger._lldb_formatters_debug_level = 3

    logger = lldb.formatters.Logger.Logger()
    logger >> f"{valobj.GetType()} summary"


    def element_type_string_summary(valobj: lldb.SBValue):
        # Grab the SBProcess object which is needed to read a c-string from memory
        process = valobj.GetProcess()

        compressed_pair_storage = valobj.EvaluateExpression("m_storage")
        # The first child of the compressed pair is the AZStd::basic_string::storage union
        storage_inst = compressed_pair_storage.GetChildAtIndex(0)
        storage_element = storage_inst.GetChildMemberWithName("m_element")
        short_string_storage = storage_element.GetChildMemberWithName("m_shortData")

        # Stores the result of summary string
        result_string = ''

        # Check if the Short String optimization is active
        is_sso_active_member = short_string_storage.EvaluateExpression("m_packed.m_ssoActive")
        is_sso_active = int(is_sso_active_member.GetValueAsUnsigned())
        if is_sso_active:
            # Read the string size and memory address
            string_size = int(short_string_storage.EvaluateExpression("m_packed.m_size").GetValueAsUnsigned())
            string_address = short_string_storage.EvaluateExpression(f'm_buffer').AddressOf()
        else:
            # Short String optimization is not active
            try:
                target = valobj.GetTarget()
                allocated_string_type = target.FindFirstType('AllocatedStringData')
                allocated_storage = storage_element.EvaluateExpression("m_allocatedData")
            except Exception as e:
                return f'error: {e}'

            string_size = allocated_storage.EvaluateExpression("m_size").GetValueAsUnsigned()
            string_address = allocated_storage.EvaluateExpression("m_data")

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


class string_synthetic_provider(lldb.SBSyntheticValueProvider):
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj

    def num_children(self) -> int:
        return 1

    def has_children(self) -> bool:
        return num_children != 0

    def get_child_index(self, name):
        if name == '0':
            return 0
        return -1

    def get_child_at_index(self, index):
        if index == 0:
            return 'foo'
        return None

    def update(self):
        return True


def __lldb_init_module(debugger, dict):
    # Import the AZStd::basic_string summary
    debugger.HandleCommand('type summary add -n "AZStd::string" -F o3destd_lldb.string_summary --category o3de')
    debugger.HandleCommand('type summary add -x "^AZStd::basic_string<.+>$" -F o3destd_lldb.string_summary --category o3de')

    # Import the AZStd::basic_string synthetic children provider
    # Commented out as we don't need synthetic children to visualize a string
    # debugger.HandleCommand('type synthetic add "^AZStd::basic_string<.+>$" --python-class o3destd_lldb.string_synthetic_provider --category o3de')

    # Enable the o3de category for formatters
    debugger.HandleCommand('type category enable o3de')
