//
// macaddress.h
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
#include <uspi/macaddress.h>
#include <uspi/util.h>
#include <uk/assert.h>

void MACAddress (TMACAddress *pThis)
{
	UK_ASSERT (pThis != 0);

	pThis->m_bValid = FALSE;
}

void MACAddress2 (TMACAddress *pThis, const u8 *pAddress)
{
	UK_ASSERT (pThis != 0);

	MACAddressSet (pThis, pAddress);
}

void _MACAddress (TMACAddress *pThis)
{
	UK_ASSERT (pThis != 0);

	pThis->m_bValid = FALSE;
}

boolean MACAddressIsEqual (TMACAddress *pThis, TMACAddress *pAddress2)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (pThis->m_bValid);
	
	return memcmp (pThis->m_Address, MACAddressGet (pAddress2), MAC_ADDRESS_SIZE) == 0 ? TRUE : FALSE;
}

void MACAddressSet (TMACAddress *pThis, const u8 *pAddress)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (pAddress != 0);

	memcpy (pThis->m_Address, pAddress, MAC_ADDRESS_SIZE);
	pThis->m_bValid = TRUE;
}

void MACAddressSetBroadcast (TMACAddress *pThis)
{
	UK_ASSERT (pThis != 0);

	memset (pThis->m_Address, 0xFF, MAC_ADDRESS_SIZE);
	pThis->m_bValid = TRUE;
}

const u8 *MACAddressGet (TMACAddress *pThis)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (pThis->m_bValid);

	return pThis->m_Address;
}

void MACAddressCopyTo (TMACAddress *pThis, u8 *pBuffer)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (pThis->m_bValid);
	UK_ASSERT (pBuffer != 0);

	memcpy (pBuffer, pThis->m_Address, MAC_ADDRESS_SIZE);
}

boolean MACAddressIsBroadcast (TMACAddress *pThis)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (pThis->m_bValid);

	for (unsigned i = 0; i < MAC_ADDRESS_SIZE; i++)
	{
		if (pThis->m_Address[i] != 0xFF)
		{
			return FALSE;
		}
	}

	return TRUE;
}

unsigned MACAddressGetSize (TMACAddress *pThis)
{
	return MAC_ADDRESS_SIZE;
}

void MACAddressFormat (TMACAddress *pThis, TString *pString)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (pThis->m_bValid);

	UK_ASSERT (pString != 0);
	StringFormat (pString, "%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned) pThis->m_Address[0], (unsigned) pThis->m_Address[1],
			(unsigned) pThis->m_Address[2], (unsigned) pThis->m_Address[3],
			(unsigned) pThis->m_Address[4], (unsigned) pThis->m_Address[5]);
}
