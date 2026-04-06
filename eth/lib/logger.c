//
// logger.c
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <uspienv/logger.h>
#include <uspienv/string.h>
#include <uspienv/synchronize.h>
#include <uspienv/alloc.h>
#include <uspienv/util.h>
#include <stdio.h>
#include <uk/assert.h>

static TLogger *s_pThis = 0;

void LoggerWrite2 (TLogger *pThis, const char *pString);

void Logger (TLogger *pThis, unsigned nLogLevel)
{
	pThis->m_nLogLevel = nLogLevel;

	s_pThis = pThis;
}

void _Logger ()
{
	s_pThis = 0;
}

boolean LoggerInitialize (TLogger *pThis)
{	
	LoggerWrite (pThis, LogNotice, "Logging started");

	return TRUE;
}

void LoggerWrite (TLogger *pThis, TLogSeverity Severity, const char *pMessage, ...)
{
	LoggerWriteV (pThis, Severity, pMessage);
}

void LoggerWriteV (TLogger *pThis, TLogSeverity Severity, const char *pMessage)
{
	if (Severity > pThis->m_nLogLevel)
	{
		return;
	}

	// if (Severity == LogPanic)
	// {
	// 	LoggerWrite2 (pThis, "\x1b[1m");
	// }

	// LoggerWrite2 (pThis, pSource);
	// LoggerWrite2 (pThis, ": ");

	// TString Message;
	// String (&Message);
	// StringFormatV (&Message, pMessage);

	// LoggerWrite2 (pThis, StringGet (&Message));

	// if (Severity == LogPanic)
	// {
	// 	LoggerWrite2 (pThis, "\x1b[0m");
	// }

	// LoggerWrite2 (pThis, "\n");

	printf("%s", pMessage);

	if (Severity == LogPanic)
	{
		UK_CRASH("Crashing in logger USPI\n");
	}

	// _String (&Message);
}

TLogger *LoggerGet (void)
{
	return s_pThis;
}

void LoggerWrite2 (TLogger *pThis, const char *pString)
{
	// ScreenDeviceWrite (pThis->m_pTarget, pString, nLength);
	printf("%s", pString);
}
