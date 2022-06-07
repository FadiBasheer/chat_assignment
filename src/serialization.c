
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

/*
** packi16() -- store a 16-bit int into a char buffer (like htons())
*/
void packi16(unsigned char *buf, unsigned int i) {
    *buf++ = i >> 8;
    *buf++ = i;
}

/*
** pack() -- store data dictated by the format string in the buffer
**
**   bits |signed   unsigned   float   string
**   -----+----------------------------------
**      8 |   c        C
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s
**
**  (16-bit unsigned length is automatically prepended to strings)
*/

unsigned int pack(unsigned char *buf, char *format, ...) {
    va_list ap;

    signed char c;              // 8-bit
    unsigned char C;

    int h;                      // 16-bit
    unsigned int H;

    char *s;                    // strings
    unsigned int len;

    unsigned int size = 0;

    va_start(ap, format);

    for (; *format != '\0'; format++) {
        switch (*format) {

            case 'C': // 8-bit unsigned
                size += 1;
                C = (unsigned char) va_arg(ap, unsigned int); // promoted
                *buf++ = C;
                break;

            case 'H': // 16-bit unsigned
                size += 2;
                H = va_arg(ap, unsigned int);
                packi16(buf, H);
                buf += 2;
                break;

            case 's': // string
                s = va_arg(ap, char*);
                len = strlen(s);
                size += len + 2;
                packi16(buf, len);
                buf += 2;
                memcpy(buf, s, len);
                buf += len;
                break;
        }
    }
    va_end(ap);
    return size;
}

unsigned int unpacku16(unsigned char *buf) {
    return ((unsigned int) buf[0] << 8) | buf[1];
}

/*
** unpack() -- unpack data dictated by the format string into the buffer
**
**   bits |signed   unsigned   float   string
**   -----+----------------------------------
**      8 |   c        C
**     16 |   h        H         f
**     32 |   l        L         d
**     64 |   q        Q         g
**      - |                               s
**
**  (string is extracted based on its stored length, but 's' can be
**  prepended with a max length)
*/
void unpack(unsigned char *buf, char *format, ...) {
    va_list ap;

    signed char *c;              // 8-bit
    unsigned char *C;

    int *h;                      // 16-bit
    unsigned int *H;

    long int *l;                 // 32-bit
    unsigned long int *L;

    long long int *q;            // 64-bit
    unsigned long long int *Q;

    float *f;                    // floats
    double *d;
    long double *g;
    unsigned long long int fhold;

    char *s;
    unsigned int len, maxstrlen = 0, count;

    va_start(ap, format);

    for (; *format != '\0'; format++) {
        switch (*format) {
            case 'c': // 8-bit
                c = va_arg(ap, signed char*);
                if (*buf <= 0x7f) { *c = *buf; } // re-sign
                else { *c = -1 - (unsigned char) (0xffu - *buf); }
                buf++;
                break;

            case 'C': // 8-bit unsigned
                C = va_arg(ap, unsigned char*);
                *C = *buf++;
                break;

            case 'H': // 16-bit unsigned
                H = va_arg(ap, unsigned int*);
                *H = unpacku16(buf);
                buf += 2;
                break;

            case 's': // string
                s = va_arg(ap, char*);
                len = unpacku16(buf);
                buf += 2;
                if (maxstrlen > 0 && len >= maxstrlen) count = maxstrlen - 1;
                else count = len;
                memcpy(s, buf, count);
                s[count] = '\0';
                buf += len;
                break;

            default:
                if (isdigit(*format)) { // track max str len
                    maxstrlen = maxstrlen * 10 + (*format - '0');
                }
        }
        if (!isdigit(*format)) maxstrlen = 0;
    }
    va_end(ap);
}
