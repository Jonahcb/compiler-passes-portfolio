#!/usr/bin/env python3
import sys
import json
from cfg import block_map, successors, add_terminators, add_entry, reassemble
from form_blocks import form_blocks


def extract_types(function):
    # Start with the function parameters
    type_map = {}
    for param in function.get("args", []):
        name = param["name"]
        ty   = param["type"]
        type_map[name] = ty

    # Now add every instruction result
    for instr in function.get("instrs", []):
        if "dest" in instr:
            dest_name = instr["dest"]
            dest_type = instr["type"]
            type_map[dest_name] = dest_type

    return type_map

def insert_get_phi(block, unique_name ,var, types):
    """
    Insert a 'get' phi node for variable `var` into the basic block `block`.
    """
    
    phi_node = {"op": "get", "dest": var, "type": types[var]}

    # Copy value into unique name
    copy_value_instr = {"op": "id", "dest": unique_name, "args": [var], "type": types[var]}

    # If the block starts with a label, insert after it; otherwise at start
    if block and isinstance(block[0], dict) and "label" in block[0]:
        if len(block) > 1:
            block.insert(1, phi_node)
            block.insert(2, copy_value_instr)
        else:
            block.append(phi_node) 
            block.append(copy_value_instr) 
    else:
        block.insert(0, phi_node) 
        block.insert(1, copy_value_instr)

    # To be used to replace all occurrences
    return unique_name


def insert_set_phi(block, unique_name, var):
    """
    Append a 'set' phi node for variable `var` to the end of the basic block `block`.
    """
    phi_node = {"op": "set", "args": [var, unique_name]}
    # If the last instr is a terminator, insert *just before* it
    if block and block[-1].get("op") in ("br", "jmp", "ret"):
        block.insert(len(block) - 1, phi_node)
    else:
        block.append(phi_node)


def to_ssa(instrs, types):
    """
    Convert a flat list of instructions `instrs` into SSA form by:
      1. Forming basic blocks
      2. Inserting 'set' phi nodes at block ends
      3. Inserting 'get' phi nodes at block entries (except entry block)
    Returns a list of SSA-transformed blocks.
    """
    # Form CFG
    blocks = block_map(form_blocks(instrs))
    add_entry(blocks)
    add_terminators(blocks)

    # Collect all variable names appearing as destinations
    variables = {instr["dest"] for block in blocks.values() for instr in block if "dest" in instr}
    variables = sorted(variables)


    # Insert undef for every variable in entry block to function
    for var in variables:
        blocks["entry"].append({"op":"const", "dest": var, "type":"undef"})

    # Insert 'get' phi at entry of each block (except first entry block) and create unique var name to use
    for block_name, block in blocks.items():
        # Skip entry block if it begins with a label
        if block_name != "entry":
            
            # Insert a 'get' for every variable that is written to
            for var in variables:
                unique_name = f"{var}.{block_name}"
                # Replace all uses of that variable with the unique name
                for instr in block:
                    if "args" in instr:
                        # form new arguements list
                        instr["args"] = [unique_name if arg == var else arg for arg in instr["args"]]
    
                    if "dest" in instr:
                        if instr["dest"] == var:
                            instr["dest"] = unique_name

                insert_get_phi(block, unique_name, var, types)  
         # Insert 'set' phi at end of each block for each var
            for var in variables:
              unique_name = f"{var}.{block_name}"
              insert_set_phi(block, unique_name, var)

        

    return blocks


def main():
    prog = json.load(sys.stdin)
    for fn in prog.get("functions", []):
        types = extract_types(fn)
        # 1) SSA‐transform the body
        ssa = to_ssa(fn["instrs"], types)
        fn["instrs"] = reassemble(ssa)

        # 2) Bootstrap entry: undef non-args, then set each var→entry slot
        entry_blk = next(iter(ssa))
        args = {a["name"] for a in fn.get("args", [])}

        prep = []
        for var, ty in types.items():
            if var not in args:
                prep.append({"op": "undef", "dest": var, "type": ty})
                prep.append({"op": "undef", "dest": f"{var}.{entry_blk}", "type": ty})
        prep += [
            {"op": "set", "args": [var, f"{var}.{entry_blk}"]}
            for var in types if var not in args
        ]

        fn["instrs"] = prep + fn["instrs"]


    with open("transformed.json", "w") as out:
        json.dump(prog, out, indent=2)
    print("Transformed program written to 'transformed.json'", file=sys.stdout)


if __name__ == "__main__":
    main()
