interface IVec<T> {
    T getAt(IVec<T> *self, int i);
    int size(void);
};

class Vector<T> {
    public T *data;
    public int length;
    public static Vector<T>* create(int n);
    public T getAt(Vector<T> *self, int i);
};

class Matrix<T> {
    public T **cells[];
    public int dims[3];
    public T get(Matrix<T> *self, int i, int j);
};
