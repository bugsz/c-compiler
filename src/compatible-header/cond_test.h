char* barf(double x) {
    if (x < 0.0) {
        return "foo";
    } else if (x == 0.0) {
        return "bar";
    } else {
        return "baz";
    }
}