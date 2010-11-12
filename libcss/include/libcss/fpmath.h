/*
 * This file is part of LibCSS.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2008 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef libcss_fpmath_h_
#define libcss_fpmath_h_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

/* 22:10 fixed point math */
typedef int32_t css_fixed;

/* Add two fixed point values */
#define FADD(a, b) ((a) + (b))
/* Subtract two fixed point values */
#define FSUB(a, b) ((a) - (b))
/* Multiply two fixed point values */
#define FMUL(a, b) ((((int64_t) (a)) * ((int64_t) (b))) >> 10)
/* Divide two fixed point values */
#define FDIV(a, b) ((((int64_t) (a)) << 10) / (b))

/* Add an integer to a fixed point value */
#define FADDI(a, b) ((a) + ((b) << 10))
/* Subtract an integer from a fixed point value */
#define FSUBI(a, b) ((a) - ((b) << 10))
/* Multiply a fixed point value by an integer */
#define FMULI(a, b) ((a) * (b))
/* Divide a fixed point value by an integer */
#define FDIVI(a, b) ((a) / (b))

/* Convert a floating point value to fixed point */
#define FLTTOFIX(a) ((css_fixed) ((a) * (float) (1 << 10)))
/* Convert a fixed point value to floating point */
#define FIXTOFLT(a) ((float) (a) / (float) (1 << 10))

/* Convert an integer to a fixed point value */
#define INTTOFIX(a) ((a) << 10)
/* Convert a fixed point value to an integer */
#define FIXTOINT(a) ((a) >> 10)

/* Useful values */
#define F_PI_2	0x00000648	/* 1.5708 (PI/2) */
#define F_PI	0x00000c91	/* 3.1415 (PI) */
#define F_3PI_2	0x000012d9	/* 4.7124 (3PI/2) */
#define F_2PI	0x00001922	/* 6.2831 (2 PI) */

#define F_90	0x00016800	/*  90 */
#define F_180	0x0002d000	/* 180 */
#define F_270	0x00043800	/* 270 */
#define F_360	0x0005a000	/* 360 */

#define F_100	0x00019000	/* 100 */
#define F_200	0x00032000	/* 200 */
#define F_300	0x0004b000	/* 300 */
#define F_400	0x00064000	/* 400 */

#ifdef __cplusplus
}
#endif

#endif

