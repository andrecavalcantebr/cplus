interface IService;
class Impl;

interface IService {
    int run(IService *self);
};

class Impl implements IService {
    private int code;
    public int run(IService *self);
};
