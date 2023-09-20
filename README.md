# The Atlas Programming Language
**P.S. Name may not be final - but it is Atlas for now**
Atlas is aimed to be a statically typed compiled language and is aimed to be very similar to C (in-fact it compiles to C). This is being made as a learning exercise and is currently under development.

## What works now?
Currently you can solve some extremely basic problems, below is from problem 1 from Project Euler.
```odin
// test/euler/problem1.atl
include "std.atl"

main fn() -> i64 {
    :: sum i64 = 0
    for :: i i64 = 0; i < 10; i = i + 1 {
        if i % 3 == 0 {
	        sum = sum + i
	    } else if i % 5 == 0 {
	        sum = sum + i
        }
    }
    putint(sum)
    -> 0
}

```

## Planned Design
Below are some sample programs that have been written to encapsulate what kind of syntax Atlas will have with a full set of features (Note: this is just a proof of concept, see below for what works currently):

```odin
// variable.atl
test_function fn(x i32) -> i32, f32 {
    -> x, x as f32 // -> arrow used for return, as keyword used for typecasting
}

main fn() {
    :: a i64 = 69 // Variables are declared with a double colon
    // OR
    :: a = 69 // similar to Go and Rust there is type inference 
    a = 70 // variable assignment is the same as expected

    :: a, b = test_function(3) // a, b are assigned 3 and 3.0 respectively
}
```

```odin
// vector.atl
Vector3 type {
    x i32
    y i32
    z i32
}
// OR
Vector3 type {
    x, y, x i32
}

main fn() {
    // Most likely the common way to instantiate variables
    :: test = Vector3 {1, 2, 3}
    // OR
    :: test Vector3 = Vector3 {1, 2, 3}
    // OR
    :: test Vector3
    test.x = 1
    test.y = 2
    test.z = 3
}
```

## TODO
The Compiler has just started development so it doesn't do much:
- [X] Parser 
- [ ] AST Generation
  - [ ] Handle characters and strings
  - [ ] Function Nodes
    - [X] Parameters
    - [X] Single Return Types
    - [ ] Multiple Return Types
    - [X] Block
  - [X] Block Nodes
  - [X] Variable Nodes
  - [X] Variable Declaration Nodes
  - [X] Assignment Nodes
  - [X] Call Nodes
    - [X] Arguments
- [X] Command line options
- [X] include - to be deprecated for import later
- [ ] import
- [ ] Type Checking
- [ ] Type inference
- [ ] Codegen (C)
  - [X] Constants
  - [X] Variable Declaration
  - [X] Function Declaration
  - [X] Assignments
  - [X] Return
  - [X] If Statements
  - [X] For Loops
  - [ ] Completely Working Expressions
  - [ ] Implement stdlib
    - [X] putchar
    - [ ] malloc
    - [ ] free
- **Note:** if the option is ticked, it means that it is working but doesn't necessarily mean it is complete from a functionality POV 
