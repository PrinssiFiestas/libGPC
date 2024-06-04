import gdb
import gdb.printing

class GPStringPrinter():
    def __init__(self, val):
        self.val = val

    def to_string(self):
        T_str_data      = gdb.lookup_type("GPStringHeader")
        val_as_data_ptr = self.val.cast(T_str_data.pointer())
        str_data        = (val_as_data_ptr - 1).dereference()

        T_cstr       = gdb.lookup_type("char").pointer()
        val_contents = self.val.cast(T_cstr).string("utf-8", length = str_data["length"])
        val_contents = str(val_contents).encode("unicode_escape").decode()
        return "\"" + val_contents + "\", " + str(str_data)

def gp_str_lookup_function(val):
    if str(val.type) == "GPString":
        return StringPrinter(val)
    return None

def gp_const_str_lookup_function(val):
    if str(val.type) == "const GPString":
        return StringPrinter(val)
    return None

gdb.pretty_printers.append(str_lookup_function)
gdb.pretty_printers.append(const_str_lookup_function)
