#include "models/watch_model.h"
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
static double number(const char **p, bool *valid)
{
    char *end;
    double value = strtod(*p, &end);
    if(end == *p || !isfinite(value)) { *valid = false; return 0; }
    *p = end;
    return value;
}

static double term(const char **p, bool *valid)
{
    double v = number(p, valid);
    while(*valid && (**p == '*' || **p == '/')) {
        char op = *(*p)++;
        double rhs = number(p, valid);
        if(op == '/' && rhs == 0) { *valid = false; return 0; }
        v = op == '*' ? v * rhs : v / rhs;
    }
    return v;
}

bool watch_calculate(const char *expression, double *result)
{
    /* Keypad inputs are decimal numbers only, not C's hex/NaN notation. */
    for(const char *s = expression; *s; ++s)
        if(!isdigit((unsigned char)*s) && !strchr(".+-*/", *s)) return false;
    bool valid = true;
    const char *p = expression;
    double value = term(&p, &valid);
    while(valid && (*p == '+' || *p == '-')) {
        char op = *p++;
        double rhs = term(&p, &valid);
        value = op == '+' ? value + rhs : value - rhs;
    }
    if(!valid || *p || !isfinite(value) || fabs(value) > 1e12) return false;
    *result = value;
    return true;
}
