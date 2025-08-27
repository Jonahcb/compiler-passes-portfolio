#!/usr/bin/env python3
import sys
import json
from cfg import block_map, successors, add_terminators, add_entry, reassemble
from form_blocks import form_blocks


def from_ssa(instrs):

    # Reconstruct CFG and ensure well-formed blocks
    blocks = block_map(form_blocks(instrs))
    add_entry(blocks)
    add_terminators(blocks)

    # Build a map of variable -> type from original instrs
    type_map = {}
    for instr in instrs:
        if "dest" in instr and "type" in instr:
            type_map[instr["dest"]] = instr["type"]

    # Process each block
    for blk_name, block in blocks.items():
        new_block = []
        for instr in block:
            op = instr.get("op")

            # Drop `get` instructions entirely
            if op == "get":
                continue

            # Replace `set x y` with `id` copy
            if op == "set":
                x, y = instr["args"]
                id_instr = {"op": "id", "dest": x, "args": [y]}
                # Preserve type if known
                if x in type_map:
                    id_instr["type"] = type_map[x]
                new_block.append(id_instr)
                continue

            # Otherwise, keep instruction as-is
            new_block.append(instr)

        # Replace block instructions
        blocks[blk_name] = new_block

    return blocks


def main():
    # Read JSON program from stdin
    program = json.load(sys.stdin)
    functions = program.get("functions", [])
    if not functions:
        print("Error: No functions found in the JSON input.", file=sys.stderr)
        sys.exit(1)

    # Process each function
    for function in functions:
        instrs = function.get("instrs")
        if instrs is None:
            name = function.get("name", "<unknown>")
            print(f"Warning: Function '{name}' has no instructions.", file=sys.stderr)
            continue

        # Lower from SSA
        blocks = from_ssa(instrs)
        # Flatten back to instr list
        nonssa_instrs = reassemble(blocks)
        function["instrs"] = nonssa_instrs

    # Write out the transformed program
    output_file = "nonssa.json"
    with open(output_file, "w") as f:
        json.dump(program, f, indent=4)
    print(f"Transformed program written to '{output_file}'", file=sys.stdout)


if __name__ == "__main__":
    main()