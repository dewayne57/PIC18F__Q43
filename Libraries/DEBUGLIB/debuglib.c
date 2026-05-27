#include <xc.h>
#include <stdbool.h>
#include "debuglib.h"

void debug_init() { 
    TRISDbits.TRISD0 = 0; // RD0 is output
    TRISDbits.TRISD1 = 0; // RD1 is output
    TRISDbits.TRISD2 = 0; // RD2 is output
    TRISDbits.TRISD3 = 0; // RD3 is output
    TRISDbits.TRISD4 = 0; // RD4 is output
    TRISDbits.TRISD5 = 0; // RD5 is output
    TRISDbits.TRISD6 = 0; // RD6 is output
    TRISDbits.TRISD7 = 0; // RD7 is output

    ANSELDbits.ANSELD0 = 0; // RD0 is digital
    ANSELDbits.ANSELD1 = 0; // RD1 is digital
    ANSELDbits.ANSELD2 = 0; // RD2 is digital
    ANSELDbits.ANSELD3 = 0; // RD3 is digital
    ANSELDbits.ANSELD4 = 0; // RD4 is digital
    ANSELDbits.ANSELD5 = 0; // RD5 is digital
    ANSELDbits.ANSELD6 = 0; // RD6 is digital
    ANSELDbits.ANSELD7 = 0; // RD7 is digital
}

void debug_clear() { 
    LATDbits.LATD0 = 1; // Clear RD0
    LATDbits.LATD1 = 1; // Clear RD1
    LATDbits.LATD2 = 1; // Clear RD2
    LATDbits.LATD3 = 1; // Clear RD3
    LATDbits.LATD4 = 1; // Clear RD4
    LATDbits.LATD5 = 1; // Clear RD5
    LATDbits.LATD6 = 1; // Clear RD6
    LATDbits.LATD7 = 1; // Clear RD7
}

void debug_led0(bool on) { 
    LATDbits.LATD0 = on ? 0 : 1;
}

void debug_led1(bool on) { 
    LATDbits.LATD1 = on ? 0 : 1;
}

void debug_led2(bool on) { 
    LATDbits.LATD2 = on ? 0 : 1;
}

void debug_led3(bool on) { 
    LATDbits.LATD3 = on ? 0 : 1;
}

void debug_led4(bool on) {
    LATDbits.LATD4 = on ? 0 : 1;
}

void debug_led5(bool on) {
    LATDbits.LATD5 = on ? 0 : 1;
}

void debug_led6(bool on) {
    LATDbits.LATD6 = on ? 0 : 1;
}

void debug_led7(bool on) {
    LATDbits.LATD7 = on ? 0 : 1;
}

