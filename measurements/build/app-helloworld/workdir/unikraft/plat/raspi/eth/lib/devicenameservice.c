//
// devicenameservice.c
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
#include <uspi/devicenameservice.h>
#include <uspi/util.h>
#include <uspi/assert.h>
#include <uspios.h>
#include <uk/assert.h>
#include <stdlib.h>

static TDeviceNameService *s_pThis = 0;

void DeviceNameService (TDeviceNameService *pThis)
{
	UK_ASSERT (pThis != 0);

	pThis->m_pList = 0;

	UK_ASSERT (s_pThis == 0);
	s_pThis = pThis;
}

void _DeviceNameService (TDeviceNameService *pThis)
{
	UK_ASSERT (pThis != 0);

	while (pThis->m_pList != 0)
	{
		TDeviceInfo *pNext = pThis->m_pList->pNext;

		UK_ASSERT (pThis->m_pList->pName != 0);
		free (pThis->m_pList->pName);
		pThis->m_pList->pName = 0;
		
		pThis->m_pList->pDevice = 0;
		
		free (pThis->m_pList);

		pThis->m_pList = pNext;
	}
	
	s_pThis = 0;
}

void DeviceNameServiceAddDevice (TDeviceNameService *pThis, const char *pName, void *pDevice, boolean bBlockDevice)
{
	UK_ASSERT (pThis != 0);

	TDeviceInfo *pInfo = (TDeviceInfo *) malloc (sizeof (TDeviceInfo));
	UK_ASSERT (pInfo != 0);

	UK_ASSERT (pName != 0);
	pInfo->pName = (char *) malloc (strlen (pName)+1);
	UK_ASSERT (pInfo->pName != 0);
	strcpy (pInfo->pName, pName);

	UK_ASSERT (pDevice != 0);
	pInfo->pDevice = pDevice;
	
	pInfo->bBlockDevice = bBlockDevice;

	pInfo->pNext = pThis->m_pList;
	pThis->m_pList = pInfo;
}

void *DeviceNameServiceGetDevice (TDeviceNameService *pThis, const char *pName, boolean bBlockDevice)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (pName != 0);

	TDeviceInfo *pInfo = pThis->m_pList;
	while (pInfo != 0)
	{
		UK_ASSERT (pInfo->pName != 0);
		if (   strcmp (pName, pInfo->pName) == 0
		    && pInfo->bBlockDevice == bBlockDevice)
		{
			UK_ASSERT (pInfo->pDevice != 0);
			return pInfo->pDevice;
		}

		pInfo = pInfo->pNext;
	}

	return 0;
}

TDeviceNameService *DeviceNameServiceGet (void)
{
	UK_ASSERT (s_pThis != 0);
	return s_pThis;
}
