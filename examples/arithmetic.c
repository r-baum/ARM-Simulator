void arithmetic() {
    unsigned int x = 0x7;
    unsigned int y = x + 0x5;
    unsigned int z = y - x;
    unsigned long a = 0x4;
    unsigned long b = a * 0x3;
    unsigned long c = b / 0x2;
}

int main() {
    arithmetic();
}