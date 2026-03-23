/*
 * FILE: main.cplus
 * DESC.: example cplus v1 program — Person module entry point
 * NOTE: v1 is identity transpiler; this is valid C23
 */

#include "person.hplus"

int main(void) {
    Person alice;
    person_set_name(&alice, "Alice");
    person_set_age(&alice, 30);

    Person bob;
    person_set_name(&bob, "Bob");
    person_set_age(&bob, 25);

    person_print(&alice);
    person_print(&bob);

    return 0;
}
