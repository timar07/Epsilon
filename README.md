# ![Epsilon logo](./assets/logo.png#gh-dark-mode-only) ![Epsilon logo](./assets/logo-light.png#gh-light-mode-only)
Epsilon is a programming language with tree-walking interpreter written in C.

## Installing
`$ make`

## Usage
`$ epsilon <filename>.e`

## Examples
```lua
-- Factorial
func fact(n: real) -> real {
    return 1 if n <= 1 else n * fact(n-1);
}
```
```lua
-- Fibonacci sum
func fib(n: real) -> real {
    return 1 if n <= 2 else fib(n-1) + fib(n-2);
}
```