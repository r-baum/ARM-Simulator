#include <stdio.h>

int mystrlen(char *s) {
    char *t = s;
    while (*s != 0) {
        s++;
    }
    return s - t;
}

/* Create a space to play some games with the actual assembly instructions */

int dummy() {
    int a = 1;
    return a;
}

void test_strlen() {
    char s[5]; // = "blah";

    s[0] = 'b';
    s[1] = 'l';
    s[2] = 'a';
    s[3] = 'h';
    s[4] = '\0';
    mystrlen(s);
}

int main() {
    char *a = "Hello World!\n";
    char *b = "blah";

    test_strlen();

    printf("strlen(\"%s\") is %d\n", a, mystrlen(a));
    printf("strlen(\"%s\") is %d\n", b, mystrlen(b));
}
