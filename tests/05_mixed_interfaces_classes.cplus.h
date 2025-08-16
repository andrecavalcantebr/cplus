interface IReset {
    void reset();
};

interface IAdd {
    int add(int x, int y);
};

class Accumulator {
    public:
        int value;
        void reset();
        int add(int v);
};
