/*
 * This file is part of the Sofia-SIP package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Pekka Pessi <pekka.pessi@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

/**@ingroup su_time
 * 
 * @IFILE torture_su_time.c  
 *
 * Tests for su_time functions.
 *
 * @author Pekka Pessi <Pekka.Pessi@nokia.com>
 * 
 * @date Created: Fri May 10 16:08:18 2002 ppessi
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include <sofia-sip/su_time.h>

#define TSTFLAGS flags

#include <sofia-sip/tstdef.h>

char const *name = "torture_su_time.c";

static int test1(int flags);
static int test2(int flags);
static int test3(int flags);

void usage(void)
{
  fprintf(stderr, 
	  "usage: %s [-v]\n", 
	  name);
}

char *lastpart(char *path)
{
  if (strchr(path, '/')) 
    return strrchr(path, '/') + 1;
  else
    return path;
}

int main(int argc, char *argv[])
{
  int flags = 0;
  int retval = 0;
  int i;

  name = lastpart(argv[0]);  /* Set our name */

  for (i = 1; argv[i]; i++) {
    if (strcmp(argv[i], "-v") == 0)
      flags |= tst_verbatim;
    else
      usage();
  }

  retval |= test1(flags); fflush(stdout);
  retval |= test2(flags); fflush(stdout);
  retval |= test3(flags); fflush(stdout);

  return retval;
}

su_time_t tv0;

void su_time(su_time_t *tv)
{
  *tv = tv0;
}

int test1(int flags)
{
  uint32_t ntp_hi, ntp_lo, ntp_mw;
  su_time_t now;
  su_ntp_t ntp, ntp0, ntp1;
  
  BEGIN();

  ntp_hi = 2208988800UL;
  ntp_lo = ((su_ntp_t)500000 << 32) / 1000000;
  ntp_mw = (ntp_hi << 16) | (ntp_lo >> 16);

  tv0.tv_sec = ntp_hi;
  tv0.tv_usec = 500000UL;
  ntp0 = (((su_ntp_t)2208988800UL) << 32) + ntp_lo;
  ntp1 = (((su_ntp_t)2208988800UL) << 32);

  now = su_now();

  TEST(now.tv_sec, 2208988800UL);
  TEST(now.tv_usec, 500000);

  ntp = su_ntp_now();
  TEST64(ntp, ntp0);
  TEST64(su_ntp_hi(ntp), ntp_hi);
  TEST64(su_ntp_lo(ntp), ntp_lo);
  TEST64(su_ntp_mw(ntp), ntp_mw);
  TEST(su_ntp_fraq(tv0), ntp_mw);

  tv0.tv_usec = 0;
  ntp = su_ntp_now();
  TEST64(ntp, ntp1);
  TEST64(su_ntp_hi(ntp), ntp_hi);
  TEST64(su_ntp_lo(ntp), 0);
  TEST64(su_ntp_mw(ntp), (ntp_hi & 0xffff) << 16);
  TEST(su_ntp_fraq(tv0), su_ntp_mw(ntp));

  END();
}

#include <sofia-sip/su_uniqueid.h>
#include <sofia-sip/su.h>			/* htonl() and guys */

int test2(int flags)
{
  char buf[64];
  su_guid_t g1[1], g2[2];
  uint64_t tl;
  uint16_t seq1, seq2;
  const uint64_t granularity = 10000000U;
  const uint64_t ntp_epoch = 
    (uint64_t)(141427) * (24 * 60 * 60L) * granularity;
  int i;

  BEGIN();

  tv0.tv_sec = 268435455;
  tv0.tv_usec = 98765;

  tl = tv0.tv_sec * granularity + tv0.tv_usec * (granularity / 1000000);
  tl += ntp_epoch;

  su_guid_generate(g1);
  seq1 = ((g1->s.clock_seq_hi_and_reserved & 0x3f) << 8) + g1->s.clock_seq_low;
  TEST(g1->s.time_low, htonl(tl & 0xffffFFFFU));
  TEST(g1->s.time_mid, htons((tl >> 32) & 0xffffU));
  TEST(g1->s.time_high_and_version, htons(((tl >> 48) & 0xfffU) | 0x1000));
  TEST(g1->s.clock_seq_hi_and_reserved & 0xc0, 0x80);

  TEST(i = su_guid_sprintf(buf, sizeof(buf), g1), su_guid_strlen);
  TEST(strlen(buf), i);

  tv0.tv_usec++;
  su_guid_generate(g1);
  seq2 = ((g1->s.clock_seq_hi_and_reserved & 0x3f) << 8) + g1->s.clock_seq_low;
  TEST(seq1, seq2);

  su_guid_generate(g2);
  seq2 = ((g2->s.clock_seq_hi_and_reserved & 0x3f) << 8) + g2->s.clock_seq_low;
  TEST(g2->s.time_low, g1->s.time_low);
  TEST(g2->s.time_mid, g1->s.time_mid);
  TEST(g2->s.time_high_and_version, g1->s.time_high_and_version);
  TEST((seq1 + 1) % 16384, seq2);

  for (i = 0; i < 32000; i++) {
    su_guid_generate(g2);
    seq2 = ((g2->s.clock_seq_hi_and_reserved & 0x3f) << 8) 
      + g2->s.clock_seq_low;
    TEST((seq1 + i + 2) % 16384, seq2);
  }

  END();
}

int test3(int flags)
{
  BEGIN();
  END();
}