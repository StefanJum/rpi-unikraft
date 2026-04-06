//
// usbendpoint.c
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014-2020  R. Stange <rsta2@o2online.de>
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
#include <uspi/usbendpoint.h>
#include <uk/assert.h>

void USBEndpoint (TUSBEndpoint *pThis, TUSBDevice *pDevice)
{
	UK_ASSERT (pThis != 0);
	pThis->m_pDevice = pDevice;
	pThis->m_ucNumber = 0;
	pThis->m_Type = EndpointTypeControl;
	pThis->m_bDirectionIn = FALSE;
	pThis->m_nMaxPacketSize = USB_DEFAULT_MAX_PACKET_SIZE;
	pThis->m_nInterval = 1;
	pThis->m_NextPID = USBPIDSetup;

	UK_ASSERT (pThis->m_pDevice != 0);
}

void USBEndpoint2 (TUSBEndpoint *pThis, TUSBDevice *pDevice, const TUSBEndpointDescriptor *pDesc)
{
	UK_ASSERT (pThis != 0);
	pThis->m_pDevice = pDevice;
	pThis->m_nInterval = 1;

	UK_ASSERT (pThis->m_pDevice != 0);

	UK_ASSERT (pDesc != 0);
	UK_ASSERT (pDesc->bLength == sizeof *pDesc || pDesc->bLength == sizeof (TUSBAudioEndpointDescriptor));
	UK_ASSERT (pDesc->bDescriptorType == DESCRIPTOR_ENDPOINT);

	switch (pDesc->bmAttributes & 0x03)
	{
	case 2:
		pThis->m_Type = EndpointTypeBulk;
		pThis->m_NextPID = USBPIDData0;
		break;

	case 3:
		pThis->m_Type = EndpointTypeInterrupt;
		pThis->m_NextPID = USBPIDData0;
		break;

	default:
		UK_ASSERT (0);	// endpoint configuration should be checked by function driver
		return;
	}
	
	pThis->m_ucNumber       = pDesc->bEndpointAddress & 0x0F;
	pThis->m_bDirectionIn   = pDesc->bEndpointAddress & 0x80 ? TRUE : FALSE;

	// suppress unaligned access
	u8 *pMaxPacketSize = (u8 *) &pDesc->wMaxPacketSize;
	pThis->m_nMaxPacketSize = pMaxPacketSize[0] | (u16) pMaxPacketSize[1] << 8;
	
	if (pThis->m_Type == EndpointTypeInterrupt)
	{
		u8 ucInterval = pDesc->bInterval;
		if (ucInterval < 1)
		{
			ucInterval = 1;
		}

		// see USB 2.0 spec chapter 9.6.6
		if (USBDeviceGetSpeed (pThis->m_pDevice) != USBSpeedHigh)
		{
			pThis->m_nInterval = ucInterval;
		}
		else
		{
			if (ucInterval > 16)
			{
				ucInterval = 16;
			}

			unsigned nValue = 1 << (ucInterval - 1);

			pThis->m_nInterval = nValue / 8;

			if (pThis->m_nInterval < 1)
			{
				pThis->m_nInterval = 1;
			}
		}
	}
}

void USBEndpointCopy (TUSBEndpoint *pThis, TUSBEndpoint *pEndpoint, TUSBDevice *pDevice)
{
	UK_ASSERT (pThis != 0);

	UK_ASSERT (pEndpoint != 0);

	pThis->m_pDevice = pDevice;
	UK_ASSERT (pThis->m_pDevice != 0);

	pThis->m_ucNumber	 = pEndpoint->m_ucNumber;
	pThis->m_Type		 = pEndpoint->m_Type;
	pThis->m_bDirectionIn	 = pEndpoint->m_bDirectionIn;
	pThis->m_nMaxPacketSize  = pEndpoint->m_nMaxPacketSize;
	pThis->m_nInterval       = pEndpoint->m_nInterval;
	pThis->m_NextPID	 = pEndpoint->m_NextPID;
}

void _USBEndpoint (TUSBEndpoint *pThis)
{
	UK_ASSERT (pThis != 0);
	pThis->m_pDevice = 0;
}

TUSBDevice *USBEndpointGetDevice (TUSBEndpoint *pThis)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (pThis->m_pDevice != 0);
	return pThis->m_pDevice;
}

u8 USBEndpointGetNumber (TUSBEndpoint *pThis)
{
	UK_ASSERT (pThis != 0);
	return pThis->m_ucNumber;
}

TEndpointType USBEndpointGetType (TUSBEndpoint *pThis)
{
	UK_ASSERT (pThis != 0);
	return pThis->m_Type;
}

boolean USBEndpointIsDirectionIn (TUSBEndpoint *pThis)
{
	UK_ASSERT (pThis != 0);
	return pThis->m_bDirectionIn;
}

void USBEndpointSetMaxPacketSize (TUSBEndpoint *pThis, u32 nMaxPacketSize)
{
	UK_ASSERT (pThis != 0);
	pThis->m_nMaxPacketSize = nMaxPacketSize;
}

u32 USBEndpointGetMaxPacketSize (TUSBEndpoint *pThis)
{
	UK_ASSERT (pThis != 0);
	return pThis->m_nMaxPacketSize;
}

unsigned USBEndpointGetInterval (TUSBEndpoint *pThis)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (pThis->m_Type == EndpointTypeInterrupt);

	return pThis->m_nInterval;
}

TUSBPID USBEndpointGetNextPID (TUSBEndpoint *pThis, boolean bStatusStage)
{
	UK_ASSERT (pThis != 0);
	if (bStatusStage)
	{
		UK_ASSERT (pThis->m_Type == EndpointTypeControl);

		return USBPIDData1;
	}
	
	return pThis->m_NextPID;
}

void USBEndpointSkipPID (TUSBEndpoint *pThis, unsigned nPackets, boolean bStatusStage)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (   pThis->m_Type == EndpointTypeControl
		|| pThis->m_Type == EndpointTypeBulk
		|| pThis->m_Type == EndpointTypeInterrupt);
	
	if (!bStatusStage)
	{
		switch (pThis->m_NextPID)
		{
		case USBPIDSetup:
			pThis->m_NextPID = USBPIDData1;
			break;

		case USBPIDData0:
			if (nPackets & 1)
			{
				pThis->m_NextPID = USBPIDData1;
			}
			break;
			
		case USBPIDData1:
			if (nPackets & 1)
			{
				pThis->m_NextPID = USBPIDData0;
			}
			break;

		default:
			UK_ASSERT (0);
			break;
		}
	}
	else
	{
		UK_ASSERT (pThis->m_Type == EndpointTypeControl);

		pThis->m_NextPID = USBPIDSetup;
	}
}

void USBEndpointResetPID (TUSBEndpoint *pThis)
{
	UK_ASSERT (pThis != 0);
	UK_ASSERT (pThis->m_Type == EndpointTypeBulk);

	pThis->m_NextPID = USBPIDData0;
}
