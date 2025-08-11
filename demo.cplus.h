/* demo.h — Exemplo mínimo para testar parser Cplus */

interface IStartable {
    void start();
    void stop();
};

class Device {
public:
    void power_on();
    void power_off();
protected:
    int voltage;
private:
    bool status;
};

class Motor extends Device implements IStartable {
public:
    void start();
    void stop();
    void set_speed(int rpm);
protected:
    int current_rpm;
private:
    bool enabled;
};
