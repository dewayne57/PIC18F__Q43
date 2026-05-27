

#ifndef DEBUGLIB_H
#define DEBUGLIB_H
    #include <xc.h>
    #include <stdbool.h>

    void debug_init(void);
    void debug_clear(); 
    void debug_led0(bool on);
    void debug_led1(bool on);
    void debug_led2(bool on);
    void debug_led3(bool on);
    void debug_led4(bool on);
    void debug_led5(bool on);
    void debug_led6(bool on);
    void debug_led7(bool on);

#endif /* DEBUGLIB_H */