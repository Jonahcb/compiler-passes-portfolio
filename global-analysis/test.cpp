
// “foo” has:
//   1) entry block (“alloca” + branch to if.header)  
//   2) if.header → splits to if.then or if.else  
//   3) if.then and if.else both jump to merge  
//   4) merge block dominates the loop.header  
//   5) loop.header dominates loop.body and loop.exit  
//   6) loop.exit jumps to return block
int foo(int x) {
    int y = 0;                // entry

    // if.header
    if (x > 0) {
        y = 1;                // if.then
    } else {
        y = -1;               // if.else
    }

    // merge
    y += 10;

    // loop.header
    int sum = 0;
    for (int i = 0; i < 3; ++i) {
        sum += y * i;         // loop.body
    }                        // loop.exit

    return sum;               // return block
}

int main() {
    return 0;
}
