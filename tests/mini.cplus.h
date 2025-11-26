class Foo
{
private
    int x;
public
    int get(Foo_ref self);
public
    void set(Foo_ref self, int val);
};

int Foo_get(Foo_ref self)
{
    return self->x;
}

void set(Foo_ref self, int val)
{
    self->x = val;
}
