typedef interface ICounter {
    int inc(ICounter *self);
    int value(ICounter *self);
} ICounter;

typedef class Counter {
    private int v;
    public static Counter* create(int init);
    public int inc(Counter *self);
    public int value(Counter *self);
} Counter;
