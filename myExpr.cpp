

#if WITH_EXPRESSION

    #define NUM_PEDALS   4
    
    #define PIN_EXPR1    20  // A6
    #define PIN_EXPR2    21  // A7
    #define PIN_EXPR1    22  // A8
    #define PIN_EXPR2    23  // A9
    
    #define CALIBRATE_TIME              6000
    #define CALIBRATE_BLINK_ON_TIME       20
    #define CALIBRATE_BLINK_OFF_TIME     150
    
    typedef struct
    {
        int     pin;
        const   char *name;
        int     calib_min;
        int     calib_max;
        
        int     cable;
        int     channel;
        int     cc_num;
        
        int     raw_value;
        
    }   expressionPedal_t;

    expressionPedal_t pedals[NUM_PEDALS] = {
        { PIN_EXPR1, "Pedal1", 9, 1010, 0, 1, 0x0F, 0},
        { PIN_EXPR2, "Pedal2", 9, 1010, 0, 1, 0x0E, 0},
    };

    int calibrate_pedal = -1;
    elapsedMillis calibrate_time = 0;    
    
#endif
