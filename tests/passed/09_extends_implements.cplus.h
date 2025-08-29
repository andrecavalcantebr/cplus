interface IFoo { int foo(void); };
interface IBar { void bar(int x); };

class Base {
    public int baseMethod(Base *self);
};

class Derived extends Base implements IFoo, IBar {
    private int state;
    public int foo(void);
    public void bar(int x);
};
