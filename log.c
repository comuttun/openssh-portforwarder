/* $OpenBSD: log.c,v 1.40 2007/05/17 07:50:31 djm Exp $ */
/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 */
/*
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"

#include <sys/types.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <errno.h>
#if defined(HAVE_STRNVIS) && defined(HAVE_VIS_H)
# include <vis.h>
#endif

#include "xmalloc.h"
#include "log.h"

#ifdef _TOH_
#ifdef _PFVERBOSE_
#include <windows.h>        /* for OutputDebugString() */
#endif /* _PFVERBOSE */
extern HWND g_hWnd;
static char buf[1024];
#ifdef _LOG_
void MyLog(LPCTSTR message);
#endif /* _LOG_ */
#endif /* _TOH_ */

#ifndef _TOH_
static LogLevel log_level = SYSLOG_LEVEL_INFO;
static int log_on_stderr = 1;
static int log_facility = LOG_AUTH;
static char *argv0;

extern char *__progname;

#define LOG_SYSLOG_VIS	(VIS_CSTYLE|VIS_NL|VIS_TAB|VIS_OCTAL)
#define LOG_STDERR_VIS	(VIS_SAFE|VIS_OCTAL)

/* textual representation of log-facilities/levels */

static struct {
	const char *name;
	SyslogFacility val;
} log_facilities[] = {
	{ "DAEMON",	SYSLOG_FACILITY_DAEMON },
	{ "USER",	SYSLOG_FACILITY_USER },
	{ "AUTH",	SYSLOG_FACILITY_AUTH },
#ifdef LOG_AUTHPRIV
	{ "AUTHPRIV",	SYSLOG_FACILITY_AUTHPRIV },
#endif
	{ "LOCAL0",	SYSLOG_FACILITY_LOCAL0 },
	{ "LOCAL1",	SYSLOG_FACILITY_LOCAL1 },
	{ "LOCAL2",	SYSLOG_FACILITY_LOCAL2 },
	{ "LOCAL3",	SYSLOG_FACILITY_LOCAL3 },
	{ "LOCAL4",	SYSLOG_FACILITY_LOCAL4 },
	{ "LOCAL5",	SYSLOG_FACILITY_LOCAL5 },
	{ "LOCAL6",	SYSLOG_FACILITY_LOCAL6 },
	{ "LOCAL7",	SYSLOG_FACILITY_LOCAL7 },
	{ NULL,		SYSLOG_FACILITY_NOT_SET }
};
#endif /* _TOH_ */

static struct {
	const char *name;
	LogLevel val;
} log_levels[] =
{
	{ "QUIET",	SYSLOG_LEVEL_QUIET },
	{ "FATAL",	SYSLOG_LEVEL_FATAL },
	{ "ERROR",	SYSLOG_LEVEL_ERROR },
	{ "INFO",	SYSLOG_LEVEL_INFO },
	{ "VERBOSE",	SYSLOG_LEVEL_VERBOSE },
	{ "DEBUG",	SYSLOG_LEVEL_DEBUG1 },
	{ "DEBUG1",	SYSLOG_LEVEL_DEBUG1 },
	{ "DEBUG2",	SYSLOG_LEVEL_DEBUG2 },
	{ "DEBUG3",	SYSLOG_LEVEL_DEBUG3 },
	{ NULL,		SYSLOG_LEVEL_NOT_SET }
};

#ifndef _TOH_
SyslogFacility
log_facility_number(char *name)
{
	int i;

	if (name != NULL)
		for (i = 0; log_facilities[i].name; i++)
			if (strcasecmp(log_facilities[i].name, name) == 0)
				return log_facilities[i].val;
	return SYSLOG_FACILITY_NOT_SET;
}
#endif /* _TOH_ */

LogLevel
log_level_number(char *name)
{
	int i;

	if (name != NULL)
		for (i = 0; log_levels[i].name; i++)
			if (strcasecmp(log_levels[i].name, name) == 0)
				return log_levels[i].val;
	return SYSLOG_LEVEL_NOT_SET;
}

/* Error messages that should be logged. */

void
error(const char *fmt,...)
{
#ifndef _TOH_
	va_list args;

	va_start(args, fmt);
	do_log(SYSLOG_LEVEL_ERROR, fmt, args);
	va_end(args);
#else /* _TOH_ */
	va_list args;
    msgboxinfo info;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

    info.caption = APP_NAME;
    info.message = buf;
    info.type = MB_OK | MB_ICONERROR;

    SendMessage(g_hWnd, MSG_SHOW_MESSAGEBOX, (WPARAM)&info, 0);
#endif /* _TOH_ */
}

void
sigdie(const char *fmt,...)
{
#ifdef DO_LOG_SAFE_IN_SIGHAND
	va_list args;

	va_start(args, fmt);
	do_log(SYSLOG_LEVEL_FATAL, fmt, args);
	va_end(args);
#endif
	_exit(1);
}


/* Log this message (information that usually should go to the log). */

void
logit(const char *fmt,...)
{
#ifndef _TOH_
	va_list args;

	va_start(args, fmt);
	do_log(SYSLOG_LEVEL_INFO, fmt, args);
	va_end(args);
#else /* _TOH_ */
	va_list args;
    msgboxinfo info;

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

    info.caption = APP_NAME;
    info.message = buf;
    info.type = MB_OK | MB_ICONINFORMATION;
    SendMessage(g_hWnd, MSG_SHOW_MESSAGEBOX, (WPARAM)&info, 0);
#endif /* _TOH_ */
}

/* More detailed messages (information that does not need to go to the log). */

void
verbose(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(SYSLOG_LEVEL_VERBOSE, fmt, args);
	va_end(args);
}

/* Debugging messages that should not be logged during normal operation. */

void
debug(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(SYSLOG_LEVEL_DEBUG1, fmt, args);
	va_end(args);
}

void
debug2(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(SYSLOG_LEVEL_DEBUG2, fmt, args);
	va_end(args);
}

void
debug3(const char *fmt,...)
{
	va_list args;

	va_start(args, fmt);
	do_log(SYSLOG_LEVEL_DEBUG3, fmt, args);
	va_end(args);
}

#ifndef _TOH_
/*
 * Initialize the log.
 */

void
log_init(char *av0, LogLevel level, SyslogFacility facility, int on_stderr)
{
#if defined(HAVE_OPENLOG_R) && defined(SYSLOG_DATA_INIT)
	struct syslog_data sdata = SYSLOG_DATA_INIT;
#endif

	argv0 = av0;

	switch (level) {
	case SYSLOG_LEVEL_QUIET:
	case SYSLOG_LEVEL_FATAL:
	case SYSLOG_LEVEL_ERROR:
	case SYSLOG_LEVEL_INFO:
	case SYSLOG_LEVEL_VERBOSE:
	case SYSLOG_LEVEL_DEBUG1:
	case SYSLOG_LEVEL_DEBUG2:
	case SYSLOG_LEVEL_DEBUG3:
		log_level = level;
		break;
	default:
		fprintf(stderr, "Unrecognized internal syslog level code %d\n",
		    (int) level);
		exit(1);
	}

	log_on_stderr = on_stderr;
	if (on_stderr)
		return;

	switch (facility) {
	case SYSLOG_FACILITY_DAEMON:
		log_facility = LOG_DAEMON;
		break;
	case SYSLOG_FACILITY_USER:
		log_facility = LOG_USER;
		break;
	case SYSLOG_FACILITY_AUTH:
		log_facility = LOG_AUTH;
		break;
#ifdef LOG_AUTHPRIV
	case SYSLOG_FACILITY_AUTHPRIV:
		log_facility = LOG_AUTHPRIV;
		break;
#endif
	case SYSLOG_FACILITY_LOCAL0:
		log_facility = LOG_LOCAL0;
		break;
	case SYSLOG_FACILITY_LOCAL1:
		log_facility = LOG_LOCAL1;
		break;
	case SYSLOG_FACILITY_LOCAL2:
		log_facility = LOG_LOCAL2;
		break;
	case SYSLOG_FACILITY_LOCAL3:
		log_facility = LOG_LOCAL3;
		break;
	case SYSLOG_FACILITY_LOCAL4:
		log_facility = LOG_LOCAL4;
		break;
	case SYSLOG_FACILITY_LOCAL5:
		log_facility = LOG_LOCAL5;
		break;
	case SYSLOG_FACILITY_LOCAL6:
		log_facility = LOG_LOCAL6;
		break;
	case SYSLOG_FACILITY_LOCAL7:
		log_facility = LOG_LOCAL7;
		break;
	default:
		fprintf(stderr,
		    "Unrecognized internal syslog facility code %d\n",
		    (int) facility);
		exit(1);
	}

	/*
	 * If an external library (eg libwrap) attempts to use syslog
	 * immediately after reexec, syslog may be pointing to the wrong
	 * facility, so we force an open/close of syslog here.
	 */
#if defined(HAVE_OPENLOG_R) && defined(SYSLOG_DATA_INIT)
	openlog_r(argv0 ? argv0 : __progname, LOG_PID, log_facility, &sdata);
	closelog_r(&sdata);
#else
	openlog(argv0 ? argv0 : __progname, LOG_PID, log_facility);
	closelog();
#endif
}
#endif /* _TOH_ */

#define MSGBUFSIZ 1024

void
do_log(LogLevel level, const char *fmt, va_list args)
{
#ifndef _TOH_
#if defined(HAVE_OPENLOG_R) && defined(SYSLOG_DATA_INIT)
	struct syslog_data sdata = SYSLOG_DATA_INIT;
#endif
	char msgbuf[MSGBUFSIZ];
	char fmtbuf[MSGBUFSIZ];
	char *txt = NULL;
	int pri = LOG_INFO;
	int saved_errno = errno;

	if (level > log_level)
		return;

	switch (level) {
	case SYSLOG_LEVEL_FATAL:
		if (!log_on_stderr)
			txt = "fatal";
		pri = LOG_CRIT;
		break;
	case SYSLOG_LEVEL_ERROR:
		if (!log_on_stderr)
			txt = "error";
		pri = LOG_ERR;
		break;
	case SYSLOG_LEVEL_INFO:
		pri = LOG_INFO;
		break;
	case SYSLOG_LEVEL_VERBOSE:
		pri = LOG_INFO;
		break;
	case SYSLOG_LEVEL_DEBUG1:
		txt = "debug1";
		pri = LOG_DEBUG;
		break;
	case SYSLOG_LEVEL_DEBUG2:
		txt = "debug2";
		pri = LOG_DEBUG;
		break;
	case SYSLOG_LEVEL_DEBUG3:
		txt = "debug3";
		pri = LOG_DEBUG;
		break;
	default:
		txt = "internal error";
		pri = LOG_ERR;
		break;
	}
	if (txt != NULL) {
		snprintf(fmtbuf, sizeof(fmtbuf), "%s: %s", txt, fmt);
		vsnprintf(msgbuf, sizeof(msgbuf), fmtbuf, args);
	} else {
		vsnprintf(msgbuf, sizeof(msgbuf), fmt, args);
	}
	strnvis(fmtbuf, msgbuf, sizeof(fmtbuf),
	    log_on_stderr ? LOG_STDERR_VIS : LOG_SYSLOG_VIS);
	if (log_on_stderr) {
		snprintf(msgbuf, sizeof msgbuf, "%s\r\n", fmtbuf);
		write(STDERR_FILENO, msgbuf, strlen(msgbuf));
	} else {
#if defined(HAVE_OPENLOG_R) && defined(SYSLOG_DATA_INIT)
		openlog_r(argv0 ? argv0 : __progname, LOG_PID, log_facility, &sdata);
		syslog_r(pri, &sdata, "%.500s", fmtbuf);
		closelog_r(&sdata);
#else
		openlog(argv0 ? argv0 : __progname, LOG_PID, log_facility);
		syslog(pri, "%.500s", fmtbuf);
		closelog();
#endif
	}
	errno = saved_errno;
#else /* _TOH_ */
#if defined(_LOG_) || defined(_PFVERBOSE_)
	char msgbuf[MSGBUFSIZ];
	char fmtbuf[MSGBUFSIZ];
	char *txt = NULL;

	switch (level) {
	case SYSLOG_LEVEL_FATAL:
        txt = "fatal";
		break;
	case SYSLOG_LEVEL_ERROR:
        txt = "error";
		break;
	case SYSLOG_LEVEL_INFO:
        txt = "info";
		break;
	case SYSLOG_LEVEL_VERBOSE:
        txt = "verbose";
		break;
	case SYSLOG_LEVEL_DEBUG1:
		txt = "debug1";
		break;
	case SYSLOG_LEVEL_DEBUG2:
		txt = "debug2";
		break;
	case SYSLOG_LEVEL_DEBUG3:
		txt = "debug3";
		break;
	default:
		txt = "internal error";
		break;
	}
    snprintf(fmtbuf, sizeof(fmtbuf), "%s: %s", txt, fmt);
    vsnprintf(msgbuf, sizeof(msgbuf), fmtbuf, args);
/*
	strnvis(fmtbuf, msgbuf, sizeof(fmtbuf), VIS_SAFE|VIS_OCTAL);
    snprintf(msgbuf, sizeof msgbuf, "%s\r\n", fmtbuf);
*/
#ifdef _LOG_
    MyLog(msgbuf);
#endif /* _LOG_ */
#ifdef _PFVERBOSE_
  OutputDebugString(msgbuf);
  OutputDebugString("\n");
#endif /* _PFVERBOSE_ */
#endif /* _LOG_ || _PFVERBOSE_ */
#endif /* _TOH_ */
}

#ifdef _TOH_
#if 0 /* clear_fatal_cleanups() is not used, for now */
void clear_options();
void EVP_cleanup();
void ERR_free_strings();
void CRYPTO_cleanup_all_ex_data();

#include "compat.h"
#include "cipher.h"
#include "key.h"
#include "kex.h"
extern Kex* xxx_kex;
extern Newkeys *current_keys[];

void clear_fatal_cleanups()
{
    /* this clean up could be already done?  better not do this here
	struct fatal_cleanup *cu, *next_cu;

	for (cu = fatal_cleanups; cu; cu = next_cu) {
		next_cu = cu->next;
		(*cu->proc) (cu->context);
        xfree(cu);
	}
    fatal_cleanups = NULL;
    */

    /* clena up for OpenSSL */
    CRYPTO_cleanup_all_ex_data();
        /* for RSA_new() and DSA_new(), called in key_new().
         * RSA_free() and DSA_new() don't clean up completely.
         */
    EVP_cleanup();
    ERR_free_strings();
        /* those two above are needed for SSLeay_add_all_algorithms()
         * and ERR_load_crypto_strings() in main().
         */

    /* clean up for options */
    clear_options();

    /* clean up for some global variables */

    /* client_version_string and server_version_strings
     * should be cleared as xxx_kex->client_version_string
     * and xxx_kex->server_version_string.
     * DO NOT FREE!
     */
    /*
    if (client_version_string) {
        xfree(client_version_string);
        client_version_string = NULL;
    }
    if (server_version_string) {
        xfree(server_version_string);
        server_version_string = NULL;
    }
    */

    /* clean up for kex */
    if (compat20) {
        if (xxx_kex->session_id) {
            xfree(xxx_kex->session_id);
            xxx_kex->session_id = NULL;
            xxx_kex->session_id_len = 0;
        }
        buffer_free(&xxx_kex->my);
        buffer_free(&xxx_kex->peer);
        if (xxx_kex->client_version_string) {
            xfree(xxx_kex->client_version_string);
            xxx_kex->client_version_string = NULL;
        }
        if (xxx_kex->server_version_string) {
            xfree(xxx_kex->server_version_string);
            xxx_kex->server_version_string = NULL;
        }
        if (xxx_kex->name) {
            xfree(xxx_kex->name);
            xxx_kex->name = NULL;
        }
        {
            int mode;
            for (mode = 0; mode < MODE_MAX; mode ++) {
                if (xxx_kex->newkeys[mode]) {
                    xfree(xxx_kex->newkeys[mode]);
                    xxx_kex->newkeys[mode] = NULL;
                }
                if (current_keys[mode]) {
                    xfree(current_keys[mode]);
                    current_keys[mode] = NULL;
                }
            }
        }
    }
}
#endif /* 0 */
#endif /* _TOH_ */
