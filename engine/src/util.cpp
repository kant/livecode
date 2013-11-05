/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "param.h"
#include "util.h"
#include "stack.h"
#include "card.h"
#include "stacklst.h"
#include "sellst.h"
#include "undolst.h"
#include "aclip.h"
#include "image.h"
#include "field.h"
#include "mcerror.h"
#include "osspec.h"
#include "redraw.h"
#include "mcssl.h"

#include "globals.h"
#include "exec.h"
#include "system.h"

#define QA_NPOINTS 10

static MCPoint qa_points[QA_NPOINTS];

static void MCU_play_message()
{
	MCAudioClip *acptr = MCacptr;
	MCacptr = NULL;
	MCStack *sptr = acptr->getmessagestack();
	if (sptr != NULL)
	{
		acptr->setmessagestack(NULL);
		sptr->getcurcard()->message_with_valueref_args(MCM_play_stopped, acptr->getname());
	}
	if (acptr->isdisposable())
		delete acptr;
}

void MCU_play()
{
	if (MCacptr != NULL && !MCacptr->play())
		MCU_play_message();
}

void MCU_play_stop()
{
	if (MCacptr != NULL)
	{
		MCacptr->stop(True);
		MCU_play_message();
	}
}

void MCU_init()
{
	int2 i;
	real8 increment = (M_PI / 2.0) / (real8)QA_NPOINTS;
	real8 angle = 0.0;

	for (i = 0 ; i < QA_NPOINTS ; i++)
	{
		qa_points[i].x = (short)(sin(angle) * (real8)MAXINT2);
		qa_points[i].y = MAXINT2 - (short)(cos(angle) * (real8)MAXINT2);
		angle += increment;
	}
	MCrandomseed = (int4)(intptr_t)&MCdispatcher + MCS_getpid() + (int4)time(NULL);
	MCU_srand();
}

void MCU_watchcursor(MCStack *sptr, Boolean force)
{
	if (!MClockcursor)
	{
		MCwatchcursor = True;
		if (sptr == NULL)
			sptr = MCmousestackptr == NULL ? MCdefaultstackptr : MCmousestackptr;
		sptr->resetcursor(force);
	}
}

void MCU_unwatchcursor(MCStack *sptr, Boolean force)
{
	if (!MClockcursor)
	{
		MCwatchcursor = False;
		if (sptr == NULL)
			sptr = MCmousestackptr == NULL ? MCdefaultstackptr : MCmousestackptr;
		sptr -> resetcursor(force);
	}
}

void MCU_resetprops(Boolean update)
{
	if (update)
	{
		if (MCwatchcursor || MCcursor != None)
		{
			if (!MClockcursor)
				MCcursor = None;
			MCwatchcursor = False;
			if (MCmousestackptr != NULL)
				MCmousestackptr->resetcursor(True);
			else
				MCdefaultstackptr->resetcursor(True);
		}
		
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		MCRedrawForceUnlockScreen();

		if (MClockmenus)
		{
			MCscreen->updatemenubar(True);
			MClockmenus = False;
		}
		if (MCdragging)
		{
			MCdragging = False;
			MCselected->redraw();
		}
	}
	MCerrorlock = 0;
	MClockerrors = MClockmessages = MClockmoves = MClockrecent = False;
	MCerrorlockptr = NULL;
	MCinterrupt = False;
	MCdragspeed = 0;
	MCdynamiccard = NULL;
	MCdynamicpath = False;
	MCexitall = False;
}

void MCU_saveprops(MCSaveprops &sp)
{
	sp.watchcursor = MCwatchcursor;
	MCwatchcursor = False;
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCRedrawSaveLockScreen(sp.lockscreen);
	sp.errorlock = MCerrorlock;
	sp.errorlockptr = MCerrorlockptr;
	sp.lockerrors = MClockerrors;
	sp.lockmessages = MClockmessages;
	sp.lockmoves = MClockmoves;
	sp.lockrecent = MClockrecent;
	sp.interrupt = MCinterrupt;
	sp.dragspeed = MCdragspeed;
	sp.dynamiccard = MCdynamiccard;
	sp.errorptr = MCerrorptr;
	sp.exitall = MCexitall;
}

void MCU_restoreprops(MCSaveprops &sp)
{
	MCwatchcursor = sp.watchcursor;
	// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
	MCRedrawRestoreLockScreen(sp.lockscreen);
	// MW-2011-08-17: [[ Redraw ]] This shouldn't be necessary anymore as the
	//   updates will be flushed shortly after as (new) lockscreen == False.
	//if (oldls && !sp.lockscreen)
	//	MCstacks->redrawall(False);
	MCerrorlock = sp.errorlock;
	MCerrorlockptr = sp.errorlockptr;
	MClockerrors = sp.lockerrors;
	MClockmessages = sp.lockmessages;
	MClockmoves = sp.lockmoves;
	MClockrecent = sp.lockrecent;
	MCinterrupt = sp.interrupt;
	MCdragspeed = sp.dragspeed;
	MCdynamiccard = sp.dynamiccard;
	MCerrorptr = sp.errorptr;
	MCexitall = sp.exitall;
}

int4 MCU_any(int4 max)
{
	return (int4)(MCU_drand() * (real8)max);
}

bool MCU_getnumberformat(uint2 fw, uint2 trail, uint2 force, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;

	MCAutoStringRef t_buffer;
	if (t_success)
		t_success = MCStringCreateMutable(0, &t_buffer);

	{
		uint2 i = MCU_max(fw - trail - 1, 0);
		while (t_success && i--)
			t_success = MCStringAppendNativeChar(*t_buffer, '0');	
		if (t_success && trail != 0)
		{
			t_success = MCStringAppendNativeChar(*t_buffer, '.');
			i = force;
			while (t_success && i--)
				t_success = MCStringAppendNativeChar(*t_buffer, '0');
			i = trail - force;
			while (t_success && i--)
				t_success = MCStringAppendNativeChar(*t_buffer, '#');
		}
	}

	if (t_success)
		return MCStringCopy(*t_buffer, r_string);

	return false;
}


void MCU_getnumberformat(MCExecPoint &ep, uint2 fw, uint2 trail, uint2 force)
{
#ifdef OLD_EXEC
	char *eptr;
	ep.reserve(fw + 1, eptr);
	uint2 i = MCU_max(fw - trail - 1, 0);
	while (i--)
		*eptr++ = '0';
	if (trail != 0)
	{
		*eptr++ = '.';
		i = force;
		while (i--)
			*eptr++ = '0';
		i = trail - force;
		while (i--)
			*eptr++ = '#';
	}
	*eptr = '\0';
	ep.commit(strlen(eptr));
#else
	MCAutoStringRef t_format;
	if (MCU_getnumberformat(fw, trail, force, &t_format))
		/* UNCHECKED */ ep . setvalueref(*t_format);
	else
		ep . clear();
#endif
}


void MCU_setnumberformat(MCStringRef d, uint2 &fw,
                         uint2 &trailing, uint2 &force)
{
	fw = MCStringGetLength(d);
	const char *sptr = MCStringGetCString(d);
	const char *eptr = sptr;
	while (eptr - sptr < fw && *eptr != '.')
		eptr++;
	if (eptr - sptr == fw)
	{
		trailing = force = 0;
		return;
	}
	eptr++;
	if (eptr - sptr == fw)
		fw--;
	force = 0;
	while (eptr - sptr < fw && *eptr != '#')
	{
		eptr++;
		force++;
	}
	trailing = force;
	while (eptr - sptr < fw && *eptr == '#')
	{
		eptr++;
		trailing++;
	}
}

#define UtoF(u) (((double)((long)(u - 2147483647L - 1))) + 2147483648.0)

real8 MCU_stoIEEE(const char *bytes)
{
	real8 f = 0;
	int4 expon;
	uint4 hiMant, loMant;

	expon = ((bytes[0] & 0x7F) << 8) | (bytes[1] & 0xFF);
	hiMant = ((unsigned long)(bytes[2] & 0xFF) << 24)
	         |    ((unsigned long)(bytes[3] & 0xFF) << 16)
	         |    ((unsigned long)(bytes[4] & 0xFF) << 8)
	         |    ((unsigned long)(bytes[5] & 0xFF));
	loMant = ((unsigned long)(bytes[6] & 0xFF) << 24)
	         |    ((unsigned long)(bytes[7] & 0xFF) << 16)
	         |    ((unsigned long)(bytes[8] & 0xFF) << 8)
	         |    ((unsigned long)(bytes[9] & 0xFF));

	if (expon == 0 && hiMant == 0 && loMant == 0)
		f = 0;
	else
	{
		if (expon == 0x7FFF)    /* MCinfinity or NaN */
			f = MCinfinity;
		else
		{
			expon -= 16383;
			f  = ldexp(UtoF(hiMant), expon-=31);
			f += ldexp(UtoF(loMant), expon-=32);
		}
	}
	if (bytes[0] & 0x80)
		return -f;
	else
		return f;
}

real8 MCU_i4tor8(int4 in)
{
	return (real8)in / 65535.0;
}

int4 MCU_r8toi4(real8 in)
{
	return (int4)(in * 65535.0);
}

#define N       16
#define RMASK    ((uint4)(1 << (N - 1)) + (1 << (N - 1)) - 1)
#define LOW(x)  ((uint4)(x) & RMASK)
#define HIGH(x) LOW((x) >> N)
#define MUL(x, y, z)    { uint4 l = (uint4)(x) * (uint4)(y); \
                (z)[0] = LOW(l); (z)[1] = HIGH(l); }
#define CARRY(x, y)     ((uint4)(x) + (uint4)(y) > RMASK)
#define ADDEQU(x, y, z) (z = CARRY(x, (y)), x = LOW(x + (y)))
#define X0      0x330E
#define X1      0xABCD
#define X2      0x1234
#define A0      0xE66D
#define A1      0xDEEC
#define A2      0x5
#define C       0xB
#define SET3(x, x0, x1, x2)     ((x)[0] = (x0), (x)[1] = (x1), (x)[2] = (x2))
#define SEED(x0, x1, x2) (SET3(x, x0, x1, x2), SET3(a, A0, A1, A2), c = C)

static uint4 x[3] = { X0, X1, X2 }, a[3] = { A0, A1, A2 }, c = C;

void MCU_srand()
{
	SEED(X0, LOW(MCrandomseed), HIGH(MCrandomseed));
}

real8 MCU_drand()
{
	static real8 two16m = 1.0 / (1L << N);
	uint4 p[2], q[2], r[2], carry0, carry1;

	MUL(a[0], x[0], p);
	ADDEQU(p[0], c, carry0);
	ADDEQU(p[1], carry0, carry1);
	MUL(a[0], x[1], q);
	ADDEQU(p[1], q[0], carry0);
	MUL(a[1], x[0], r);
	x[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] +
	           a[0] * x[2] + a[1] * x[1] + a[2] * x[0]);
	x[1] = LOW(p[1] + r[0]);
	x[0] = LOW(p[0]);
	return (two16m * (two16m * (two16m * x[0] + x[1]) + x[2]));
}

// custom strtok that only skips single delimiters
static char *olds = NULL;
char *MCU_strtok(char *s, const char *delim)
{
	if (s == NULL)
	{
		if (olds == NULL)
		{
			MCS_seterrno(EINVAL);
			return NULL;
		}
		else
			s = olds;
	}
	if (*s == '\0')
	{
		olds = NULL;
		return NULL;
	}
	char *token = s;
	s = strpbrk(token, delim);
	if (s == NULL)
		olds = NULL;
	else
	{
		*s = '\0';
		olds = s + 1;
	}
	return token;
}

/* WRAPPER */ bool MCU_strtol(MCStringRef p_string, int4& r_l)
{
	Boolean t_converted;
	uint4 l = MCStringGetLength(p_string);
	const char *sptr = MCStringGetCString(p_string);
	r_l = MCU_strtol(sptr, l, '\0', t_converted);
	return True == t_converted;
}

int4 MCU_strtol(const char *&sptr, uint4 &l, int1 c, Boolean &done,
                Boolean reals, Boolean octals)
{
	done = False;
	MCU_skip_spaces(sptr, l);
	if (!l)
		return 0;
	Boolean negative = False;
	int4 value = 0;
	if (*sptr == '-' || *sptr == '+')
	{
		negative = *sptr == '-';
		sptr++;
		l--;
	}
	if (!l)
		return 0;
	uint4 startlength = l;
	uint2 base = 10;
	if (l && *sptr == '0')
		if (l > 2 && MCS_tolower(*(sptr + 1)) == 'x')
		{
			base = 16;
			sptr += 2;
			l -= 2;
		}
		else
			if (octals)
			{
				base = 8;
				sptr++;
				l--;
			}
	while (l)
	{
		if (isdigit((uint1)*sptr))
		{
			int4 v = *sptr - '0';
			if (base < 16 && value > MAXINT4 / base - v)  // prevent overflow
				return 0;
			value *= base;
			value += v;
		}
		else
			if (isspace((uint1)*sptr))
			{
				MCU_skip_spaces(sptr, l);
				if (l && *sptr == c)
				{
					sptr++;
					l--;
				}
				break;
			}
			else
				if (l && c && *sptr == c)
				{
					sptr++;
					l--;
					break;
				}
				else
					if (*sptr == '.')
					{
						if (startlength > 1)
						{
							if (reals)
							{
								// MDW-2013-06-09: [[ Bug 10964 ]] Round integral values to nearest
								//   (consistent with getuint4() and round()).
								if (*(sptr+1) > '4')
								{
									value++;
								}
								do
								{
									sptr++;
									l--;
								}
								while (l && isdigit((uint1)*sptr));
							}
							else
								do
								{
									sptr++;
									l--;
								}
								while (l && *sptr == '0');
							if (l == 0)
								break;
							if (*sptr == c || isspace((uint1)*sptr))
							{
								sptr++;
								l--;
								break;
							}
						}
						return 0;
					}
					else
					{
						char c = MCS_tolower(*sptr);
						if (base == 16 && c >= 'a' && c <= 'f')
						{
							value *= base;
							value += c - 'a' + 10;
						}
						else
							return 0;
					}
		sptr++;
		l--;
	}
	if (negative)
		value = -value;
	MCU_skip_spaces(sptr, l);
	done = True;
	return value;
}

void MCU_strip(char *sptr, uint2 trailing, uint2 force)
{
	if (trailing == 0)
		return;
	char *eptr = &sptr[strlen(sptr)];
	sptr = strchr(sptr, '.');
	uint2 count = force;
	while (count--)
		sptr++;
	while (--eptr > sptr)
		if (*eptr == '0')
		{
			*eptr = '\0';
			if (*(eptr - 1) == '.')
			{
				*(eptr - 1) = '\0';
				break;
			}
		}
		else
			break;
}

/*
This function is defined as follows:
Let x >= 0 and y > 0.
	x wrap y  := ((x - 1) mod y) + 1
	x wrap -y := x wrap y
	-x wrap y := -(x wrap y)
*/
real8 MCU_fwrap(real8 p_x, real8 p_y)
{
	real8 t_y;
	t_y = p_y > 0 ? p_y : -p_y;
	if (p_x >= 0)
		return (fmod(p_x - 1, t_y) + 1);
	else
		return -(fmod(-p_x - 1, t_y) + 1);
		
}

bool MCU_r8tos(real8 n, uint2 fw, uint2 trailing, uint2 force, MCStringRef &r_string)
{
	char *t_str = nil;
	uint4 t_s = 0;
	if (MCU_r8tos(t_str, t_s, n, fw, trailing, force) == 0)
	{
		delete[] t_str;
		return false;
	}
	
	MCStringRef t_string;
	if (!MCStringCreateWithCStringAndRelease((char_t *)t_str, t_string))
	{
		delete[] t_str;
		return false;
	}
	
	r_string = t_string;
	return true;
}

uint4 MCU_r8tos(char *&d, uint4 &s, real8 n,
                uint2 fw, uint2 trailing, uint2 force)
{
	if (d == NULL || s <  R8L)
	{
		delete d;
		d = new char[R8L];
		s = R8L;
	}
	if (n < 0.0 && n >= -MC_EPSILON)
		n = 0.0;
	sprintf(d, "%0*.*f", fw, trailing, n);
	MCU_strip(d, trailing, force);
	
	// 2007-09-11: [[ Bug 5321 ]] If the first character is '-', we must check
	//   to see if the value is actually '0', and if it is remove the '-'.
	if (*d == '-')
	{
		bool t_is_zero;
		t_is_zero = true;
		for(char *dptr = d + 1; *dptr != '\0'; ++dptr)
			if (*dptr == 'e')
				break;
			else if (*dptr != '.' && *dptr != '0')
			{
				t_is_zero = false;
				break;
			}

		if (t_is_zero)
			memcpy(d, d + 1, strlen(d));
	}

	return strlen(d);
}

/* WRAPPER */
bool MCU_stor8(MCStringRef p_string, real8 &r_d, bool p_convert_octals)
{
	return True == MCU_stor8(MCStringGetOldString(p_string), r_d, p_convert_octals);
}

Boolean MCU_stor8(const MCString &s, real8 &d, Boolean convertoctals)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	int4 i = MCU_strtol(sptr, l, '\0', done, False, convertoctals);
	if (done)
	{
		d = i;
		return l == 0;
	}
	sptr = s.getstring();
	l = MCU_min(R8L - 1U, s.getlength());
	MCU_skip_spaces(sptr, l);
	// bugs in MSL means we need to check these things
	// MW-2006-04-21: [[ Purify ]] This was incorrect - we need to ensure l > 1 before running most
	//   of these tests.
	if (l == 0 || (l > 1 && ((MCS_tolower((uint1)sptr[1]) == 'x' && (l == 2 || !isxdigit((uint1)sptr[2]))
	        || (sptr[1] == '+' || sptr[1] == '-')))))
		return False;
	char buff[R8L];
	memcpy(buff, sptr, l);
	buff[l] = '\0';
	const char *newptr;
	d = strtod((char *)buff, (char **)&newptr);
	if (newptr == buff)
		return False;
	l = buff + l - newptr;
	MCU_skip_spaces(newptr, l);
	if (l != 0)
		return False;
	return True;
}

real8 MCU_strtor8(const char *&r_str, uint4 &r_len, int1 p_delim, Boolean &r_done, Boolean convertoctals)
{
	const char *sptr = r_str;
	uint4 l = r_len;
	Boolean done;
	real8 d;
	int4 i = MCU_strtol(sptr, l, p_delim, done, False, convertoctals);
	if (done)
	{
		r_done = True;
		r_str = sptr;
		r_len = l;
		return i;
	}
	MCU_skip_spaces(r_str, r_len);
	// bugs in MSL means we need to check these things
	// MW-2006-04-21: [[ Purify ]] This was incorrect - we need to ensure l > 1 before running most
	//   of these tests.
	if (r_len == 0 || (r_len > 1 && ((MCS_tolower((uint1)r_str[1]) == 'x' && (r_len == 2 || !isxdigit((uint1)r_str[2]))
	        || (r_str[1] == '+' || r_str[1] == '-')))))
	{
		r_done = False;
		return i;
	}
	char buff[R8L];
	uint4 buff_len = MCU_min(R8L - 1U, r_len);
	memcpy(buff, r_str, buff_len);
	buff[buff_len] = '\0';
	const char *newptr;
	d = strtod((char *)buff, (char **)&newptr);
	if (newptr == buff)
	{
		r_done = False;
		return d;
	}
	uint4 t_diff = newptr - buff;
	r_len = r_len - t_diff;
	r_str += t_diff;
	MCU_skip_spaces(r_str, r_len);
	if (r_len && *r_str == p_delim)
	{
			r_str++;
			r_len--;
	}
	r_done = True;

	return d;
}

/* WRAPPER */ bool MCU_stoi2(MCStringRef p_string, int2 &r_d)
{
	return True == MCU_stoi2(MCStringGetOldString(p_string), r_d);
}

Boolean MCU_stoi2(const MCString &s, int2 &d)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return False;
	return True;
}

/* WRAPPER */ bool MCU_stoui2(MCStringRef p_string, uint2 &r_d)
{
	return True == MCU_stoui2(MCStringGetOldString(p_string), r_d);
}

Boolean MCU_stoui2(const MCString &s, uint2 &d)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return False;
	return True;
}

Boolean MCU_stoi2x2(const MCString &s, int2 &d1, int2 &d2)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d1 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	d2 = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return False;
	return True;
}

/* WRAPPER */ bool MCU_stoi2x2(MCStringRef p_string, int16_t& r_d1, int16_t& r_d2)
{
	return True == MCU_stoi2x2(MCStringGetOldString(p_string), r_d1, r_d2);
}

Boolean MCU_stoi2x4(const MCString &s, int2 &d1, int2 &d2, int2 &d3, int2 &d4)
{
	int32_t t_d1, t_d2, t_d3, t_d4;
	if (!MCU_stoi4x4(s, t_d1, t_d2, t_d3, t_d4))
		return False;
	
	d1 = t_d1;
	d2 = t_d2;
	d3 = t_d3;
	d4 = t_d4;
	
	return True;
}

/* WRAPPER */ bool MCU_stoi2x4(MCStringRef p_string, int16_t& r_d1, int16_t& r_d2, int16_t& r_d3, int16_t& r_d4)
{
	return True == MCU_stoi2x4(MCStringGetOldString(p_string), r_d1, r_d2, r_d3, r_d4);
}

/* WRAPPER */ bool MCU_stoi4x4(MCStringRef p_string, int32_t& r_d1, int32_t& r_d2, int32_t& r_d3, int32_t& r_d4)
{
	return True == MCU_stoi4x4(MCStringGetOldString(p_string), r_d1, r_d2, r_d3, r_d4);
}

Boolean MCU_stoi4x4(const MCString &s, int32_t &d1, int32_t &d2, int32_t &d3, int32_t &d4)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d1 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	d2 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	d3 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	d4 = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return False;
	return True;
}

Boolean MCU_stoi4x2(const MCString &s, int32_t &d1, int32_t &d2)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d1 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	d2 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return False;
	return True;
}

/* WRAPPER */ bool MCU_stoi4(MCStringRef p_string, int4& r_d)
{
    return True == MCU_stoi4(MCStringGetOldString(p_string), r_d);
}

Boolean MCU_stoi4(const MCString &s, int4 &d)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d = MCU_strtol(sptr, l, '\0', done);
	if (!done || l != 0)
		return False;
	return True;
}
/* WRAPPER */ bool MCU_stoui4(MCStringRef p_string, uint4 &r_d)
{
	return True == MCU_stoui4(MCStringGetOldString(p_string), r_d);
}

Boolean MCU_stoui4(const MCString &s, uint4 &d)
{
	const char *sptr = s.getstring();
	uint4 l = s.getlength();
	Boolean done;
	d = MCU_strtol(sptr, l, '\0', done);
	if (!done || l != 0)
		return False;
	return True;
}

bool MCU_stoui4x2(MCStringRef p_string, uint4 &r_d1, uint4 &r_d2)
{
    const char *sptr = MCStringGetCString(p_string);
	uint4 l = MCStringGetLength(p_string);
	Boolean done;
	r_d1 = MCU_strtol(sptr, l, ',', done, True, False);
	if (!done || l == 0)
		return false;
	r_d2 = MCU_strtol(sptr, l, '\0', done, True, False);
	if (!done || l != 0)
		return false;
	return true;
}

/* WRAPPER */ bool MCU_stob(MCStringRef p_string, bool& r_condition)
{
	Boolean t_condition;
	bool t_success;
	t_success = True == MCU_stob(MCStringGetOldString(p_string), t_condition);
	if (t_success)
	{
		r_condition = t_condition == True;
		return true;
	}

	return false;
}

Boolean MCU_stob(const MCString &s, Boolean &condition)
{
	if (s.getlength() == 4 && (s.getstring() == MCtruestring
	                           || !MCU_strncasecmp(s.getstring(),
	                                               MCtruestring, 4)))
	{
		condition = True;
		return True;
	}
	if (s.getlength() == 5 && (s.getstring() == MCfalsestring
	                           || !MCU_strncasecmp(s.getstring(),
	                                               MCfalsestring, 5)))
	{
		condition = False;
		return True;
	}
	return False;
}

void MCU_lower(char *dptr, const MCString &s)
{
	uint4 length = s.getlength();
	const uint1 *sptr = (uint1 *)s.getstring();
	uint4 i;
	for (i = 0 ; i < length ; i++)
		*dptr++ = MCS_tolower(*sptr++);
}

Boolean MCU_offset(const MCString &part, const MCString &whole,
                   uint4 &offset, Boolean casesensitive)
{
	uint4 tl = part.getlength();
	uint4 sl = whole.getlength();
	offset = 0;

	if (tl > sl || tl == 0 || sl == 0)
		return False;
	uint4 length = sl - tl;
	uint4 i;
	const uint1 *pptr = (uint1 *)part.getstring();
	const uint1 *wptr = (uint1 *)whole.getstring();
	if (casesensitive)
		for (i = 0 ; i <= length ; i++)
		{
			const uint1 *sptr = pptr;
			const uint1 *dptr = wptr + i;
			if (*sptr != *dptr)
				continue;
			int4 diff = 0;
			uint4 n = tl;
			while (n--)
				if (*dptr++ != *sptr++)
				{
					diff = 1;
					break;
				}
			if (diff == 0)
			{
				offset = i;
				return True;
			}
		}
	else
		for (i = 0 ; i <= length ; i++)
		{
			int4 diff = 0;

			char t_p, t_w;
			t_p = *pptr;
			t_w = *(wptr + i);

			t_p = MCS_tolower(t_p);
			t_w = MCS_tolower(t_w);

			if (t_p != t_w)
				continue;
			else
				diff = MCU_strncasecmp((char *)pptr, (char *)wptr + i, tl);

			if (diff == 0)
			{
				offset = i;
				return True;
			}
		}
	return False;
}

void MCU_chunk_offset(MCExecPoint &ep, MCString &w,
                      Boolean whole, Chunk_term delimiter)
{
	uint4 i;
	uint4 chunkstart = 0;
	const char *wptr = w.getstring();
	uint4 count = 1;
	if (delimiter == CT_WORD)
	{
		Boolean t_match = false;

		uint4 length = w.getlength();
		if (length == 0)
			count = 0;
		while (chunkstart < length)
		{
			while (chunkstart < length && isspace((uint1)wptr[chunkstart]))
				chunkstart++;
			uint4 wordstart = chunkstart;
			if (wptr[chunkstart] == '"')
			{
				chunkstart++;
				while (chunkstart < length && wptr[chunkstart] != '"'
				        && wptr[chunkstart] != '\n')
					chunkstart++;
				if (chunkstart < length && wptr[chunkstart] == '"')
					chunkstart++;
			}
			else
				while (chunkstart < length && !isspace((uint1)wptr[chunkstart]))
					chunkstart++;
			if (whole)
			{
				if (chunkstart - wordstart == ep.getsvalue().getlength())
				{
					if (ep.getcasesensitive())
					{
						if (strncmp(wptr + wordstart, ep.getsvalue().getstring(),
						            ep.getsvalue().getlength()) == 0)
						{
							t_match = true;
							break;
						}
					}
					else
						if (MCU_strncasecmp(wptr + wordstart, ep.getsvalue().getstring(),
						                    ep.getsvalue().getlength()) == 0)
						{
							t_match = true;
							break;
						}
				}
			}
			else
			{
				MCString word(wptr + wordstart, chunkstart - wordstart);
				if (MCU_offset(ep.getsvalue(), word, i, ep.getcasesensitive()))
				{
					t_match = true;
					break;
				}
			}
			count++;
		}
		if (!t_match)
			count = 0;
	}
	else
	{
		char c = delimiter == CT_LINE ? ep.getlinedel() : ep.getitemdel();
		while (True)
		{
			if (MCU_offset(ep.getsvalue(), w, i, ep.getcasesensitive()))
			{
				if (whole)
				{
					uint4 l = ep.getsvalue().getlength();
					// MW-2006-04-21: [[ Purify ]] i + l != w.getlength() should be first
					if ((i != 0 && w.getstring()[i - 1] != c)
					        || (i + l != w.getlength() && w.getstring()[i + l] != c))
					{
						while (i != w.getlength() && w.getstring()[i] != c)
							i++;
						if (i == 0)
						{// delimiter is in find string
							count = 0;
							break;
						}
						w.set(w.getstring() + i, w.getlength() - i);
						continue;
					}
					else
						i += w.getstring() - &wptr[chunkstart];
				}
				i += chunkstart;
				while (chunkstart < i)
					if (wptr[chunkstart++] == c)
						count++;
			}
			else
				count = 0;
			break;
		}
	}
	ep.setnvalue(count);
}

void MCU_additem(char *&dptr, const char *sptr, Boolean first)
{
	uint4 dlength = strlen(dptr);
	uint4 slength;
	if (sptr == NULL)
		slength = 0;
	else
		slength = strlen(sptr);
	MCU_realloc((char **)&dptr, dlength, dlength + slength + 2, sizeof(char));
	if (!first)
		dptr[dlength++] = ',';
	if (sptr == NULL)
		dptr[dlength++] = '\0';
	else
		strcpy(&dptr[dlength], sptr);
}

void MCU_addline(char *&dptr, const char *sptr, Boolean first)
{
	uint4 dlength = strlen(dptr);
	uint4 slength;
	if (sptr == NULL)
		slength = 0;
	else
		slength = strlen(sptr);
	MCU_realloc((char **)&dptr, dlength, dlength + slength + 2, sizeof(char));
	if (!first)
		dptr[dlength++] = '\n';
	if (sptr == NULL)
		dptr[dlength++] = '\0';
	else
		strcpy(&dptr[dlength], sptr);
}

void MCU_break_string(const MCString &s, MCString *&ptrs, uint2 &nptrs,
                      Boolean isunicode)
{
	delete ptrs;
	ptrs = NULL;
	nptrs = 0;
	uint4 len = s.getlength();
	if (isunicode && (long)len & 1)
		len--;
	const char *string = s.getstring();
	if (string == NULL)
		return;
	const char *sptr = string;
	nptrs = 1;
	uint4 tlen = len;
	while (tlen)
	{
		tlen -= MCU_charsize(isunicode);
		if (MCU_comparechar(sptr, '\n', isunicode) && tlen)
			nptrs++;
		sptr += MCU_charsize(isunicode);
	}
	ptrs = (MCString *)new char[nptrs * sizeof(MCString)]; // GCC bug
	uint2 curptr = 0;
	sptr = string;
	const char *eptr =  sptr;
	Boolean wasfound = False;
	uint4 l = len;
	ptrs[0].set(sptr, len);
	while (l)
	{
		if (((wasfound = MCU_strchr(eptr, l, '\n', isunicode)) == True && l)
		        || sptr != NULL)
		{
			uint2 length = wasfound ? eptr - sptr : (string + len) - sptr ;
			ptrs[curptr++].set(sptr, length);
			if (!wasfound)
				break;
			eptr += MCU_charsize(isunicode);
			l -= MCU_charsize(isunicode);
			sptr = eptr;
		}
	}
}

// AL-2013-14-07 [[ Bug 10445 ]] Sort international on Android
#if defined(_MAC_DESKTOP) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
extern compare_t MCSystemCompareInternational(MCStringRef, MCStringRef);
#endif

static void msort(MCSortnode *b, uint4 n, MCSortnode *t, Sort_type form, Boolean reverse)
{
	if (n <= 1)
		return;

	uint4 n1 = n / 2;
	uint4 n2 = n - n1;
	MCSortnode *b1 = b;
	MCSortnode *b2 = b + n1;

	msort(b1, n1, t, form, reverse);
	msort(b2, n2, t, form, reverse);

	MCSortnode *tmp = t;
	while (n1 > 0 && n2 > 0)
	{
		// NOTE:
		//
		// This code assumes the types in the MCSortnodes are correct for the
		// requested sort type. Bad things will happen if this isn't true...
		bool first;
		switch (form)
		{
		case ST_INTERNATIONAL:
			{
				const char *s1, *s2;
				s1 = MCStringGetCString(b1->svalue);
				s2 = MCStringGetCString(b2->svalue);
				
				// WARNING: this will *not* work properly on anything other
				// than OSX, iOS or Android: the LC_COLLATE locale facet is set to the
				// locale "en_US.<native encoding>"...
				//
				// Additionally, UTF-16 strings don't work at all.
                
                // AL-2013-14-07 [[ Bug 10445 ]] Sort international on Android
#if defined(_MAC_DESKTOP) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
				int result = MCSystemCompareInternational(b1->svalue, b2->svalue);
#else
				int result = strcoll(s1, s2);
#endif
				
				first = reverse ? result >= 0 : result <= 0;
				break;
			}

		case ST_TEXT:
			{
				const char *s1, *s2;
				s1 = MCStringGetCString(b1->svalue);
				s2 = MCStringGetCString(b2->svalue);
				
				// WARNING:
				//
				// This provides codepoint order sorting (not lexical sorting)
				// for strings. It will not, however, do the right thing with
				// UTF-16LE strings (the encoding used on x86 and ARM)...
				int result = strcmp(s1, s2);
				
				first = reverse ? result >= 0 : result <= 0;
				break;
			}
				
		default:
			{
				first = reverse
							? MCNumberFetchAsReal(b1->nvalue) >= MCNumberFetchAsReal(b2->nvalue)
							: MCNumberFetchAsReal(b1->nvalue) <= MCNumberFetchAsReal(b2->nvalue);
				break;
			}
		}
		
		if (first)
		{
			*tmp++ = *b1++;
			n1--;
		}
		else
		{
			*tmp++ = *b2++;
			n2--;
		}
	}
	for (uindex_t i = 0; i < n1; i++)
		tmp[i] = b1[i];
	for (uindex_t i = 0; i < (n - n2); i++)
		b[i] = t[i];
}

void MCU_sort(MCSortnode *items, uint4 nitems,
              Sort_type dir, Sort_type form)
{
	if (nitems <= 1)
		return;
	MCSortnode *tmp = new MCSortnode[nitems];
	msort(items, nitems, tmp, form, dir == ST_DESCENDING);
	delete[] tmp;
}

#if !defined(_DEBUG_MEMORY)
void MCU_realloc(char **data, uint4 osize, uint4 nsize, uint4 csize)
{
	char *ndata = new char[nsize * csize];
	if (data != NULL)
	{
		if (nsize > osize)
			memcpy(ndata, *data, osize * csize);
		else
			memcpy(ndata, *data, nsize * csize);
		delete *data;
	}
	*data = ndata;
}
#else
void _dbg_MCU_realloc(char **data, uint4 osize, uint4 nsize, uint4 csize, const char *f, int l)
{
	char *ndata = (char *)_malloc_dbg(nsize * csize, _NORMAL_BLOCK, f, l); //_old_new(f, l) char[nsize * csize];
	if (data != NULL)
	{
		if (nsize > osize)
			memcpy(ndata, *data, osize * csize);
		else
			memcpy(ndata, *data, nsize * csize);
		delete *data;
	}
	*data = ndata;
}
#endif

/* WRAPPER */ bool MCU_matchname(MCNameRef p_name, Chunk_term type, MCNameRef name)
{
    return MCU_matchname(MCNameGetOldString(p_name), type, name) == True;
}

Boolean MCU_matchname(const MCString &test, Chunk_term type, MCNameRef name)
{
    if (name == nil)
        return False;
    
    // AL-2013-07-29: [[ Bug 10981 ]] Allow the empty name for objects.
	if (MCNameIsEmpty(name) && test == MCnullmcstring)
		return True;

	if (MCNameIsEqualToOldString(name, test, kMCCompareCaseless))
		return True;

	Boolean match = False;
	MCString tname = MCNameGetOldString(name);
	static const char *nametable[] =
	    {
	        MCstackstring, MCaudiostring,
	        MCvideostring, MCbackgroundstring,
	        MCcardstring, MCnullstring,
	        MCgroupstring, MCnullstring,
	        MCbuttonstring, MCnullstring,
	        MCnullstring, MCscrollbarstring,
	        MCimagestring, MCgraphicstring,
	        MCepsstring, MCmagnifierstring,
	        MCcolorstring, MCfieldstring
	    };
	
	// MW-2013-07-29: [[ Bug 11068 ]] Make sure that we only match a reference
	//   of the form 'field "..."', and throw an error if not.
	const char *sptr = test.getstring();
	uint4 l = test.getlength();
	if (MCU_strchr(sptr, l, '"')
			&& l > tname.getlength() + 1
	        && sptr[tname.getlength() + 1] == '"'
	        && !MCU_strncasecmp(sptr + 1, tname.getstring(), tname.getlength())
	        && sptr - test.getstring() >= (int)strlen(nametable[type - CT_STACK])
	        && !MCU_strncasecmp(test.getstring(), nametable[type - CT_STACK],
	                            strlen(nametable[type - CT_STACK])))
	{
		if (l == tname.getlength() + 2)
			match = True;
		
		if (!match)
			MCLog("[[ Bug 11068 ]] match name '%s' to '%.*s' attempted and failed due to better checking", MCNameGetCString(name), test . getlength(), test . getstring());
	}

	return match;
}

void MCU_snap(int2 &p)
{
	if (!MCgrid)
		return;
	p = (p + (MCgridsize >> 1)) / MCgridsize * MCgridsize;
}

void MCU_roundrect(MCPoint *&points, uint2 &npoints,
                   const MCRectangle &rect, uint2 radius)
{
	uint2 i, j;

	if (points == NULL || npoints != 4 * QA_NPOINTS + 1)
	{
		delete points;
		points = new MCPoint[4 * QA_NPOINTS + 1];
		npoints = 4 * QA_NPOINTS + 1;
	}

	MCRectangle tr = rect;
	tr.width--;
	tr.height--;

	uint2 rr_width, rr_height;
	if (radius < tr.width >> 1)
		rr_width = radius;
	else
		rr_width = tr.width >> 1;
	if (radius < tr.height >> 1)
		rr_height = radius;
	else
		rr_height = tr.height >> 1;

	i = 0;
	for (j = 0 ; j < QA_NPOINTS ; j++)
	{
		points[i].x = tr.x + tr.width - rr_width
		              + (qa_points[j].x * rr_width / MAXINT2);
		points[i].y = tr.y + (qa_points[j].y * rr_height / MAXINT2);
		i++;
	}
	j = QA_NPOINTS;
	do
	{
		j--;
		points[i].x = tr.x + tr.width - rr_width
		              + (qa_points[j].x * rr_width / MAXINT2);
		points[i].y = tr.y + tr.height
		              - (qa_points[j].y * rr_height / MAXINT2);
		i++;
	}
	while (j);
	for (j = 0 ; j < QA_NPOINTS ; j++)
	{
		points[i].x = tr.x + rr_width - (qa_points[j].x * rr_width / MAXINT2);
		points[i].y = tr.y + tr.height
		              -(qa_points[j].y * rr_height / MAXINT2);
		i++;
	}
	j = QA_NPOINTS;
	do
	{
		j--;
		points[i].x = tr.x + rr_width
		              - (qa_points[j].x * rr_width / MAXINT2);
		points[i].y = tr.y + (qa_points[j].y * rr_height / MAXINT2);
		i++;
	}
	while (j);
	points[i] = points[0];
}

void MCU_unparsepoints(MCPoint *points, uint2 npoints, MCExecPoint &ep)
{
	uint2 i;
	//ep.getbuffer(I2L * 2 * npoints + 1);
	ep.clear();
	for (i = 0 ; i < npoints ; i++)
	{
		if (points[i].x != MININT2)
		{
			char buf[I2L * 2];
			sprintf(buf, "%d,%d", points[i].x, points[i].y);
	 		ep.concatcstring(buf, EC_RETURN, i == 0);
		}
		else
			ep.concatcstring(MCnullstring, EC_RETURN, i == 0);
	}
}


Boolean MCU_parsepoints(MCPoint *&points, uindex_t &noldpoints, MCStringRef data)
{
	Boolean allvalid = True;
	uint2 npoints = 0;
	uint4 l = MCStringGetLength(data);
	const char *sptr = MCStringGetCString(data);
	while (l)
	{
		Boolean done1, done2;
		// MDW-2013-06-09: [[ Bug 11041 ]] Round non-integer values to nearest.
		int2 i1= (int2)MCU_strtol(sptr, l, ',', done1, True);
		int2 i2 = (int2)MCU_strtol(sptr, l, ',', done2, True);
		while (l && !isdigit((uint1)*sptr) && *sptr != '-' && *sptr != '+')
		{
			l--;
			sptr++;
		}
		if (!done1 || !done2)
		{
			i1 = i2 = MININT2;
			allvalid = False;
		}
		if (npoints + 1 > noldpoints)
			MCU_realloc((char **)&points, npoints, npoints + 1, sizeof(MCPoint));
		points[npoints].x = i1;
		points[npoints++].y = i2;
		if (MCStringGetLength(data) - l > 2 && *(sptr - 1) == '\n'
		        && *(sptr - 2) == '\n')
		{
			if (npoints + 1 > noldpoints)
				MCU_realloc((char **)&points, npoints, npoints + 1, sizeof(MCPoint));
			points[npoints].x = MININT2;
			points[npoints++].y = MININT2;
			allvalid = False;
		}
	}
	noldpoints = npoints;
	return allvalid;
}

Boolean MCU_parsepoint(MCPoint &point, MCStringRef data)
{
	const char *sptr = MCStringGetCString(data);
	uint4 l = MCStringGetLength(data);
	Boolean done1, done2;
	// MDW-2013-06-09: [[ Bug 11041 ]] Round non-integer values to nearest.
	int2 i1= (int2)(MCU_strtol(sptr, l, ',', done1, True));
	int2 i2 = (int2)(MCU_strtol(sptr, l, ',', done2, True));
	if (!done1 || !done2)
	{
		i1 = i2 = MININT2;
		return False;
	}
	point.x = i1;
	point.y = i2;
	return True;
}

void MCU_set_rect(MCRectangle &rect, int2 x, int2 y, uint2 w, uint2 h)
{
	rect.x = x;
	rect.y = y;
	rect.width = w;
	rect.height = h;
}

void MCU_set_rect(MCRectangle32 &rect, int32_t x, int32_t y, int32_t w, int32_t h)
{
	rect.x = x;
	rect.y = y;
	rect.width = w;
	rect.height = h;
}

Boolean MCU_point_in_rect(const MCRectangle &srect, int2 x, int2 y)
{
	if (x >= srect.x && x < srect.x + srect.width
	        && y >= srect.y && y < srect.y + srect.height)
		return True;
	return False;
}

Boolean MCU_rect_in_rect(const MCRectangle &p, const MCRectangle &w)
{
	if (p.x >= w.x && p.x + p.width <= w.x + w.width
	        && p.y >= w.y && p.y + p.height <= w.y + w.height)
		return True;
	return False;
}


static inline double distance_to_point(int4 x, int4 y, int4 px, int4 py)
{
	double dx, dy;

	dx = px - x;
	dy = py - y;

	return dx * dx + dy * dy;
}

double MCU_squared_distance_from_line(int4 sx, int4 sy, int4 ex, int4 ey, int4 x, int4 y)
{
	double dx, dy;
	double d;

	dx = ex - sx;
	dy = ey - sy;

	if (dx == 0 && dy == 0)
		d = distance_to_point(x, y, sx, sy);
	else
	{
		double pdx, pdy;
		double u;

		pdx = x - sx;
		pdy = y - sy;

		u = (pdx * dx + pdy * dy) / (dx * dx + dy * dy);

		if (u <= 0)
			d = distance_to_point(x, y, sx, sy);
		else if (u >= 1)
			d = distance_to_point(x, y, ex, ey);
		else
			d = distance_to_point((int4)(sx + u * dx), (int4)(sy + u * dy), x, y);
	}

	return d;
}


Boolean MCU_point_on_line(MCPoint *points, uint2 npoints,
                          int2 x, int2 y, uint2 linesize)
{
	// OK-2008-12-04: [[Bug 7292]] - Old code replaced with stuff copied from pathprocess.cpp
	uint2 i;
	for (i = 0 ; i < npoints -  1 ; i++)
	{
		// SMR 1913 expand radius for hit testing lines
		linesize >>= 1;
		linesize *= linesize;

		double t_distance;
		t_distance = MCU_squared_distance_from_line(points[i]. x, points[i] . y, points[i + 1] . x, points[i + 1] . y, x, y);
		if (t_distance < linesize + (4 * 4))
			return True;
	}
	return False;
}

Boolean MCU_point_in_polygon(MCPoint *points, uint2 npoints, int2 x, int2 y)
{
	// SMR 1958 don't do check if no points
	if (npoints <= 1)
		return False;
	MCU_offset_points(points, npoints, -x, -y);

	MCPoint *endLp = &points[npoints];
	MCPoint *lp = points;

	uint2 ncross = 0;
	int2 sign = lp->y < 0 ? -1 : 1;
	int2 nextSign;
	lp++;
	for(; lp < endLp ; lp++, sign = nextSign)
	{
		nextSign = lp->y < 0 ? -1 : 1;
		if (sign != nextSign)
		{
			if (lp[-1].x > 0)
			{
				if (lp->x > 0)
				{
					ncross++;
					continue;
				}
			}
			else
			{
				if (lp->x < 0)
					continue;
			}
			if (lp[-1].x - lp[-1].y * (lp->x - lp[-1].x) / (lp->y - lp[-1].y) > 0)
				ncross++;
		}
	}
	MCU_offset_points(points, npoints, x, y);
	if (ncross & 1)
		return True;
	return False;
}

void MCU_offset_points(MCPoint *points, uint2 npoints, int2 xoff, int2 yoff)
{
	uint2 i;
	for (i = 0 ; i < npoints ; i++)
	{
		if (points[i].x != MININT2)
		{
			points[i].x += xoff;
			points[i].y += yoff;
		}
	}
}

MCRectangle MCU_compute_rect(int2 x1, int2 y1, int2 x2, int2 y2)
{
	MCRectangle drect;

	if (x1 < x2)
	{
		drect.x = x1;
		drect.width = x2 - x1 + 1;
	}
	else
	{
		drect.x = x2;
		drect.width = x1 - x2 + 1;
	}
	if (y1 < y2)
	{
		drect.y = y1;
		drect.height = y2 - y1 + 1;
	}
	else
	{
		drect.y = y2;
		drect.height = y1 - y2 + 1;
	}
	return drect;
}

MCRectangle MCU_center_rect(const MCRectangle &one, const MCRectangle &two)
{
	MCRectangle drect;
	drect.x = one.x + ((one.width - two.width) >> 1);
	drect.y = one.y + ((one.height - two.height) >> 1);
	drect.width = two.width;
	drect.height = two.height;
	return drect;
}

MCRectangle MCU_bound_rect(const MCRectangle &srect,
                           int2 x, int2 y, uint2 width, uint2 height)
{
	MCRectangle drect = srect;
	if (drect.x + drect.width > x + width)
		drect.x = x + width - drect.width;
	if (drect.x < x)
		drect.x = x;
	if (drect.y + drect.height > y + height)
		drect.y = y + height - drect.height;
	if (drect.y < y)
		drect.y = y;
	return drect;
}

MCRectangle MCU_clip_rect(const MCRectangle &srect,
                          int2 x, int2 y, uint2 width, uint2 height)
{
	MCRectangle drect = srect;
	if (srect.x < x)
	{
		drect.x = x;
		if (x - srect.x > srect.width)
			drect.width = 0;
		else
			drect.width -= x - srect.x;
	}
	if (srect.x + srect.width > x + width)
		if (srect.x > x + width)
			drect.width = 0;
		else
			drect.width = x + width - drect.x;
	if (srect.y < y)
	{
		drect.y = y;
		if (y - srect.y > srect.height)
			drect.height = 0;
		else
			drect.height -= y - srect.y;
	}
	if (srect.y + srect.height > y + height)
		if (srect.y > y + height)
			drect.height = 0;
		else
			drect.height = y + height - drect.y;
	return drect;
}

MCRectangle MCU_intersect_rect(const MCRectangle &one, const MCRectangle &two)
{
	MCRectangle drect;

	if (one . width == 0 || one . height == 0)
		return one;

	if (two . width == 0 || two . height == 0)
		return two;

	if (one.x > two.x)
		drect.x = one.x;
	else
		drect.x = two.x;
	if (one.y > two.y)
		drect.y = one.y;
	else
		drect.y = two.y;
	if (one.x + one.width > two.x + two.width)
		if (two.x + two.width < drect.x)
			drect.width = 0;
		else
			drect.width = two.x + two.width - drect.x;
	else
		if (one.x + one.width < drect.x)
			drect.width = 0;
		else
			drect.width = one.x + one.width - drect.x;
	if (one.y + one.height > two.y + two.height)
		if (two.y + two.height < drect.y)
			drect.height = 0;
		else
			drect.height = two.y + two.height - drect.y;
	else
		if (one.y + one.height < drect.y)
			drect.height = 0;
		else
			drect.height = one.y + one.height - drect.y;
	return drect;
}

MCRectangle MCU_union_rect(const MCRectangle &one, const MCRectangle &two)
{
	MCRectangle drect;
	int2 lrx, lry;

	if (one . width == 0 || one . height == 0)
		return two;
	else if (two . width == 0 || two . height == 0)
		return one;

	if (one.x + one.width > two.x + two.width)
		lrx = one.x + one.width;
	else
		lrx = two.x + two.width;
	if (one.y + one.height > two.y + two.height)
		lry = one.y + one.height;
	else
		lry = two.y + two.height;
	if (one.x < two.x)
		drect.x = one.x;
	else
		drect.x = two.x;
	if (one.y < two.y)
		drect.y = one.y;
	else
		drect.y = two.y;
	drect.width = lrx - drect.x;
	drect.height = lry - drect.y;
	return drect;
}

MCRectangle MCU_subtract_rect(const MCRectangle &one, const MCRectangle &two)
{
	MCRectangle drect = one;
	if (one.x == two.x && one.width == two.width)
		if (one.y > two.y)
		{
			uint2 overlap = two.y + two.height - one.y;
			drect.height -= overlap;
			drect.y += overlap;
		}
		else
			drect.height -= one.y + one.height - two.y;
	else
		if (one.y == two.y && one.height == two.height)
			if (one.x > two.x)
			{
				uint2 overlap = two.x + two.width - one.x;
				drect.width -= overlap;
				drect.x += overlap;
			}
			else
				drect.width -= one.x + one.width - two.x;
	return drect;
}

MCRectangle MCU_reduce_rect(const MCRectangle &srect, int2 amount)
{
	MCRectangle drect;
	drect.x = srect.x + amount;
	drect.y = srect.y + amount;
	if (amount << 1 > srect.width)
		drect.width = 0;
	else
		drect.width = srect.width - (amount << 1);
	if (amount << 1 > srect.height)
		drect.height = 0;
	else
		drect.height = srect.height - (amount << 1);
	return drect;
}

MCRectangle MCU_scale_rect(const MCRectangle &srect, int2 factor)
{
	MCRectangle drect;
	if (factor < 0)
	{
		factor = -factor;
		drect.x = srect.x / factor;
		drect.y = srect.y / factor;
		drect.width = srect.width / factor;
		drect.height = srect.height / factor;
	}
	else
	{
		drect.x = srect.x * factor;
		drect.y = srect.y * factor;
		drect.width = srect.width * factor;
		drect.height = srect.height * factor;
	}
	return drect;
}

MCRectangle MCU_offset_rect(const MCRectangle& r, int2 ox, int2 oy)
{
	MCRectangle nr;
	nr . x = r . x + ox;
	nr . y = r . y + oy;
	nr . width = r . width;
	nr . height = r . height;
	return nr;
}

MCRectangle MCU_recttoroot(MCStack *sptr, const MCRectangle &orect)
{
	return sptr -> recttoroot(orect);
}

void MCU_getshift(uint4 mask, uint2 &shift, uint2 &outmask)
{
	shift = 0;
	while (shift < 32 && !(mask & 1))
	{
		mask >>= 1;
		shift++;
	}
	uint2 i = shift;
	uint2 j = 0;
	while (i < 32 && mask & 1)
	{
		mask >>= 1;
		i++;
		j++;
	}
	outmask = j;
}

void MCU_choose_tool(MCExecContext& ctxt, MCStringRef p_input, Tool p_tool)
{
	Tool t_new_tool;
	MColdtool = MCcurtool;
	MCAutoStringRef t_tool_name;
	if (p_tool != T_UNDEFINED)
	{
		t_new_tool = p_tool;
		MCStringCreateWithCString(MCtoolnames[t_new_tool], &t_tool_name);
	}
	else
	{
		t_tool_name = p_input;
		if (MCStringGetLength(*t_tool_name) < 3)
		{
			ctxt . LegacyThrow(EE_CHOOSE_BADTOOL);
			return;
		}
		uint2 i;
		for (i = 0 ; i <= T_TEXT ; i++)
			if (strncmp(MCStringGetCString(*t_tool_name), MCtoolnames[i], 3) == 0)
			{
				t_new_tool = (Tool)i;
				break;
			}
		if (i > T_TEXT)
		{
			ctxt . LegacyThrow(EE_CHOOSE_BADTOOL);
			return;
		}
	}
	if (t_new_tool == MCcurtool)
		return;

	if (MCeditingimage != NULL)
		MCeditingimage -> canceldraw();

	MCcurtool = t_new_tool;

	MCundos->freestate();
	if (MCcurtool != T_POINTER)
		MCselected->clear(True);
	if (MCactiveimage != NULL && MCcurtool != T_SELECT)
		MCactiveimage->endsel();
	MCeditingimage = NULL;
	if (MCactivefield != NULL
	        && MCactivefield->getstack()->gettool(MCactivefield) != T_BROWSE)
		MCactivefield->getstack()->kunfocus();
	ctxt . GetObject()->getstack()->resetcursor(True);
	if (MCcurtool == T_BROWSE)
		MCstacks->restartidle();
	if (MCtopstackptr != NULL)
		MCtopstackptr->updatemenubar();
	ctxt . GetObject()->message_with_valueref_args(MCM_new_tool, *t_tool_name);
}

Exec_stat MCU_choose_tool(MCExecPoint &ep, Tool littool, uint2 line, uint2 pos)
{
	Tool t_new_tool;
	MColdtool = MCcurtool;
	if (littool != T_UNDEFINED)
	{
		t_new_tool = littool;
		ep.setstaticcstring(MCtoolnames[t_new_tool]);
	}
	else
	{
		if (ep.getsvalue().getlength() < 3)
		{
			MCeerror->add(EE_CHOOSE_BADTOOL, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
		uint2 i;
		for (i = 0 ; i <= T_TEXT ; i++)
            // SN-13-10-04: [[ Bug 11193 ]] set the tool to Browse fails - case-sensitive
			if (MCU_strncasecmp(MCtoolnames[i], ep.getsvalue().getstring(), 3) == 0)
			{
				t_new_tool = (Tool)i;
				break;
			}
		if (i > T_TEXT)
		{
			MCeerror->add(EE_CHOOSE_BADTOOL, line, pos, ep.getsvalue());
			return ES_ERROR;
		}
	}
	if (t_new_tool == MCcurtool)
		return ES_NORMAL;

	if (MCeditingimage != NULL)
		MCeditingimage -> canceldraw();

	MCcurtool = t_new_tool;

	MCundos->freestate();
	if (MCcurtool != T_POINTER)
		MCselected->clear(True);
	if (MCactiveimage != NULL && MCcurtool != T_SELECT)
		MCactiveimage->endsel();
	MCeditingimage = nil;
	if (MCactivefield != NULL
	        && MCactivefield->getstack()->gettool(MCactivefield) != T_BROWSE)
		MCactivefield->getstack()->kunfocus();
	ep.getobj()->getstack()->resetcursor(True);
	if (MCcurtool == T_BROWSE)
		MCstacks->restartidle();
	if (MCtopstackptr != NULL)
		MCtopstackptr->updatemenubar();
	ep.getobj()->message_with_valueref_args(MCM_new_tool, ep.getvalueref());
	return ES_NORMAL;
}

Exec_stat MCU_dofrontscripts(Handler_type htype, MCNameRef mess, MCParameter *params)
{
	Exec_stat stat = ES_NOT_HANDLED;

	if (MCfrontscripts != NULL)
	{
		MCObjectList *optr = MCfrontscripts;
		do
		{
			if (!optr->getremoved())
			{
				// MW-2011-01-05: Make sure dynamicpath global is sensible.
				Boolean olddynamic = MCdynamicpath;
				MCdynamicpath = MCdynamiccard != NULL;

				// PASS STATE FIX
				Exec_stat oldstat = stat;
				stat = optr->getobject()->handle(htype, mess, params, nil);
				
				MCdynamicpath = olddynamic;
				
				if (stat != ES_NOT_HANDLED && stat != ES_PASS)
					break;

				if (oldstat == ES_PASS && stat == ES_NOT_HANDLED)
					stat = ES_PASS;
			}
			optr = optr->next();
		}
		while (optr != MCfrontscripts);
	}

	return stat;
}
//
//bool MCU_path2std(MCStringRef p_path, MCStringRef& r_stdpath)
//{
//	uindex_t t_length = MCStringGetLength(p_path);
//	if (t_length == 0)
//		return MCStringCopy(p_path, r_stdpath);
//
//	MCAutoNativeCharArray t_path;
//	if (!t_path.New(t_length))
//		return false;
//
//	const char_t *t_src = MCStringGetNativeCharPtr(p_path);
//	char_t *t_dst = t_path.Chars();
//
//	for (uindex_t i = 0; i < t_length; i++)
//	{
//#ifdef _MACOSX
//		if (t_src[i] == '/')
//			t_dst[i] = ':';
//		else if (t_src[i] == ':')
//			t_dst[i] = '/';
//		else
//			t_dst[i] = t_src[i];
//#else
//		if (t_src[i] == '/')
//			t_dst[i] = '\\';
//		else if (t_src[i] == '\\')
//			t_dst[i] = '/';
//		else
//			t_dst[i] = t_src[i];
//#endif
//	}
//
//	return t_path.CreateStringAndRelease(r_stdpath);
//}
//
//void MCU_path2std(char *dptr)
//{
//	if (dptr == NULL || !*dptr)
//		return;
//	do
//	{
//#ifdef _MACOSX
//		if (*dptr == '/')
//			*dptr = ':';
//		else if (*dptr == ':')
//			*dptr = '/';
//#else
//		if (*dptr == '/')
//			*dptr = '\\';
//		else if (*dptr == '\\')
//			*dptr = '/';
//#endif
//	}
//	while (*++dptr);
//}

bool MCU_path2native(MCStringRef p_path, MCStringRef& r_native_path)
{
#ifdef _WIN32
	uindex_t t_length = MCStringGetLength(p_path);
	if (t_length == 0)
		return MCStringCopy(p_path, r_native_path);

	MCAutoNativeCharArray t_path;
	if (!t_path.New(t_length))
		return false;

	const char_t *t_src = MCStringGetNativeCharPtr(p_path);
	char_t *t_dst = t_path.Chars();

	for (uindex_t i = 0; i < t_length; i++)
	{
		if (t_src[i] == '/')
			t_dst[i] = '\\';
		else if (t_src[i] == '\\')
			t_dst[i] = '/';
		else
			t_dst[i] = t_src[i];
	}

	return t_path.CreateStringAndRelease(r_native_path);
#else
	return MCStringCopy(p_path, r_native_path);
#endif
}

void MCU_path2native(char *dptr)
{
	if (dptr == NULL || !*dptr)
		return;
#if defined _WIN32
	do
	{
		if (*dptr == '/')
			*dptr = '\\';
		else if (*dptr == '\\')
			*dptr = '/';
	}
	while (*++dptr);
#endif
}

// MW-2004-11-26: Copy null-terminated string at p_src to p_dest, the strings
//   are allowed to overlap.
inline void strmove(char *p_dest, const char *p_src)
{
	while(*p_src != 0)
		*p_dest++ = *p_src++;
	*p_dest = 0;
}

// MW-2004-11-26: Replace strcpy with strmov - overalapping regions (VG)
void MCU_fix_path(MCStringRef in, MCStringRef& r_out)
{
	char *cstr = strdup(MCStringGetCString(in));

	char *fptr = cstr; //pointer to search forward in curdir
	while (*fptr)
	{
		if (*fptr == '/' && *(fptr + 1) == '.'
		        && *(fptr + 2) == '.' && *(fptr + 3) == '/')
		{//look for "/../" pattern
			if (fptr == cstr)
				strmove(fptr, fptr + 3);
			else
			{
				char *bptr = fptr - 1;
				while (True)
				{ //search backword for '/'
					if (*bptr == '/')
					{
						strmove(bptr, fptr + 3);
						fptr = bptr;
						break;
					}
					else
						bptr--;
				}
			}
		}
		else
			if (*fptr == '/' && *(fptr + 1) == '.' && *(fptr + 2) == '/')
				strmove(fptr, fptr + 2); //erase the '/./'
			else
#ifdef _MACOSX
				if (*fptr == '/' && *(fptr + 1) == '/')
#else
				if (fptr != cstr && *fptr == '/' && *(fptr + 1) == '/')
#endif

					strmove(fptr, fptr + 1); //erase the extra '/'
				else
					fptr++;
	}
	/* UNCHECKED */ MCStringCreateWithCStringAndRelease((char_t *)cstr, r_out);

}

bool MCFiltersBase64Encode(MCDataRef p_src, MCStringRef& r_dst);

void MCU_base64encode(MCDataRef in, MCStringRef &out)
{
	/* UNCHECKED */ MCFiltersBase64Encode(in, out);
}

bool MCFiltersBase64Decode(MCStringRef p_src, MCDataRef& r_dst);

void MCU_base64decode(MCStringRef in, MCDataRef &out)
{
	/* UNCHECKED */ MCFiltersBase64Decode(in, out);
}

bool MCFiltersUrlEncode(MCStringRef p_source, MCStringRef& r_result);

void MCU_urlencode(MCStringRef p_url, MCStringRef &r_encoded)
{
	/* UNCHECKED */ MCFiltersUrlEncode(p_url, r_encoded);
}

bool MCFiltersUrlDecode(MCStringRef p_source, MCStringRef& r_result);

void MCU_urldecode(MCStringRef p_source, MCStringRef& r_result)
{
	/* UNCHECKED */ MCFiltersUrlDecode(p_source, r_result);
}

Boolean MCU_freeinserted(MCObjectList *&l)
{
	if (l != NULL)
	{
		MCObjectList *optr = l;
		do
		{
			if (optr->getremoved())
			{
				optr->remove
				(l);
				delete optr;
				return True;
			}
			optr = optr->next();
		}
		while (optr != l);
	}
	return False;
}

void MCU_cleaninserted()
{
	while (MCU_freeinserted(MCbackscripts))
		;
	while (MCU_freeinserted(MCfrontscripts))
		;
}

Exec_stat MCU_change_color(MCColor &c, MCStringRef &n, MCExecPoint &ep,
                           uint2 line, uint2 pos)
{
	MCColor color;
	MCStringRef t_name;
	MCAutoStringRef string;
	ep . copyasstringref(&string);

	t_name = nil;
	if (!MCscreen->parsecolor(*string, color, &t_name))
	{
		MCeerror->add(EE_PROPERTY_BADCOLOR, line, pos, *string);
		return ES_ERROR;
	}

	MCscreen->alloccolor(color);
	c = color;
	if (n != nil)
		MCValueRelease(n);
	if (t_name != nil)
		n = t_name;
	else
		n = nil;
	return ES_NORMAL;
}

void MCU_get_color(MCExecPoint& ep, MCStringRef name, MCColor& c)
{
	ep.setcolor(c, name != nil ? MCStringGetCString(name) : nil);
}

void MCU_dofunc(Functions func, uint4 &nparams, real8 &n,
                real8 tn, real8 oldn, MCSortnode *titems)
{
	real8 tp;
	switch (func)
	{
            // JS-2013-06-19: [[ StatsFunctions ]] Support for 'arithmeticMean' (was average)
        case F_ARI_MEAN:
            n += tn;
            nparams++;
			break;
            // JS-2013-06-19: [[ StatsFunctions ]] Support for 'averageDeviation'
        case F_AVG_DEV:
            tn = tn - oldn;
            n += abs(tn);
            nparams++;
            break;
            // JS-2013-06-19: [[ StatsFunctions ]] Support for 'geometricMean'
        case F_GEO_MEAN:
            if (nparams == 0)
                n = 1;
            tp = 1 / oldn;
            n *= pow(tn, tp);
            nparams++;
            break;
            // JS-2013-06-19: [[ StatsFunctions ]] Support for 'harmonicMean'
        case F_HAR_MEAN:
            n += 1/tn;
            nparams++;
            break;
        case F_MAX:
            if (nparams++ == 0 || tn > n)
                n = tn;
            break;
        case F_MIN:
            if (nparams++ == 0 || tn < n)
                n = tn;
            break;
        case F_SUM:
            n += tn;
            break;
        case F_MEDIAN:
            /* UNCHECKED */ MCNumberCreateWithReal(tn, titems[nparams].nvalue);
            nparams++;
            break;
            // JS-2013-06-19: [[ StatsFunctions ]] Support for 'populationStdDev', 'populationVariance', 'sampleStdDev' (was stdDev), 'sampleVariance'
        case F_POP_STD_DEV:
        case F_POP_VARIANCE:
        case F_SMP_STD_DEV:
        case F_SMP_VARIANCE:
            tn = tn - oldn;
            n += tn * tn;
            nparams++;
            break;
        case  F_UNDEFINED:
            nparams++;
            break;
        default:
            break;
	}
}

// MW-2013-06-25: [[ Bug 10983 ]] This function returns true if the given string
//   could be a url. It checks for strings of the form:
//     <letter> (<letter> | <digit> | '+' | '.' | '-')+ ':' <char>+
// MW-2013-07-01: [[ Bug 10975 ]] Update to a MCU_* utility function.
bool MCU_couldbeurl(const MCString& p_potential_url)
{
	uint4 t_length;
	const char *t_url;
	t_length = p_potential_url . getlength();
	t_url = p_potential_url . getstring();
	
	// If the first char isn't a letter, then we are done.
	if (t_length == 0 || !isalpha(t_url[0]))
		return false;
	
	uint4 t_colon_index;
	for(t_colon_index = 0; t_colon_index < t_length; t_colon_index++)
	{
		char t_char;
		t_char = t_url[t_colon_index];
		
		// If we find the ':' we are done (end of scheme).
		if (t_url[t_colon_index] == ':')
			break;
		
		// If the character isn't something allowed in a scheme name, we are done.
		if (!isalpha(t_char) && !isdigit(t_char) && t_char != '+' && t_char != '.' && t_char != '-')
			return false;
	}
	
	// If the scheme name < 2 chars, or there is nothing after it, we are done.
	if (t_colon_index < 2 || t_colon_index + 1 == t_length)
		return false;
	
	// If we get here then we could well have a url.
	return true;
}

void MCU_geturl(MCExecContext& ctxt, MCStringRef p_target, MCStringRef &r_output)
{
	MCAutoStringRef t_filename;
	if (MCStringGetLength(p_target) > 5 && MCStringBeginsWithCString(p_target, (const char_t*)"file:", kMCCompareCaseless))
	{
		MCStringCopySubstring(p_target, MCRangeMake(5, MCStringGetLength(p_target)-5), &t_filename);
		if (MCS_loadtextfile(*t_filename, r_output))
			return;
	}
	else if (MCStringGetLength(p_target) > 8 && MCStringBeginsWithCString(p_target, (const char_t*)"binfile:", kMCCompareCaseless))
	{
        MCAutoDataRef t_data;
		MCStringCopySubstring(p_target, MCRangeMake(8, MCStringGetLength(p_target)-8), &t_filename);
		if (MCS_loadbinaryfile(*t_filename, &t_data))
			if (ctxt.ConvertToString(*t_data, r_output))
				return;
	}
	else if (MCStringGetLength(p_target) > 8 && MCStringBeginsWithCString(p_target, (const char_t*)"resfile:", kMCCompareCaseless))
	{
		MCStringCopySubstring(p_target, MCRangeMake(8, MCStringGetLength(p_target)-8), &t_filename);	
		MCS_loadresfile(*t_filename, r_output);
		return;
	}
    else
	{
        // MW-2013-06-25: [[ Bug 10983 ]] Take more care to check if we do in fact
		//   have something that could be a url.
		// MW-2013-07-01: [[ Bug 10975 ]] Change to use MCU_couldbeurl utility function.
		if (MCU_couldbeurl(MCStringGetOldString(p_target)))
		{
			MCS_geturl(ctxt . GetObject(), p_target);
			MCurlresult->copyasvalueref((MCValueRef&)r_output);
			return;
		}
		else
            r_output = MCValueRetain(kMCEmptyString);
        
        return;
	}
	
    r_output = MCValueRetain(kMCEmptyString);
	ctxt.Throw();
}

void MCU_geturl(MCExecPoint &ep)
{
    MCAutoStringRef t_filename, t_output;
    MCStringCreateWithOldString(ep.getsvalue(), &t_filename);
    
    MCExecContext ctxt(ep);
    
    MCU_geturl(ctxt, *t_filename, &t_output);
    
    ep.setvalueref(*t_output);
}

void MCU_puturl(MCExecContext &ctxt, MCStringRef p_url, MCStringRef p_data)
{
	if (MCStringBeginsWithCString(p_url, (const char_t*)"file:", kMCCompareCaseless))
	{
		MCAutoStringRef t_path;
		/* UNCHECKED */ MCStringCopySubstring(p_url, MCRangeMake(5, MCStringGetLength(p_url) - 5), &t_path);
		MCS_savetextfile(*t_path, p_data);
	}
	else if (MCStringBeginsWithCString(p_url, (const char_t*)"binfile:", kMCCompareCaseless))
	{
		MCAutoStringRef t_path;
		MCAutoDataRef t_data;
		/* UNCHECKED */ MCStringCopySubstring(p_url, MCRangeMake(8, MCStringGetLength(p_url) - 8), &t_path);
		/* UNCHECKED */ ctxt.ConvertToData(p_data, &t_data);
		MCS_savebinaryfile(*t_path, *t_data);
	}
	else if (MCStringBeginsWithCString(p_url, (const char_t*)"resfile:", kMCCompareCaseless))
	{
		MCAutoStringRef t_path;
		MCAutoDataRef t_data;
		/* UNCHECKED */ MCStringCopySubstring(p_url, MCRangeMake(8, MCStringGetLength(p_url) - 8), &t_path);
		/* UNCHECKED */ ctxt.ConvertToData(p_data, &t_data);
		MCS_saveresfile(*t_path, *t_data);
	}
	else
	{
		MCAutoDataRef t_data;
		/* UNCHECKED */ ctxt.ConvertToData(p_data, &t_data);
		MCS_putintourl(ctxt.GetObject(), *t_data, p_url);
	}
}

void MCU_puturl(MCExecPoint &dest, MCExecPoint &data)
{
	MCAutoStringRef t_url;
	MCAutoStringRef t_data;
	/* UNCHECKED */ dest.copyasstringref(&t_url);
	/* UNCHECKED */ data.copyasstringref(&t_data);
	
	MCExecContext ctxt(data);
	MCU_puturl(ctxt, *t_url, *t_data);
}

////////////////////////////////////////////////////////////////////////////////

struct Language2Charset
{
	MCNameRef* langname;
	Lang_charset charset;
};

static Language2Charset langtocharsets[] = {
	{ &MCN_ansi, LCH_ENGLISH},
	{ &MCN_arabic, LCH_ARABIC },
	{ &MCN_bulgarian, LCH_BULGARIAN },
	{ &MCN_chinese, LCH_CHINESE},
	{ &MCN_english, LCH_ENGLISH},
	{ &MCN_greek, LCH_GREEK },
	{ &MCN_hebrew, LCH_HEBREW},
	{ &MCN_japanese, LCH_JAPANESE },
	{ &MCN_korean, LCH_KOREAN},
	{ &MCN_lithuanian, LCH_LITHUANIAN },
	{ &MCN_polish, LCH_POLISH},
	{ &MCN_roman, LCH_ROMAN },
	{ &MCN_russian, LCH_RUSSIAN },
	{ &MCN_simple_chinese, LCH_SIMPLE_CHINESE},
	{ &MCN_thai, LCH_THAI},
	{ &MCN_turkish, LCH_TURKISH },
	{ &MCN_ukrainian, LCH_UKRAINIAN},
	{ &MCN_unicode, LCH_UNICODE},
	{ &MCN_utf8, LCH_UTF8},
	{ &MCN_vietnamese, LCH_VIETNAMESE },
	{ &MCN_w_char, LCH_UNICODE},
	{ &MCN_asterisk_char, LCH_ENGLISH }
};

MCNameRef MCU_charsettolanguage(uint1 charset)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(langtocharsets); i++)
		if (langtocharsets[i].charset == charset)
			return *langtocharsets[i].langname;
	return kMCEmptyName;
}

uint1 MCU_languagetocharset(MCNameRef p_language)
{
	for (uinteger_t i = 0; i < ELEMENTS(langtocharsets); i++)
		if (MCNameIsEqualTo(p_language, *langtocharsets[i].langname))
			return i;

	return 0;
}

/* LEGACY */ uint1 MCU_languagetocharset(MCStringRef langname)
{
	MCNewAutoNameRef t_langname;
	/* UNCHECKED */  MCNameCreate(langname, &t_langname);
	return MCU_languagetocharset(*t_langname);
}

//////////

struct  CharSet2WinCharset
{
	int1 charset;
	uint1 wincharset;
};

static CharSet2WinCharset charset2wincharsets[] = {
            { LCH_ENGLISH, 0},
            { LCH_ENGLISH, 1},
            { LCH_JAPANESE, 128 },
            { LCH_CHINESE, 136},
            { LCH_ARABIC, 178},
            { LCH_ARABIC, 179},
            { LCH_ARABIC, 180},
            { LCH_HEBREW, 177},
            { LCH_HEBREW, 181},
            { LCH_GREEK, 161},
            { LCH_KOREAN, 129},
            { LCH_THAI,	222},
            {LCH_RUSSIAN, 204},
            { LCH_SIMPLE_CHINESE, 134}
        };


uint1 MCU_wincharsettocharset(uint2 wincharset)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(langtocharsets); i++)
		if (charset2wincharsets[i].wincharset == wincharset)
			return charset2wincharsets[i].charset;
	return 0;
}

uint1 MCU_charsettowincharset(uint1 charset)
{
	uint2 i;
	for (i = 0; i < ELEMENTS(langtocharsets); i++)
		if (charset2wincharsets[i].charset == charset)
			return charset2wincharsets[i].wincharset;
	return 0;
}

uint1 MCU_unicodetocharset(uint2 uchar)
{
	if (uchar <= MAXUINT1)
		return LCH_ENGLISH;
	if (uchar >= 0x0080 && uchar <= 0x024F)
		return LCH_GREEK;
	if (uchar >= 0x0600 && uchar <= 0x06FF)
		return LCH_ARABIC;
	if (uchar >= 0x0400 && uchar <= 0x04FF)
		return LCH_RUSSIAN;
	if (uchar >= 0x0370 && uchar <= 0x03FF)
		return LCH_GREEK;
	if (uchar >= 0x0590 && uchar <= 0x05FF)
		return LCH_HEBREW;
	if (uchar >= 0x0E00 && uchar <= 0x0E7F)
		return LCH_THAI;
	if (uchar >= 0x1F00 && uchar <= 0x1FFF)
		return LCH_GREEK;
	if (uchar >= 0xAC00 && uchar <= 0xD7A3)
		return LCH_KOREAN;
	return LCH_JAPANESE;
}

// MW-2005-02-08: New implementation of multibytetounicode
// MW-2006-04-13: Bad me - sign extending p_mbstring in an inappropriate manner caused a little problem!
void MCU_multibytetounicode(const char *p_mbstring, uint4 p_mblength,
                            char *p_buffer, uint4 p_capacity, uint4& r_used, uint1 p_mbcharset)
{
	if (p_mbcharset == LCH_UTF8)
		r_used = UTF8ToUnicode(p_mbstring, p_mblength, (uint2 *)p_buffer, p_capacity);
	else
        r_used = MCsystem ->TextConvert((const void*)p_mbstring, p_mblength, (void*)p_buffer, p_capacity, p_mbcharset, LCH_UNICODE);
}

// MW-2005-02-08: New implementation of unicodetomultibyte
void MCU_unicodetomultibyte(const char *p_ucstring, uint4 p_uclength,
                            char *p_buffer, uint4 p_capacity, uint4& r_used, uint1 p_mbcharset)
{
	if (p_mbcharset == LCH_UTF8)
		r_used = UnicodeToUTF8((uint2 *)p_ucstring, p_uclength, p_buffer, p_capacity);
	else
        r_used = MCsystem ->TextConvert((const void*)p_ucstring, p_uclength, (void*)p_buffer, p_capacity, LCH_UNICODE, p_mbcharset);
}

//////////

bool MCU_multibytetounicode(MCDataRef p_input, MCDataRef &r_output)
{
    MCAutoStringRef t_string;

    if (!MCStringDecode(p_input, kMCStringEncodingUTF8, false, &t_string))
        return false;

    if (!MCStringEncode(*t_string, kMCStringEncodingUTF16, false, r_output))
        return false;

	return true;
}

bool MCU_multibytetounicode(const MCString& p_src, uinteger_t p_charset, MCStringRef& r_unicode)
{
	const char *startptr = (const char*)p_src . getstring();
	uint4 length = p_src . getlength();
	uint4 dlength;
	MCU_multibytetounicode(startptr, length, NULL, 0, dlength, p_charset);

	MCAutoNativeCharArray t_buffer;
	if (!t_buffer.New(dlength))
		return false;

	char *dptr = (char*)t_buffer.Chars();
	MCU_multibytetounicode(startptr, length, dptr, dlength, dlength, p_charset);

	return t_buffer.CreateStringAndRelease(r_unicode);
}

bool MCU_multibytetounicode(MCStringRef p_src, uinteger_t p_charset, MCStringRef& r_unicode)
{
	return MCU_multibytetounicode(MCStringGetOldString(p_src), p_charset, r_unicode);
}

bool MCU_unicodetomultibyte(MCDataRef p_input, MCDataRef& r_output)
{
    MCAutoStringRef t_string;

    if (!MCStringDecode(p_input, kMCStringEncodingUTF16, false, &t_string))
        return false;

    if (!MCStringEncode(*t_string, kMCStringEncodingUTF8, false, r_output))
        return false;

	return true;
}

bool MCU_unicodetomultibyte(const MCString& p_src, uinteger_t p_charset, MCStringRef& r_multibyte)
{
	const char *startptr = (const char*)p_src . getstring();
	uint4 length = p_src . getlength();
	uint4 dlength;
	MCU_unicodetomultibyte(startptr, length, NULL, 0, dlength, p_charset);

	MCAutoNativeCharArray t_buffer;
	if (!t_buffer.New(dlength))
		return false;

	char *dptr = (char*)t_buffer.Chars();
	MCU_unicodetomultibyte(startptr, length, dptr, dlength, dlength, p_charset);

	return t_buffer.CreateStringAndRelease(r_multibyte);
}

bool MCU_unicodetomultibyte(MCStringRef p_src, uinteger_t p_charset, MCStringRef& r_multibyte)
{
	return MCU_unicodetomultibyte(MCStringGetOldString(p_src), p_charset, r_multibyte);
}

bool MCU_utf8tonative(MCStringRef p_utf8, MCStringRef& r_native)
{
	MCAutoStringRef t_unicode;
	return MCU_multibytetounicode(p_utf8, LCH_UTF8, &t_unicode) && MCU_unicodetomultibyte(*t_unicode, LCH_ENGLISH, r_native);
}

bool MCU_nativetoutf8(MCStringRef p_native, MCStringRef& r_utf8)
{
	MCAutoStringRef t_unicode;
	return MCU_multibytetounicode(p_native, LCH_ENGLISH, &t_unicode) && MCU_unicodetomultibyte(*t_unicode, LCH_UTF8, r_utf8);
}


bool MCU_mapunicode(const MCString& p_src, bool is_unicode, bool to_unicode, MCStringRef& r_string)
{
	if (to_unicode == is_unicode)
	{
		if (MCStringCreateWithNativeChars((const char_t *)p_src . getstring(), p_src . getlength(), r_string))
			return true;
	}
	else if (to_unicode)
	{
		if (MCU_multibytetounicode(p_src, LCH_ROMAN, r_string))
			return true;
	}
	else
	{
		if (MCU_unicodetomultibyte(p_src, LCH_ROMAN, r_string))
			return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool MCU_disjointrangeinclude(MCInterval*& x_ranges, int& x_count, int p_from, int p_to)
{
	MCInterval *t_new_ranges;
	t_new_ranges = (MCInterval *)malloc(sizeof(MCInterval) * (x_count + 1));
	if (t_new_ranges == NULL)
		return false;
	
	int t_new_count;
	t_new_count = 0;

	int t_range;
	t_range = 0;

	// Copy all source ranges completely before the new range
	while(t_range < x_count && x_ranges[t_range] . to + 1 < p_from)
		t_new_ranges[t_new_count++] = x_ranges[t_range++];

	int t_new_from;
	t_new_from = t_range < x_count ? MCU_min(x_ranges[t_range] . from, p_from) : p_from;

	// MW-2010-10-18: [[ Bug 9102 ]] Make sure the range after the new one is included if they touch.
	// Iterate though the input while it overlaps the new range
	while(t_range < x_count && x_ranges[t_range] . from - 1 <= p_to)
		t_range++;

	int t_new_to;
	t_new_to = t_range > 0 ? MCU_max(x_ranges[t_range - 1] . to, p_to) : p_to;

	t_new_ranges[t_new_count] . from = t_new_from;
	t_new_ranges[t_new_count] . to = t_new_to;
	t_new_count++;

	while(t_range < x_count)
		t_new_ranges[t_new_count++] = x_ranges[t_range++];

	if (x_ranges != NULL)
		free(x_ranges);

	x_ranges = t_new_ranges;
	x_count = t_new_count;

	return true;
}

bool MCU_disjointrangecontains(MCInterval* p_ranges, int p_count, int p_element)
{
	if (p_count == 0)
		return false;

	unsigned int t_low;
	t_low = 0;

	unsigned int t_high;
	t_high = p_count;

	while(t_low < t_high)
	{
		unsigned int t_mid;
		t_mid = t_low + (t_high - t_low) / 2;

		if (p_element < p_ranges[t_mid] . from)
			t_high = t_mid;
		else if (p_element > p_ranges[t_mid] . to)
			t_low = t_mid + 1;
		else
			return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

// MW-2013-05-02: [[ x64 ]] The 'x_length' parameter is always IO_header::len
//   which is now size_t, so match it.
IO_stat MCU_dofakewrite(char*& x_buffer, size_t& x_length, const void *p_data, uint4 p_size, uint4 p_count)
{
	uint4 t_capacity;
	if (x_length > 65536)
		t_capacity = (x_length + 65535) & ~65535;
	else
		t_capacity = (x_length + 4095) & ~4095;

	uint4 t_new_capacity;
	t_new_capacity = x_length + p_size * p_count;
	if (t_new_capacity > t_capacity)
	{
		if (t_new_capacity >= 65536)
			t_new_capacity = (t_new_capacity + 65535) & ~65535;
		else
			t_new_capacity = (t_new_capacity + 4095) & ~4095;

		char *t_new_buffer;
		t_new_buffer = (char *)realloc(x_buffer, t_new_capacity);
		if (t_new_buffer == NULL)
			return IO_ERROR;

		x_buffer = t_new_buffer;
	}

	memcpy(x_buffer + x_length, p_data, p_size * p_count);
	x_length += p_size * p_count;

	return IO_NORMAL;
}

///////////////////////////////////////////////////////////////////////////////

bool MCString::split(char p_char, MCString& r_head, MCString& r_tail)
{
	const char *t_string;
	uint4 t_length;

	t_string = sptr;
	t_length = length;

	if (!MCU_strchr(t_string, t_length, p_char, False))
	{		
		r_head = *this;
		r_tail . set(NULL, 0);
	}

	r_head . set(sptr, t_string - sptr);
	r_tail . set(t_string + 1, t_length);
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////

MCDictionary::MCDictionary(void)
{
	m_nodes = NULL;
}

MCDictionary::~MCDictionary(void)
{
	while(m_nodes != NULL)
	{
		Node *t_next;
		t_next = m_nodes -> next;
		delete (char *)(m_nodes -> buffer);
		delete m_nodes;
		m_nodes = t_next;
	}
}

void MCDictionary::Set(uint4 p_id, MCString p_value)
{
	Node *t_node;
	t_node = Find(p_id);
	if (t_node == NULL)
	{
		t_node = new Node;
		t_node -> next = m_nodes;
		t_node -> key = p_id;
		m_nodes = t_node;
	}
	else
		free(t_node -> buffer);

	t_node -> buffer = memdup(p_value . getstring(), p_value . getlength());
	t_node -> length = p_value . getlength();
}

bool MCDictionary::Get(uint4 p_id, MCString& r_value)
{
	Node *t_node;
	t_node = Find(p_id);
	if (t_node == NULL)
		return false;

	r_value . set((const char *)t_node -> buffer, t_node -> length);
	return true;
}

void MCDictionary::Pickle(void*& r_buffer, uint4& r_length)
{
	uint4 t_size;
	t_size = 4 + 4 + 4;
	for(Node *t_node = m_nodes; t_node != NULL; t_node = t_node -> next)
		t_size += ((t_node -> length + 3) & ~3) + 8;

	char *t_buffer;
	t_buffer = new char[t_size];
	
	char *t_buffer_ptr;
	t_buffer_ptr = t_buffer;
	((uint4 *)t_buffer_ptr)[0] = MCSwapInt32HostToNetwork('QDCT');
	((uint4 *)t_buffer_ptr)[1] = MCSwapInt32HostToNetwork(t_size);
	((uint4 *)t_buffer_ptr)[2] = 0;
	t_buffer_ptr += 12;

	for(Node *t_node = m_nodes; t_node != NULL; t_node = t_node -> next)
	{
		((uint4 *)t_buffer_ptr)[0] = MCSwapInt32HostToNetwork(t_node -> key);
		((uint4 *)t_buffer_ptr)[1] = MCSwapInt32HostToNetwork(t_node -> length);
		memcpy(t_buffer_ptr + 8, t_node -> buffer, t_node -> length);
		t_buffer_ptr += 8 + (t_node -> length + 3) & ~3;
	}

	((uint4 *)t_buffer)[2] = MCSwapInt32HostToNetwork(Checksum(t_buffer + 12, t_size - 12));

	r_buffer = t_buffer;
	r_length = t_size;
}

bool MCDictionary::Unpickle(const void* p_buffer, uint4 p_length)
{
	if (p_length < 12)
		return false;

	const char *t_buffer;
	t_buffer = (const char *)p_buffer;

	uint4 t_header;
	t_header = MCSwapInt32NetworkToHost(((uint4 *)t_buffer)[0]);

	uint4 t_size;
	t_size = MCSwapInt32NetworkToHost(((uint4 *)t_buffer)[1]);

	uint4 t_checksum;
	t_checksum = MCSwapInt32NetworkToHost(((uint4 *)t_buffer)[2]);

	if (t_header != 'QDCT')
		return false;

	if (t_size != p_length)
		return false;

	t_buffer += 12;
	t_size -= 12;

	if (t_checksum != Checksum(t_buffer, t_size))
		return false;

	while(t_size != 0)
	{
		uint4 t_node_key;
		t_node_key = MCSwapInt32NetworkToHost(((uint4 *)t_buffer)[0]);

		uint4 t_node_size;
		t_node_size = MCSwapInt32NetworkToHost(((uint4 *)t_buffer)[1]);

		t_buffer += 8;
		t_size -= 8;

		if (t_size < t_node_size)
			return false;
		
		Set(t_node_key, MCString(t_buffer, t_node_size));

		t_buffer += (t_node_size + 3) & ~3;
		t_size -= (t_node_size + 3) & ~3;
	}

	return true;
}

MCDictionary::Node *MCDictionary::Find(uint4 p_id)
{
	for(Node *t_node = m_nodes; t_node != NULL; t_node = t_node -> next)
		if (t_node -> key == p_id)
			return t_node;
	return NULL;
}

uint32_t MCDictionary::Checksum(const void *p_data, uint32_t p_length)
{
	uint32_t t_a, t_b;
	t_a = 1;
	t_b = 0;
	
	uint8_t *t_data;
	t_data = (uint8_t *)p_data;
	
	for(uint32_t t_i = 0; t_i < p_length; t_i++, t_data++)
	{
		t_a += *t_data;
		t_b += t_a;
		
		if (t_a > 0xFFFFFF00)
			t_a %= 65521;
		
		if (t_b > 0xFFFFFF00)
			t_b %= 65521;
	}

	return (((t_b % 65521) << 16) | (t_a % 65521));
}

bool MCU_compare_strings_native(const char *p_a, bool p_a_isunicode, const char *p_b, bool p_b_isunicode)
{
	MCExecPoint *t_convert_a = new MCExecPoint();
	MCExecPoint *t_convert_b = new MCExecPoint();
	MCString t_a;
	MCString t_b;

	t_convert_a->setsvalue(p_a);
	if (p_a_isunicode)
		t_convert_a->utf16tonative();
	t_a = t_convert_a->getsvalue();

	t_convert_b->setsvalue(p_b);
	if (p_b_isunicode)
		t_convert_b->utf16tonative();
	t_b = t_convert_b->getsvalue();

	bool t_compval = (t_a == t_b) == True;

	delete t_convert_a;
	delete t_convert_b;

	return t_compval;
}

///////////////////////////////////////////////////////////////////////////////

// MW-2013-05-21: [[ RandomBytes ]] Utility function for generating random bytes
//   which uses OpenSSL if available, otherwise falls back on system support.
bool MCU_random_bytes(size_t p_bytecount, MCDataRef& r_bytes)
{
#ifdef MCSSL
	// If SSL is available, then use that.
	static bool s_donotuse_ssl = false;
	if (!s_donotuse_ssl)
	{
        MCAutoByteArray t_buffer;
        
        if (InitSSLCrypt())
        {
            return (t_buffer.New(p_bytecount) &&
                    (SSL_random_bytes(t_buffer.Bytes(), p_bytecount)) &&
                    t_buffer.CreateData(r_bytes));
        }
		s_donotuse_ssl = true;
	}
#endif

	// Otherwise use the system provided CPRNG.
	return MCS_random_bytes(p_bytecount, r_bytes);
}
