/* demo.cplus — Exemplo para testar parser Cplus */

#include <stdbool.h>

interface IStartable {
    //interface methods must match signatures exactly
    void start(void *self);
    void stop(void *self);
};

class Device {
    public void power_on(class Device *self);
    void power_off(class Device *self);
    protected int voltage;
    private bool status;
};

class Motor extends Device implements IStartable {
    public void init(Motor *self);
    public void deinit(Motor *self);
    public void start(void *self);
    public void stop(void *self);
    public void set_speed(class Motor *self, int rpm);
    protected int current_rpm;
    private bool enabled;
};

