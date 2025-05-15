int callee(int x, int y) {
    int z = x + y;
    return z;
}

void caller() {
    int a = 0x33;
    int b = 0x44;
    int c = callee(a, b);
}

int main() {
    caller();
}