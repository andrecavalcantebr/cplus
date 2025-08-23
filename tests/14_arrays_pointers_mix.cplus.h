interface IStore<T> {
    int put(IStore<T> *self, T item);
    T get(IStore<T> *self, int idx);
};

class Store<T> {
    private T *buffer[4];
    private int used;
    public static Store<T>* create(int cap);
    public int put(Store<T> *self, T item);
    public T get(Store<T> *self, int idx);
};
