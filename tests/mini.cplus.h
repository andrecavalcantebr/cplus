class Foo
{
private
    int x;
public
    int get(Foo_ref self);
public
    void set(Foo_ref self, int val);
};

public
int get(Foo_ref self)
{
    return self->x;
}

public
void set(Foo_ref self, int val)
{
    self->x = val;
}
