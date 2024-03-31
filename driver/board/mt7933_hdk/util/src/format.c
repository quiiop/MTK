/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2010
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#include <stdarg.h>
#include "common.h"

#define MAXFRACT        100000

//
// Functional Prototypes
//
static void pOutputByte(unsigned char c);
static void pOutputNumHex(unsigned long n, long depth);
static void pOutputNumDecimal(unsigned long n);
static void pOutputNumDecimalWithMaxWidth(long n, unsigned int maxWidth);
static void OutputString(const unsigned char *s);

//__attribute__ ((unused, __section__ ("INTERNAL_SRAM"))) char *szSprintf;
char *szSprintf;

//
// Routine starts
//
/*****************************************************************************
*
*
*   @func   void    | EdbgOutputDebugString | Simple formatted debug output string routine
*
*   @rdesc  none
*
*   @parm   LPCSTR  |   sz,... |
*               Format String:
*
*               @flag Format string | type
*               @flag u | unsigned
*               @flag d | int
*               @flag c | char
*               @flag s | string
*               @flag x | 4-bit hex number
*               @flag B | 8-bit hex number
*               @flag H | 16-bit hex number
*               @flag X | 32-bit hex number
*
*   @comm
*           Same as FormatString, but output to serial port instead of buffer.
*/

void EdbgOutputDebugString(LPCSTR sz, ...)
{
    unsigned char   c;
    double          dl;
    long            l;
    int         format_width = 0;
    va_list         vl;

    va_start(vl, sz);

    while (*sz) {
        c = *sz++;
        switch (c) {
            case '%':
                /* clear format width */
                format_width = 0;

            label_format_continue:
                c = *sz++;

                switch (c) {
                    case 'x':
                        pOutputNumHex(va_arg(vl, unsigned long), 0);
                        break;
                    case 'B':
                        pOutputNumHex(va_arg(vl, unsigned long), 2);
                        break;
                    case 'H':
                        pOutputNumHex(va_arg(vl, unsigned long), 4);
                        break;
                    case 'X':
                        pOutputNumHex(va_arg(vl, unsigned long), 8);
                        break;
                    case 'd':
                        l = va_arg(vl, long);
                        pOutputNumDecimalWithMaxWidth(l, format_width);

                        /* clear format width value */
                        format_width = 0;
                        break;
                    case 'u':
                        pOutputNumDecimal(va_arg(vl, unsigned long));
                        break;
                    case 's':
                        OutputString((const unsigned char *)va_arg(vl, char *));
                        break;
                    case '%':
                        pOutputByte('%');
                        break;
                    case 'c':
                        c = va_arg(vl, int);
                        pOutputByte(c);
                        break;
                    case 'f': {
#if defined(MT6576_FPGA)|defined(MT6575_FPGA)
                            int             *pi;
                            int             *pj;
                            int             iTemp;
#endif
                            dl = va_arg(vl, double);
                            /*
                             *  2010/01/10 { FIX ME
                             *      The words of a double seems flip after va_arg()
                             *      Just switch them back to work aroud it.
                             *      Need to check the root cause......
                             *  Angelo 20100523 {
                             *      This flip doesn't happen at MT6576 EVB.
                             *      But I doesn't check how about the latest MT6576 FPGA
                             *      One could remove it after FPGA is checked pass.
                             */
#if defined(MT6576_FPGA)|defined(MT6575_FPGA)
                            pi = (int *)(&dl);
                            pj = pi + 1;
                            iTemp = *pi;
                            *pi = *pj;
                            *pj = iTemp;
#endif
                            /*
                             *  20100523|}
                             *  2010/01/10 }
                             */
                            if (dl < 0) {
                                pOutputByte('-');
                                dl = -dl;
                            }
                            if (dl >= 1.0) {
                                pOutputNumDecimal((int)dl);
                            } else {
                                pOutputByte('0');
                            }
                            pOutputByte('.');
                            pOutputNumDecimal((int)((dl - (double)(int)dl) * (double)(MAXFRACT)));
                            break;
                        }
                    case '0' ... '9':
                        format_width = format_width * 10 + (c - '0');
                        goto label_format_continue;
                        break;
                    default:
                        /* clear format width value */
                        format_width = 0;
                        pOutputByte(' ');
                        break;
                }
                break;
            case '\r':
                if (*sz == '\n')
                    sz ++;
                c = '\n';
            // fall through
            case '\n':
                pOutputByte('\r');
            // fall through
            default:
                pOutputByte(c);
        }
    }

    va_end(vl);
}

/*****************************************************************************
*
*
*   @func   void    |   FormatString | Simple formatted output string routine
*
*   @rdesc  Returns length of formatted string
*
*   @parm   unsigned char * |   pBuf |
*               Pointer to string to return formatted output.  User must ensure
*               that buffer is large enough.
*
*   @parm   const unsigned char * |   sz,... |
*               Format String:
*
*               @flag Format string | type
*               @flag u | unsigned
*               @flag d | int
*               @flag c | char
*               @flag s | string
*               @flag x | 4-bit hex number
*               @flag B | 8-bit hex number
*               @flag H | 16-bit hex number
*               @flag X | 32-bit hex number
*
*   @comm
*           Same as OutputFormatString, but output to buffer instead of serial port.
*/
unsigned int FormatString(unsigned char *pBuf, const unsigned char *sz, ...)
{
    unsigned char   c;
    double          dl;
    va_list         vl;

    va_start(vl, sz);

    szSprintf = (char *) pBuf;
    while (*sz) {
        c = *sz++;
        switch (c) {
            case '%':
                c = *sz++;
                switch (c) {
                    case 'x':
                        pOutputNumHex(va_arg(vl, unsigned long), 0);
                        break;
                    case 'B':
                        pOutputNumHex(va_arg(vl, unsigned long), 2);
                        break;
                    case 'H':
                        pOutputNumHex(va_arg(vl, unsigned long), 4);
                        break;
                    case 'X':
                        pOutputNumHex(va_arg(vl, unsigned long), 8);
                        break;
                    case 'd': {
                            long    l;

                            l = va_arg(vl, long);
                            if (l < 0) {
                                pOutputByte('-');
                                l = - l;
                            }
                            pOutputNumDecimal((unsigned long)l);
                        }
                        break;
                    case 'u':
                        pOutputNumDecimal(va_arg(vl, unsigned long));
                        break;
                    case 's':
                        OutputString((const unsigned char *) va_arg(vl, char *));
                        break;
                    case '%':
                        pOutputByte('%');
                        break;
                    case 'c':
                        c = va_arg(vl, int);
                        pOutputByte(c);
                        break;
                    case 'f': {
                            int             *pi;
                            int             *pj;
                            int             iTemp;
                            dl = va_arg(vl, double);
                            /*
                             *  2010/01/10 { FIX ME
                             *      The words of a double seems flip after va_arg()
                             *      Just switch them back to work aroud it.
                             *      Need to check the root cause......
                             */
                            pi = (int *)(&dl);
                            pj = pi + 1;
                            iTemp = *pi;
                            *pi = *pj;
                            *pj = iTemp;
                            /*
                             *  2010/01/10 }
                             */
                            if (dl < 0) {
                                pOutputByte('-');
                                dl = -dl;
                            }
                            if (dl >= 1.0) {
                                pOutputNumDecimal((int)dl);
                            } else {
                                pOutputByte('0');
                            }
                            pOutputByte('.');
                            pOutputNumDecimal((int)((dl - (double)(int)dl) * (double)(MAXFRACT)));
                            break;
                        }
                    default:
                        pOutputByte(' ');
                        break;
                }
                break;
            case '\r':
                if (*sz == '\n')
                    sz ++;
                c = '\n';
            // fall through
            case '\n':
                pOutputByte('\r');
            // fall through
            default:
                pOutputByte(c);
        }
    }
    pOutputByte(0);
    c = szSprintf - (char *)pBuf;
    szSprintf = 0;
    va_end(vl);
    return (c);
}

/*****************************************************************************
*
*
*   @func   void    |   pOutputByte | Sends a byte out of the monitor port.
*
*   @rdesc  none
*
*   @parm   unsigned int |   c |
*               Byte to send.
*
*/
static void pOutputByte(
    unsigned char c
)
{
    if (szSprintf)
        *szSprintf++ = c;
    else
        WriteDebugByte(c);
}


/*****************************************************************************
*
*
*   @func   void    |   pOutputNumHex | Print the hex representation of a number through the monitor port.
*
*   @rdesc  none
*
*   @parm   unsigned long |   n |
*               The number to print.
*
*   @parm   long | depth |
*               Minimum number of digits to print.
*
*/
static void pOutputNumHex(unsigned long n, long depth)
{
    if (depth) {
        depth--;
    }

    if ((n & ~0xf) || depth) {
        pOutputNumHex(n >> 4, depth);
        n &= 0xf;
    }

    if (n < 10) {
        pOutputByte((unsigned char)(n + '0'));
    } else {
        pOutputByte((unsigned char)(n - 10 + 'A'));
    }
}


/*****************************************************************************
*
*
*   @func   void    |   pOutputNumDecimal | Print the decimal representation of a number through the monitor port.
*
*   @rdesc  none
*
*   @parm   unsigned long |   n |
*               The number to print.
*
*/
static void pOutputNumDecimal(unsigned long n)
{
    if (n >= 10) {
        pOutputNumDecimal(n / 10);
        n %= 10;
    }
    pOutputByte((unsigned char)(n + '0'));
}

static unsigned long getIntWidth(long n)
{
    unsigned long width = 0;

    if (n < 0)
        ++width, n = -n;

    do {
        ++width;
        n /= 10;
    } while (n);

    return width;
}

static void pOutputBytes(unsigned char c, unsigned int len)
{
    while (len--)
        pOutputByte(c);
}

static void pOutputNumDecimalWithMaxWidth(long n, unsigned int maxWidth)
{
    unsigned int width = getIntWidth(n);
    unsigned int space_cnt = 0;

    if (maxWidth > width)
        space_cnt = maxWidth - width;


    pOutputBytes(' ', space_cnt);
    if (n < 0) {
        pOutputByte('-');
        n = -n;
    }
    pOutputNumDecimal(n);
}


/*****************************************************************************
*
*
*   @func   void    |   OutputString | Sends an unformatted string to the monitor port.
*
*   @rdesc  none
*
*   @parm   const unsigned char * |   s |
*               points to the string to be printed.
*
*   @comm
*           backslash n is converted to backslash r backslash n
*/
static void OutputString(const unsigned char *s)
{
    while (*s) {
        if (*s == '\n') {
            WriteDebugByte('\r');
        }
        WriteDebugByte(*s++);
    }
}

// This routine will take a binary IP address as represent here and return a dotted decimal version of it
char *inet_ntoa(DWORD dwIP)
{

    static char szDottedD[16];

    FormatString((unsigned char *) szDottedD, (const unsigned char *)"%u.%u.%u.%u",
                 (BYTE)dwIP, (BYTE)(dwIP >> 8), (BYTE)(dwIP >> 16), (BYTE)(dwIP >> 24));

    return szDottedD;

} // inet_ntoa()

// This routine will take a dotted decimal IP address as represent here and return a binary version of it
DWORD inet_addr(char *pszDottedD)
{

    DWORD dwIP = 0;
    DWORD cBytes;
    char *pszLastNum;
    int atoi(const char *s);

    // Replace the dots with NULL terminators
    pszLastNum = pszDottedD;
    for (cBytes = 0; cBytes < 4; cBytes++) {
        while (*pszDottedD != '.' && *pszDottedD != '\0')
            pszDottedD++;
        if (*pszDottedD == '\0' && cBytes != 3)
            return 0;
        *pszDottedD = '\0';
        dwIP |= (atoi(pszLastNum) & 0xFF) << (8 * cBytes);
        pszLastNum = ++pszDottedD;
    }

    return dwIP;

} // inet_ntoa()
