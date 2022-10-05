func towerOfHanoi(n: real, s: string, d: string, a: string) -> real {
    if n = 1 {
        output "Disc 1: from " + (str s) + " to " + (str d);
        return 1;
    }

    towerOfHanoi(n-1, s, a, d);
    output "Disc " + (str n) + ": from " + (str s) + " to " + (str d);
    towerOfHanoi(n-1, a, d, s);
}

towerOfHanoi(5, "A", "B", "C");