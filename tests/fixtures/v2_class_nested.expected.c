typedef struct Address Address;
struct Address {
    char street[128];
    int  zip;
};

typedef struct Person Person;
struct Person {
    char    name[64];
    int     age;
    Address addr;
};

int main(void) {
    Person p;
    p.age = 30;
    return 0;
}
