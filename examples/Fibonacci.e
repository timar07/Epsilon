func fib(a: real) -> real {
    return 1 if a <= 2 else fib(a-1) + fib(a-2);
}

output fib(10);
