#include <stdio.h>

struct { const char* name; int age; } person;

int main() {
    person.name = "Ada";
    person.age = 20;
    printf("%s %d", person.name, person.age);
    return 0;
}
