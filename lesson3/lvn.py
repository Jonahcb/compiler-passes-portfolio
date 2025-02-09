import json
import sys
import itertools
from form_blocks import form_blocks

# List of opcodes for effect operations (maybe don't actually need this)
EFFECT_OPS = ['br', 'jmp', 'ret', 'call', 'print', 'store', 'alloc', 'free']

# Global variable for numbering system
n = 0


def flatten(ll):
    """Flatten an iterable of iterable to a single list.
    """
    return list(itertools.chain(*ll))

# List of commutative operations
COMMUTE_OPS = ["add", "mul"] # should I add 'and' and 'or'

# Dict of foldable operation and their lambda functions
FOLDABLE_OPS = {"add": lambda x, y: x + y,
                "sub": lambda x, y: x - y,
                "mul": lambda x, y: x * y,
                "div": lambda x, y: x // y,
                "eq": lambda x, y: x == y,
                "lt": lambda x, y: x < y,
                "gt": lambda x, y: x > y,
                "le": lambda x, y: x <= y,
                "ge": lambda x, y: x >= y,
                "not": lambda x: not x,
                "and": lambda x, y: x and y,
                "or": lambda x, y: x or y}


# Dead code elimination
def trivial_dce_pass(func):
  """Remove instructions from `func` that are never used as arguments
  to any other instruction. Return a bool indicating whether we deleted
  anything.
  """
  # Form blocks from the function
  blocks = list(form_blocks(func['instrs']))

  # Reset flag
  changed = False

  # Initialize set to hold used variables
  used = set()

  # Add every variable that appears as an argument to our used set (starting assumption)
  for block in blocks:
      for instr in block:
          for arg in instr.get('args', []):
              used.add(arg)


 # Remove all instructions that assign to an unused variable
  for block in blocks:
      to_remove = []
      for instr in block:
         if instr.get('dest') is not None:
             if instr['dest'] not in used:
                 changed = True
                 to_remove.append(instr)
    # Remove all accumulated instructions
      for instr in to_remove:
          block.remove(instr)
    
  # Reassemble the function.
  func['instrs'] = flatten(blocks)

  return changed



def drop_killed_local(blocks):
    """Delete instructions in a single block whose result is unused
    before the next assignment. Return a bool indicating whether
    anything changed.
    """
    for block in blocks:
        # Initialize convergence flag
        changed = True
        # Stop if the block is not changing or is empty
        while (changed and block != []):
            # Reset change flag
            changed = False
            # init a dict to hold variables that have already been defined
            defined_vars = {}

            # init a set to hold variables that are used before being reassigned
            used = set()

            # Initialize a list of instructions to delete
            to_remove = []

            # loop through the instructions
            for instr in block:
                
                # accumulate all the used variables
                for arg in instr.get('args', []):
                    if arg in  defined_vars:
                        # if we use a variable, take it out of our defined_vars dict
                        del defined_vars[arg]
                
                # check for any reassignments before use
                if instr.get('dest') is not None:
                    # The variable has not been used and it is reassigned
                    if instr['dest'] not in used and instr['dest'] in defined_vars:
                        # Mark the instruction for removal
                        to_remove.append(instr)
                        # Set change flag
                        changed = True
                
            # Loop to remove all the instructions we said we need to remove
            for instr in to_remove:
                block.remove(instr)


def generate_num():
    global n
    n = n + 1
    return n

def fold(value, var2num, num2val):
    # Only fold if the operation is foldable
    if value[0] in FOLDABLE_OPS:
        # Attempt to fold under the assumption that both arguments are constants
        try:
            # if one of the arguments is not a number, just give up
            if not (isinstance(num2val[value[1][0]][1], (int, float)) and isinstance(num2val[value[1][1]][1], (int, float))):
              raise TypeError

            # Apply the operation to both arguments
            new_val = FOLDABLE_OPS[value[0]](num2val[value[1][0]][1], num2val[value[1][1]][1])
            return ('const', new_val)
        # If one of the values is not a constant or is an unknown input
        except (KeyError, TypeError):
         # give up and return
            return None
        
def read_first(block):
    # Im not sure if this works
    used = set()
    assigned = set()

    for instr in block:
        if instr.get('args'):
            for arg in instr['args']:
                if arg not in assigned:
                    used.add(arg)
        if instr.get('dest'):
            assigned.add(instr['dest'])
    
    return used


# main lvn function
def local_value_numbering(block):
   
  # Initialize multiple tables for easy lookup
  num2var = {}
  val2num = {}
  num2val = {}

  # Initialize variable cloud.
  var2num = {}

  # Add all inputs to our tables
  used = read_first(block)
  for var in used:
    value = ('unknown')
    num = generate_num()

    # Add row to lvn table
    num2var[num] = var
    val2num[value] = num
    var2num[var] = num
    num2val[num] = value

  # Loop through each instruction.
  for instr in block:

    # Ignore call instructions (I am a bit confused by this)
    if instr.get('dest') is None:
        continue

    # Initalize list to hold arguments
    args = []

    # Constant assignments don't have args
    if instr['op'] == 'const':
        value = ('const', instr['value'])

    # Constant propagation: 
    elif instr['op'] == 'id':
        # Access the only argument and find its current representation in our table
        arg = instr['args'][0]
        num = var2num[arg]

       # If that points to a constant then just assign the constant, else just compute its value
        if num2val[num][0] == 'const':
            # Reconstruct the instruction
            instr.update({"op": "const", "value": num2val[num][1]})

            # Calculate the value in our table for this new instruction
            value = ('const', instr['value'])
        else:
            value = ('id', (var2num[arg],))
    else:
        # Build tuple of the actual identifiers for the args
        for arg in instr["args"]:
           num = var2num[arg]  
           # Handle copy propagation by identifying args that point to ('id', _) values (I don't think this is copy propogation)
           if num2val[num][0] == 'id':
              # Take the nested identifier in the id 
              num = num2val[num][1][0]
           # append the raw argument
           args.append(num)

        # All arguments must be in our cloud so use the unique identifier they point to

        # Canonicalize args to exploit commutativity
        if (instr['op'] in COMMUTE_OPS):
            args.sort()

        args = tuple(args)
        value = (instr['op'], args )

    # Before altering our tables, let's try to fold our instruction
    new_val = fold(value, var2num, num2val)

    # If we successfully folded our instr to a constant, find the constant in our table
    if new_val is not None:
        num = val2num.get(new_val)

        # Change the instruction accordingly (may need to fix how it assigns the type)
        instr.update({"op": "const", "type": 'int', "value": new_val[1]})
        del instr['args']

        # Generate new value (replaces the old value, idk if this is correct way to do it)
        value = ('const',new_val[1])
    
    # if we didn't fold it just proceed as usual
    else:
        num = val2num.get(value)

    # If the value has been computed before just point to the identifier (num)
    if num is not None:
        var2num[instr['dest']] = num
    else:
        # Generate new number
        num = generate_num()

        # Add row to lvn table
        num2var[num] = instr['dest']
        val2num[value] = num
        var2num[instr['dest']] = num
    
    num2val[num] = value

    # Reconstruct the instruction
    # Constant assignments don't change
    if instr['op'] == 'const':
        continue
    else:
       # Use cloud and num to change the arguments
        instr['args'] = [num2var[new_num] for new_num in num2val[num][1]]



if __name__ == '__main__':
    prog = json.load(sys.stdin)
    for func in prog['functions']:
        blocks = list(form_blocks(func['instrs']))

        for block in blocks:
          local_value_numbering(block)
        
        func['instrs'] = flatten(blocks)

        trivial_dce_pass(func)

        

    json.dump(prog, sys.stdout, indent=2, sort_keys=True)