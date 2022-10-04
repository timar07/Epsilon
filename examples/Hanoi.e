func towerOfHanoi(n: real, s: string, d: string, a: string) -> real {
    if n = 1 {
        output "Move disk 1 from source " + s + " to destination " + d;
        return 1;
    }

    towerOfHanoi(n-1, s, a, d);
    output "Move disk" + string(n) + "from source " + s + " to destination " + d;
    towerOfHanoi(n-1, a, d, s);
}

towerOfHanoi(5, "A", "B", "C");