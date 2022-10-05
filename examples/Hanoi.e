func towerOfHanoi(n: real, s: string, d: string, a: string) -> real {
    if n = 1 {
        output "Move disk 1 from source " + (str s) + " to destination " + (str d);
        return 1;
    }

    towerOfHanoi(n-1, s, a, d);
    output "Move disk " + (str n) + " from source " + (str s) + " to destination " + (str d);
    towerOfHanoi(n-1, a, d, s);
}

towerOfHanoi(5, "A", "B", "C");