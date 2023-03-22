# The Atlas Programming Language
**P.S. Name may not be final - but it is Atlas for now**
Atlas is aimed to be a statically typed compiled language and is aimed to be very similar to C. This is being made as a learning exercise and is currently under development.

The Compiler has just started development so it doesn't do anything as of yet:
- [X] Parser 
- [ ] AST Generation
  - [ ] Function Nodes
    - [ ] Arguments
    - [ ] Return Types
    - [X] Block
  - [X] Block Nodes
  - [X] Variable Nodes
  - [X] Variable Declaration Nodes
  - [ ] Assignment Nodes
  - [ ] Call Nodes
- [ ] Type Checking
- [ ] Codegen (either LLVM or NASM)

## Planned Design
Below are some sample programs that have been written to encapsulate what kind of syntax Atlas will have

Note: These are not working examples due to repeating variable declarations
```C
// variable.atl
test_function(int x) -> int, int {
    -> x, x as f32 // -> arrow used for return, as keyword used for typecasting
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
