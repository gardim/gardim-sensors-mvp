#include <utils.h>

long map_function(long x, long in_min, long in_max, long out_min, long out_max) 
{
    const long dividend = out_max - out_min;
    const long divisor = in_max - in_min;
    const long delta = x - in_min;
    if(divisor == 0){
        return -1; 
    }
    return (delta * dividend + (divisor / 2)) / divisor + out_min;
}