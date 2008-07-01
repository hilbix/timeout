/* $Header$
 *
 * Execute a program only a certain time.
 * Rewrite of a program which is over 10 years old.
 *
 * Copyright (C)2006 Valentin Hilbig, webmaster@scylla-charybdis.com
 * This shall be independent of tinolib.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Log$
 * Revision 1.1  2008-07-01 18:55:08  tino
 * save
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>

#include <unistd.h>

#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define	MAX_SIG	100
static struct signals
  {
    int		sig;
    long	delay;
  }	maxsig[MAX_SIG];
static int		sigs;

static int		mypid;
static const char	*arg0;

static void
ex(int ret, const char *s, ...)
{
  va_list	list;
  int		e;

  e	= errno;
  fprintf(stderr, "[%ld] %s error: ", (long)mypid, arg0);
  va_start(list, s);
  vfprintf(stderr, s, list);
  va_end(list);
  fprintf(stderr, ": %s\n", strerror(e));
  exit(ret);
}

static void
setarg0(const char *s)
{
  char	*tmp;

  mypid	= getpid();
  arg0	= s;
  if ((tmp=strrchr(arg0, '/'))!=0)
    arg0	= tmp+1;
}

int
main(int argc, char **argv)
{
  setarg0(argv[0]);
  for (sigs=0; argc>1 && **++argv=='-'; sigs++, argc--)
    {
      unsigned long long	u, dly;
      char			*end;

      u				= strtoull(*argv+1, &end, 0);
      dly			= 1;
      if (end && *end=='@')
	{
	  end++;
	  dly			= strtoull(end+1, &end, 0);
	}
      if (u!=(unsigned long long)(int)u || dly!=(unsigned long long)(int)dly)
	ex(-1, "overflow in number: %s", *argv);
      if (!end || *end)
	ex(-1, "invalid number: %s", *argv);
      maxsig[sigs].sig		= u;
      maxsig[sigs].delay	= dly;
    }
  000;
  return 0;
}
