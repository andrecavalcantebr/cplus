/*
 * FILE: person.cplus
 * DESC.: example cplus v1 program — Person module implementation
 * NOTE: v1 is identity transpiler; this is valid C23
 */

#include "person.hplus"

#include <stdio.h>
#include <string.h>

void person_set_name(Person *p, const char *name) {
    (void)strncpy(p->name, name, sizeof(p->name) - 1U);
    p->name[sizeof(p->name) - 1U] = '\0';
}

void person_set_age(Person *p, int age) {
    p->age = age;
}

const char *person_get_name(const Person *p) {
    return p->name;
}

int person_get_age(const Person *p) {
    return p->age;
}

void person_print(const Person *p) {
    printf("Person { name: \"%s\", age: %d }\n", p->name, p->age);
}
