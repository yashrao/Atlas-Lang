# The Atlas Programming Language
**P.S. Name may not be final - but it is Atlas for now**
Atlas is aimed to be a statically typed compiled language and is aimed to be very similar to C. This is being made as a learning exercise and is currently under development.

The Compiler has just started development so it doesn't do anything as of yet:
- [X] Parser 
- [ ] AST Generation
  - [ ] Function Nodes
    - [ ] Parameters
    - [X] Return Types
    - [X] Block
  - [X] Block Nodes
  - [X] Variable Nodes
  - [X] Variable Declaration Nodes
  - [X] Assignment Nodes
  - [X] Call Nodes
    - [ ] Arguments
- [ ] Type Checking
- [ ] Codegen (LLVM)
  - [X] Constants
  - [X] Variable Declaration
  - [X] Function Declaration
  - [ ] Assignments
  - [X] Return
  
- **Note:** if the option is ticked, it means that it is working but doesn't necessarily mean it is complete from a functionality POV 

## Planned Design
Below are some sample programs that have been written to encapsulate what kind of syntax Atlas will have

Note: These are not working examples due to repeating variable declarations
```C
// variable.atl
test_function(int x) -> int, float {
    -> x, x as float // -> arrow used for return, as keyword used for typecasting
}

main () {
    int :: a = 69 // Variables are declared with a double colon
    // OR
    :: a = 69 // similar to Go and Rust there is type inference 
    a = 70 // variable assignment is the same as expected

    :: a, b = test_function(3) // a, b are assigned 3 and 3.0 respectively
}
```

```C
// vector.atl
type Vector3 {
    i32 :: x
    i32 :: y
    i32 :: z
}
// OR
type Vector3 {
    i32 :: x, y, x
}

main () {
    // Most likely the common way to instantiate variables
    :: test = Vector3 {1, 2, 3}
    // OR
    Vector3 :: test = Vector3 {1, 2, 3}
    // OR
    Vector3 :: test
    test.x = 1
    test.y = 2
    test.z = 3
}
```

## What works now?
This incredibly useless and simple program demonstrates what is working at the moment.
```C
// simple.atl
random_function () -> int {
    -> 10
}

main () {
    i32 :: test = 1 * 2 + 4 * 3
    i64 :: test2 = 4
    int :: test2 = random_function() + 1 + test2 // int is treated as i64 by default 
    -> 5
}
```
