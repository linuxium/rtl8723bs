/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *                                        
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/

//============================================================
// include files
//============================================================

#include "odm_precomp.h"

void
odm_DynamicBBPowerSavingInit(
	IN		void *					pDM_VOID
	)
{
	PDM_ODM_T		pDM_Odm = (PDM_ODM_T)pDM_VOID;
	pPS_T	pDM_PSTable = &pDM_Odm->DM_PSTable;

	pDM_PSTable->PreCCAState = CCA_MAX;
	pDM_PSTable->CurCCAState = CCA_MAX;
	pDM_PSTable->PreRFState = RF_MAX;
	pDM_PSTable->CurRFState = RF_MAX;
	pDM_PSTable->Rssi_val_min = 0;
	pDM_PSTable->initialize = 0;
}


void
odm_1R_CCA(
	IN		void *					pDM_VOID
	)
{
	PDM_ODM_T		pDM_Odm = (PDM_ODM_T)pDM_VOID;
	pPS_T	pDM_PSTable = &pDM_Odm->DM_PSTable;

	if(pDM_Odm->RSSI_Min!= 0xFF)
	{
		 
		if(pDM_PSTable->PreCCAState == CCA_2R)
		{
			if(pDM_Odm->RSSI_Min >= 35)
				pDM_PSTable->CurCCAState = CCA_1R;
			else
				pDM_PSTable->CurCCAState = CCA_2R;
			
		}
		else{
			if(pDM_Odm->RSSI_Min <= 30)
				pDM_PSTable->CurCCAState = CCA_2R;
			else
				pDM_PSTable->CurCCAState = CCA_1R;
		}
	}
	else{
		pDM_PSTable->CurCCAState=CCA_MAX;
	}
	
	if(pDM_PSTable->PreCCAState != pDM_PSTable->CurCCAState)
	{
		if(pDM_PSTable->CurCCAState == CCA_1R)
		{
			if(  pDM_Odm->RFType ==ODM_2T2R )
			{
				ODM_SetBBReg(pDM_Odm, 0xc04  , bMaskByte0, 0x13);
				//PHY_SetBBReg(pAdapter, 0xe70, bMaskByte3, 0x20);
			}
			else
			{
				ODM_SetBBReg(pDM_Odm, 0xc04  , bMaskByte0, 0x23);
				//PHY_SetBBReg(pAdapter, 0xe70, 0x7fc00000, 0x10c); // Set RegE70[30:22] = 9b'100001100
			}
		}
		else
		{
			ODM_SetBBReg(pDM_Odm, 0xc04  , bMaskByte0, 0x33);
			//PHY_SetBBReg(pAdapter,0xe70, bMaskByte3, 0x63);
		}
		pDM_PSTable->PreCCAState = pDM_PSTable->CurCCAState;
	}
}

void
ODM_RF_Saving(
	IN		void *					pDM_VOID,
	IN	u1Byte		bForceInNormal 
	)
{
	PDM_ODM_T		pDM_Odm = (PDM_ODM_T)pDM_VOID;
	pPS_T	pDM_PSTable = &pDM_Odm->DM_PSTable;
	u1Byte	Rssi_Up_bound = 30 ;
	u1Byte	Rssi_Low_bound = 25;
	if(pDM_Odm->PatchID == 40 ) //RT_CID_819x_FUNAI_TV
	{
		Rssi_Up_bound = 50 ;
		Rssi_Low_bound = 45;
	}
	if(pDM_PSTable->initialize == 0){
		
		pDM_PSTable->Reg874 = (ODM_GetBBReg(pDM_Odm, 0x874, bMaskDWord)&0x1CC000)>>14;
		pDM_PSTable->RegC70 = (ODM_GetBBReg(pDM_Odm, 0xc70, bMaskDWord)&BIT3)>>3;
		pDM_PSTable->Reg85C = (ODM_GetBBReg(pDM_Odm, 0x85c, bMaskDWord)&0xFF000000)>>24;
		pDM_PSTable->RegA74 = (ODM_GetBBReg(pDM_Odm, 0xa74, bMaskDWord)&0xF000)>>12;
		//Reg818 = PHY_QueryBBReg(pAdapter, 0x818, bMaskDWord);
		pDM_PSTable->initialize = 1;
	}

	if(!bForceInNormal)
	{
		if(pDM_Odm->RSSI_Min != 0xFF)
		{			 
			if(pDM_PSTable->PreRFState == RF_Normal)
			{
				if(pDM_Odm->RSSI_Min >= Rssi_Up_bound)
					pDM_PSTable->CurRFState = RF_Save;
				else
					pDM_PSTable->CurRFState = RF_Normal;
			}
			else{
				if(pDM_Odm->RSSI_Min <= Rssi_Low_bound)
					pDM_PSTable->CurRFState = RF_Normal;
				else
					pDM_PSTable->CurRFState = RF_Save;
			}
		}
		else
			pDM_PSTable->CurRFState=RF_MAX;
	}
	else
	{
		pDM_PSTable->CurRFState = RF_Normal;
	}
	
	if(pDM_PSTable->PreRFState != pDM_PSTable->CurRFState)
	{
		if(pDM_PSTable->CurRFState == RF_Save)
		{
			ODM_SetBBReg(pDM_Odm, 0x874  , 0x1C0000, 0x2); //Reg874[20:18]=3'b010
			ODM_SetBBReg(pDM_Odm, 0xc70, BIT3, 0); //RegC70[3]=1'b0
			ODM_SetBBReg(pDM_Odm, 0x85c, 0xFF000000, 0x63); //Reg85C[31:24]=0x63
			ODM_SetBBReg(pDM_Odm, 0x874, 0xC000, 0x2); //Reg874[15:14]=2'b10
			ODM_SetBBReg(pDM_Odm, 0xa74, 0xF000, 0x3); //RegA75[7:4]=0x3
			ODM_SetBBReg(pDM_Odm, 0x818, BIT28, 0x0); //Reg818[28]=1'b0
			ODM_SetBBReg(pDM_Odm, 0x818, BIT28, 0x1); //Reg818[28]=1'b1
		}
		else
		{
			ODM_SetBBReg(pDM_Odm, 0x874  , 0x1CC000, pDM_PSTable->Reg874); 
			ODM_SetBBReg(pDM_Odm, 0xc70, BIT3, pDM_PSTable->RegC70); 
			ODM_SetBBReg(pDM_Odm, 0x85c, 0xFF000000, pDM_PSTable->Reg85C);
			ODM_SetBBReg(pDM_Odm, 0xa74, 0xF000, pDM_PSTable->RegA74); 
			ODM_SetBBReg(pDM_Odm,0x818, BIT28, 0x0);  
		}
		pDM_PSTable->PreRFState =pDM_PSTable->CurRFState;
	}
}