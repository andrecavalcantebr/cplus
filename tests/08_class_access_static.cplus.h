class Device {
    public static int version(void);
    public int id;
    protected static int live_count;
    private char *name;

    public int rename(Device *self, const char *s);
};
