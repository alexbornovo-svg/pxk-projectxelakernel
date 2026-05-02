#ifndef MATH_H
#define MATH_H

#include "types.h"

static int32_t absval(int32_t num)
{
    if (num >= 0)
    {
        return num;
    }
    else
    {
        return num * -1;
    }
}

#endif