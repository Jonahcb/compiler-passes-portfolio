from cfg import *
from form_blocks import form_blocks
import sys
import json

def defined(instrs, args):
  
  # Break the program up into blocks
  blocks = list(form_blocks(instrs))

  # Form the cfg
  #our_block_map = add_entry(add_terminators((block_map(blocks))))
  our_block_map = (block_map(blocks))
  
  # Add terminators
  add_terminators(our_block_map)

  # Add entry
  add_entry(our_block_map)

  preds, succs = edges(our_block_map)

  into = {block: [] for block in our_block_map}
  out = {block: [] for block in our_block_map}

  # Initialize entry and exit nodes with inputs to the function
  arg_list = []

  # Always grab first key
  first_key = next(iter(our_block_map))

  for arg in args:
    arg_list.append(arg['name'])

  into[first_key] = arg_list
  out[first_key] = arg_list


  worklist = list(our_block_map.keys())

  # While worklist is not empty:
  while worklist != []:

    # Always choose first block
    block = worklist.pop(0)

    # Add what is going in
    for pred in preds[block]:
      # Append the reaching def. of the predessor to the list at index b
      into[block].extend(out[pred])
      
      # get rid of duplicates
      into[block] = list(set(into[block]))
      

    # Calculate what is leaving
    new_def = []
    for instr in our_block_map[block]:
      # If a new variable is defined then it survives
      if instr.get('dest'):
        new_def.append(instr['dest'])

    outvalue = []

    # all definitions that came into this block and definitions in this block survive
    for new_definition in new_def:
      outvalue.append(new_definition)
    
    # add definitions from predecessors
    for new_definition in into[block]:
      if new_definition not in outvalue:
        outvalue.append(new_definition)


    if set(outvalue) != set(out[block]):
      # add any blocks that this block is a predecessor of back to the worklist
      for successor in succs[block]:
        if successor not in worklist:
          worklist.append(successor)
    
    out[block] = outvalue
  
  for block in list(our_block_map.keys()):
    in_vars = ",".join(into[block]) if into[block] else  "∅"
    out_vars = ",".join(out[block]) if out[block] else  "∅"
  return into, out

if __name__ == "__main__":
  prog = json.load(sys.stdin)
  
  # process the first function
  instrs = prog["functions"][0]["instrs"]

  # need to bring in its arguments as well
  args = prog["functions"][0]["args"]

  into, out = defined(instrs, args)
