/* $Header$
 *
 * Execute a program only a certain time.
 * Rewrite of a program which is over 10 years old.
 *
 * Copyright (C)2008 Valentin Hilbig <webmaster@scylla-charybdis.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 *
 * $Log$
 * Revision 1.3  2008-11-02 00:40:07  tino
 * Bugfix, idle mode was inverted
 *
 * Revision 1.2  2008-11-02 00:13:56  tino
 * First version
 */

#include "tino/alarm.h"
#include "tino/proc.h"

#define	MAX_SIG	100
static struct signals
  {
    int		sig;
    long	delay;
  }			sig[MAX_SIG];
static int		sigs;

static int		mypid;
static const char	*arg0;
static unsigned long	timeout;
static unsigned long	seconds;
static long		mark;
static int		verbose;

static void
vex(int ret, TINO_VA_LIST list, const char *type, int e)
{
  fprintf(stderr, "[%ld] %s %s: ", (long)mypid, arg0, type);
  tino_vfprintf(stderr, list);
  if (e)
    fprintf(stderr, ": %s\n", strerror(e));
  else
    fprintf(stderr, "\n");
  exit(ret);
}

static void
info(const char *s, ...)
{
  tino_va_list	list;

  if (!verbose)
    return;
  fprintf(stderr, "[%ld] %s info: ", (long)mypid, arg0);
  tino_va_start(list, s);
  tino_vfprintf(stderr, &list);
  tino_va_end(list);
  fprintf(stderr, "\n");
}

static void
ex(const char *s, ...)
{
  tino_va_list	list;

  tino_va_start(list, s);
  vex(210, &list, "error", errno);
  /* never reached	*/
}

static void
to(const char *s, ...)
{
  tino_va_list	list;

  tino_va_start(list, s);
  vex(211, &list, "timeout", 0);
  /* never reached	*/
}

static void
fin(int ret, const char *s, ...)
{
  tino_va_list	list;

  tino_va_start(list, s);
  vex(ret, &list, "info", 0);
  /* never reached	*/
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

static int
cb(void *user, long delta, time_t now, long run)
{
  if (seconds==timeout)
    mark++;
  else
    seconds++;
  if (verbose>1)
    {
      fprintf(stderr, "%lu \r", timeout-seconds);
      fflush(stderr);
    }
  return 0;
}

static void
schedule(void)
{
  seconds	= 0;
  mark		= 0;
}

static void
check(const char *what)
{
  TINO_ALARM_RUN();
  if (mark)
    to("STDIN");
}

static void
do_copy(int idle)
{
  for (;;)
    {
      char	buf[BUFSIZ];
      int	got;
      size_t	pos;

      if ((got=tino_file_readI(0, buf, sizeof buf))==0)
	return;

      check("STDIN");

      if (got<0)
	{
	  if (errno==EINTR)
	    continue;
	  ex("read STDIN");
	}

      if (idle)
	schedule();

      for (pos=0; pos<got; )
	{
	  int	put;

	  put	= tino_file_writeI(1, buf+pos, got-pos);

	  check("STDOUT");

	  if (put<0)
	    {
	      if (errno==EINTR)
		continue;
	      ex("write STDOUT");
	    }

	  if (!put)
	    ex("zero write?");

	  if (idle)
	    schedule();

	  pos	+= put;
	}
    }
}

static void
do_fork(char **argv)
{
  int		nr;
  int		status, result;
  pid_t		pid, tmp;
  const char	*s;

  pid	= tino_fork_exec(0, 1, 2, argv, NULL, 0, NULL);
  info("[%ld] exec %s", (long)pid, argv[0]);
  nr	= 0;
  while ((tmp=waitpid((pid_t)-1, &status, 0))!=pid)
    {
      TINO_ALARM_RUN();
      if (mark>0)
	{
	  if (nr>=sigs)
	    to("out of signals");
	  info("[%ld] send signal %d", (long)pid, sig[nr].sig);
	  kill(pid, sig[nr].sig);
	  mark	= 1l-sig[nr].delay;
	  nr++;
	}

      if (pid==(pid_t)-1)
	{
	  if (errno==EINTR)
	    continue;
	  ex("wait error");
	}
    }
  s	= tino_wait_child_status_string(status, &result);
  if (verbose)
    fin(result, "[%ld] result %s: %s", (long)pid, argv[0], s);
  exit(result);
}

int
main(int argc, char **argv)
{
  char	*end;

  setarg0(argv[0]);
  for (sigs=0; --argc>=1 && **++argv=='-'; )
    {
      unsigned long long	u, dly;
      char			*end;

      if (!strcmp(*argv, "-v"))
	{
	  verbose++;
	  continue;
	}
      u			= strtoull(*argv+1, &end, 0);
      dly		= 1;
      if (end && *end=='@')
	{
	  end++;
	  dly		= strtoull(end+1, &end, 0);
	}
      if (u!=(unsigned long long)(int)u || dly!=(unsigned long long)(int)dly)
	ex("overflow in number: %s", *argv);
      if (!end || *end)
	ex("invalid option: %s", *argv);
      sig[sigs].sig	= u;
      sig[sigs].delay	= dly;
      sigs++;
    }
  if (!sigs)
    {
      sig[sigs].sig	= 1;
      sig[sigs].delay	= 1;
      sigs++;
      sig[sigs].sig	= 15;
      sig[sigs].delay	= 1;
      sigs++;
      sig[sigs].sig	= 9;
      sig[sigs].delay	= 1;
      sigs++;
    }

  if (argc<1)
    {
      fprintf(stderr, "Usage: %s [-v] [-signal[@seconds]...] seconds [-|program [args...]]\n", arg0);
      return 210;
    }

  timeout			= strtoull(*argv, &end, 0);
  if (!end || *end)
    ex("invalid seconds: %s", *argv);

  tino_alarm_set(1, cb, NULL);
  if (argc==2 && !strcmp(argv[1], "-"))
    do_copy(1);
  else if (argc==1)
    do_copy(0);
  else
    do_fork(argv+1);
  return 0;
}
