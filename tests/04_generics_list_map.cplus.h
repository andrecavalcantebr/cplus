interface IterableInt {
    int next();
};

class Bag {
    public:
        List<int> data;        // generic level 1
        int push(int v);

    protected:
        Map<Key,Value> dict;   // generic with two args (still level 1)

    private:
        int capacity;
};
