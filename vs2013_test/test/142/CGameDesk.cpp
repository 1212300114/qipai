#include "CGameDesk.h"
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <iostream>
#include <assert.h>
#include "comstruct.h"
#include "myLog.h"
#include "version.h"
#include "CommonDef.hpp"
#include "MsgHead.h"
#include "basemessage.h"
#include "TimeLib.hpp"
#include "zqinutil.h"
#include "ComDef.hpp"
#include "type.h"
#include "MsgDef.hpp"

//#include "Rand.h"
#include "ConfigMgr.h"
#include "DBConfigMgr.h"
#include "MJAlogrithm.h"
#include "Rand.h"

using namespace ServerFrame;

#define CONFIG_PALYER_COUNT  std::min(int(m_vecPlayer.size()),GAME_PLAYER)

extern "C"
{
	IGameDesk*  CreateNewGameDesk()
	{
		CGameDesk *pSO = new CGameDesk();		
		return pSO;
	}
}

//-----------------------------------------------------------------------------
//���캯��
CGameDesk::CGameDesk(void)
{
	m_nRoomLevel    = 0;
	m_nRoomID       = 0;
	m_nDeskIndex	= -1;
	m_bPlaying		= false;
	m_nGameStation = GS_WAIT_ARGEE;
	m_iNtStation = -1;
	m_dwNtUserID = 0;
	m_nPlayMode = 0;

	m_un64RoomLevelMinMoney = 0;
	memset(&m_ManageInfoStruct,0,sizeof(m_ManageInfoStruct));

	for ( int i = 0 ; i < 4/*GAME_PLAYER*/ ; i++ )
	{
		PlayerInfo* pPlayer = new PlayerInfo(&m_playerHuTips);
		m_vecPlayer.push_back(pPlayer);
		pPlayer->setNaiZiCard(0x41);	//��������

		m_bPlayerTuoGuan[i] = false;
		m_bPlayerTimerRunning[i] = false;
		m_nPlayerTimeOutCount[i] = 0;
	}

	memset(m_byAllCardList,0,sizeof(m_byAllCardList));
	memset(m_byDicePoint,0,sizeof(m_byDicePoint));

	m_iTotalMjCardCount = 0;
	m_iLeftMjCount = 0;
	m_iStartIndex  = 0;
	m_iEndIndex    = 0;

	m_byStartSeat  = 0;
	m_byStartDun   = 0;

	m_iTotalRound  = 8;
	m_iCurRound    = 0;

	m_iCurOperateStation = 0;
	m_iCurSeatOpStatus = TSEAT_STATUS_OP_ING;

	m_byLastOutMjSetaNo = -1;
	m_byLastOutMj       = 0;

	m_byCurMjShowSeat   = -1;

	m_eGameDjsType     = EGPDJS_NO;
	m_tmGameDjsStartMS = CTimeLib::GetCurMSecond();
	m_tmGameDjsTimes   = 0;
	m_iGameDjsStation  = -1;

	memset(&m_GameEndInfo,0,sizeof(m_GameEndInfo));
	memset(m_arrSaveDetailScore,0,sizeof(m_arrSaveDetailScore));
	memset(m_arrSaveDetailCount,0,sizeof(m_arrSaveDetailCount));

	m_iCfgZhuaNiaoCount    = 2;
	m_bCfgCanDianPaoHu     = false;
	m_bCfgZhuangXianScore  = true;
	m_bCfgHuQiDui          = true;
	m_bCfgHongZhong        = false;
	m_byCfgLaiziCard       = 0;
	m_bCfgGuoHouBuGang	   = false;
	m_bCfgXianPengHouGang  = false;
	m_byAddNiaoCount = 1;

	m_iNextNtStation = -1;
	//m_nSaveAutoHuChairID = -1;
	m_nFinalMoPaiChair = -1;

	memset(&m_CardRoomConfig,0,sizeof(m_CardRoomConfig));

	m_nTimeOutLimit = 2;
	m_nOperateTime = 30;
	m_nReadyTime = 30;
	m_nWaitForFullTime = 30;
}

//��������
CGameDesk::~CGameDesk(void)
{
	for ( int i = 0 ; i < m_vecPlayer.size() ; i++ )
	{
		if ( NULL != m_vecPlayer[i] )
		{
			delete m_vecPlayer[i];
			m_vecPlayer[i] = NULL;
		}
	}

	m_vecPlayer.clear();
}

//-------------------------------------------------------------------------
// ��IGameDesk����
//-------------------------------------------------------------------------
// proxy���ص���Ϣ
int CGameDesk::HandleProxyMsg(unsigned int unMsgID, void* pMsgHead, const char* pMsgParaBuf, const int nLen)
{
	CMsgHead* pMsgHeadPtr   = (CMsgHead*)pMsgHead;

	MYLOG_DEBUG(logger, "GameDesk::HandleProxyMsg unMsgID=%d, m_unMsgID=%d, m_usMsgType=%d, pMsgParaBuf=%d, nLen=%d", 
		unMsgID, pMsgHeadPtr->m_unMsgID,  pMsgHeadPtr->m_usMsgType, pMsgParaBuf, nLen);

// 	switch (unMsgID)
// 	{
// 		// ���㷵��
// 	case MSG_ID_SS_GAMEDB_101_USER_SETTLEMENT:
// 		{
// 			CRspUpdateUserGameDataPara msgRsp;
// 
// 			short nBufLenEncode = 0;
// 			char szMsgSendBuf[1024] = {0};
// 			if (msgRsp.Decode(pMsgParaBuf, nLen) < 0)
// 			{
// 				MYLOG_ERROR(logger, "GameDesk::HandleProxyMsg CRspUpdateUserGameDataPara Decode failed!");
// 
// 				return -1;
// 			}
// 			break;
// 		}
// 
// 	default:
// 		{
// 			break;
// 		}
// 	}
	return 0;
}

// ����ͻ�����Ϣ������Lotus
int CGameDesk::HandleClientMsg(unsigned int unUin, int nDeskStation, void* pMsgHead, const char* pMsgParaBuf, const int nLen)
{
	if (NULL == pMsgHead || NULL == pMsgParaBuf)
	{
		return -1;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(nDeskStation);
	if ( NULL == pPlayer)
	{
		MYLOG_ERROR(logger,"CGameDesk::HandleClientMsg ASS_GM_GAME_INFO GetPlayerInfo == NULL..[nDeskStation=%d]",nDeskStation);
		return -4;
	}

	//LogMsg(pMsgParaBuf, nLen);
	NetMessageHead* pMsg = (NetMessageHead*)pMsgParaBuf;
	pMsg->uMessageSize = hton32(pMsg->uMessageSize);					// ���ݰ���С
	pMsg->uMainID      = hton32(pMsg->uMainID);							// ����������
	pMsg->uAssistantID = hton32(pMsg->uAssistantID);					// ������������ ID
	pMsg->uHandleCode  = hton32(pMsg->uHandleCode);						// ���ݰ��������
	pMsg->dwClientIP   = hton32(pMsg->dwClientIP);                      // ����������ip����   20120319
	pMsg->uCheck       = hton32(pMsg->uCheck);					     	// �����ֶ�

	// ����Ϸ��Ϣͷ����
	int nNetMessageHead = sizeof(NetMessageHead);

	CMsgHead* pMsgHeadPtr   = (CMsgHead*)pMsgHead;

	//MYLOG_DEBUG(logger, "GameDesk::HandleClientMsg m_unMsgID=%d m_usMsgType=%d nDeskStation=%d, uMainID=%d, uAssistantID=%d, nNetMessageHead=%d, nLen=%d, pMsg->uMessageSize=%d", 
	//	pMsgHeadPtr->m_unMsgID, pMsgHeadPtr->m_usMsgType, nDeskStation, pMsg->uMainID, pMsg->uAssistantID, nNetMessageHead, nLen, pMsg->uMessageSize);

	if (MDM_GM_GAME_FRAME == pMsg->uMainID)
	{
		if (ASS_GM_GAME_INFO == pMsg->uAssistantID)
		{
			// ��ȡ״̬
			if ( nDeskStation < 0 || nDeskStation >= CONFIG_PALYER_COUNT )
			{
				MYLOG_ERROR(logger,"CGameDesk::HandleClientMsg ASS_GM_GAME_INFO nDeskStation Error..[nDeskStation=%d]",nDeskStation);
				return -3;
			}

			return OnGetGameStation(nDeskStation,pPlayer->GetGameUserInfo());
		}
		else
		{
			MYLOG_ERROR(logger, "CGameDesk::HandleClientMsg uMainID=%d, uAssistantID=%d", pMsg->uMainID, pMsg->uAssistantID);
			return -1;
		}
	}

	if (MDM_GM_GAME_NOTIFY != pMsg->uMainID)
	{
		MYLOG_ERROR(logger, "GameDesk::HandleClientMsg uMainID=%d, uAssistantID=%d", pMsg->uMainID, pMsg->uAssistantID);
		return -2;
	}

	int code = pMsg->uAssistantID;

	// ȥ����Ϣͷ
	char* pData  = (char*)pMsgParaBuf + nNetMessageHead;
	int   uSize  = nLen - nNetMessageHead;

	switch (code)
	{
	case ASS_GM_AGREE_GAME:  //ͬ����Ϸ
		{			
			UserAgreeGame(nDeskStation, pPlayer->GetGameUserInfo());
			break;
		}
	case ASS_GM_NORMAL_TALK: //������Ϣ
		{
			//MYLOG_DEBUG(logger,"CGameDesk::HandleClientMsg ASS_GM_NORMAL_TALK recv..[roomID=%d,DeskNo=%d,nDeskStation=%d,unUin=%d,uSize=%d,sizeof(MSG_GR_RS_NormalTalk)=%d]",GetRoomID(),GetTableID(),nDeskStation,unUin,uSize,sizeof(MSG_GR_RS_NormalTalk));
			SendDeskBroadCast(ASS_GM_NORMAL_TALK,(BYTE*)pData,uSize);
			break;
		}
	case SUB_C_USER_DO_ACTION: //��ҽ��в���
		{
			CMD_C_USER_DO_ACTION* pMsg = NULL;
			pMsg = (CMD_C_USER_DO_ACTION*)pData;
			if (NULL == pMsg)
			{
				return -1;
			}

			if (uSize != sizeof(CMD_C_USER_DO_ACTION))
			{
				MYLOG_ERROR(logger,"Error:CGameDesk::HandleClientMsg SUB_C_USER_DO_ACTION size Error..[roomID=%d,DeskNo=%d,uSize=%d], sizeof(CMD_C_USER_DO_ACTION)=%d",GetRoomID(),GetTableID(),uSize,sizeof(CMD_C_USER_DO_ACTION));
				return 0;
			}

			pMsg->dwAction = hton32(pMsg->dwAction);


			MYLOG_DEBUG(logger,"CGameDesk::OnMsgUserDoAction..[roomID=%d,DeskNo=%d,iDeskStation=%d,unUin=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x ,pMsg->dwAction==%0x]"
				,GetRoomID(),GetTableID(),nDeskStation,unUin,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction(),pMsg->dwAction);

			m_nPlayerTimeOutCount[nDeskStation] = 0;
			KillGameTimer(TIME_PLAYER_1+nDeskStation);

			int iRet = OnMsgUserDoAction(nDeskStation,pMsg);
			if ( iRet < 0 )
			{
				MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction return fail..[roomID=%d,DeskNo=%d,iDeskStation=%d,unUin=%d,iRet=%d]",GetRoomID(),GetTableID(),nDeskStation,unUin,iRet);
			}
			break;
		}

	case SUB_C_USER_TUOGUAN: //�й�
		{
			//Ч����Ϣ
			if (uSize!=sizeof(CMD_C_USER_TUOGUAN))
			{
				MYLOG_ERROR(logger, "nLen!=sizeof(CMD_C_USER_TUOGUAN nLen=%d sizeof(CMD_C_USER_TUOGUAN)=%d", nLen, sizeof(CMD_C_USER_TUOGUAN));
				return false;
			}

			//��Ϣ����
			CMD_C_USER_TUOGUAN * pTData=(CMD_C_USER_TUOGUAN *)pData;

			OnUserTuoGuan(nDeskStation, pTData->byIsTuoGuan);
			break;
		}

	default:
		{
			MYLOG_ERROR(logger, "HandleNotifyMessage UnProcCommand  Cmd id[%d]", code);
			return -1;
		}
	}
	return 0;
}

int CGameDesk::OnMsgUserDoAction(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg)
{
	if ( NULL == pMsg )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction NULL == pMsg..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	if ( pMsg->dwAction&EACTION_OUTCARD )
	{
		//����
		return OnMsgProcAction_OutCard(iDeskStation,pMsg);
	}
	else if (pMsg->dwAction&EACTION_CHI_LEFT || pMsg->dwAction&EACTION_CHI_MID || pMsg->dwAction&EACTION_CHI_RIGHT)
	{
		//����
		return OnMsgProcAction_EatCard(iDeskStation,pMsg);
	}
	else if (pMsg->dwAction&EACTION_PENG)
	{
		//����
		return OnMsgProcAction_PengCard(iDeskStation,pMsg);
	}
	else if (pMsg->dwAction&EACTION_GANG_DIAN)
	{
		//��--���
		return OnMsgProcAction_DianGang(iDeskStation,pMsg);
	}
	else if (pMsg->dwAction&EACTION_GANG_BU)
	{
		//��--����
		return OnMsgProcAction_BuGang(iDeskStation,pMsg);
	}
	else if (pMsg->dwAction&EACTION_GANG_AN)
	{
		//��--�Ը�
		return OnMsgProcAction_AnGang(iDeskStation,pMsg);
	}
	else if (pMsg->dwAction&EACTION_HU_ZIMO)
	{
		//��--����
		return OnMsgProcAction_ZiMoHu(iDeskStation,pMsg);
	}
	else if (pMsg->dwAction&EACTION_HU_DIAN)
	{
		//��--���ں�
		return OnMsgProcAction_DianPaoHu(iDeskStation,pMsg);
	}
	else if (pMsg->dwAction&EACTION_GIVEUP)
	{
		//��(����)
		return OnMsgProcAction_GiveUp(iDeskStation,pMsg);
	}

	MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction not handle..[roomID=%d,DeskNo=%d,iDeskStation=%d,pMsg->dwAction=%0x]",GetRoomID(),GetTableID(),iDeskStation,pMsg->dwAction);
	return -2;
}

//����
int CGameDesk::OnMsgProcAction_OutCard(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg)
{
	if ( NULL == pMsg )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_OutCard NULL == pMsg..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	if ( iDeskStation < 0 || iDeskStation >= CONFIG_PALYER_COUNT )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_OutCard nDeskStation Error..[nDeskStation=%d]",iDeskStation);
		return -2;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_OutCard NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -3;
	}

	//�ж��Ƿ��ǵ�ǰ���
	if ( GetCurOperateStation() != iDeskStation )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_OutCard.. Not Cur user..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d]",GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation());
		return -4;
	}

	if ( TSEAT_STATUS_OP_ING != GetCurSeatOpStatus() )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_OutCard ..Not enable out card..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurSeatOpStatus=%0x]",GetRoomID(),GetTableID(),iDeskStation,GetCurSeatOpStatus());
		return -5;
	}

	BYTE byOutMJCard = pMsg->byCard[0];
	int iRet = pPlayer->DoOutCard(byOutMJCard);
	if( iRet < 0 )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction  OnMsgProcAction_OutCard pPlayer->DoOutCard fail..[roomID=%d,DeskNo=%d,iDeskStation=%d,pMsg->byCard[0]=%d ,dwPlayerEnableAction=%0x, iRet=%d]",GetRoomID(),GetTableID(),iDeskStation,byOutMJCard,pPlayer->GetEnableAction(),iRet);
		return -6;
	}

	MYLOG_INFO(logger,"OnMsgProcAction_OutCard iDeskStation=%d", iDeskStation);

	m_byLastOutMjSetaNo = iDeskStation;
	m_byLastOutMj       = byOutMJCard;

	SetCurMjShowSeat(m_byLastOutMjSetaNo);

	//���Ƴɹ�
	pPlayer->ClearEnableAction();

	//֪ͨ�ͻ���
	std::vector<BYTE> vecCardList;
	vecCardList.push_back(byOutMJCard);
	NotifyClientOperatesSuccess(iDeskStation,EACTION_OUTCARD,vecCardList);

	//֪ͨ�ܺ�����
	NotifyEnableHuCards(iDeskStation);

	//����Ƿ���������� �Գ����� ���в���
	if ( CheckOutCardOtherEnableOperates(byOutMJCard,iDeskStation,false, false) )
	{
		SetCurSeatOpStatus(TSEAT_STATUS_WAIT_OTHER_OP_MYOUT_CARD);  //�ȴ�������Ҳ���

		//����ʱ
		SetDjs(EGPDJS_WAIT_OTHERCHOOSE,m_nOperateTime,GetCurOperateStation());
	}
	else
	{
		SetCurSeatOpStatus(TSEAT_STATUS_OP_COMPLETE); //���

		//��һ�����
		TurnToNextStation();
	}

	return 0;
}

//��⵱ǰ����ܽ��еĲ���
bool CGameDesk::CheckCurPlayerEnableOperates(bool bNeedCheckZiMoHu/*=true*/)
{
	PlayerInfo* pPlayer = GetPlayerInfo(GetCurOperateStation());
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction CheckCurPlayerEnableOperates NULL == pPlayer..[roomID=%d,DeskNo=%d,m_iCurSeatOpStatus=%0x,m_iCurOperateStation=%d,m_iNtStation=%d]"
			,GetRoomID(),GetTableID(),GetCurSeatOpStatus(),GetCurOperateStation(),m_iNtStation);
		return false;
	}

	//begin ��������˵ı��
	for (int iPlayerIndex=0;iPlayerIndex<CONFIG_PALYER_COUNT;iPlayerIndex++)
	{
		PlayerInfo* pPlayerTemp = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pPlayerTemp )
		{
			continue;
		}

		pPlayerTemp->ClearEnableAction();
	}
	//end

	MYLOG_INFO(logger,"CheckCurPlayerEnableOperates ChairID=%d MoPaiCount=%d", GetCurOperateStation(), pPlayer->GetMoPaiCount());

	vector<stOpCardList> vecOutList;
	DWORD  dwPlayerEnableAction = 0; //�ܲ����Ķ���  �ο�E_ACTION_TYPE 

	//�ܷ����
	dwPlayerEnableAction|=EACTION_OUTCARD;

	MYLOG_INFO(logger,"pPlayer->CheckMyCanBuGang m_bCfgGuoHouBuGang=%d", m_bCfgGuoHouBuGang);
	//�ܷ񲹸�
	if ( pPlayer->CheckMyCanBuGang(vecOutList, m_bCfgGuoHouBuGang))
	{
		dwPlayerEnableAction|=EACTION_GANG_BU;
		MYLOG_INFO(logger,"pPlayer->CheckMyCanBuGang dwPlayerEnableAction=%d", dwPlayerEnableAction);
	}

	//�ܷ񰵸�
	if ( pPlayer->CheckMyCanAnGang(vecOutList,GetCfgLaiziCard()))
	{
		dwPlayerEnableAction|=EACTION_GANG_AN;
	}

	//�ܷ�������
	if ( bNeedCheckZiMoHu && pPlayer->CheckMyCanZiMoHu(m_bCfgHuQiDui,GetCfgLaiziCard()) )
	{
		dwPlayerEnableAction|=EACTION_HU_ZIMO;

		DWORD dwHuExtendsType = 0;
		//���ϻ�
		if ( pPlayer->GetGangZiMoHu() )
		{
			dwHuExtendsType |= EHUEXTENDS_TYPE_GANG_HUA;
		}

		//�ĺ���
		if ( pPlayer->Get4HongZhongHu())
		{
			dwHuExtendsType |= EHUEXTENDS_TYPE_SIHONGZHONG;
		}

		pPlayer->SetHuExtendsType(dwHuExtendsType);
	}
	else
	{
		pPlayer->SetHuExtendsType(EHUEXTENDS_TYPE_NO);
	}

	//��
	if ( IsCanGiveUp(dwPlayerEnableAction) )
	{
		dwPlayerEnableAction|=EACTION_GIVEUP;
	}

	DWORD dwFlagAll = GetTickCountUs();
	// ������ʾ
//	if (pPlayer->CheckMyCanTing_14(false))
	{		
		stCardData stCardShow;
		for (int i=0; i<CONFIG_PALYER_COUNT; ++i)
		{
			PlayerInfo* pPlayerShow = GetPlayerInfo(i);
			if (pPlayerShow)
				pPlayerShow->getShowCardsData(stCardShow, i==GetCurOperateStation());
		}

		DWORD dwFlag = GetTickCountUs();
		vector<stHuTips> vctHuTips = pPlayer->CheckMyCanTing_HuTips(stCardShow);
		if (vctHuTips.size() > 0)
		{			
 			MYLOG_DEBUG(logger,"pino_log CheckMyCanTing_HuTips vctHuTips=%d Time:%d us", vctHuTips.size(), GetTickCountUs()-dwFlag);
// 			for (size_t i=0; i<vctHuTips.size(); ++i)
// 			{
// 				MYLOG_DEBUG(logger,"pino_log CheckMyCanTing_HuTips byCardOut=0x%02X", vctHuTips[i].byCardOut);
// 				for (int n=0; n<10; ++n)
// 				{
// 					if (vctHuTips[i].stData[n].byCardHu > 0)
// 					{
// 						MYLOG_DEBUG(logger, "pino_log CheckMyCanTing_HuTips byCardHu:0x%02X num=%d val=%d", vctHuTips[i].stData[n].byCardHu, vctHuTips[i].stData[n].byNum, ntohl(vctHuTips[i].stData[n].nVal));
// 					}
// 				}			
// 			}
			int  nBuffSize = sizeof(CMD_S_TingLock_HuTips);
			char szBuffer[2048] = {0};
			CMD_S_TingLock_HuTips stTipsTemp;
			stTipsTemp.byUnlockNum = vctHuTips.size();
			memcpy(szBuffer, &stTipsTemp, nBuffSize);
			memcpy(szBuffer+nBuffSize, &vctHuTips[0], sizeof(stHuTips)*stTipsTemp.byUnlockNum);		
			SendGameData(GetCurOperateStation(), SUB_S_TING_LOCK_HUTIPS, (BYTE*)szBuffer, nBuffSize+sizeof(stHuTips)*stTipsTemp.byUnlockNum);
		}
	}
	MYLOG_DEBUG(logger,"pino_log CheckMyCanTing_14 TimeAll:%d us", GetTickCountUs()-dwFlagAll);

	//֪ͨ��Ҳ���
	char szBuffer[2048]={0};
	CMD_S_NOTIFY_USER_OPERATE* pSendMsg = (CMD_S_NOTIFY_USER_OPERATE*)szBuffer;
	pSendMsg->dwEnableAction = ntoh32(dwPlayerEnableAction);
	pSendMsg->byOutMjSeatNo  = GetCurOperateStation();
	pSendMsg->byOutMj        = 0;
	pSendMsg->byCardNum      = std::min((int)(vecOutList.size()),10);
	for ( int iCardIndex = 0;  iCardIndex < pSendMsg->byCardNum ; iCardIndex++ )
	{
		const stOpCardList& OneTempCardIfo = vecOutList[iCardIndex];
		memcpy(&pSendMsg->lsCardInfo[iCardIndex],&OneTempCardIfo,sizeof(stOpCardList));
	}

	int nMsgParaLen = sizeof(CMD_S_NOTIFY_USER_OPERATE) + (pSendMsg->byCardNum-1)*sizeof(stOpCardList);
	SendGameData(GetCurOperateStation(),SUB_S_NOTIFY_USER_OPERATE,(BYTE*)szBuffer,nMsgParaLen);

	MYLOG_DEBUG(logger,"CGameDesk::OnMsgUserDoAction CheckCurPlayerEnableOperates SendData..[roomID=%d,DeskNo=%d,m_iCurOperateStation=%d,dwPlayerEnableAction=%0x]"
		,GetRoomID(),GetTableID(),GetCurOperateStation(),dwPlayerEnableAction);

	//������
	pSendMsg->dwEnableAction = dwPlayerEnableAction;
	pPlayer->SetEnableServerActionData(pSendMsg);

	//����ʱ
	SetDjs(EGPDJS_OUTCARD,m_nOperateTime,GetCurOperateStation());

	m_nTempCount = 0;
	KillGameTimer(TIME_ONE_COUNT);
	SetGameTimer(TIME_ONE_COUNT, 1000);

	int nCurSeat = GetCurOperateStation();
	int nDelayTime = (m_nOperateTime+2)*1000;
	if (m_bPlayerTuoGuan[nCurSeat] /*|| m_arUserInfo[player]._bIsNetCut*/)
	{
		nDelayTime = 2000;
	}
	KillGameTimer(TIME_PLAYER_1+nCurSeat);
	SetGameTimer(TIME_PLAYER_1+nCurSeat, nDelayTime);

	return true;
}

//�����������Ƿ��ܶԳ����ƽ��в���
bool CGameDesk::CheckOutCardOtherEnableOperates(BYTE byOutMJCard,BYTE byOutMJSeatNo,bool bOnlyCheckDianHu,bool bCheckQiangGangHu)
{
	bool bOtherHasOperates = false;
	for (int i=0;i<CONFIG_PALYER_COUNT;i++)
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if ( NULL == pPlayer || byOutMJSeatNo == i )
		{
			continue;
		}

		pPlayer->ClearEnableAction();

		vector<stOpCardList> vecOutList;
		DWORD  dwPlayerEnableAction = 0; //�ܲ����Ķ���  �ο�E_ACTION_TYPE 

		//�ܷ���ں�
		bool bCanQiangGangHu = false;
		if (bCheckQiangGangHu)
		{
			bCanQiangGangHu = pPlayer->CheckMyCanDianHu(byOutMJCard,m_bCfgHuQiDui,GetCfgLaiziCard(),bCheckQiangGangHu);
		}

		if (bCanQiangGangHu)
		{
			dwPlayerEnableAction|=EACTION_HU_DIAN;
			//pPlayer->SetHuExtendsType(EHUEXTENDS_TYPE_SIHONGZHONG);
		}
		else
		{
			pPlayer->SetHuExtendsType(EHUEXTENDS_TYPE_NO);
		}

		if ( !bOnlyCheckDianHu )
		{
			if ( IsNextStation(byOutMJSeatNo,i) )
			{
				//��һ��λ��
				//�ܳ�
				if( pPlayer->CheckMyCanChi_Left(byOutMJCard,vecOutList) )
				{
					dwPlayerEnableAction|=EACTION_CHI_LEFT;
				}

				if( pPlayer->CheckMyCanChi_Mid(byOutMJCard,vecOutList) )
				{
					dwPlayerEnableAction|=EACTION_CHI_MID;
				}

				if( pPlayer->CheckMyCanChi_Right(byOutMJCard,vecOutList) )
				{
					dwPlayerEnableAction|=EACTION_CHI_RIGHT;
				}
			}

			//�ܷ���
			if ( pPlayer->CheckMyCanPeng(byOutMJCard,vecOutList,GetCfgLaiziCard()) )
			{
				dwPlayerEnableAction|=EACTION_PENG;
			}

			//�ܷ���
			if ( pPlayer->CheckMyCanDianGang(byOutMJCard,vecOutList,GetCfgLaiziCard()) )
			{
				dwPlayerEnableAction|=EACTION_GANG_DIAN;
			}
		}

		//��
		if ( IsCanGiveUp(dwPlayerEnableAction) )
		{
			dwPlayerEnableAction|=EACTION_GIVEUP;
		}

		if (  dwPlayerEnableAction&EACTION_CHI_LEFT || dwPlayerEnableAction&EACTION_CHI_MID || dwPlayerEnableAction&EACTION_CHI_RIGHT
			||dwPlayerEnableAction&EACTION_PENG
			||dwPlayerEnableAction&EACTION_GANG_DIAN
			||dwPlayerEnableAction&EACTION_HU_DIAN
		  )
		{
			bOtherHasOperates = true;

			//֪ͨ��Ҳ���
			char szBuffer[2048]={0};
			CMD_S_NOTIFY_USER_OPERATE* pSendMsg = (CMD_S_NOTIFY_USER_OPERATE*)szBuffer;
			pSendMsg->dwEnableAction = ntoh32(dwPlayerEnableAction);
			pSendMsg->byOutMjSeatNo  = byOutMJSeatNo;
			pSendMsg->byOutMj        = byOutMJCard;
			pSendMsg->byCardNum      = std::min(int(vecOutList.size()),10);
			for ( int iCardIndex = 0;  iCardIndex < pSendMsg->byCardNum ; iCardIndex++ )
			{
				const stOpCardList& OneTempCardIfo = vecOutList[iCardIndex];
				memcpy(&pSendMsg->lsCardInfo[iCardIndex],&OneTempCardIfo,sizeof(stOpCardList));
			}

			int nMsgParaLen = sizeof(CMD_S_NOTIFY_USER_OPERATE) + (pSendMsg->byCardNum-1)*sizeof(stOpCardList);
			SendGameData(i,SUB_S_NOTIFY_USER_OPERATE,(BYTE*)szBuffer,nMsgParaLen);

			//������
			pSendMsg->dwEnableAction = dwPlayerEnableAction;
			pPlayer->SetEnableServerActionData(pSendMsg);

			//������ʱ��
			int nDelayTime = (m_nOperateTime+2)*1000;
			if (m_bPlayerTuoGuan[i] /*|| m_arUserInfo[player]._bIsNetCut*/)
			{
				nDelayTime = 2000;
			}
			KillGameTimer(TIME_PLAYER_1+i);
			SetGameTimer(TIME_PLAYER_1+i, nDelayTime);
		}
	}

	if (bOtherHasOperates)
	{
		m_nTempCount = 0;
		KillGameTimer(TIME_ONE_COUNT);
		SetGameTimer(TIME_ONE_COUNT, 1000);
	}

	return bOtherHasOperates;
}

//����
int CGameDesk::OnMsgProcAction_EatCard(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg)
{
	if ( NULL == pMsg )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_EatCard NULL == pMsg..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	if ( iDeskStation < 0 || iDeskStation >= CONFIG_PALYER_COUNT )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_EatCard nDeskStation Error..[nDeskStation=%d]",iDeskStation);
		return -2;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_EatCard NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -3;
	}


	return 0;
}

//����
int CGameDesk::OnMsgProcAction_PengCard(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg)
{
	if ( NULL == pMsg )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_PengCard NULL == pMsg..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	if ( iDeskStation < 0 || iDeskStation >= CONFIG_PALYER_COUNT )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_PengCard nDeskStation Error..[nDeskStation=%d]",iDeskStation);
		return -2;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_PengCard NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -3;
	}

	//�Լ����� ���Լ�����
	if ( GetCurOperateStation() == iDeskStation )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_PengCard m_iCurOperateStation == iDeskStation..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation());
		return -4;
	}

	//��������
	if ( !pPlayer->IsEnableOperates(EACTION_PENG) )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_PengCard.. pPlayer->IsEnableOperates(EACTION_PENG) == false..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction());

		//֪ͨ������ҵ�ǰ��ʾ
		NotifyAdjustPlayerEnableAction(iDeskStation,pMsg->dwAction);
		return -5;
	}

	//���õȴ�״̬
	pPlayer->SetWaitOneDoAction(*pMsg);
	int iRet = CheckDoWaitOperate(iDeskStation);
	if ( iRet < 0 )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_PengCard.. CheckDoWaitOperate() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x] ,iRet=%d"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction(),iRet);
		return -6;
	}
	return 0;
}

//��--���
int CGameDesk::OnMsgProcAction_DianGang(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg)
{
	if ( NULL == pMsg )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianGang NULL == pMsg..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	if ( iDeskStation < 0 || iDeskStation >= CONFIG_PALYER_COUNT )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianGang nDeskStation Error..[nDeskStation=%d]",iDeskStation);
		return -2;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianGang NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -3;
	}

	//�Լ����� ��� �Լ�����
	if ( GetCurOperateStation() == iDeskStation )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianGang m_iCurOperateStation == iDeskStation..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation());
		return -4;
	}

	//���ܵ��
	if ( !pPlayer->IsEnableOperates(EACTION_GANG_DIAN) )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianGang.. pPlayer->IsEnableOperates(EACTION_GANG_DIAN) == false..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction());

		//֪ͨ������ҵ�ǰ��ʾ
		NotifyAdjustPlayerEnableAction(iDeskStation,pMsg->dwAction);
		return -5;
	}

	//���õȴ�״̬
	pPlayer->SetWaitOneDoAction(*pMsg);
	int iRet = CheckDoWaitOperate(iDeskStation);
	if ( iRet < 0 )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianGang.. CheckDoWaitOperate() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x] ,iRet=%d"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction(),iRet);
		return -6;
	}

	return 0;
}

//��--����
int CGameDesk::OnMsgProcAction_BuGang(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg)
{
	if ( NULL == pMsg )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_BuGang NULL == pMsg..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	if ( iDeskStation < 0 || iDeskStation >= CONFIG_PALYER_COUNT )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_BuGang nDeskStation Error..[nDeskStation=%d]",iDeskStation);
		return -2;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_BuGang NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -3;
	}

	//���ܱ������Լ�
	if ( GetCurOperateStation() != iDeskStation )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_BuGang m_iCurOperateStation != iDeskStation..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation());
		return -4;
	}

	//���ܲ���
	if ( !pPlayer->IsEnableOperates(EACTION_GANG_BU) )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_BuGang.. pPlayer->IsEnableOperates(EACTION_GANG_BU) == false..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction());

		//֪ͨ������ҵ�ǰ��ʾ
		NotifyAdjustPlayerEnableAction(iDeskStation,pMsg->dwAction);
		return -5;
	}

	//���ܲ���
	int iRet = pPlayer->DoBuGang(pMsg->byCard[0]);
	if ( iRet < 0 )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_BuGang.. pPlayer->DoBuGang() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,pMsg->byCard[0]=%d,dwPlayerEnableAction=%0x] ,iRet=%d"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),pMsg->byCard[0],pPlayer->GetEnableAction(),iRet);

		return -6;
	}

	//���ܳɹ���
	pPlayer->ClearEnableAction();

	//֪ͨ�ͻ���
	std::vector<BYTE> vecCardList;
	vecCardList.push_back(pMsg->byCard[0]);
	NotifyClientOperatesSuccess(iDeskStation,EACTION_GANG_BU,vecCardList);

	//�ж���������ܷ����
	//����Ƿ���������� �Գ����� ���в���
	if ( CheckOutCardOtherEnableOperates(pMsg->byCard[0],iDeskStation,true, true) )
	{
		m_byLastOutMjSetaNo = iDeskStation;
		m_byLastOutMj       = pMsg->byCard[0];

		SetCurSeatOpStatus(TSEAT_STATUS_WAIT_OTHER_OP_MYGANG_CARD);  //�ȴ�������� �� �Ҳ��ܵ��ƽ��в���
	}
	else
	{
		TurnToAimStation(iDeskStation,true,false);
	}

	return 0;
}

//��--����
int CGameDesk::OnMsgProcAction_AnGang(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg)
{
	if ( NULL == pMsg )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_AnGang NULL == pMsg..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	if ( iDeskStation < 0 || iDeskStation >= CONFIG_PALYER_COUNT )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_AnGang nDeskStation Error..[nDeskStation=%d]",iDeskStation);
		return -2;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_AnGang NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -3;
	}

	//���ܱ������Լ�
	if ( GetCurOperateStation() != iDeskStation )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_AnGang m_iCurOperateStation != iDeskStation..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation());
		return -4;
	}

	//���ܰ���
	if ( !pPlayer->IsEnableOperates(EACTION_GANG_AN) )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_AnGang.. pPlayer->IsEnableOperates(EACTION_GANG_AN) == false..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction());

		//֪ͨ������ҵ�ǰ��ʾ
		NotifyAdjustPlayerEnableAction(iDeskStation,pMsg->dwAction);
		return -5;
	}

	//���ܲ���
	int iRet = pPlayer->DoAnGang(pMsg->byCard[0]);
	if ( iRet < 0 )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_AnGang.. pPlayer->DoAnGang() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,pMsg->byCard[0]=%d,dwPlayerEnableAction=%0x] ,iRet=%d"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),pMsg->byCard[0],pPlayer->GetEnableAction(),iRet);
		return -6;
	}

	//���ܳɹ���
	pPlayer->ClearEnableAction();

	//֪ͨ�ͻ���
	std::vector<BYTE> vecCardList;
	vecCardList.push_back(pMsg->byCard[0]);
	NotifyClientOperatesSuccess(iDeskStation,EACTION_GANG_AN,vecCardList);

	TurnToAimStation(iDeskStation,true,false);
	return 0;
}

//��--����
int CGameDesk::OnMsgProcAction_ZiMoHu(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg)
{
	if ( NULL == pMsg )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_ZiMoHu NULL == pMsg..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	if ( iDeskStation < 0 || iDeskStation >= CONFIG_PALYER_COUNT )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_ZiMoHu nDeskStation Error..[nDeskStation=%d]",iDeskStation);
		return -2;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_ZiMoHu NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -3;
	}

	//�����������Լ�
	if ( GetCurOperateStation() != iDeskStation )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_ZiMoHu m_iCurOperateStation != iDeskStation..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation());
		return -4;
	}

	//����������
	if ( !pPlayer->IsEnableOperates(EACTION_HU_ZIMO) )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_ZiMoHu.. pPlayer->IsEnableOperates(EACTION_HU_ZIMO) == false..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction());

		//֪ͨ������ҵ�ǰ��ʾ
		NotifyAdjustPlayerEnableAction(iDeskStation,pMsg->dwAction);
		return -5;
	}

	//����������
	int iRet = pPlayer->DoZiMoHu(m_bCfgHuQiDui,GetCfgLaiziCard());
	if ( iRet < 0 )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_ZiMoHu.. pPlayer->DoZiMoHu() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,dwPlayerEnableAction=%0x],iRet=%d"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),pPlayer->GetEnableAction(),iRet);
		return -6;
	}

	//�������ɹ���
	pPlayer->ClearEnableAction();

	//֪ͨ�ͻ���
	std::vector<BYTE> vecCardList;
	vecCardList.push_back(m_byLastOutMj);
	NotifyClientOperatesSuccess(iDeskStation,EACTION_HU_ZIMO,vecCardList);

	SetNextNtStation(iDeskStation);

	//����
	OnMsgProcAction_JieSuan(false);

	return 0;
}

//��--���ں�
int CGameDesk::OnMsgProcAction_DianPaoHu(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg)
{
	if ( NULL == pMsg )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianPaoHu NULL == pMsg..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	if ( iDeskStation < 0 || iDeskStation >= CONFIG_PALYER_COUNT )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianPaoHu nDeskStation Error..[nDeskStation=%d]",iDeskStation);
		return -2;
	}

	if ( !m_bCfgCanDianPaoHu )
	{
		//���ܵ��ں�
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianPaoHu cann't dian pao hu..[nDeskStation=%d,m_bCanDianPaoHu=%d]",iDeskStation,m_bCfgCanDianPaoHu);
		return -2;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianPaoHu NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -3;
	}

	//�Լ����� ���ں� �Լ�����
	if ( GetCurOperateStation() == iDeskStation )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianPaoHu m_iCurOperateStation == iDeskStation..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation());
		return -4;
	}

	//���ܵ��ں�
	if ( !pPlayer->IsEnableOperates(EACTION_HU_DIAN) )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianPaoHu.. pPlayer->IsEnableOperates(EACTION_HU_DIAN) == false..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction());

		//֪ͨ������ҵ�ǰ��ʾ
		NotifyAdjustPlayerEnableAction(iDeskStation,pMsg->dwAction);
		return -5;
	}


	//���ں� ����
	bool bQiangGangHu = (TSEAT_STATUS_WAIT_OTHER_OP_MYGANG_CARD == GetCurSeatOpStatus()) ? true:false;
	int iRet = pPlayer->DoDianPaoHu(m_byLastOutMj,m_byLastOutMjSetaNo,bQiangGangHu,m_bCfgHuQiDui,GetCfgLaiziCard());
	if ( iRet < 0 )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianPaoHu.. pPlayer->DoDianPaoHu() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x,bQiangGangHu=%d] ,iRet=%d"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction(),bQiangGangHu,iRet);
		return -6;
	}

	//���ں��ɹ���
	pPlayer->ClearEnableAction();

	//�����ҵ������������Ȩ��
	ClearAllPlayerLowQuanXian(pMsg->dwAction);

	//ɾ����
	if ( bQiangGangHu )
	{
		//���ܺ�
		//ɾ��������� �ĸ���
		PlayerInfo* pLastOutCardPlayer = GetPlayerInfo(m_byLastOutMjSetaNo);
		if ( NULL != pLastOutCardPlayer )
		{
			int iRetRemove = pLastOutCardPlayer->RemoveOneBuGangToPeng(m_byLastOutMj);
			if ( iRetRemove < 0 )
			{
				MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianPaoHu.. pLastOutCardPlayer->RemoveOneBuGangToPeng() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d] ,iRet=%d"
					,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,iRet);
			}
		}
	}
	else
	{
		//���ں�
		//ɾ��������ҳ�����
		PlayerInfo* pLastOutCardPlayer = GetPlayerInfo(m_byLastOutMjSetaNo);
		if ( NULL != pLastOutCardPlayer )
		{
			int iRetRemove = pLastOutCardPlayer->RemoveOneOutCardFromTail(m_byLastOutMj);
			if ( iRetRemove < 0 )
			{
				MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianPaoHu.. pLastOutCardPlayer->RemoveOneOutCardFromTail() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d] ,iRet=%d"
					,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,iRet);
			}
		}
	}

	//֪ͨ�ͻ���
	std::vector<BYTE> vecCardList;
	vecCardList.push_back(m_byLastOutMj);
	NotifyClientOperatesSuccess(iDeskStation,EACTION_HU_DIAN,vecCardList);

	bool bHasOtherEnableDianHu = false;
	for (int i=0;i<CONFIG_PALYER_COUNT;i++)
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(i);
		if ( NULL == pTempPlayer || i == GetCurOperateStation() )
		{
			continue;
		}

		if ( pTempPlayer->GetEnableAction() != 0 && pTempPlayer->IsEnableOperates(EACTION_HU_DIAN) )
		{
			bHasOtherEnableDianHu = true;
			break;
		}
	}

	if ( !bHasOtherEnableDianHu )
	{
		//�ж��Ƿ��Ƕ��˺�
		int iHuPlayerCount = 0;
		for (int i=0;i<CONFIG_PALYER_COUNT;i++)
		{
			PlayerInfo* pTempPlayer = GetPlayerInfo(i);
			if ( NULL == pTempPlayer || i == GetCurOperateStation() )
			{
				continue;
			}

			if ( pTempPlayer->GetHasHu() )
			{
				iHuPlayerCount++;
			}
		}

		if ( iHuPlayerCount > 1 )
		{
			SetNextNtStation(GetCurOperateStation());
		}
		else
		{
			SetNextNtStation(iDeskStation);
		}

		//����
		OnMsgProcAction_JieSuan(false);
	}

	return 0;
}

//��(����)
int CGameDesk::OnMsgProcAction_GiveUp(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg)
{
	if ( NULL == pMsg )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction NULL == pMsg..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	if ( iDeskStation < 0 || iDeskStation >= CONFIG_PALYER_COUNT )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction nDeskStation Error..[nDeskStation=%d]",iDeskStation);
		return -2;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -3;
	}

	//�ж�����Ƿ��ܲ��ܣ�������
	if (!m_bCfgGuoHouBuGang && pPlayer->IsEnableOperates(EACTION_GANG_BU))
	{
		pPlayer->AddCannotBuGangCard_UseGiveUpGang();
	}

	//��ǰ��Ҳ������ƣ�������в���
	if ( GetCurOperateStation() == iDeskStation )
	{
		//MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_GiveUp m_iCurOperateStation == iDeskStation..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d]"
		//	,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation());
		//return -4;

		if( IsCanGiveUp(pPlayer->GetEnableAction()) ) 
		{
			//֪ͨ�ͻ���
			std::vector<BYTE> vecCardList;
			NotifyClientOperatesSuccess(iDeskStation,EACTION_GIVEUP,vecCardList,iDeskStation);
		}
		return 0;
	}

	//�ж��Ƿ�������
	if( !IsCanGiveUp(pPlayer->GetEnableAction()) ) 
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_GiveUp..fail. can't give up.[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction());
		return -5;
	}

	//�ж��Ƿ��ܵ��ں��������˼ҹ��ⲻ�������
	//�ܺ��������
	if ( pPlayer->IsEnableOperates(EACTION_HU_DIAN) )
	{
		pPlayer->SetDisableDianPaoHuFlag();
	}

	if ( TSEAT_STATUS_WAIT_OTHER_OP_MYOUT_CARD == GetCurSeatOpStatus() )
	{
		pPlayer->ClearEnableAction();

		//֪ͨ�ͻ���
		std::vector<BYTE> vecCardList;
		NotifyClientOperatesSuccess(iDeskStation,EACTION_GIVEUP,vecCardList,iDeskStation);

		//ͳ���ܵĻ�û�й�����
		int iTotalWaitOtherCount = 0;
		for (int i=0;i<CONFIG_PALYER_COUNT;i++)
		{
			PlayerInfo* pTempPlayer = GetPlayerInfo(i);
			if ( NULL == pTempPlayer || i == GetCurOperateStation() )
			{
				continue;
			}

			if (pTempPlayer->GetEnableAction() != 0)
			{
				iTotalWaitOtherCount++;
			}
		}

		//���������������һ����Ұ�
		if ( iTotalWaitOtherCount <= 0 )
		{
			if ( GetHuPlayerCount() > 0 )
			{
				//����
				OnMsgProcAction_JieSuan(false);
			}
			else
			{
				TurnToNextStation();
			}
		}
		else
		{
			CheckDoWaitOperate();
		}
	}
	else if ( TSEAT_STATUS_WAIT_OTHER_OP_MYGANG_CARD == GetCurSeatOpStatus() )
	{
		pPlayer->ClearEnableAction();

		//֪ͨ�ͻ���
		std::vector<BYTE> vecCardList;
		NotifyClientOperatesSuccess(iDeskStation,EACTION_GIVEUP,vecCardList,iDeskStation);

		//ͳ���ܵĻ�û�й�����
		int iTotalWaitOtherCount = 0;
		for (int i=0;i<CONFIG_PALYER_COUNT;i++)
		{
			PlayerInfo* pTempPlayer = GetPlayerInfo(i);
			if ( NULL == pTempPlayer || i == GetCurOperateStation() )
			{
				continue;
			}

			if (pTempPlayer->GetEnableAction() != 0)
			{
				iTotalWaitOtherCount++;
			}
		}

		//���������
		if ( iTotalWaitOtherCount <= 0 )
		{
			if ( GetHuPlayerCount() > 0 )
			{
				//����
				OnMsgProcAction_JieSuan(false);
			}
			else
			{
				//ģ��
				TurnToAimStation(iDeskStation,true,false);
			}
		}
		else
		{
			CheckDoWaitOperate();
		}
	}
	else if ( TSEAT_STATUS_OP_ING == GetCurSeatOpStatus() )
	{
		//������
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_GiveUp..TSEAT_STATUS_OP_ING == GetCurSeatOpStatus().[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction());
	}
	return 0;
}

//��ȡû�в��������Ȩ�����ֵ
int CGameDesk::GetMaxNotOperateRightIndex(int& iOutMaxDeskStation,DWORD& dwOutMaxOneAction)
{
	iOutMaxDeskStation = -1;
	dwOutMaxOneAction  = 0;

	int iMaxRightIndex = 0;

	for (int iPlayerIndex=0;iPlayerIndex<CONFIG_PALYER_COUNT;iPlayerIndex++)
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pTempPlayer )
		{
			continue;
		}

		if( 0 == pTempPlayer->GetEnableAction() )
		{
			continue;
		}

		const CMD_C_USER_DO_ACTION& doAction = pTempPlayer->GetWaitOneDoAction();
		if ( 0 != doAction.dwAction )
		{
			continue;
		}

		DWORD dwTempCompareMaxAction = pTempPlayer->GetHasMaxOpEnableRight(dwOutMaxOneAction);
		if ( 0 == dwTempCompareMaxAction )
		{
			continue;
		}

		dwOutMaxOneAction = dwTempCompareMaxAction;
		iOutMaxDeskStation = iPlayerIndex;
		iMaxRightIndex = pTempPlayer->GetActionRightIndex(dwOutMaxOneAction);
	}

	return iMaxRightIndex;
}

//�����ҵ͵�Ȩ��
int  CGameDesk::ClearAllPlayerLowQuanXian(DWORD dwOneAction)
{
	for (int iPlayerIndex=0;iPlayerIndex<CONFIG_PALYER_COUNT;iPlayerIndex++)
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pTempPlayer )
		{
			continue;
		}

		bool bHasReset = pTempPlayer->ClearLowQuanXian(dwOneAction);
		if ( bHasReset )
		{
			//֪ͨ������ҵ�ǰ��ʾ
			NotifyAdjustPlayerEnableAction(iPlayerIndex,0);
		}
	}
	return 0;
}

//��ȡ�Ѿ����������Ȩ�����ֵ
int CGameDesk::GetMaxHasOperateRightIndex(int& iOutMaxDeskStation,DWORD& dwOutMaxOneAction)
{
	iOutMaxDeskStation = -1;
	dwOutMaxOneAction = 0;

	int iMaxRightIndex = 0;
	for (int iPlayerIndex=0;iPlayerIndex<CONFIG_PALYER_COUNT;iPlayerIndex++)
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pTempPlayer )
		{
			continue;
		}

		const CMD_C_USER_DO_ACTION& doAction = pTempPlayer->GetWaitOneDoAction();
		if ( 0 == doAction.dwAction )
		{
			continue;
		}

		if ( 0 == dwOutMaxOneAction  || pTempPlayer->IsHasMaxWaitRight(dwOutMaxOneAction) )
		{
			dwOutMaxOneAction = doAction.dwAction;

			iOutMaxDeskStation = iPlayerIndex;

			iMaxRightIndex = pTempPlayer->GetActionRightIndex(dwOutMaxOneAction);
		}
	}

	return iMaxRightIndex;
}

//��ȡ���������
int  CGameDesk::GetHuPlayerCount()
{
	int iCount = 0;
	for (int iPlayerIndex=0;iPlayerIndex<CONFIG_PALYER_COUNT;iPlayerIndex++)
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pTempPlayer )
		{
			continue;
		}

		if( pTempPlayer->GetHasHu() )
		{
			iCount++;
		}
	}

	return iCount;
}

//��Ⲣ��ִ�еȴ��Ĳ���
int CGameDesk::CheckDoWaitOperate(int iNotifyWatiOpStation/*=-1*/)
{
	//���û�в�����������
	int   iOutNotOpMaxDeskStation = -1;
	DWORD dwOutNotOpMaxOneAction  = 0;
	int iNotOpMaxRightIndex = GetMaxNotOperateRightIndex(iOutNotOpMaxDeskStation,dwOutNotOpMaxOneAction);

	//��ȡ�Ѿ�������������
	int   iOutHasOpMaxDeskStation = -1;
	DWORD dwOutHasOpMaxOneAction  = 0;
	int iHasOpMaxRightIndex = GetMaxHasOperateRightIndex(iOutHasOpMaxDeskStation,dwOutHasOpMaxOneAction);

	int iDeskStation = iOutHasOpMaxDeskStation;

	if ( iHasOpMaxRightIndex > iNotOpMaxRightIndex )
	{
		if ( iDeskStation < 0 || iDeskStation >= CONFIG_PALYER_COUNT )
		{
			MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction CheckDoWaitOperate nDeskStation Error..[nDeskStation=%d]",iDeskStation);
			return -2;
		}

		PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
		if ( NULL == pPlayer )
		{
			MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction CheckDoWaitOperate NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
			return -3;
		}

		//�Ѿ������� ���� û�в�����  ֱ�Ӳ���
		switch ( dwOutHasOpMaxOneAction )
		{
		case EACTION_CHI_LEFT:  //����--���(@**)
		case EACTION_CHI_MID:   //����--�г�(*@*)
		case EACTION_CHI_RIGHT: //����--�ҳ�(**@)
			{
				//�齫��ͷ����ʾ
				SetCurMjShowSeat(-1);
			}
			break;

		case EACTION_PENG:  //����
			{
				int iRet = pPlayer->DoPengCard(m_byLastOutMj,m_byLastOutMjSetaNo, m_bCfgXianPengHouGang);
				if ( iRet < 0 )
				{
					MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction CheckDoWaitOperate EACTION_PENG.. pPlayer->DoPengCard() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x] ,iRet=%d"
						,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction(),iRet);
					return -6;
				}

				//���ɹ���
				pPlayer->ClearEnableAction();

				//�����ҵ������������Ȩ��
				ClearAllPlayerLowQuanXian(dwOutHasOpMaxOneAction);

				//ɾ��������ҳ�����
				PlayerInfo* pLastOutCardPlayer = GetPlayerInfo(m_byLastOutMjSetaNo);
				if ( NULL != pLastOutCardPlayer )
				{
					int iRetRemove = pLastOutCardPlayer->RemoveOneOutCardFromTail(m_byLastOutMj);
					if ( iRetRemove < 0 )
					{
						MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction CheckDoWaitOperate EACTION_PENG.. pLastOutCardPlayer->RemoveOneOutCardFromTail() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d] ,iRet=%d"
							,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,iRet);
					}
				}

				//�齫��ͷ����ʾ
				SetCurMjShowSeat(-1);

				//֪ͨ�ͻ���
				std::vector<BYTE> vecCardList;
				vecCardList.push_back(m_byLastOutMj);
				NotifyClientOperatesSuccess(iDeskStation,EACTION_PENG,vecCardList);

				//////////////////////////////////////////////////////////////////////////
				//ת����ǰ���
				SetCurOperateStation(iDeskStation);
				SetCurSeatOpStatus(TSEAT_STATUS_OP_ING);

				//��⵱ǰ����ܽ��еĲ���
				CheckCurPlayerEnableOperates(false);
				//TurnToAimStation(iDeskStation,false,false);
				//////////////////////////////////////////////////////////////////////////
			}
			break;

		case EACTION_GANG_DIAN: //��--���
			{
				int iRet = pPlayer->DoDianGang(m_byLastOutMj,m_byLastOutMjSetaNo);
				if ( iRet < 0 )
				{
					MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianGang.. pPlayer->DoDianGang() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x] ,iRet=%d"
						,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction(),iRet);
					return -6;
				}

				MYLOG_INFO(logger,"CGameDesk::OnMsgUserDoAction CheckDoWaitOperate pPlayer->DoDianGang success ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x]"
					,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction());

				//��ܳɹ���
				pPlayer->ClearEnableAction();

				//�����ҵ������������Ȩ��
				ClearAllPlayerLowQuanXian(dwOutHasOpMaxOneAction);

				//ɾ��������ҳ�����
				PlayerInfo* pLastOutCardPlayer = GetPlayerInfo(m_byLastOutMjSetaNo);
				if ( NULL != pLastOutCardPlayer )
				{
					int iRetRemove = pLastOutCardPlayer->RemoveOneOutCardFromTail(m_byLastOutMj);
					if ( iRetRemove < 0 )
					{
						MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnMsgProcAction_DianGang.. pLastOutCardPlayer->RemoveOneOutCardFromTail() return fail ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d] ,iRet=%d"
							,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,iRet);
					}
				}

				//�齫��ͷ����ʾ
				SetCurMjShowSeat(-1);

				//֪ͨ�ͻ���
				std::vector<BYTE> vecCardList;
				vecCardList.push_back(m_byLastOutMj);
				NotifyClientOperatesSuccess(iDeskStation,EACTION_GANG_DIAN,vecCardList);

				TurnToAimStation(iDeskStation,true,false);
			}
			break;
		default:
			{
				MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction CheckDoWaitOperate not proc..  ..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d]"
					,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo);
			}

		}
	}
	else if ( iHasOpMaxRightIndex < iNotOpMaxRightIndex )
	{
		//û�в����� ���� �Ѿ�������  ��Ҫ�ȴ�
		MYLOG_INFO(logger,"CGameDesk::OnMsgUserDoAction CheckDoWaitOperate need wait...[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d, iNotOpMaxRightIndex=%d,dwOutNotOpMaxOneAction=%0x,iOutNotOpMaxDeskStation=%d   ,iHasOpMaxRightIndex=%d,dwOutHasOpMaxOneAction=%0x,iOutHasOpMaxDeskStation=%d]"
			,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,iNotOpMaxRightIndex,dwOutNotOpMaxOneAction,iOutNotOpMaxDeskStation,iHasOpMaxRightIndex,dwOutHasOpMaxOneAction,iOutHasOpMaxDeskStation);

		//֪ͨ�ȴ�
		if ( iNotifyWatiOpStation >= 0 )
		{
			NotifyClientOperatesFail(iNotifyWatiOpStation,ERR_ACTION_CODE_WAIT_OTHER_OPERATE);
		}
	}

	return 0;
}

//ץ��
int CGameDesk::OnMsgProcAction_ZhuaNiao()
{
	if ( 1 == m_GameEndInfo.byIsLiuJu )
	{
		return 0;
	}

	//��ʼ����λ��
	int iNiaoStartStation = 0;
	for ( int i = 0 ; i < CONFIG_PALYER_COUNT; i++ )
	{
		int iZiMouHuCount   = m_GameEndInfo.userDetail[i].byLsDetailCount[ECSCORE_HU_ZIMO];
		int iDianPaoHuCount = m_GameEndInfo.userDetail[i].byLsDetailCount[ECSCORE_DIAN_PAO];   //����  �ڷ�
		int iJiePaoCount    = m_GameEndInfo.userDetail[i].byLsDetailCount[ECSCORE_JIE_PAO];    //����  ����

		int iBeiQiangGangCount = m_GameEndInfo.userDetail[i].byLsDetailCount[ECSCORE_BEI_QGANG]; //������ �ڷ�
		int iQiangGangHuCount  = m_GameEndInfo.userDetail[i].byLsDetailCount[ECSCORE_QGANG_HU];  //���ܺ� ����

		if ( iZiMouHuCount > 0 )
		{
			iNiaoStartStation = i;
		}
		else if ( iJiePaoCount > 0 || iQiangGangHuCount > 0 ) //����
		{
			iNiaoStartStation = i;
		}
		else if ( iDianPaoHuCount > 1 || iBeiQiangGangCount > 1 ) //����
		{
			iNiaoStartStation = i;
			break;
		}
	}

	m_GameEndInfo.byZhuaNiaoSeatNo = iNiaoStartStation;
	m_GameEndInfo.byZhuaNiaoCount  = 0;
	for ( int i = 0 ; i < std::min(m_iCfgZhuaNiaoCount,MAX_ZHUANIAO_COUNT) ; i++ )
	{
		MYLOG_INFO(logger,"OnMsgProcAction_ZhuaNiao m_iLeftMjCount=%d m_GameEndInfo.byZhuaNiaoCount=%d", m_iLeftMjCount, m_GameEndInfo.byZhuaNiaoCount);	

		int iTouchCount = 1;
		std::list<BYTE> lsOutDeleteMj;
		int iRet = DeleteMJDCard(true, lsOutDeleteMj,iTouchCount,false);
		if ( iRet < 0 || lsOutDeleteMj.size() <= 0 )
		{
			MYLOG_ERROR(logger,"OnMsgProcAction_ZhuaNiao22 m_iLeftMjCount=%d m_GameEndInfo.byZhuaNiaoCount=%d iRet=%d", m_iLeftMjCount, m_GameEndInfo.byZhuaNiaoCount, iRet);
			break;
		}

		m_GameEndInfo.lsZhuaNiao[i].byNiaoMj = lsOutDeleteMj.front();
		m_GameEndInfo.lsZhuaNiao[i].byIsZhongNiao = CMJAlogrithm::IsNiaoCard(m_GameEndInfo.lsZhuaNiao[i].byNiaoMj);

		BYTE byPoint = CMJAlogrithm::GetGameCardPoint(m_GameEndInfo.lsZhuaNiao[i].byNiaoMj);
		m_GameEndInfo.lsZhuaNiao[i].bySeatNo      = (iNiaoStartStation + byPoint - 1)%CONFIG_PALYER_COUNT;

		m_GameEndInfo.byZhuaNiaoCount++;
	}

	int iTotalZhongNiaoCount = 0;
	for ( int i = 0 ; i < m_GameEndInfo.byZhuaNiaoCount ; i++ )
	{
		if ( 1 == m_GameEndInfo.lsZhuaNiao[i].byIsZhongNiao )
		{
			iTotalZhongNiaoCount += 1;
		}
	}

	for ( int iPlayerIndex = 0 ; iPlayerIndex < CONFIG_PALYER_COUNT; iPlayerIndex++ )
	{
		int iZiMouHuCount   = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_HU_ZIMO];
		int iDianPaoHuCount = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_DIAN_PAO];   //����  �ڷ�
		int iJiePaoCount    = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_JIE_PAO];    //����  ����

		int iBeiQiangGangCount = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_BEI_QGANG]; //������ �ڷ�
		int iQiangGangHuCount  = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_QGANG_HU];  //���ܺ� ����

		if ( iZiMouHuCount > 0 || iJiePaoCount > 0 || iQiangGangHuCount > 0 )
		{
			m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_ZHONG_NIAO] = iTotalZhongNiaoCount;
		}
	}

	MYLOG_INFO(logger,"OnMsgProcAction_ZhuaNiao m_GameEndInfo.byZhuaNiaoCount=%d", m_GameEndInfo.byZhuaNiaoCount);

	for (int i=0; i<m_GameEndInfo.byZhuaNiaoCount; i++)
	{
		MYLOG_INFO(logger,"AAA CGameDesk::OnMsgUserDoAction OnMsgProcAction_ZhuaNiao byNiaoMj=%d byIsZhongNiao=%d bySeatNo=%d",
			m_GameEndInfo.lsZhuaNiao[i].byNiaoMj, m_GameEndInfo.lsZhuaNiao[i].byIsZhongNiao, m_GameEndInfo.lsZhuaNiao[i].bySeatNo);
	}

	return 0;
}

//����
int CGameDesk::OnMsgProcAction_JieSuan(bool bLiuJu)
{
	MYLOG_DEBUG(logger,"CGameDesk::OnMsgUserDoAction OnMsgProcAction_JieSuan ..[roomID=%d,DeskNo=%d,bLiuJu=%d]",GetRoomID(),GetTableID(),bLiuJu);

	m_GameEndInfo.byBankerSeatNo   = m_iNtStation;

	if ( bLiuJu )
	{
		m_GameEndInfo.byIsLiuJu = 1;
		SetNextNtStation(m_nFinalMoPaiChair);
	}
	else
	{
		m_GameEndInfo.byIsLiuJu = 0;
	}

	//����
	for ( int iPlayerIndex = 0; iPlayerIndex < CONFIG_PALYER_COUNT ; iPlayerIndex++ )
	{
		PlayerInfo* pPlayer = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pPlayer )
		{
			continue;
		}

		//�ǳ�
		memcpy(m_GameEndInfo.userDetail[iPlayerIndex].szUserNick,pPlayer->GetUserNick(),std::min(GAME_NICK_LEN,CONST_MAX_USER_NAME_LEN));

		//����
		bool bHasHuCard    = pPlayer->GetHasHu();
		int  iHuPaoStation = pPlayer->GetHuPaoStation();
		bool bQiangGangHu  = pPlayer->GetQiangGangHu();
		if ( bHasHuCard )
		{
// 			if ( iHuPaoStation >= 0 && iHuPaoStation < CONFIG_PALYER_COUNT )
// 			{
// 				if ( bQiangGangHu )
// 				{
// 					//���ܺ�
// 					m_GameEndInfo.userDetail[iHuPaoStation].byLsDetailCount[ECSCORE_BEI_QGANG] += 1; //������ 
// 					m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_QGANG_HU]    = 1; //���ܺ� ����һ��
// 				}
// 				else
// 				{
// 					//���ں�
// 					m_GameEndInfo.userDetail[iHuPaoStation].byLsDetailCount[ECSCORE_DIAN_PAO] += 1; //����
// 					m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_JIE_PAO]    = 1; //���� ����һ��
// 				}
// 			}
// 			else
// 			{
// 				//������
// 				m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_HU_ZIMO] = 1;
// 			}

			if ( iHuPaoStation >= 0 && iHuPaoStation < CONFIG_PALYER_COUNT )
			{
				if ( bQiangGangHu )
				{
					//���ܺ�
					m_GameEndInfo.userDetail[iHuPaoStation].byLsDetailCount[ECSCORE_BEI_QGANG] += 1; //������ 
					m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_QGANG_HU]    = 1; //���ܺ� ����һ��
					m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_HU_ZIMO]    = 1; //���ܺ� ����һ�� ������
				}
				else
				{
					//���ں�
					m_GameEndInfo.userDetail[iHuPaoStation].byLsDetailCount[ECSCORE_DIAN_PAO] += 1; //����
					m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_JIE_PAO]    = 1; //���� ����һ��
				}
			}
			else
			{
				//������
				m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_HU_ZIMO] = 1;
			}
		}

		std::vector<MJCPGNode> vecMJCPGList;
		if( pPlayer->GetMJCPGNodeList(vecMJCPGList) > 0 )
		{
			//�����б�
			int iCount = 0;
			for ( int i = 0 ; i < vecMJCPGList.size() ; i++ )
			{
				if ( vecMJCPGList[i].byBlockCardType == BLOCK_KEZI_PENG )
				{
					m_GameEndInfo.userDetail[iPlayerIndex].byPengCardList[iCount] = vecMJCPGList[i].byMjCard[0];
					iCount++;
				}
			}

			//�������б�
			iCount = 0;
			for ( int i = 0 ; i < vecMJCPGList.size() ; i++ )
			{
				if ( vecMJCPGList[i].byBlockCardType == BLOCK_GANG_DIAN || vecMJCPGList[i].byBlockCardType == BLOCK_GANG_BU )
				{
					m_GameEndInfo.userDetail[iPlayerIndex].byMingGangCardList[iCount] = vecMJCPGList[i].byMjCard[0];
					iCount++;

					if ( vecMJCPGList[i].byBlockCardType == BLOCK_GANG_DIAN )
					{
						m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_JIE_GANG]   += 1;

						BYTE byFangGangSeat = vecMJCPGList[i].bySeat;
						m_GameEndInfo.userDetail[byFangGangSeat].byLsDetailCount[ECSCORE_DIAN_GANG]+= 1;
					}
					else
					{
						m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_BU_GANG]   += 1;
					}
				}
			}

			//�������б�
			iCount = 0;
			for ( int i = 0 ; i < vecMJCPGList.size() ; i++ )
			{
				if ( vecMJCPGList[i].byBlockCardType == BLOCK_GANG_AN  )
				{
					m_GameEndInfo.userDetail[iPlayerIndex].byAnGangCardList[iCount] = vecMJCPGList[i].byMjCard[0];
					iCount++;

					m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_AN_GANG]   += 1;
				}
			}

			//�����б�
			iCount = 0;
			for ( int i = 0 ; i < vecMJCPGList.size() ; i++ )
			{
				if ( vecMJCPGList[i].byBlockCardType == BLOCK_SUN_CHI  )
				{
					for ( int k = 0 ; k < 3 ; k++ )
					{
						m_GameEndInfo.userDetail[iPlayerIndex].byChiList[iCount][k] = vecMJCPGList[i].byMjCard[k];
					}
					iCount++;
				}
			}
		}

		//�����齫
		std::list<BYTE> lsOutHandMj;
		if( pPlayer->GetHandMjCard(lsOutHandMj) > 0 )
		{
			//���ں��� �����Ʒ���󣬿ͻ����Լ���
			if (  m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_JIE_PAO] > 0 
			   || m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_QGANG_HU] > 0 )
			{
				lsOutHandMj.push_back(m_byLastOutMj);
			}

			int iTemp = 0;
			std::list<BYTE>::iterator it = lsOutHandMj.begin();
			for ( ; it != lsOutHandMj.end() && iTemp < HAND_MJ_MAX ; it++ ,iTemp++)
			{
				m_GameEndInfo.userDetail[iPlayerIndex].byHandMjs[iTemp] = (*it);
			}
		}
	}

	//ץ��
	OnMsgProcAction_ZhuaNiao();

	int nTax = m_pGameManager->GetRoomInfo(m_nRoomID)->unTax;
	if (1 == m_pGameManager->IsRoomCardPlaform())
	{
		if (IsGoldRoom())
		{
			nTax = m_nTax;
		}
		else
		{
			nTax = 0;
		}
	}

	if (nTax > 0)
	{
		//�ȿ�˰
		for (int i=0; i<CONFIG_PALYER_COUNT; i++)
		{
			PlayerInfo* pPlayer = GetPlayerInfo(i);
			if ( NULL == pPlayer )
			{
				continue;
			}

			pPlayer->AddUserMoney(-nTax);

			// ��д��Ϸ����
			m_pGameManager->ModifyGameMoney(GAME_ID, m_nRoomID, m_nDeskIndex, pPlayer->GetUserID(), E_CHAGEMONEY_GAME_REMOVE, m_nTax);
			MYLOG_INFO(logger, "OnMsgProcAction_JieSuan tax=%d i=%d Money=%ld", m_nTax, i, pPlayer->GetUserMoney());
		}
	}

	//�������
	CardRoomScoreStatement CardRoomInfo;
	CalcGameEndScore(CardRoomInfo);

	for (int i=0; i<m_GameEndInfo.byZhuaNiaoCount; i++)
	{
		MYLOG_INFO(logger,"BBB CGameDesk::OnMsgUserDoAction OnMsgProcAction_ZhuaNiao byNiaoMj=%d byIsZhongNiao=%d bySeatNo=%d",
			m_GameEndInfo.lsZhuaNiao[i].byNiaoMj, m_GameEndInfo.lsZhuaNiao[i].byIsZhongNiao, m_GameEndInfo.lsZhuaNiao[i].bySeatNo);
	}

	//////////////////////////////////////////////////////////////////////////
	//����
	CMD_S_GameEnd   SendGameEndInfo;
	memcpy(&SendGameEndInfo,&m_GameEndInfo,sizeof(m_GameEndInfo));
	for ( int i = 0 ; i < CONFIG_PALYER_COUNT; i++ )
	{
		//��ӡ��־

		MYLOG_INFO(logger,"CGameDesk::OnMsgUserDoAction OnMsgProcAction_JieSuan ..[roomID=%d,DeskNo=%d,bLiuJu=%d,iDeskStation=%d, ECSCORE_HU_ZIMO=%d ,ECSCORE_DIAN_PAO=%d ,ECSCORE_JIE_PAO=%d ,ECSCORE_DIAN_GANG=%d , ECSCORE_JIE_GANG=%d, ECSCORE_AN_GANG=%d , ECSCORE_BU_GANG=%d , ECSCORE_ZHONG_NIAO =%d , ECSCORE_BEI_QGANG=%d, ECSCORE_QGANG_HU = %d , iCurTotalMoney=%lld , m_iNtStation=%d]",GetRoomID(),GetTableID(),bLiuJu,i
			,SendGameEndInfo.userDetail[i].byLsDetailCount[0],SendGameEndInfo.userDetail[i].byLsDetailCount[1],SendGameEndInfo.userDetail[i].byLsDetailCount[2],SendGameEndInfo.userDetail[i].byLsDetailCount[3]
		,SendGameEndInfo.userDetail[i].byLsDetailCount[4],SendGameEndInfo.userDetail[i].byLsDetailCount[5],SendGameEndInfo.userDetail[i].byLsDetailCount[6],SendGameEndInfo.userDetail[i].byLsDetailCount[7] 
		,SendGameEndInfo.userDetail[i].byLsDetailCount[8],SendGameEndInfo.userDetail[i].byLsDetailCount[9], SendGameEndInfo.userDetail[i].iCurTotalMoney,m_iNtStation);

		SendGameEndInfo.userDetail[i].iCurTotalPan   = ntoh32(SendGameEndInfo.userDetail[i].iCurTotalPan);
		SendGameEndInfo.userDetail[i].iCurTotalMoney = ntoh64(SendGameEndInfo.userDetail[i].iCurTotalMoney);
		SendGameEndInfo.userDetail[i].iCurHuMoney	 = ntoh64(SendGameEndInfo.userDetail[i].iCurHuMoney);
		SendGameEndInfo.userDetail[i].iCurGangMoney  = ntoh64(SendGameEndInfo.userDetail[i].iCurGangMoney);
		SendGameEndInfo.userDetail[i].iCurNiaoMoney  = ntoh64(SendGameEndInfo.userDetail[i].iCurNiaoMoney);
		SendGameEndInfo.userDetail[i].iTotalMoney    = ntoh64(SendGameEndInfo.userDetail[i].iTotalMoney);
		SendGameEndInfo.userDetail[i].iTax           = ntoh32(SendGameEndInfo.userDetail[i].iTax);
		SendGameEndInfo.userDetail[i].dwHuExtendsType= ntoh32(SendGameEndInfo.userDetail[i].dwHuExtendsType);
	}

	SendDeskBroadCast(SUB_S_GAME_END,(BYTE*)&SendGameEndInfo, sizeof(SendGameEndInfo));

	//��������
	m_nGameStation = GS_WAIT_NEXT;
	ReSetGameState(GF_NORMAL);


	//��ϷΪ����������ʱ
	if (m_pGameManager->IsRoomCardPlaform() == 1)
	{
		//����
		m_pGameManager->WriteBackDeskRealtimeData((void*)&CardRoomInfo, E_ALGORITHM_TYPE_CARD_ROOM_SCORE);
	}

	//����
	m_pGameManager->OnGameFinish(GetRoomID(),m_nDeskIndex);
	////��ʱ����뿪
	//SetGameTimer(TIME_CHECK_END_MONEY,2000);

	//׼������ʱ
	for (int player=0; player<CONFIG_PALYER_COUNT; player++)
	{
		KillGameTimer(TIME_PLAYER_1+player);
		SetGameTimer(TIME_PLAYER_1+player, m_nReadyTime*1000);
	}

	return 0;
}

//�������
void CGameDesk::CalcGameEndScore(CardRoomScoreStatement& CardRoomInfo)
{
	memset(m_arrSaveDetailScore,0,sizeof(m_arrSaveDetailScore));
	memset(m_arrSaveDetailCount,0,sizeof(m_arrSaveDetailCount));

	//���ҷ���λ��
	int iFangPaoStation = -1;
	for ( int iPlayerIndex = 0; iPlayerIndex < CONFIG_PALYER_COUNT ; iPlayerIndex++ )
	{
		int iDianPaoHuCount    = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_DIAN_PAO];  //����
		int iBeiQiangGangCount = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_BEI_QGANG]; //������ �ڷ�
		if ( iDianPaoHuCount > 0 || iBeiQiangGangCount > 0 )
		{
			iFangPaoStation = iPlayerIndex;
		}
	}

	//ͳ����Ӯ��ϸ
	for ( int iPlayerIndex = 0; iPlayerIndex < CONFIG_PALYER_COUNT ; iPlayerIndex++ )
	{
		PlayerInfo* pPlayer = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pPlayer )
		{
			MYLOG_ERROR(logger,"CalcGameEndScore iPlayerIndex=%d Player=NULL");
			continue;
		}

		int iZiMouHuCount   = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_HU_ZIMO];  //����
		int iDianPaoHuCount = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_DIAN_PAO]; //����
		int iJiePaoCount    = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_JIE_PAO];  //����

		int iDianGangCount  = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_DIAN_GANG];//���
		int iJieGangCount   = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_JIE_GANG]; //�Ӹ�

		int iAnGangCount    = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_AN_GANG];  //����
		int iBuGangCount    = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_BU_GANG];  //����

		int iZhongNiaoCount = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_ZHONG_NIAO];  //����

		int iBeiQiangGangCount = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_BEI_QGANG]; //������ �ڷ�
		int iQiangGangHuCount  = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_QGANG_HU];  //���ܺ� ����

		int iEveryNormalSubScore_ZiMo = 2;      //������ͨ
		int iEveryNtXianSubScore_ZiMo = 3;      //����ׯ��
		int iEveryNormalSubScore_DianPao = 1/*CONFIG_PALYER_COUNT - 1*/;   //������ͨ
		int iEveryNtXianSubScore_DianPao = 2/*CONFIG_PALYER_COUNT*/;       //����ׯ��

		//����
		if ( iZiMouHuCount > 0 )
		{
			//����������û�к��У������һ����
			int nHongZhongCount = pPlayer->GetHongZhongCountFromHandCards();
			if (0 == nHongZhongCount)
			{
				iZhongNiaoCount += m_byAddNiaoCount;
				m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_ZHONG_NIAO]+=m_byAddNiaoCount;
			}
			MYLOG_INFO(logger,"CalcGameEndScore iPlayerIndex=%d nHongZhongCount=%d iZhongNiaoCount=%d", iPlayerIndex, nHongZhongCount, iZhongNiaoCount);

			m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_ZIMO] = 1;

			//���ܺ��������������ܵ����Ҫ������
			if (iQiangGangHuCount > 0)
			{
				m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_QIANG_GANG] = 1;

				//���ұ����ܵ����
				int nBeiQiangGang = -1;
				for (int bqgang=0; bqgang<CONFIG_PALYER_COUNT; bqgang++)
				{
					PlayerInfo* pQGPlayer = GetPlayerInfo(iPlayerIndex);
					if ( NULL == pQGPlayer )
					{
						continue;
					}
					if (m_GameEndInfo.userDetail[bqgang].byLsDetailCount[ECSCORE_BEI_QGANG] > 0)
					{
						nBeiQiangGang = bqgang;
						break;
					}
				}
				if (-1 == nBeiQiangGang)
				{
					MYLOG_ERROR(logger, "-1 == nBeiQiangGang !!!!!!");
				}
				else
				{
					//�б�������ң���ô��������� Ҫȫ�� ������ķ�
					INT64 n64ChangeScoreTemp = 0;//(CONFIG_PALYER_COUNT-1);
					//ׯ�з�
					if ( m_bCfgZhuangXianScore )
					{
						if ( m_iNtStation == iPlayerIndex )//|| m_iNtStation == nBeiQiangGang )
						{
							//ׯ�����ܺ�
							n64ChangeScoreTemp = (CONFIG_PALYER_COUNT-1) * iEveryNtXianSubScore_ZiMo;
						}
						else
						{
							//�м����ܺ�
							n64ChangeScoreTemp = iEveryNtXianSubScore_ZiMo + (CONFIG_PALYER_COUNT-2) * iEveryNormalSubScore_ZiMo;
						}
					}
					else
					{
						n64ChangeScoreTemp = (CONFIG_PALYER_COUNT-1);
						n64ChangeScoreTemp *= iEveryNormalSubScore_ZiMo;
					}

					m_arrSaveDetailCount[nBeiQiangGang][E_CARD_GAME_SCORE_BEI_QGANG] += 1;

					m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney       += n64ChangeScoreTemp;
					m_GameEndInfo.userDetail[iPlayerIndex].iCurHuMoney          += n64ChangeScoreTemp;

					m_GameEndInfo.userDetail[nBeiQiangGang].iCurTotalMoney        -= n64ChangeScoreTemp;
					m_GameEndInfo.userDetail[nBeiQiangGang].iCurHuMoney           -= n64ChangeScoreTemp;

					m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZIMO]  += n64ChangeScoreTemp;
					m_arrSaveDetailScore[nBeiQiangGang][E_CARD_GAME_SCORE_BEI_QGANG] -= n64ChangeScoreTemp;

					MYLOG_INFO(logger,"CGameDesk::OnMsgUserDoAction CalcGameEndScore QiangHangHu[roomID=%d,DeskNo=%d,iHuPlayerIndex=%d,iFengHuSeat=%d,iChangeScoreTemp=%lld]",
						GetRoomID(),GetTableID(),iPlayerIndex,nBeiQiangGang,n64ChangeScoreTemp);
				}
			}
			//�����ܺ����
			else
			{
				for ( int iTempIndex = 0 ; iTempIndex < CONFIG_PALYER_COUNT ; iTempIndex++ )
				{
					if ( iTempIndex == iPlayerIndex )
					{
						continue;
					}

					m_arrSaveDetailCount[iTempIndex][E_CARD_GAME_SCORE_BEI_MO] += 1;

					if ( m_bCfgZhuangXianScore )
					{
						if ( m_iNtStation == iPlayerIndex || m_iNtStation == iTempIndex )
						{
							//ׯ������
							m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney += iEveryNtXianSubScore_ZiMo;
							m_GameEndInfo.userDetail[iPlayerIndex].iCurHuMoney += iEveryNtXianSubScore_ZiMo;

							m_GameEndInfo.userDetail[iTempIndex].iCurTotalMoney   -= iEveryNtXianSubScore_ZiMo;
							m_GameEndInfo.userDetail[iTempIndex].iCurHuMoney   -= iEveryNtXianSubScore_ZiMo;

							m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZIMO] += iEveryNtXianSubScore_ZiMo;
							m_arrSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_MO] -= iEveryNtXianSubScore_ZiMo;
						}
						else
						{
							m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney += iEveryNormalSubScore_ZiMo;
							m_GameEndInfo.userDetail[iPlayerIndex].iCurHuMoney += iEveryNormalSubScore_ZiMo;

							m_GameEndInfo.userDetail[iTempIndex].iCurTotalMoney   -= iEveryNormalSubScore_ZiMo;
							m_GameEndInfo.userDetail[iTempIndex].iCurHuMoney   -= iEveryNormalSubScore_ZiMo;

							m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZIMO] += iEveryNormalSubScore_ZiMo;
							m_arrSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_MO] -= iEveryNormalSubScore_ZiMo;
						}
					}
					else
					{
						m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney += iEveryNormalSubScore_ZiMo;
						m_GameEndInfo.userDetail[iPlayerIndex].iCurHuMoney   -= iEveryNormalSubScore_ZiMo;

						m_GameEndInfo.userDetail[iTempIndex].iCurTotalMoney   -= iEveryNormalSubScore_ZiMo;
						m_GameEndInfo.userDetail[iTempIndex].iCurHuMoney   -= iEveryNormalSubScore_ZiMo;

						m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZIMO] += iEveryNormalSubScore_ZiMo;
						m_arrSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_MO] -= iEveryNormalSubScore_ZiMo;
					}
				}
			}

			m_GameEndInfo.userDetail[iPlayerIndex].dwHuExtendsType    = pPlayer->GetHuExtendsType();
		}

		//���
		if ( iDianGangCount > 0 )
		{
			m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_DIAN_GANG] += iDianGangCount;
			m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_DIAN_GANG] -= iDianGangCount*(CONFIG_PALYER_COUNT-1);

			m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney           -= iDianGangCount*(CONFIG_PALYER_COUNT-1);  //���
			m_GameEndInfo.userDetail[iPlayerIndex].iCurGangMoney           -= iDianGangCount*(CONFIG_PALYER_COUNT-1);  //���
		}

		//�Ӹ�
		if ( iJieGangCount > 0 )
		{
			m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_JIE_GANG] += iJieGangCount;
			m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_JIE_GANG] += iJieGangCount*(CONFIG_PALYER_COUNT-1);

			m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney          += iJieGangCount*(CONFIG_PALYER_COUNT-1);  //�Ӹ�
			m_GameEndInfo.userDetail[iPlayerIndex].iCurGangMoney          += iJieGangCount*(CONFIG_PALYER_COUNT-1);  //�Ӹ�
		}

		//����
		int iEveryAnGangSubCount = 2;
		if ( iAnGangCount > 0 )
		{
			m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_AN_GANG] += iAnGangCount;

			for ( int iTempIndex = 0 ; iTempIndex < CONFIG_PALYER_COUNT ; iTempIndex++ )
			{
				if ( iTempIndex == iPlayerIndex )
				{
					continue;
				}

				m_arrSaveDetailCount[iTempIndex][E_CARD_GAME_SCORE_BEI_AN_GANG] += iAnGangCount;

				m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney += iAnGangCount*iEveryAnGangSubCount;
				m_GameEndInfo.userDetail[iPlayerIndex].iCurGangMoney += iAnGangCount*iEveryAnGangSubCount;

				m_GameEndInfo.userDetail[iTempIndex].iCurTotalMoney   -= iAnGangCount*iEveryAnGangSubCount;
				m_GameEndInfo.userDetail[iTempIndex].iCurGangMoney   -= iAnGangCount*iEveryAnGangSubCount;

				m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_AN_GANG]   += iAnGangCount*iEveryAnGangSubCount;
				m_arrSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_AN_GANG] -= iAnGangCount*iEveryAnGangSubCount;
			}
		}

		//����
		int iEveryBuGangSubCount = 1;
		if ( iBuGangCount > 0 )
		{
			m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_BU_GANG] += iBuGangCount;

			for ( int iTempIndex = 0 ; iTempIndex < CONFIG_PALYER_COUNT ; iTempIndex++ )
			{
				if ( iTempIndex == iPlayerIndex )
				{
					continue;
				}

				m_arrSaveDetailCount[iTempIndex][E_CARD_GAME_SCORE_BEI_BU_GANG] += iBuGangCount;

				m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney += iBuGangCount*iEveryBuGangSubCount;
				m_GameEndInfo.userDetail[iPlayerIndex].iCurGangMoney += iBuGangCount*iEveryBuGangSubCount;

				m_GameEndInfo.userDetail[iTempIndex].iCurTotalMoney   -= iBuGangCount*iEveryBuGangSubCount;
				m_GameEndInfo.userDetail[iTempIndex].iCurGangMoney   -= iBuGangCount*iEveryBuGangSubCount;

				m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_BU_GANG]   += iBuGangCount*iEveryBuGangSubCount;
				m_arrSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_BU_GANG] -= iBuGangCount*iEveryBuGangSubCount;
			}
		}

		//����
		int iEveryNiaoSubCount = 2;
		if ( iZhongNiaoCount > 0 )
		{
			if ( iZiMouHuCount > 0 )
			{
				//���ܺ��������������ܵ����Ҫ�����ҵ����
				if (iQiangGangHuCount > 0)
				{
					m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_QIANG_GANG] = 1;

					//���ұ����ܵ����
					int nBeiQiangGang = -1;
					for (int bqgang=0; bqgang<CONFIG_PALYER_COUNT; bqgang++)
					{
						PlayerInfo* pQGPlayer = GetPlayerInfo(iPlayerIndex);
						if ( NULL == pQGPlayer )
						{
							continue;
						}
						if (m_GameEndInfo.userDetail[bqgang].byLsDetailCount[ECSCORE_BEI_QGANG] > 0)
						{
							nBeiQiangGang = bqgang;
							break;
						}
					}
					if (-1 == nBeiQiangGang)
					{
						MYLOG_ERROR(logger, "-1 == nBeiQiangGang !!!!!!");
					}
					else
					{
						m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO] += iZhongNiaoCount;
						m_arrSaveDetailCount[nBeiQiangGang][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] += iZhongNiaoCount;

						INT64 n64NiaoScore = iZhongNiaoCount*iEveryNiaoSubCount*(CONFIG_PALYER_COUNT-1);

						m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney += n64NiaoScore;
						m_GameEndInfo.userDetail[iPlayerIndex].iCurNiaoMoney += n64NiaoScore;

						m_GameEndInfo.userDetail[nBeiQiangGang].iCurTotalMoney   -= n64NiaoScore;
						m_GameEndInfo.userDetail[nBeiQiangGang].iCurNiaoMoney   -= n64NiaoScore;

						m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO]   += n64NiaoScore;
						m_arrSaveDetailScore[nBeiQiangGang][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] -= n64NiaoScore;

						MYLOG_INFO(logger,"CGameDesk::OnMsgUserDoAction CalcGameEndScore QiangHangHu[roomID=%d,DeskNo=%d,iPlayerIndex=%d,nBeiQiangGang=%d,n64NiaoScore=%lld]",
							GetRoomID(),GetTableID(),iPlayerIndex,nBeiQiangGang,n64NiaoScore);
					}
				}
				//�����ܺ����
				else
				{
					//������
					m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO] += iZhongNiaoCount;

					for ( int iTempIndex = 0 ; iTempIndex < CONFIG_PALYER_COUNT ; iTempIndex++ )
					{
						if ( iTempIndex == iPlayerIndex )
						{
							continue;
						}

						m_arrSaveDetailCount[iTempIndex][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] += iZhongNiaoCount;

						m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney += iZhongNiaoCount*iEveryNiaoSubCount;
						m_GameEndInfo.userDetail[iPlayerIndex].iCurNiaoMoney += iZhongNiaoCount*iEveryNiaoSubCount;

						m_GameEndInfo.userDetail[iTempIndex].iCurTotalMoney   -= iZhongNiaoCount*iEveryNiaoSubCount;
						m_GameEndInfo.userDetail[iTempIndex].iCurNiaoMoney   -= iZhongNiaoCount*iEveryNiaoSubCount;

						m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO]   += iZhongNiaoCount*iEveryNiaoSubCount;
						m_arrSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] -= iZhongNiaoCount*iEveryNiaoSubCount;
					}
				}
			}
			//�޵��ں�
// 			else
// 			{
// 				//���ں� 
// 				if ( ( iJiePaoCount > 0 || iQiangGangHuCount > 0 ) && iFangPaoStation >= 0 )
// 				{
// 					m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO]        += iZhongNiaoCount;
// 					m_arrSaveDetailCount[iFangPaoStation][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] += iZhongNiaoCount;
// 
// 					m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO]        += iZhongNiaoCount*iEveryNiaoSubCount;
// 					m_arrSaveDetailScore[iFangPaoStation][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] -= iZhongNiaoCount*iEveryNiaoSubCount;
// 
// 					m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney                   += iZhongNiaoCount*iEveryNiaoSubCount;   //����
// 					m_GameEndInfo.userDetail[iFangPaoStation].iCurTotalMoney                -= iZhongNiaoCount*iEveryNiaoSubCount;   //����
// 				}
// 			}
		}

		MYLOG_DEBUG(logger, "CalcGameEndScore ChairID=%d UserID=%d HuScore=%ld GangScore=%ld NiaoScore=%ld Total=%ld",
			iPlayerIndex, pPlayer->GetUserID(), m_GameEndInfo.userDetail[iPlayerIndex].iCurHuMoney, m_GameEndInfo.userDetail[iPlayerIndex].iCurGangMoney, 
			m_GameEndInfo.userDetail[iPlayerIndex].iCurNiaoMoney, m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney);
	}

	//��ҳ�������ҽ�Ҳ��������
	if (IsGoldRoom())
	{
		bool bNeedReCalcScore = false;
		double lfPercent[GAME_PLAYER] = {1,1,1,1};

		for ( int iPlayer = 0 ; iPlayer < CONFIG_PALYER_COUNT ; iPlayer++ )
		{
			for ( int iCalcIndex = E_CARD_GAME_SCORE_NONE ; iCalcIndex < E_CARD_GAME_SCORE_TYPE_MAX ; iCalcIndex++ )
			{
				m_arrSaveDetailScore[iPlayer][iCalcIndex] *= m_llBaseMoney;
			}

			m_GameEndInfo.userDetail[iPlayer].iCurTotalMoney *= m_llBaseMoney;
			m_GameEndInfo.userDetail[iPlayer].iCurGangMoney *= m_llBaseMoney;
			m_GameEndInfo.userDetail[iPlayer].iCurHuMoney *= m_llBaseMoney;
			m_GameEndInfo.userDetail[iPlayer].iCurNiaoMoney *= m_llBaseMoney;
		}

		for (int i=0; i<CONFIG_PALYER_COUNT; i++)
		{
			PlayerInfo* pPlayer = GetPlayerInfo(i);
			if ( NULL == pPlayer )
			{
				continue;
			}
			double lf64GameScore = m_GameEndInfo.userDetail[i].iCurTotalMoney;
			if (pPlayer->GetUserMoney() + lf64GameScore < 0 )
			{
				bNeedReCalcScore = true;
				lfPercent[i] = pPlayer->GetUserMoney() / (-lf64GameScore);
			}
			MYLOG_DEBUG(logger, "CalcGameEndScore playerMoney=%ld, score=%f, percent=%f", pPlayer->GetUserMoney(),  lf64GameScore, lfPercent[i]);
		}

		if (bNeedReCalcScore)
		{
			for ( int iPlayer = 0 ; iPlayer < CONFIG_PALYER_COUNT ; iPlayer++ )
			{
				lfPercent[iPlayer] *= m_llBaseMoney;
			}

			//���������¼������
			ReCalcGameEndScore(lfPercent);
		}
	}

	//����������ʱ���ݻ�д
	CardRoomInfo.m_nRoomID = m_nRoomID;
	CardRoomInfo.m_nDeskID = m_nDeskIndex;

	//���� �� д���ݿ�
	for ( int iPlayerIndex = 0; iPlayerIndex < CONFIG_PALYER_COUNT ; iPlayerIndex++ )
	{
		PlayerInfo* pPlayer = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pPlayer )
		{
			continue;
		}

		//��ϷΪ����������ʱ
		if (m_pGameManager->IsRoomCardPlaform() == 1)
		{
			long long iTotalCalcChangeMoney = 0;
			for ( int iCalcIndex = E_CARD_GAME_SCORE_ZIMO ; iCalcIndex < E_CARD_GAME_SCORE_TYPE_MAX ; iCalcIndex++ )
			{
				if ( m_arrSaveDetailCount[iPlayerIndex][iCalcIndex] <= 0 )
				{
					continue;
				}

				iTotalCalcChangeMoney += m_arrSaveDetailScore[iPlayerIndex][iCalcIndex];

				CardRoomScoreNode ScoreNode;
				ScoreNode.m_unUin         = pPlayer->GetUserID();
				ScoreNode.m_nStaition     = pPlayer->GetDeskStation();
				ScoreNode.m_nType         = iCalcIndex;
				ScoreNode.m_nNum          = m_arrSaveDetailCount[iPlayerIndex][iCalcIndex];
				ScoreNode.m_llChangeScore = m_arrSaveDetailScore[iPlayerIndex][iCalcIndex];
				ScoreNode.m_llCurrentScore = pPlayer->GetUserMoney() + iTotalCalcChangeMoney;
				//MYLOG_DEBUG(logger, "jimmy ScoreNode.m_unUin=%d,ScoreNode.m_nStaition=%d,ScoreNode.m_llChangeScore=%lld,ScoreNode.m_llCurrentScore=%lld ,m_arUserInfo[i]._nPoint=%d",ScoreNode.m_unUin,ScoreNode.m_nStaition,ScoreNode.m_llChangeScore,ScoreNode.m_llCurrentScore,m_arUserInfo[i]._nPoint);
				CardRoomInfo.m_vctUserScoreList.push_back(ScoreNode);
			}

			if ( CardRoomInfo.m_vctUserScoreList.size() == 0 )
			{
				CardRoomScoreNode ScoreNode;
				ScoreNode.m_unUin         = pPlayer->GetUserID();
				ScoreNode.m_nStaition     = pPlayer->GetDeskStation();
				ScoreNode.m_nType         = 0;
				ScoreNode.m_nNum          = 0;
				ScoreNode.m_llChangeScore = 0;
				ScoreNode.m_llCurrentScore = pPlayer->GetUserMoney();
				CardRoomInfo.m_vctUserScoreList.push_back(ScoreNode);
			}

			m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney = iTotalCalcChangeMoney;


			//������
			m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalPan = abs(m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney);

			//˰��
			m_GameEndInfo.userDetail[iPlayerIndex].iTax         = 0;

			//�ӵ�����
			int iAddUserMoney = m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney - m_GameEndInfo.userDetail[iPlayerIndex].iTax;
			pPlayer->AddUserMoney(iAddUserMoney);

			m_GameEndInfo.userDetail[iPlayerIndex].iTotalMoney  = pPlayer->GetUserMoney();
		}
		else
		{
			//������
			m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalPan = abs(m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney);

			//˰��
			m_GameEndInfo.userDetail[iPlayerIndex].iTax         = 0;

			//�ӵ�����
			int iAddUserMoney = m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney - m_GameEndInfo.userDetail[iPlayerIndex].iTax;
			pPlayer->AddUserMoney(iAddUserMoney);

			m_GameEndInfo.userDetail[iPlayerIndex].iTotalMoney  = pPlayer->GetUserMoney();


			//�����˲�����
			if ( !pPlayer->IsBeRobot() )
			{
				long long n64MoneyChange = m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney - m_GameEndInfo.userDetail[iPlayerIndex].iTax; 

				MYLOG_DEBUG(logger, "CGameDesk::CalcGameEndScore end jiesuan : [room = %d,desk = %d , user = %d, %d ,  n64MoneyChange = %lld] , iCurTotalMoney =%lld , m_uTax=%d,UserMoney=%lld"
					,GetRoomID(),GetTableID(),iPlayerIndex,pPlayer->GetUserID(),n64MoneyChange,m_GameEndInfo.userDetail[iPlayerIndex].iCurTotalMoney,m_GameEndInfo.userDetail[iPlayerIndex].iTax,pPlayer->GetUserMoney());

				// �仯������
				unsigned int unChageType = E_CHAGEMONEY_GAME_ADD;
				if (n64MoneyChange < 0)
				{
					n64MoneyChange= -n64MoneyChange;
					unChageType = E_CHAGEMONEY_GAME_REMOVE;
				}
				m_pGameManager->ModifyGameMoney(GAME_ID, GetRoomID(), GetTableID(), pPlayer->GetUserID() , unChageType, n64MoneyChange);

				//MYLOG_DEBUG(logger, "CGameDesk::CalcGameEndScore end jiesuan: [room = %d,desk = %d , user = %d, %d ,  n64MoneyChange = %lld]",GetRoomID(),GetTableID(),i,m_arUserInfo[i]._dwUserID,n64MoneyChange);
			}
		}

	}

	for (int i=0; i<CONFIG_PALYER_COUNT; i++)
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if ( NULL == pPlayer )
		{
			continue;
		}
		MYLOG_DEBUG(logger, "CalcGameEndScore Finish i=%d playerMoney=%ld m_GameEndInfo.userDetail[i].iTotalMoney=%ld", 
			i, pPlayer->GetUserMoney(), m_GameEndInfo.userDetail[i].iTotalMoney)
	}
}

int CGameDesk::ReCalcGameEndScore(double lfPercent[])
{
	MYLOG_DEBUG(logger, "ReCalcGameEndScore lfPercent =  %lf %lf %lf %lf", lfPercent[0], lfPercent[1], lfPercent[2], lfPercent[3]);

	float arrFSaveDetailScore[GAME_PLAYER][E_CARD_GAME_SCORE_TYPE_MAX];  //�������Ͷ�Ӧ�ķ���,�±�ο� E_CARD_GAME_SCORE_TYPE
	memset(arrFSaveDetailScore,0,sizeof(arrFSaveDetailScore));

	stOneUserEndInfo_double userEndInfo_lf[GAME_PLAYER];
	memset(userEndInfo_lf, 0, sizeof(userEndInfo_lf));

	//���ҷ���λ��
	int iFangPaoStation = -1;
	for ( int iPlayerIndex = 0; iPlayerIndex < CONFIG_PALYER_COUNT ; iPlayerIndex++ )
	{
		int iDianPaoHuCount    = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_DIAN_PAO];  //����
		int iBeiQiangGangCount = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_BEI_QGANG]; //������ �ڷ�
		if ( iDianPaoHuCount > 0 || iBeiQiangGangCount > 0 )
		{
			iFangPaoStation = iPlayerIndex;
		}
	}

	//ͳ����Ӯ��ϸ
	for ( int iPlayerIndex = 0; iPlayerIndex < CONFIG_PALYER_COUNT ; iPlayerIndex++ )
	{
		PlayerInfo* pPlayer = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pPlayer )
		{
			MYLOG_ERROR(logger,"ReCalcGameEndScore iPlayerIndex=%d Player=NULL");
			continue;
		}

		int iZiMouHuCount   = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_HU_ZIMO];  //����
		int iDianPaoHuCount = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_DIAN_PAO]; //����
		int iJiePaoCount    = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_JIE_PAO];  //����

		int iDianGangCount  = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_DIAN_GANG];//���
		int iJieGangCount   = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_JIE_GANG]; //�Ӹ�

		int iAnGangCount    = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_AN_GANG];  //����
		int iBuGangCount    = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_BU_GANG];  //����

		int iZhongNiaoCount = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_ZHONG_NIAO];  //����

		int iBeiQiangGangCount = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_BEI_QGANG]; //������ �ڷ�
		int iQiangGangHuCount  = m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_QGANG_HU];  //���ܺ� ����

		int iEveryNormalSubScore_ZiMo = 2;      //������ͨ
		int iEveryNtXianSubScore_ZiMo = 3;      //����ׯ��
		int iEveryNormalSubScore_DianPao = 1/*CONFIG_PALYER_COUNT - 1*/;   //������ͨ
		int iEveryNtXianSubScore_DianPao = 2/*CONFIG_PALYER_COUNT*/;       //����ׯ��

		//����
		if ( iZiMouHuCount > 0 )
		{
			//����������û�к��У������һ����
			//int nHongZhongCount = pPlayer->GetHongZhongCountFromHandCards();
			//if (0 == nHongZhongCount)
			//{
			//	iZhongNiaoCount += m_byAddNiaoCount;
				//m_GameEndInfo.userDetail[iPlayerIndex].byLsDetailCount[ECSCORE_ZHONG_NIAO]+=m_byAddNiaoCount;
			//}
			//MYLOG_INFO(logger,"ReCalcGameEndScore iPlayerIndex=%d nHongZhongCount=%d", iPlayerIndex, nHongZhongCount);

			//m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_ZIMO] = 1;

			//���ܺ��������������ܵ����Ҫ������
			if (iQiangGangHuCount > 0)
			{
				//m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_QIANG_GANG] = 1;

				//���ұ����ܵ����
				int nBeiQiangGang = -1;
				for (int bqgang=0; bqgang<CONFIG_PALYER_COUNT; bqgang++)
				{
					PlayerInfo* pQGPlayer = GetPlayerInfo(iPlayerIndex);
					if ( NULL == pQGPlayer )
					{
						continue;
					}
					if (m_GameEndInfo.userDetail[bqgang].byLsDetailCount[ECSCORE_BEI_QGANG] > 0)
					{
						nBeiQiangGang = bqgang;
						break;
					}
				}
				if (-1 == nBeiQiangGang)
				{
					MYLOG_ERROR(logger, "-1 == nBeiQiangGang !!!!!!");
				}
				else
				{
					//�б�������ң���ô��������� Ҫȫ�� ������ķ�
					INT64 n64ChangeScoreTemp = 0;//(CONFIG_PALYER_COUNT-1);
					//ׯ�з�
					if ( m_bCfgZhuangXianScore )
					{
						if ( m_iNtStation == iPlayerIndex )//|| m_iNtStation == nBeiQiangGang )
						{
							//ׯ�����ܺ�
							n64ChangeScoreTemp = (CONFIG_PALYER_COUNT-1) * iEveryNtXianSubScore_ZiMo;
						}
						else
						{
							//�м����ܺ�
							n64ChangeScoreTemp = iEveryNtXianSubScore_ZiMo + (CONFIG_PALYER_COUNT-2) * iEveryNormalSubScore_ZiMo;
						}
					}
					else
					{
						n64ChangeScoreTemp = (CONFIG_PALYER_COUNT-1);
						n64ChangeScoreTemp *= iEveryNormalSubScore_ZiMo;
					}

					//m_arrSaveDetailCount[nBeiQiangGang][E_CARD_GAME_SCORE_BEI_QGANG] += 1;

					userEndInfo_lf[iPlayerIndex].lfCurTotalMoney       += n64ChangeScoreTemp * lfPercent[nBeiQiangGang];
					userEndInfo_lf[iPlayerIndex].lfCurHuMoney          += n64ChangeScoreTemp * lfPercent[nBeiQiangGang];

					userEndInfo_lf[nBeiQiangGang].lfCurTotalMoney        -= n64ChangeScoreTemp * lfPercent[nBeiQiangGang];
					userEndInfo_lf[nBeiQiangGang].lfCurHuMoney           -= n64ChangeScoreTemp * lfPercent[nBeiQiangGang];

					arrFSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZIMO]  += n64ChangeScoreTemp * lfPercent[nBeiQiangGang];
					arrFSaveDetailScore[nBeiQiangGang][E_CARD_GAME_SCORE_BEI_QGANG] -= n64ChangeScoreTemp * lfPercent[nBeiQiangGang];

					MYLOG_INFO(logger,"ReCalcGameEndScore QiangHangHu[roomID=%d,DeskNo=%d,iHuPlayerIndex=%d,iFengHuSeat=%d,iChangeScoreTemp=%lld]",
						GetRoomID(),GetTableID(),iPlayerIndex,nBeiQiangGang,n64ChangeScoreTemp);
				}
			}
			//�����ܺ����
			else
			{
				for ( int iTempIndex = 0 ; iTempIndex < CONFIG_PALYER_COUNT ; iTempIndex++ )
				{
					if ( iTempIndex == iPlayerIndex )
					{
						continue;
					}

					//m_arrSaveDetailCount[iTempIndex][E_CARD_GAME_SCORE_BEI_MO] += 1;

					if ( m_bCfgZhuangXianScore )
					{
						if ( m_iNtStation == iPlayerIndex || m_iNtStation == iTempIndex )
						{
							//ׯ������
							userEndInfo_lf[iPlayerIndex].lfCurTotalMoney	+= iEveryNtXianSubScore_ZiMo * lfPercent[iTempIndex];
							userEndInfo_lf[iPlayerIndex].lfCurHuMoney		+= iEveryNtXianSubScore_ZiMo * lfPercent[iTempIndex];

							userEndInfo_lf[iTempIndex].lfCurTotalMoney		-= iEveryNtXianSubScore_ZiMo * lfPercent[iTempIndex];
							userEndInfo_lf[iTempIndex].lfCurHuMoney			-= iEveryNtXianSubScore_ZiMo * lfPercent[iTempIndex];

							arrFSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZIMO] += iEveryNtXianSubScore_ZiMo * lfPercent[iTempIndex];
							arrFSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_MO] -= iEveryNtXianSubScore_ZiMo * lfPercent[iTempIndex];
						}
						else
						{
							userEndInfo_lf[iPlayerIndex].lfCurTotalMoney	+= iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];
							userEndInfo_lf[iPlayerIndex].lfCurHuMoney		+= iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];

							userEndInfo_lf[iTempIndex].lfCurTotalMoney		-= iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];
							userEndInfo_lf[iTempIndex].lfCurHuMoney			-= iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];

							arrFSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZIMO] += iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];
							arrFSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_MO] -= iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];
						}
					}
					else
					{
						userEndInfo_lf[iPlayerIndex].lfCurTotalMoney	+= iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];
						userEndInfo_lf[iPlayerIndex].lfCurHuMoney		-= iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];

						userEndInfo_lf[iTempIndex].lfCurTotalMoney		-= iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];
						userEndInfo_lf[iTempIndex].lfCurHuMoney			-= iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];

						arrFSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZIMO] += iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];
						arrFSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_MO] -= iEveryNormalSubScore_ZiMo * lfPercent[iTempIndex];
					}
				}
			}
		}

		//���
		if ( iDianGangCount > 0 )
		{
			//m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_DIAN_GANG] += iDianGangCount;
			arrFSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_DIAN_GANG] -= iDianGangCount*(CONFIG_PALYER_COUNT-1) * lfPercent[iPlayerIndex];

			userEndInfo_lf[iPlayerIndex].lfCurTotalMoney           -= iDianGangCount*(CONFIG_PALYER_COUNT-1) * lfPercent[iPlayerIndex];  //���
			userEndInfo_lf[iPlayerIndex].lfCurGangMoney            -= iDianGangCount*(CONFIG_PALYER_COUNT-1) * lfPercent[iPlayerIndex];  //���
		}

		//�Ӹ�
		if ( iJieGangCount > 0 )
		{
			//m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_JIE_GANG] += iJieGangCount;
			arrFSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_JIE_GANG] += iJieGangCount*(CONFIG_PALYER_COUNT-1) * lfPercent[iPlayerIndex];

			userEndInfo_lf[iPlayerIndex].lfCurTotalMoney          += iJieGangCount*(CONFIG_PALYER_COUNT-1) * lfPercent[iPlayerIndex];  //�Ӹ�
			userEndInfo_lf[iPlayerIndex].lfCurGangMoney           += iJieGangCount*(CONFIG_PALYER_COUNT-1) * lfPercent[iPlayerIndex];  //�Ӹ�
		}

		//����
		int iEveryAnGangSubCount = 2;
		if ( iAnGangCount > 0 )
		{
			//m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_AN_GANG] += iAnGangCount;

			for ( int iTempIndex = 0 ; iTempIndex < CONFIG_PALYER_COUNT ; iTempIndex++ )
			{
				if ( iTempIndex == iPlayerIndex )
				{
					continue;
				}

				//m_arrSaveDetailCount[iTempIndex][E_CARD_GAME_SCORE_BEI_AN_GANG] += iAnGangCount;

				userEndInfo_lf[iPlayerIndex].lfCurTotalMoney += iAnGangCount*iEveryAnGangSubCount * lfPercent[iTempIndex];
				userEndInfo_lf[iPlayerIndex].lfCurGangMoney  += iAnGangCount*iEveryAnGangSubCount * lfPercent[iTempIndex];

				userEndInfo_lf[iTempIndex].lfCurTotalMoney   -= iAnGangCount*iEveryAnGangSubCount * lfPercent[iTempIndex];
				userEndInfo_lf[iTempIndex].lfCurGangMoney    -= iAnGangCount*iEveryAnGangSubCount * lfPercent[iTempIndex];

				arrFSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_AN_GANG]   += iAnGangCount*iEveryAnGangSubCount * lfPercent[iTempIndex];
				arrFSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_AN_GANG] -= iAnGangCount*iEveryAnGangSubCount * lfPercent[iTempIndex];
			}
		}

		//����
		int iEveryBuGangSubCount = 1;
		if ( iBuGangCount > 0 )
		{
			//m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_BU_GANG] += iBuGangCount;

			for ( int iTempIndex = 0 ; iTempIndex < CONFIG_PALYER_COUNT ; iTempIndex++ )
			{
				if ( iTempIndex == iPlayerIndex )
				{
					continue;
				}

				//m_arrSaveDetailCount[iTempIndex][E_CARD_GAME_SCORE_BEI_BU_GANG] += iBuGangCount;

				userEndInfo_lf[iPlayerIndex].lfCurTotalMoney += iBuGangCount*iEveryBuGangSubCount * lfPercent[iTempIndex];
				userEndInfo_lf[iPlayerIndex].lfCurGangMoney  += iBuGangCount*iEveryBuGangSubCount * lfPercent[iTempIndex];

				userEndInfo_lf[iTempIndex].lfCurTotalMoney   -= iBuGangCount*iEveryBuGangSubCount * lfPercent[iTempIndex];
				userEndInfo_lf[iTempIndex].lfCurGangMoney    -= iBuGangCount*iEveryBuGangSubCount * lfPercent[iTempIndex];

				arrFSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_BU_GANG]   += iBuGangCount*iEveryBuGangSubCount * lfPercent[iTempIndex];
				arrFSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_BU_GANG] -= iBuGangCount*iEveryBuGangSubCount * lfPercent[iTempIndex];
			}
		}

		//����
		int iEveryNiaoSubCount = 2;
		if ( iZhongNiaoCount > 0 )
		{
			if ( iZiMouHuCount > 0 )
			{
				//���ܺ��������������ܵ����Ҫ�����ҵ����
				if (iQiangGangHuCount > 0)
				{
					//m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_QIANG_GANG] = 1;

					//���ұ����ܵ����
					int nBeiQiangGang = -1;
					for (int bqgang=0; bqgang<CONFIG_PALYER_COUNT; bqgang++)
					{
						PlayerInfo* pQGPlayer = GetPlayerInfo(iPlayerIndex);
						if ( NULL == pQGPlayer )
						{
							continue;
						}
						if (m_GameEndInfo.userDetail[bqgang].byLsDetailCount[ECSCORE_BEI_QGANG] > 0)
						{
							nBeiQiangGang = bqgang;
							break;
						}
					}
					if (-1 == nBeiQiangGang)
					{
						MYLOG_ERROR(logger, "-1 == nBeiQiangGang !!!!!!");
					}
					else
					{
						//m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO] += iZhongNiaoCount;
						//m_arrSaveDetailCount[nBeiQiangGang][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] += iZhongNiaoCount;

						INT64 n64NiaoScore = iZhongNiaoCount*iEveryNiaoSubCount*(CONFIG_PALYER_COUNT-1);

						userEndInfo_lf[iPlayerIndex].lfCurTotalMoney += n64NiaoScore * lfPercent[nBeiQiangGang];
						userEndInfo_lf[iPlayerIndex].lfCurNiaoMoney  += n64NiaoScore * lfPercent[nBeiQiangGang];

						userEndInfo_lf[nBeiQiangGang].lfCurTotalMoney   -= n64NiaoScore * lfPercent[nBeiQiangGang];
						userEndInfo_lf[nBeiQiangGang].lfCurNiaoMoney    -= n64NiaoScore * lfPercent[nBeiQiangGang];

						arrFSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO]		 += n64NiaoScore * lfPercent[nBeiQiangGang];
						arrFSaveDetailScore[nBeiQiangGang][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] -= n64NiaoScore * lfPercent[nBeiQiangGang];

						MYLOG_INFO(logger,"ReCalcGameEndScore QiangHangHu[roomID=%d,DeskNo=%d,iPlayerIndex=%d,nBeiQiangGang=%d,n64NiaoScore=%lld fPercent[nBeiQiangGang]=%lf]",
							GetRoomID(),GetTableID(),iPlayerIndex,nBeiQiangGang,n64NiaoScore, lfPercent[nBeiQiangGang]);
					}
				}
				//�����ܺ����
				else
				{
					//������
					//m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO] += iZhongNiaoCount;

					for ( int iTempIndex = 0 ; iTempIndex < CONFIG_PALYER_COUNT ; iTempIndex++ )
					{
						if ( iTempIndex == iPlayerIndex )
						{
							continue;
						}

						//m_arrSaveDetailCount[iTempIndex][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] += iZhongNiaoCount;

						userEndInfo_lf[iPlayerIndex].lfCurTotalMoney += iZhongNiaoCount*iEveryNiaoSubCount * lfPercent[iTempIndex];
						userEndInfo_lf[iPlayerIndex].lfCurNiaoMoney += iZhongNiaoCount*iEveryNiaoSubCount * lfPercent[iTempIndex];

						userEndInfo_lf[iTempIndex].lfCurTotalMoney   -= iZhongNiaoCount*iEveryNiaoSubCount * lfPercent[iTempIndex];
						userEndInfo_lf[iTempIndex].lfCurNiaoMoney   -= iZhongNiaoCount*iEveryNiaoSubCount * lfPercent[iTempIndex];

						arrFSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO]   += iZhongNiaoCount*iEveryNiaoSubCount * lfPercent[iTempIndex];
						arrFSaveDetailScore[iTempIndex][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] -= iZhongNiaoCount*iEveryNiaoSubCount * lfPercent[iTempIndex];
					}
				}
			}
			//�޵��ں�
			// 			else
			// 			{
			// 				//���ں� 
			// 				if ( ( iJiePaoCount > 0 || iQiangGangHuCount > 0 ) && iFangPaoStation >= 0 )
			// 				{
			// 					m_arrSaveDetailCount[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO]        += iZhongNiaoCount;
			// 					m_arrSaveDetailCount[iFangPaoStation][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] += iZhongNiaoCount;
			// 
			// 					m_arrSaveDetailScore[iPlayerIndex][E_CARD_GAME_SCORE_ZHONG_NIAO]        += iZhongNiaoCount*iEveryNiaoSubCount;
			// 					m_arrSaveDetailScore[iFangPaoStation][E_CARD_GAME_SCORE_BEI_ZHONG_NIAO] -= iZhongNiaoCount*iEveryNiaoSubCount;
			// 
			// 					userEndInfo_lf[iPlayerIndex].iCurTotalMoney                   += iZhongNiaoCount*iEveryNiaoSubCount;   //����
			// 					userEndInfo_lf[iFangPaoStation].iCurTotalMoney                -= iZhongNiaoCount*iEveryNiaoSubCount;   //����
			// 				}
			// 			}
		}
	}

	for (int i=0; i<CONFIG_PALYER_COUNT; i++)
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if ( NULL == pPlayer )
		{
			continue;
		}

		MYLOG_DEBUG(logger, "ReCalcGameEndScore i=%d Money=%ld", i, pPlayer->GetUserMoney());

		MYLOG_DEBUG(logger, "ReCalcGameEndScore ChairID=%d UserID=%d HuScore=%lf GangScore=%lf NiaoScore=%lf Total=%lf",
			i, pPlayer->GetUserID(), userEndInfo_lf[i].lfCurHuMoney, userEndInfo_lf[i].lfCurGangMoney, 
			userEndInfo_lf[i].lfCurNiaoMoney, userEndInfo_lf[i].lfCurTotalMoney);
	}

	//��������
	for (int i=0; i<CONFIG_PALYER_COUNT; i++)
	{
		for (int index=E_CARD_GAME_SCORE_ZIMO; index<E_CARD_GAME_SCORE_TYPE_MAX; index++)
		{
			m_arrSaveDetailScore[i][index] = (INT64)arrFSaveDetailScore[i][index];
		}

		m_GameEndInfo.userDetail[i].iCurTotalMoney = (INT64)userEndInfo_lf[i].lfCurTotalMoney;
		m_GameEndInfo.userDetail[i].iCurGangMoney = (INT64)userEndInfo_lf[i].lfCurGangMoney;
		m_GameEndInfo.userDetail[i].iCurHuMoney = (INT64)userEndInfo_lf[i].lfCurHuMoney;
		m_GameEndInfo.userDetail[i].iCurNiaoMoney = (INT64)userEndInfo_lf[i].lfCurNiaoMoney;
	}

	for (int i=0; i<CONFIG_PALYER_COUNT; i++)
	{
		MYLOG_DEBUG(logger, "ReCalcGameEndScore i=%d; %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", i
			, arrFSaveDetailScore[i][1], arrFSaveDetailScore[i][2], arrFSaveDetailScore[i][3], arrFSaveDetailScore[i][4], arrFSaveDetailScore[i][5]
		, arrFSaveDetailScore[i][6], arrFSaveDetailScore[i][7], arrFSaveDetailScore[i][8], arrFSaveDetailScore[i][9], arrFSaveDetailScore[i][10]
		, arrFSaveDetailScore[i][11], arrFSaveDetailScore[i][12], arrFSaveDetailScore[i][13], arrFSaveDetailScore[i][14], arrFSaveDetailScore[i][15]);
	}

	for (int i=0; i<CONFIG_PALYER_COUNT; i++)
	{
		MYLOG_DEBUG(logger, "ReCalcGameEndScore i=%d; %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", i
			, m_arrSaveDetailScore[i][1], m_arrSaveDetailScore[i][2], m_arrSaveDetailScore[i][3], m_arrSaveDetailScore[i][4], m_arrSaveDetailScore[i][5]
		, m_arrSaveDetailScore[i][6], m_arrSaveDetailScore[i][7], m_arrSaveDetailScore[i][8], m_arrSaveDetailScore[i][9], m_arrSaveDetailScore[i][10]
		, m_arrSaveDetailScore[i][11], m_arrSaveDetailScore[i][12], m_arrSaveDetailScore[i][13], m_arrSaveDetailScore[i][14], m_arrSaveDetailScore[i][15]);
	}

	return 0;
}

//֪ͨ�ͻ��˲����ɹ�
int  CGameDesk::NotifyClientOperatesSuccess(int iDeskStation,DWORD dwAction,const std::vector<BYTE>& vecCardList,int iNotifyStation/*=-1*/)
{
	CMD_S_USER_ACTION_R_SUCC sMsg;
	memset(&sMsg,0,sizeof(sMsg));
	sMsg.bySeatNo     = iDeskStation;
	sMsg.dwAction     = ntoh32(dwAction);
	sMsg.byOutMjSetaNo = m_byLastOutMjSetaNo;
	sMsg.byOutMj      = m_byLastOutMj;
	sMsg.bySatrtIndex = m_iStartIndex;
	sMsg.byEndIndex   = m_iEndIndex;
	sMsg.iLeftMjCount = ntoh32(m_iLeftMjCount);
	sMsg.byCardNum    = std::min(int(vecCardList.size()),4);

	for ( int i = 0 ; i < sMsg.byCardNum ; i++ )
	{
		sMsg.byCard[i]    = vecCardList[i];
	}

	if ( iNotifyStation >= 0 && iNotifyStation < CONFIG_PALYER_COUNT )
	{
		SendGameData(iNotifyStation,SUB_S_USER_ACTION_R_SUCC,(BYTE*)&sMsg,sizeof(sMsg));
	}
	else
	{
		SendDeskBroadCast(SUB_S_USER_ACTION_R_SUCC,(BYTE*)&sMsg,sizeof(sMsg));
	}
	return 0;
}

//֪ͨ�ͻ��˲���ʧ��
int  CGameDesk::NotifyClientOperatesFail(int iNotifyStation,int iCode)
{
	if ( iNotifyStation >= 0 && iNotifyStation < CONFIG_PALYER_COUNT )
	{
		CMD_S_USER_ACTION_R_FAIL sMsg;
		memset(&sMsg,0,sizeof(sMsg));
		sMsg.iCode = ntoh32(iCode);

		SendGameData(iNotifyStation,SUB_S_USER_ACTION_R_FAIL,(BYTE*)&sMsg,sizeof(sMsg));
	}

	return 0;
}

//֪ͨ������ҵ�ǰ��ʾ
int  CGameDesk::NotifyAdjustPlayerEnableAction(int iDeskStation,DWORD dwOneAction)
{
	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		return -1;
	}

	const SERVER_SAVE_USER_OPERATE&  SaveUserEnableOperates = pPlayer->GetEnableServerActionData();
	const CMD_C_USER_DO_ACTION& DoWaitAction                = pPlayer->GetWaitOneDoAction();
	MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction ProcPlayerFailAction..[roomID=%d,DeskNo=%d,iDeskStation=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x,DoWaitAction.dwAction=%0x,dwOneAction=%0x]"
		,GetRoomID(),GetTableID(),iDeskStation,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction(),DoWaitAction.dwAction,dwOneAction);

	//�ж��Ƿ�����������
	if ( SaveUserEnableOperates.dwEnableAction != 0 && DoWaitAction.dwAction == 0 )
	{
		//֪ͨ��Ҳ���
		char szBuffer[2048]={0};
		CMD_S_NOTIFY_USER_OPERATE* pSendMsg = (CMD_S_NOTIFY_USER_OPERATE*)szBuffer;
		pSendMsg->dwEnableAction = ntoh32(SaveUserEnableOperates.dwEnableAction);
		pSendMsg->byOutMjSeatNo  = SaveUserEnableOperates.byOutMjSeatNo;
		pSendMsg->byOutMj        = SaveUserEnableOperates.byOutMj;
		pSendMsg->byCardNum      = std::min((int)(SaveUserEnableOperates.vecCardList.size()),10);
		for ( int iCardIndex = 0;  iCardIndex < pSendMsg->byCardNum ; iCardIndex++ )
		{
			const stOpCardList& OneTempCardIfo = SaveUserEnableOperates.vecCardList[iCardIndex];
			memcpy(&pSendMsg->lsCardInfo[iCardIndex],&OneTempCardIfo,sizeof(stOpCardList));
		}

		int nMsgParaLen = sizeof(CMD_S_NOTIFY_USER_OPERATE) + (pSendMsg->byCardNum-1)*sizeof(stOpCardList);
		SendGameData(iDeskStation,SUB_S_NOTIFY_USER_OPERATE,(BYTE*)szBuffer,nMsgParaLen);
	}
	else
	{
		//֪ͨʧ��
		NotifyClientOperatesFail(iDeskStation,ERR_ACTION_CODE_WAIT_OTHER_OPERATE);
	}
	return 0;
}

/// �ȴ�ͬ��ʱ�·�״̬
int CGameDesk::GetStationWhileWaitAgree(int nDeskStation, bool bIsWatch, UINT uIndex)
{
 	GameStation_2 GameStation;
 	memset(&GameStation,0,sizeof(GameStation));
 	GameStation.bStation  = GS_WAIT_ARGEE;
 	//��Ϸ�汾�˶�
 	GameStation.iVersion  = GAME_MAX_VER;			//��Ϸ�߰汾
 	GameStation.iVersion2 = GAME_LESS_VER;			//�Ͱ汾

	//������Ϣ
	GameStation.iLsConfig[ECFGTYPE_LAIZICARD] = ntoh32(GetCfgLaiziCard());

 	//����ʱ��
 	GameStation.byBankerSeatNo  = m_iNtStation;
 	GameStation.iTotalRound     = ntoh32(m_iTotalRound);
	GameStation.iLeftRound      = ntoh32(m_iTotalRound-m_iCurRound);

	//ͬ����
	for ( int i = 0 ; i < CONFIG_PALYER_COUNT ; i++ )
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if ( NULL == pPlayer )
		{
			continue;
		}

		GameStation.bIsAgree[i] = pPlayer->GetIsAgree();
	}

 	SendGameStation(nDeskStation, uIndex, &GameStation,sizeof(GameStation));
	return 0;
}

/// �ȴ�������ʱ�·�״̬
int CGameDesk::GetStationWhileWaitTouShaiZi(int nDeskStation, bool bIsWatch, UINT uIndex)
{
	GameStation_3 GameStation;
	memset(&GameStation,0,sizeof(GameStation));
	GameStation.bStation  = GS_NT_TOUZEZI;
	//��Ϸ�汾�˶�
	GameStation.iVersion  = GAME_MAX_VER;			//��Ϸ�߰汾
	GameStation.iVersion2 = GAME_LESS_VER;			//�Ͱ汾

	//������Ϣ
	GameStation.iLsConfig[ECFGTYPE_LAIZICARD] = ntoh32(GetCfgLaiziCard());

	//����ʱ��
	GameStation.byBankerSeatNo  = m_iNtStation;
	GameStation.iTotalRound     = ntoh32(m_iTotalRound);
	GameStation.iLeftRound      = ntoh32(m_iTotalRound-m_iCurRound);

	GameStation.bySatrtIndex    = m_iStartIndex;
	GameStation.byEndIndex      = m_iEndIndex;
	GameStation.iLeftMjCount    = ntoh32(m_iLeftMjCount);

	SendGameStation(nDeskStation, uIndex, &GameStation,sizeof(GameStation));
	return 0;
}

/// �ȴ�����ʱ�·�״̬
int CGameDesk::GetStationWhileWaitSendCard(int nDeskStation, bool bIsWatch, UINT uIndex)
{
	GameStation_4 GameStation;
	memset(&GameStation,0,sizeof(GameStation));
	GameStation.bStation        = GS_SEND_CARD;
	//��Ϸ�汾�˶�
	GameStation.iVersion        = GAME_MAX_VER;			//��Ϸ�߰汾
	GameStation.iVersion2       = GAME_LESS_VER;			//�Ͱ汾

	//������Ϣ
	GameStation.iLsConfig[ECFGTYPE_LAIZICARD] = ntoh32(GetCfgLaiziCard());

	//����ʱ��
	GameStation.byBankerSeatNo  = m_iNtStation;
	GameStation.iTotalRound     = ntoh32(m_iTotalRound);
	GameStation.iLeftRound      = ntoh32(m_iTotalRound-m_iCurRound);

	GameStation.byDicePoint[0]  = m_byDicePoint[0];
	GameStation.byDicePoint[1]  = m_byDicePoint[1];

	GameStation.bySatrtIndex    = m_iStartIndex;
	GameStation.byEndIndex      = m_iEndIndex;
	GameStation.iLeftMjCount    = ntoh32(m_iLeftMjCount);

	SendGameStation(nDeskStation, uIndex, &GameStation,sizeof(GameStation));
	return 0;
}

/// ��Ϸ��ʱ�·�״̬
int CGameDesk::GetStationWhileWaitPlaying(int nDeskStation, bool bIsWatch, UINT uIndex)
{
	GameStation_5 GameStation;
	memset(&GameStation,0,sizeof(GameStation));
	GameStation.bStation        = GS_PLAY_GAME;
	//��Ϸ�汾�˶�
	GameStation.iVersion        = GAME_MAX_VER;			//��Ϸ�߰汾
	GameStation.iVersion2       = GAME_LESS_VER;			//�Ͱ汾

	//������Ϣ
	GameStation.iLsConfig[ECFGTYPE_LAIZICARD] = ntoh32(GetCfgLaiziCard());

	//����ʱ��
	GameStation.byBankerSeatNo  = m_iNtStation;
	GameStation.iTotalRound     = ntoh32(m_iTotalRound);
	GameStation.iLeftRound      = ntoh32(m_iTotalRound-m_iCurRound);

	GameStation.byDicePoint[0]  = m_byDicePoint[0];
	GameStation.byDicePoint[1]  = m_byDicePoint[1];

	GameStation.bySatrtIndex    = m_iStartIndex;
	GameStation.byEndIndex      = m_iEndIndex;
	GameStation.iLeftMjCount    = ntoh32(m_iLeftMjCount);

	//����ʱ
	GameStation.bySeatNo        = m_iGameDjsStation;
	int iDjsLeftCount = 0;
	time_t tmCurTime = CTimeLib::GetCurMSecond();
	if ( tmCurTime < m_tmGameDjsStartMS + m_tmGameDjsTimes*1000 )
	{
		iDjsLeftCount = (m_tmGameDjsStartMS + m_tmGameDjsTimes*1000 - tmCurTime)/1000;
	}
	GameStation.iDjsTimes       = ntoh32(iDjsLeftCount);

	//��
	for ( int iPlayerIndex = 0; iPlayerIndex < CONFIG_PALYER_COUNT ; iPlayerIndex++ )
	{
		PlayerInfo* pPlayer = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pPlayer )
		{
			continue;
		}

		std::vector<MJCPGNode> vecMJCPGList;
		if( pPlayer->GetMJCPGNodeList(vecMJCPGList) > 0 )
		{
			//�����б�
			int iCount = 0;
			for ( int i = 0 ; i < vecMJCPGList.size() ; i++ )
			{
				if ( vecMJCPGList[i].byBlockCardType == BLOCK_KEZI_PENG )
				{
					GameStation.byPengCardList[iPlayerIndex][iCount] = vecMJCPGList[i].byMjCard[0];
					iCount++;
				}
			}

			//�������б�
			iCount = 0;
			for ( int i = 0 ; i < vecMJCPGList.size() ; i++ )
			{
				if ( vecMJCPGList[i].byBlockCardType == BLOCK_GANG_DIAN || vecMJCPGList[i].byBlockCardType == BLOCK_GANG_BU )
				{
					GameStation.byMingGangCardList[iPlayerIndex][iCount] = vecMJCPGList[i].byMjCard[0];
					iCount++;
				}
			}
			
			//�������б�
			iCount = 0;
			for ( int i = 0 ; i < vecMJCPGList.size() ; i++ )
			{
				if ( vecMJCPGList[i].byBlockCardType == BLOCK_GANG_AN  )
				{
					GameStation.byAnGangCardList[iPlayerIndex][iCount] = vecMJCPGList[i].byMjCard[0];
					iCount++;
				}
			}

			//�����б�
			iCount = 0;
			for ( int i = 0 ; i < vecMJCPGList.size() ; i++ )
			{
				if ( vecMJCPGList[i].byBlockCardType == BLOCK_SUN_CHI  )
				{
					for ( int k = 0 ; k < 3 ; k++ )
					{
						GameStation.byChiList[iPlayerIndex][iCount][k] = vecMJCPGList[i].byMjCard[k];
					}
					iCount++;
				}
			}
		}

		std::list<BYTE> lsOutHandMj;
		if( pPlayer->GetHandMjCard(lsOutHandMj) > 0 )
		{
			//�Լ������齫
			if ( nDeskStation == iPlayerIndex )
			{
				int iTemp = 0;
				std::list<BYTE>::iterator it = lsOutHandMj.begin();
				for ( ; it != lsOutHandMj.end() && iTemp < HAND_MJ_MAX ; it++ ,iTemp++)
				{
					GameStation.byMyHandMjs[iTemp] = (*it);
				}
			}
			else
			{
				//���������������
				GameStation.byOtherSeatMjCount[iPlayerIndex] = lsOutHandMj.size();
			}
		}

		//��ҳ�����
		int iEveryPOutCard = MAX_MJ_TOTAL_COUTN/CONFIG_PALYER_COUNT;
		std::list<BYTE> ppOutMjList;
		if( pPlayer->GetOutMjList(ppOutMjList) > 0 )
		{
			int iCount = 0;
			std::list<BYTE>::iterator it = ppOutMjList.begin();
			for ( ; it != ppOutMjList.end() && iCount < iEveryPOutCard; it++,iCount++ )
			{
				GameStation.byOutCardList[iEveryPOutCard*iPlayerIndex+iCount] = (*it);
			}
		}
	}

	//��ǰ�����齫��ͷ��ʾ���,255��ʾû�м�ͷָ��
	GameStation.byCurMjShowSeat     = GetCurMjShowSeat();
	GameStation.byCurOperateStation = GetCurOperateStation();
	GameStation.byCurOpStatus       = m_iCurSeatOpStatus;

	SendGameStation(nDeskStation, uIndex, &GameStation,sizeof(GameStation));
		
	//֪ͨ�ͻ����ܽ��в���
	PlayerInfo* pPlayer = GetPlayerInfo(nDeskStation);
	if ( NULL != pPlayer )
	{		
		const SERVER_SAVE_USER_OPERATE  SaveUserEnableOperates = pPlayer->GetEnableServerActionData();
		if ( SaveUserEnableOperates.dwEnableAction != 0 )
		{
			//֪ͨ��Ҳ���
			char szBuffer[2048]={0};
			CMD_S_NOTIFY_USER_OPERATE* pSendMsg = (CMD_S_NOTIFY_USER_OPERATE*)szBuffer;
			pSendMsg->dwEnableAction = ntoh32(SaveUserEnableOperates.dwEnableAction);
			pSendMsg->byOutMjSeatNo  = SaveUserEnableOperates.byOutMjSeatNo;
			pSendMsg->byOutMj        = SaveUserEnableOperates.byOutMj;
			pSendMsg->byCardNum      = std::min((int)(SaveUserEnableOperates.vecCardList.size()),10);
			for ( int iCardIndex = 0;  iCardIndex < pSendMsg->byCardNum ; iCardIndex++ )
			{
				const stOpCardList& OneTempCardIfo = SaveUserEnableOperates.vecCardList[iCardIndex];
				memcpy(&pSendMsg->lsCardInfo[iCardIndex],&OneTempCardIfo,sizeof(stOpCardList));
			}

			int nMsgParaLen = sizeof(CMD_S_NOTIFY_USER_OPERATE) + (pSendMsg->byCardNum-1)*sizeof(stOpCardList);
			SendGameData(nDeskStation,SUB_S_NOTIFY_USER_OPERATE,(BYTE*)szBuffer,nMsgParaLen);

			//������ʾ��Ϣ
			if ( GetCurOperateStation() == nDeskStation )
			{
				stCardData stCardShow;
				for (int i=0; i<CONFIG_PALYER_COUNT; ++i)
				{
					PlayerInfo* pPlayerShow = GetPlayerInfo(i);
					if (pPlayerShow)
						pPlayerShow->getShowCardsData(stCardShow, i==GetCurOperateStation());
				}
				DWORD dwFlag = GetTickCount();
				vector<stHuTips> vctHuTips = pPlayer->CheckMyCanTing_HuTips(stCardShow);
				MYLOG_DEBUG(logger,"pino_log GetStationWhileWaitPlaying byUnlockNum=%d Time:%d ms", vctHuTips.size(), GetTickCount()-dwFlag);			

				int  nBuffSize = sizeof(CMD_S_TingLock_HuTips);
				char szBuffer[2048] = {0};
				CMD_S_TingLock_HuTips stTipsTemp;
				stTipsTemp.byUnlockNum = vctHuTips.size();
				memcpy(szBuffer, &stTipsTemp, nBuffSize);
				memcpy(szBuffer+nBuffSize, &vctHuTips[0], sizeof(stHuTips)*stTipsTemp.byUnlockNum);
				SendGameData(nDeskStation, SUB_S_TING_LOCK_HUTIPS, (BYTE*)szBuffer, nBuffSize+sizeof(stHuTips)*stTipsTemp.byUnlockNum);	
			}
		}
	}

	//֪ͨ������ҵĶ��߱��
	for ( int iPlayerIndex = 0; iPlayerIndex < CONFIG_PALYER_COUNT ; iPlayerIndex++ )
	{
		PlayerInfo* pPlayer = GetPlayerInfo(iPlayerIndex);
		if ( NULL == pPlayer )
		{
			continue;
		}

		MSG_NOTIFY_USER_NET_CUT sMsg;
		memset(&sMsg, 0, sizeof(sMsg));
		sMsg.byNetCutSeatNo = iPlayerIndex;
		sMsg.bIsNetCut      = pPlayer->GetIsNetCut();
		SendGameData(nDeskStation,ASS_NOTIFY_USER_NET_CUT,(BYTE*)&sMsg, sizeof(sMsg));
	}

	BroadcastUserTuoGuanMsg();

	//֪ͨ�ܺ�����
	NotifyEnableHuCards(nDeskStation);
	return 0;
}

/// ����״̬
int CGameDesk::GetStationWhileGameEnd(int nDeskStation, bool bIsWatch, UINT uIndex)
{
	PlayerInfo* pPlayer = GetPlayerInfo(nDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger, "Error CGameDesk::OnGetGameStation NULL == pPlayer.. [nDeskStation=%d]", nDeskStation);
		return -1;
	}

	if ( pPlayer->GetIsAgree() )
	{
		MYLOG_DEBUG(logger, "CGameDesk::GetStationWhileGameEnd..GetStationWhileWaitAgree [nDeskStation=%d]", nDeskStation);

		//�Ѿ�ͬ��
		GetStationWhileWaitAgree(nDeskStation,bIsWatch,uIndex);
	}
	else
	{
		//û��ͬ��
		GameStation_6 GameStation;
		memset(&GameStation,0,sizeof(GameStation));
		GameStation.bStation        = GS_WAIT_NEXT;
		//��Ϸ�汾�˶�
		GameStation.iVersion        = GAME_MAX_VER;			//��Ϸ�߰汾
		GameStation.iVersion2       = GAME_LESS_VER;			//�Ͱ汾

		//������Ϣ
		GameStation.iLsConfig[ECFGTYPE_LAIZICARD] = ntoh32(GetCfgLaiziCard());

		//����ʱ��
		GameStation.byBankerSeatNo  = m_iNtStation;
		GameStation.iTotalRound     = ntoh32(m_iTotalRound);
		GameStation.iLeftRound      = ntoh32(m_iTotalRound-m_iCurRound);

		//ͬ����
		for ( int i = 0 ; i < CONFIG_PALYER_COUNT ; i++ )
		{
			PlayerInfo* pPlayer = GetPlayerInfo(i);
			if ( NULL == pPlayer )
			{
				continue;
			}

			GameStation.bIsAgree[i] = pPlayer->GetIsAgree();
		}

		//������Ϣ
		CMD_S_GameEnd   SendGameEndInfo;
		memcpy(&SendGameEndInfo,&m_GameEndInfo,sizeof(m_GameEndInfo));
		for ( int i = 0 ; i < CONFIG_PALYER_COUNT; i++ )
		{
			SendGameEndInfo.userDetail[i].iCurTotalPan   = ntoh32(SendGameEndInfo.userDetail[i].iCurTotalPan);
			SendGameEndInfo.userDetail[i].iCurTotalMoney = ntoh64(SendGameEndInfo.userDetail[i].iCurTotalMoney);
			SendGameEndInfo.userDetail[i].iTotalMoney    = ntoh64(SendGameEndInfo.userDetail[i].iTotalMoney);
			SendGameEndInfo.userDetail[i].iCurGangMoney  = ntoh64(SendGameEndInfo.userDetail[i].iCurGangMoney);
			SendGameEndInfo.userDetail[i].iCurHuMoney    = ntoh64(SendGameEndInfo.userDetail[i].iCurHuMoney);
			SendGameEndInfo.userDetail[i].iCurNiaoMoney  = ntoh64(SendGameEndInfo.userDetail[i].iCurNiaoMoney);
			SendGameEndInfo.userDetail[i].iTax           = ntoh32(SendGameEndInfo.userDetail[i].iTax);
			SendGameEndInfo.userDetail[i].dwHuExtendsType= ntoh32(SendGameEndInfo.userDetail[i].dwHuExtendsType);
		}
		memcpy(&GameStation.sGameEndInfo,&SendGameEndInfo,sizeof(SendGameEndInfo));

		SendGameStation(nDeskStation, uIndex, &GameStation,sizeof(GameStation));
	}
	return 0;
}

inline bool CGameDesk::SendGameStation(BYTE bDeskStation, UINT uIndex, void * pStationData, UINT uSize)
{
	MYLOG_DEBUG(logger, "CGameDesk::OnGetGameStation SendGameStation..  [nDeskStation=%d,m_nGameStation=%d]", bDeskStation,m_nGameStation);

	SendGameData(bDeskStation,ASS_GM_GAME_STATION,(BYTE*)pStationData, uSize);
	return true;
}
/// ��ȡ��Ϸ״̬�������͵��ͻ���
int CGameDesk::OnGetGameStation(int nDeskStation, UserInfoForGame_t& userInfo)
{
	if ( (nDeskStation<0) || (nDeskStation>CONFIG_PALYER_COUNT))
	{
		MYLOG_ERROR(logger, "Error CGameDesk::OnGetGameStation  nDeskStation=%d", nDeskStation);
		return -1;
	}

	MYLOG_DEBUG(logger, "CGameDesk::OnGetGameStation..  [nDeskStation=%d,userID:%d,IsRobot=%d,m_nGameStation=%d]", nDeskStation,userInfo._dwUserID,userInfo._bIsRobot,m_nGameStation);

	/// �Ƿ��Թ�
	bool bIsWatch = false;//(m_arUserInfo[nDeskStation]._dwUserID != userInfo._dwUserID);
	switch (m_nGameStation)
	{
	case GS_WAIT_SETGAME:		//��Ϸû�п�ʼ״̬
	case GS_WAIT_ARGEE:			//�ȴ���ҿ�ʼ״̬
		{
			GetStationWhileWaitAgree(nDeskStation, bIsWatch, 0/*userInfo._SocketIndex*/);
			break;
		}
	case  GS_NT_TOUZEZI:								//ׯ��������״̬
		{
			GetStationWhileWaitTouShaiZi(nDeskStation, bIsWatch, 0/*userInfo._SocketIndex*/);
			break;
		}
	case GS_SEND_CARD:					      			//����״̬
		{
			GetStationWhileWaitSendCard(nDeskStation, bIsWatch, 0/*userInfo._SocketIndex*/);
			break;
		}
	case GS_PLAY_GAME:					      			//��Ϸ��״̬
		{
			GetStationWhileWaitPlaying(nDeskStation, bIsWatch, 0/*userInfo._SocketIndex*/);
			break;
		}
	case GS_WAIT_NEXT:           //�ȴ���һ�̿�ʼ 
		{
			GetStationWhileGameEnd(nDeskStation, bIsWatch, 0/*userInfo._SocketIndex*/);
			break;
		}
	}

	return 0;
}

/// �������
int CGameDesk::UserReCome(int nDeskStation, UserInfoForGame_t& userInfo)
{
	if ( (nDeskStation<0) || (nDeskStation>=CONFIG_PALYER_COUNT))
	{
		return -1;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(nDeskStation);

	if (NULL != pPlayer && pPlayer->GetUserID() == userInfo._dwUserID)
	{
		pPlayer->SetIsNetCut(false);
		NotifyUserNetCut(nDeskStation,false);

		MYLOG_DEBUG(logger, "CGameDesk::UserReCome .. userID = %d",userInfo._dwUserID);
	}
	else
	{
		MYLOG_ERROR(logger,"CGameDesk::UserReCome Desk[%d] table[%d] UserID[%d] not be reconnect user[%d]" , m_nDeskIndex, nDeskStation, pPlayer->GetUserID(), userInfo._dwUserID);

		return -1;
	}
	return 0;
}

bool CGameDesk::SetGameTimer(int nTimeID, unsigned int unMSDelay)
{
	MYLOG_DEBUG(logger, "CGameDesk::SetGameTimer [nTimeID=%d,unMSDelay=%d usRoomID=%d, DeskIndex=%d]", nTimeID,unMSDelay, GetRoomID(), m_nDeskIndex);

	if (unMSDelay < 100)
	{
		//MYLOG_DEBUG(logger, "[CGameDesk::SetGameTimer unMSDelay <100][nTimeID=%d,unMSDelay=%d]",nTimeID, unMSDelay);
		unMSDelay = 100;
	}

	if (TIME_PLAYER_1 <= nTimeID && nTimeID <= TIME_PLAYER_4)
	{
		byte byChairID = nTimeID - TIME_PLAYER_1;
		m_bPlayerTimerRunning[byChairID] = true;
	}

	if (NULL != m_pGameManager)
	{
		T_Timer stTimer     = {0};
		stTimer.unTimeID    = GetRoomID()*TIME_MUL_D + m_nDeskIndex*TIME_SPACE + TIME_START_ID + nTimeID;
		stTimer.unMSDelay   = unMSDelay;
		stTimer.ucTimerType = 1;
		stTimer.usRoomID    = GetRoomID();
		stTimer.usDeskIndex = m_nDeskIndex;

		int iRet = m_pGameManager->SetGameTimer(stTimer);
		if ( iRet < 0 )
		{
			MYLOG_ERROR(logger, "Error CGameDesk::SetGameTimer fail..  iRet=%d", iRet);
		}
		return (0 <= iRet);
	}
	else
	{
		MYLOG_ERROR(logger, "Error CGameDesk::SetGameTimer  m_pGameManager=%d", m_pGameManager);
		return false;
	}
}


int CGameDesk::KillGameTimer(int nTimeID)
{
	MYLOG_INFO(logger,"CGameDesk::KillGameTimer uTimerID=%d", nTimeID);

	if (TIME_PLAYER_1 <= nTimeID && nTimeID <= TIME_PLAYER_4)
	{
		byte byChairID = nTimeID - TIME_PLAYER_1;
		m_bPlayerTimerRunning[byChairID] = false;
	}

	if (NULL != m_pGameManager)
	{
		return m_pGameManager->KillGameTimer(GetRoomID()*TIME_MUL_D + m_nDeskIndex*TIME_SPACE + TIME_START_ID + nTimeID);
	}

	return -1;
}

/// ��ʱ��ʱ�䵽
int CGameDesk::OnGameTimer(unsigned int unTimerID, void* pData)
{
	// ��ʱ��IDת����
	unsigned int unRealTimerID = unTimerID - TIME_START_ID - m_nDeskIndex*TIME_SPACE - GetRoomID()*TIME_MUL_D;

	UINT uTimerID = unRealTimerID;

	if (TIME_PLAYER_1 <= unTimerID && unTimerID <= TIME_PLAYER_4)
	{
		byte byChairID = unTimerID - TIME_PLAYER_1;
		m_bPlayerTimerRunning[byChairID] = false;
	}

	switch(uTimerID)
	{
	case TIME_GAME_BASE_TIME:  //��Ϸ�л�����ʱ��
		{
			OnDjsBaseTime();
			break;
		}
	//case TIME_CHECK_END_MONEY: //���Ǯ�Ƿ�
	//	{
	//		KillGameTimer(TIME_CHECK_END_MONEY);
	//		CheckUserMoneyLeftGame();

	//		break;
	//	}
	case TIME_GAME_AUTO_HU:
		{
			KillGameTimer(TIME_GAME_AUTO_HU);
			
			//MYLOG_DEBUG(logger,"OnGameTimer TIME_GAME_AUTO_HU m_nSaveAutoHuChairID=%d", m_nSaveAutoHuChairID);
// 			if (uSize != sizeof(CMD_C_USER_DO_ACTION))
// 			{
// 				MYLOG_ERROR(logger,"Error:CGameDesk::HandleClientMsg SUB_C_USER_DO_ACTION size Error..[roomID=%d,DeskNo=%d,uSize=%d], sizeof(CMD_C_USER_DO_ACTION)=%d",GetRoomID(),GetTableID(),uSize,sizeof(CMD_C_USER_DO_ACTION));
// 				return 0;
// 			}
// 
// 			pMsg->dwAction = hton32(pMsg->dwAction);
// 
// 
// 			MYLOG_DEBUG(logger,"CGameDesk::OnMsgUserDoAction..[roomID=%d,DeskNo=%d,iDeskStation=%d,unUin=%d,m_iCurOperateStation=%d,m_byLastOutMj=%d,m_byLastOutMjSetaNo=%d,dwPlayerEnableAction=%0x ,pMsg->dwAction==%0x]"
// 				,GetRoomID(),GetTableID(),nDeskStation,unUin,GetCurOperateStation(),m_byLastOutMj,m_byLastOutMjSetaNo,pPlayer->GetEnableAction(),pMsg->dwAction);

// 			if (m_nSaveAutoHuChairID < CONFIG_PALYER_COUNT)
// 			{
// 				CMD_C_USER_DO_ACTION userDoAction;
// 				memset(&userDoAction, 0, sizeof(userDoAction));
// 				userDoAction.dwAction = EACTION_HU_ZIMO;
// 				int iRet = OnMsgUserDoAction(m_nSaveAutoHuChairID,&userDoAction);
// 				m_nSaveAutoHuChairID = -1;
// 			}
			
			break;
		}

	case TIME_NOT_ENOUGH_PLAYER:
		{
			MYLOG_INFO(logger,"CGameDesk::OnGameTimer uTimerID=TIME_NOT_ENOUGH_PLAYER");
			KillGameTimer(TIME_NOT_ENOUGH_PLAYER);

			//�ǽ�ҳ�return
			if (!IsGoldRoom())
			{
				break;
			}

			m_pGameManager->DissolveRoom(m_nRoomID, m_nDeskIndex, E_ERROR_DISSOLVED_COIN_ROOM_4_NOT_ENOUGH_USER);
			break;
		}

	case TIME_ONE_COUNT:
		{
			m_nTempCount++;
			KillGameTimer(TIME_ONE_COUNT);
			SetGameTimer(TIME_ONE_COUNT, 1000);

			//if ( 0 == (m_nTempCount%5) )
			{
				MYLOG_INFO(logger, "RoomID=%d TableID=%d OnTimer TIME_ONE_COUNT times=%d %d %d %d, tuoguan=%d %d %d %d m_nTempCount=%d", m_nRoomID, m_nDeskIndex,
					m_nPlayerTimeOutCount[0], m_nPlayerTimeOutCount[1], m_nPlayerTimeOutCount[2], m_nPlayerTimeOutCount[3], 
					m_bPlayerTuoGuan[0], m_bPlayerTuoGuan[1], m_bPlayerTuoGuan[2], m_bPlayerTuoGuan[3], m_nTempCount);
			}

			break;
		}

	case TIME_PLAYER_1:
	case TIME_PLAYER_2:
	case TIME_PLAYER_3:
	case TIME_PLAYER_4:  //��Ϸ��Ҳ�����ʱ��
		{
			KillGameTimer(uTimerID);
			MYLOG_INFO(logger,"CGameDesk::OnGameTimer TIME_PLAYER_1 uTimerID=%d", uTimerID);

			OnPlayerOpTimer(uTimerID);

			break;
		}

	case TIME_SEND_CARD_FINISH:  //���ƺ���ʱ
		{
			KillGameTimer(uTimerID);
			MYLOG_INFO(logger,"CGameDesk::OnGameTimer TIME_SEND_CARD_FINISH uTimerID=%d", uTimerID);

			//��⵱ǰ����ܽ��еĲ���
			CheckCurPlayerEnableOperates();
			break;
		}
	}
	return 0;
}

/// ��Ҷ���
int CGameDesk::UserNetCut(int nDeskStation, UserInfoForGame_t& userInfo)
{
	int nRet = 0;
	if ( (nDeskStation<0) || (nDeskStation>=CONFIG_PALYER_COUNT))
	{
		MYLOG_ERROR(logger, "[CGameDesk::UserNetCut nDeskStation  error] [nDeskStation=%d, userID = %d]", nDeskStation, userInfo._dwUserID);
		return -1;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(nDeskStation);

	if (NULL != pPlayer && pPlayer->GetUserID() == userInfo._dwUserID)
	{
		MYLOG_INFO(logger, "CGameDesk::UserNetCut UserID = %d", userInfo._dwUserID);

		pPlayer->SetIsNetCut(true);
		NotifyUserNetCut(nDeskStation,true);

		/// ���������Ϸ�У�����Ϊ����
		if (m_nGameStation> GS_WAIT_ARGEE && m_nGameStation<GS_WAIT_NEXT)
		{
			//��ϷΪ����������ʱ
			if (m_pGameManager->IsRoomCardPlaform() == 1)
			{

			}
			else
			{
				//�����˶���������������Ϸ
				if ( GetTotalOnlineCount() == 0 )
				{
					m_nGameStation = GS_WAIT_ARGEE;//GS_WAIT_SETGAME;
					GameFinish(GAME_PLAYER, GF_SAFE);
				}
			}
		}
	}
	else
	{
		//ErrorPrintf("[%d]����λ��Ϊ[%d]�����[%d]��Ӧ���Ƕ������[%d]", m_nDeskIndex, nDeskStation, m_arUserInfo[nDeskStation]._dwUserID, userInfo._dwUserID);

		if ( NULL != pPlayer )
		{
			MYLOG_ERROR(logger,"DeskID[%d] table[%d] user[%d] not be cut user[%d]", m_nDeskIndex, nDeskStation, pPlayer->GetUserID(), userInfo._dwUserID);
		}
		else
		{
			MYLOG_ERROR(logger,"DeskID[%d] table[%d] user[%d] not be cut user[%d]", m_nDeskIndex, nDeskStation, 0, userInfo._dwUserID);
		}
		
		return -1;
	}
	return 0;
}
/// ���������ĳλ��
int CGameDesk::UserSitDesk(int nDeskStation, UserInfoForGame_t& userInfo)
{
	if ( (nDeskStation<0) || (nDeskStation>=GAME_PLAYER/*CONFIG_PALYER_COUNT*/))
	{
		MYLOG_ERROR(logger, "[Error: CGameDesk::UserSitDesk nDeskStation  error] [RoomID=%d, DeskNo=%d, nDeskStation=%d, userID = %d ,isRobot=%d]",GetRoomID(),GetTableID() ,nDeskStation, userInfo._dwUserID,userInfo._bIsRobot);
		return -1;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(nDeskStation);

	//������Ϸ��
	if ( IsPlayingByGameStation() )
	{
		if ( NULL != pPlayer && pPlayer->GetUserID() == userInfo._dwUserID )
		{
			//�����Լ�,���³ɹ�
			pPlayer->SetIsNetCut(false);
			NotifyUserNetCut(nDeskStation,false);
			return 0;
		}

		return E_ERROR_CODE_GAME_DESK_PLAYING;
	}

	//������Ϸ��
	//��λ���Ѿ�����
	if (NULL != pPlayer)
	{
		//���Լ��Ѿ�������
		if ( pPlayer->GetUserID() == userInfo._dwUserID )
		{
			//֪ͨ���¶��߱��
			pPlayer->SetIsNetCut(false);
			NotifyUserNetCut(nDeskStation,false);
			return 0;
		}

		//ErrorPrintf("[%d]����λ��Ϊ[%d]�Ѿ������[%d]�����[%d]����ʧ��", m_nDeskIndex, nDeskStation, pPlayer->GetUserID(), userInfo._dwUserID);
		MYLOG_ERROR(logger,"[Error: CGameDesk::UserSitDesk]  DeskID[%d] table[%d] has user[%d] ,user[%d] sit fail..", m_nDeskIndex, nDeskStation, pPlayer->GetUserID(), userInfo._dwUserID);

		return E_ERROR_CODE_GAME_DESK_PLAYING;
	}
	else
	{
		MYLOG_DEBUG(logger, "[CGameDesk::UserSitDesk nDeskStation ..] [roomID=%d,DeskNo=%d,nDeskStation=%d, userID = %d,m_pGameManager->IsRoomCardPlaform()=%d , CONFIG_PALYER_COUNT=%d]", GetRoomID(),GetTableID(),nDeskStation, userInfo._dwUserID,m_pGameManager->IsRoomCardPlaform(),CONFIG_PALYER_COUNT);

		//��ϷΪ����������ʱ
		if (m_pGameManager->IsRoomCardPlaform() == 1)
		{
			int iCountPlayer = GetTotalPlayerCount();
			if ( iCountPlayer <= 0 )
			{
				MYLOG_DEBUG(logger, "[CGameDesk::UserSitDesk nDeskStation ResetCardRoomConfigInfo..] [roomID=%d,DeskNo=%d,nDeskStation=%d, userID = %d,m_pGameManager->IsRoomCardPlaform()=%d , iCountPlayer=%d]", GetRoomID(),GetTableID(),nDeskStation, userInfo._dwUserID,m_pGameManager->IsRoomCardPlaform(),iCountPlayer);

				//���ÿ������������� 
				ResetCardRoomConfigInfo();
				//��ҳ�������ʱ������ʱ�������˲��룬��ɢ����
				if (IsGoldRoom())
				{
					KillGameTimer(TIME_NOT_ENOUGH_PLAYER);
					SetGameTimer(TIME_NOT_ENOUGH_PLAYER, m_nWaitForFullTime*1000);	
				}
			}

			//������
			if (GAME_PLAYER == (iCountPlayer+1))
			{
				KillGameTimer(TIME_NOT_ENOUGH_PLAYER);
			}
		}

		if ( (nDeskStation<0) || (nDeskStation>=CONFIG_PALYER_COUNT))
		{
			MYLOG_ERROR(logger, "[Error: CGameDesk::UserSitDesk nDeskStation  error] [RoomID=%d, DeskNo=%d, nDeskStation=%d, userID = %d ,isRobot=%d , CONFIG_PALYER_COUNT=%d]",GetRoomID(),GetTableID() ,nDeskStation, userInfo._dwUserID,userInfo._bIsRobot,CONFIG_PALYER_COUNT);
			return -2;
		}

		m_vecPlayer[nDeskStation]->ResetUserData();
		m_vecPlayer[nDeskStation]->SetDeskStation(nDeskStation);
		m_vecPlayer[nDeskStation]->SetGameUserInfo(userInfo);

		if (IsGoldRoom())
		{
			KillGameTimer(TIME_PLAYER_1+nDeskStation);
			SetGameTimer(TIME_PLAYER_1+nDeskStation, m_nReadyTime*1000);
		}
	}

	int nCountPlayer = GetTotalPlayerCount();
	MYLOG_DEBUG(logger, "[CGameDesk::UserSitDesk nCountPlayer=%d", nCountPlayer);

	return 0;
}
/// �������
int CGameDesk::UserLeftDesk(int nDeskStation, UserInfoForGame_t& userInfo)
{
	if ( (nDeskStation<0) || (nDeskStation>=CONFIG_PALYER_COUNT))
	{
		MYLOG_ERROR(logger, "[CGameDesk::UserLeftDesk nDeskStation  error] [nDeskStation=%d, userID = %d,isRobot=%d]", nDeskStation, userInfo._dwUserID,userInfo._bIsRobot);
		return -1;
	}
	/// ���������Ϸ�У����Զ��ߴ���
	if (m_nGameStation > GS_WAIT_ARGEE && m_nGameStation < GS_WAIT_NEXT)
	{
		MYLOG_DEBUG(logger,"[CGameDesk::UserLeftDesk nDeskStation in game palying]: [RoomID=%d,deskID = %d, station = %d, userID=%d]",GetRoomID(),m_nDeskIndex,nDeskStation,userInfo._dwUserID);

		return -2;
	}
	if (0 == userInfo._dwUserID)
	{
		MYLOG_DEBUG(logger,"CGameDesk::UserLeftDesk 0 == userInfo._dwUserID :[RoomID=%d,deskID = %d, station = %d, userID=%d]",GetRoomID(),m_nDeskIndex,nDeskStation,userInfo._dwUserID);
	}

	PlayerInfo* pPlayer = GetPlayerInfo(nDeskStation);

	/// ��Ϸ���ڽ����У�������������
	if (NULL != pPlayer && pPlayer->GetUserID() == userInfo._dwUserID)
	{
		if (IsGoldRoom())
		{
			KillGameTimer(TIME_PLAYER_1);
			SetGameTimer(TIME_PLAYER_1, 1000);
		}

		MYLOG_DEBUG(logger,"CGameDesk::UserLeftDesk nDeskStation  clear data: [RoomID=%d,deskID = %d, station = %d, userID=%d]",GetRoomID(),m_nDeskIndex,nDeskStation,userInfo._dwUserID);

		pPlayer->ResetUserData();

		//m_iNtStation = -1; //ׯ�������³�ȡ
		/// �ж��Ƿ������˶�ͬ����
		int nAgreeCount  = GetTotalAgreeCount();
		int iCountPlayer = GetTotalPlayerCount();

		/// ��������˶�ͬ�⿪ʼ������Ϸ��ʼ
		if (nAgreeCount >= CONFIG_PALYER_COUNT && nAgreeCount == iCountPlayer)
		{
			GameBegin(0);
		}
	}
	else
	{
		MYLOG_DEBUG(logger,"CGameDesk::UserLeftDesk nDeskStation other user: [RoomID=%d,deskID = %d, station = %d, userID=%d]",GetRoomID(),m_nDeskIndex,nDeskStation,userInfo._dwUserID);

		return -3;
	}

	return 0;
}
int CGameDesk::UpdateUserInfo(int nDeskStation, UserInfoForGame_t& userInfo)
{
	if (nDeskStation < 0 || nDeskStation >= CONFIG_PALYER_COUNT)
	{
		MYLOG_ERROR(logger, "[CGameDesk::UpdateUserInfo nDeskStation  error] [nDeskStation=%d, userID = %d]", nDeskStation, userInfo._dwUserID);
		return 0;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(nDeskStation);

	if (NULL != pPlayer && pPlayer->GetUserID() == userInfo._dwUserID)
	{
		pPlayer->SetUserMoney(userInfo._iUserMoney);
	}
	return 0;
}
/// ���ͬ����Ϸ
int CGameDesk::UserAgreeGame(int nDeskStation, UserInfoForGame_t& userInfo)
{
	if ( (nDeskStation < 0) || (nDeskStation > CONFIG_PALYER_COUNT))
	{
		MYLOG_ERROR(logger, "[CGameDesk::UserAgreeGame nDeskStation  error] [roomID=%d,DeskNo=%d,nDeskStation=%d, userID = %d ,isRobot=%d]",GetRoomID(),GetTableID(), nDeskStation, userInfo._dwUserID,userInfo._bIsRobot);
		return -1;
	}
	/// ������ǵȴ�ͬ��״̬���򷵻�ʧ��
	if (m_nGameStation != GS_WAIT_ARGEE && m_nGameStation != GS_WAIT_NEXT)
	{
		//ErrorPrintf("���ǵȴ�ͬ��״̬ʱ���յ�ͬ����Ϣ");
		MYLOG_ERROR(logger,"CGameDesk::UserAgreeGame UserAgreeGame fail..not wait agree status");
		return -2;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(nDeskStation);

	MYLOG_DEBUG(logger, "[CGameDesk::UserAgreeGame nDeskStation ..] [roomID=%d,DeskNo=%d,nDeskStation=%d, userID = %d]", GetRoomID(),GetTableID(),nDeskStation, userInfo._dwUserID);

	if (NULL == pPlayer)
	{
		MYLOG_ERROR(logger,"CGameDesk::UserAgreeGame UserAgreeGame NULL == pPlayer.. [roomID=%d,DeskNo=%d,nDeskStation=%d, userID = %d]", GetRoomID(),GetTableID(),nDeskStation, userInfo._dwUserID);
		return 0;
	}

	// ����û�������
	if (m_nRoomCardPlaform == E_ENTITY_ROOM_TYPE_COIN)
	{
		if (pPlayer->GetUserMoney() < m_llLessTakeMoney)
		{
			MYLOG_ERROR(logger, "UserAgreeGame UserID=%d money=%ld, m_llLessTakeMoney=%ld", pPlayer->GetUserID(), pPlayer->GetUserMoney(), m_llLessTakeMoney);
			KickUserOutTable(nDeskStation, E_ERROR_USER_NOT_ENOUGH_MONEY_2_PLAY);
			return E_ERROR_USER_NOT_ENOUGH_MONEY_2_PLAY;
		}
	}

	pPlayer->SetIsAgree(true);

	//ɱ���ȴ����׼����ʱ��
	KillGameTimer(TIME_PLAYER_1+nDeskStation);

	//֪ͨ������ ���Ѿ�ͬ��
	MSG_GR_R_UserAgree UserAgree;
	UserAgree.bAgreeGame=TRUE;
	UserAgree.bDeskNO=GetTableID();
	UserAgree.bDeskStation=nDeskStation; 
	for (int i =0; i < CONFIG_PALYER_COUNT; i++)
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(i);
		if (NULL == pTempPlayer)
		{
			continue;
		}
		SendGameData( i, ASS_GM_AGREE_GAME, (BYTE*)&UserAgree, sizeof(UserAgree));
	}
	SendWatchData( CONFIG_PALYER_COUNT, ASS_GM_AGREE_GAME, (BYTE*)&UserAgree, sizeof(UserAgree));
	//end

	//���Ǯ�Ƿ�
	CheckUserMoneyLeftGame();

	/// �ж��Ƿ������˶�ͬ����
	int nAgreeCount = GetTotalAgreeCount();
	int iCountPlayer = GetTotalPlayerCount();

	MYLOG_DEBUG(logger, "[CGameDesk::UserAgreeGame nDeskStation 222..] [roomID=%d,DeskNo=%d,nDeskStation=%d, userID = %d ,nAgreeCount=%d , iCountPlayer=%d , CONFIG_PALYER_COUNT=%d]", GetRoomID(),GetTableID()
		,nDeskStation, userInfo._dwUserID,nAgreeCount,iCountPlayer,CONFIG_PALYER_COUNT);

	/// ��������˶�ͬ�⿪ʼ������Ϸ��ʼ
	if (nAgreeCount >= CONFIG_PALYER_COUNT && nAgreeCount == iCountPlayer)
	{
		GameBegin(0);
	}
	return 0;
}

/// ��Ϸ�Ƿ��ڽ�����
bool CGameDesk::IsPlayingByGameStation(void)
{
	bool bIsPlaying = (m_nGameStation > GS_WAIT_ARGEE && m_nGameStation < GS_WAIT_NEXT);

	if (bIsPlaying)
	{
		if (GetTotalPlayerCount() <= 0)
		{
			m_bPlaying = false;
			m_nGameStation = GS_WAIT_ARGEE;
		}
	}
	return bIsPlaying;
}
/// ĳ��������Ƿ�����Ϸ��
/// ��ע����Ϸ�����������Ϸ��δ��ע���мң���Ϊ������Ϸ��
bool CGameDesk::IsPlayGame(int nDeskStation)
{
	bool bIsPlaying = (m_nGameStation > GS_WAIT_ARGEE && m_nGameStation < GS_WAIT_NEXT);

	if (bIsPlaying)
	{
		if (GetTotalPlayerCount() <= 0)
		{
			m_bPlaying = false;
			m_nGameStation = GS_WAIT_ARGEE;
		}
	}
	return bIsPlaying;
}

// ����Ƿ��������(������Ϸ�У�ĳЩ��Ϸ�������ʱ���ǲ��������µ�)
bool CGameDesk::CanSit2Desk()
{
	return !IsPlayingByGameStation();
}

//�Ƿ�Ϊ������Ϸ
bool CGameDesk::IsChessCard()
{ 
	return true;
}

/// ��ʼ������
int CGameDesk::InitialDesk(unsigned int unRoomID, int nDeskNo, int nMaxPeople, IGameMainManager* pIMainManager, unsigned int unExchangeRate)
{
	///// �ж���������Ƿ񳬳���Χ
	//if (nMaxPeople > CONFIG_PALYER_COUNT)
	//{
	//	//ErrorPrintf("������������Ϸ�����������");
	//	MYLOG_ERROR(logger,"Error: desk people count not equal max count.[nMaxPeople:%d,CONFIG_PALYER_COUNT:%d]", nMaxPeople , CONFIG_PALYER_COUNT);
	//	return -1;
	//}

	// ��ȡ������Ϣ
	const ManageInfoStruct* pDeskInfoData = pIMainManager->GetRoomInfo(unRoomID);
	if (NULL == pDeskInfoData)
	{
		MYLOG_ERROR(logger, "Error: CGameDesk::InitialDesk pIMainManager->GetRoomInfo error. [roomID=%d,DeskNo=%d]", unRoomID,nDeskNo);
		return -2;
	}

	// 3����ʼ���ݿ��ȡ
	int iRet = CDBConfigMgr::Instance()->Init(pIMainManager);
	if ( iRet < 0 )
	{
		MYLOG_ERROR(logger, "Error:CGameDesk::InitialDesk() CDBConfigMgr::Instance()->Init fail..[iRet=%d]", iRet);
		return -5;
	}

	iRet = CDBConfigMgr::Instance()->GetTimeConfig(unRoomID,m_nReadyTime, m_nOperateTime, m_nWaitForFullTime);
	if ( iRet < 0 )
	{
		MYLOG_ERROR(logger, "Error:CGameDesk::InitialDesk() CDBConfigMgr::Instance()->GetTimeConfig fail..[iRet=%d] m_nRoomID=%d", iRet, unRoomID);
		return -6;
	}

	m_nRoomLevel = pDeskInfoData->unRoomLevel;
	m_nRoomID    = unRoomID;
	m_nDeskIndex = nDeskNo;
	m_pGameManager = pIMainManager;
	memcpy(&m_ManageInfoStruct,pDeskInfoData,sizeof(m_ManageInfoStruct));

	//��ȡ�ȼ���Ϣ
	T_LevelDef*  pLevelDef = m_pGameManager->GetLevelInfo(m_nRoomLevel);
	if ( NULL != pLevelDef )
	{
		m_un64RoomLevelMinMoney = pLevelDef->m_un64MinMoney;
		MYLOG_DEBUG(logger,"CGameDesk::InitialDesk  m_pGameManager->GetLevelInfo..[RoomID=%d,DeskID=%d,m_nRoomLevel=%d,m_un64RoomLevelMinMoney=%lld]",GetRoomID(),GetTableID(),m_nRoomLevel,m_un64RoomLevelMinMoney);
	}
	else
	{
		MYLOG_ERROR(logger,"ERROR:CGameDesk::InitialDesk m_pGameManager->GetLevelInfo == NULL..[RoomID=%d,DeskID=%d,m_nRoomLevel=%d]",GetRoomID(),GetTableID(),m_nRoomLevel);
	}

	CConfigMgr::Instance()->InitRoom(m_nRoomID,m_nRoomLevel);

	//��ʼ������
	m_iCfgZhuaNiaoCount = CONFIG.m_iCfgZhuaNiaoCount;
	m_bCfgCanDianPaoHu  = CONFIG.m_bCfgCanDianPaoHu;
	m_bCfgZhuangXianScore = CONFIG.m_bCfgZhuangXianScore;
	m_bCfgHuQiDui       = CONFIG.m_bCfgHuQiDui;
	m_bCfgHongZhong     = CONFIG.m_bCfgHongZhong;
	m_byCfgLaiziCard    = (m_bCfgHongZhong ? MJ_TYPE_WIND_MID:0);
	ResetPlayerCount(CONFIG.m_iCfgGamePlayerCount);

	MYLOG_DEBUG(logger,"CGameDesk::InitialDesk m_byCfgLaiziCard=%d RoomID=%d DeskID=%d", m_byCfgLaiziCard, m_nRoomID, m_nDeskIndex);

	return 0;
}
///������Ϸ�û���Ϸ��ǿ��
int CGameDesk::ForceQuit()
{
	if ( IsPlayingByGameStation() )
	{
		MYLOG_INFO(logger,"CGameDesk::ForceQuit IsPlayingByGameStation = true..[RoomID=%d,DeskID=%d,m_nRoomLevel=%d]",GetRoomID(),GetTableID(),m_nRoomLevel);

		m_nGameStation = GS_WAIT_ARGEE;
		KillAllTimer();

		ReSetGameState(0);

		for(int i = 0;i < CONFIG_PALYER_COUNT; i ++)
		{
			PlayerInfo* pPlayer = GetPlayerInfo(i);
			if ( NULL != pPlayer )
			{
				pPlayer->ResetUserData();
			}
		}

		m_pGameManager->OnGameFinish(GetRoomID(),m_nDeskIndex);
	}
	else
	{
		MYLOG_INFO(logger,"CGameDesk::ForceQuit IsPlayingByGameStation = false..[RoomID=%d,DeskID=%d,m_nRoomLevel=%d]",GetRoomID(),GetTableID(),m_nRoomLevel);

		m_nGameStation = GS_WAIT_ARGEE;
		KillAllTimer();

		ReSetGameState(0);

		for(int i = 0;i < CONFIG_PALYER_COUNT; i ++)
		{
			PlayerInfo* pPlayer = GetPlayerInfo(i);
			if ( NULL != pPlayer )
			{
				pPlayer->ResetUserData();
			}
		}
	}
	return 0;
}

//�����м�ʱ��
void CGameDesk::KillAllTimer()
{
	for (int i=TIME_PLAYER_1; i<TIME_MAX_ID; i++)
	{
		KillGameTimer(i);
	}
}

//������Ϸ״̬
bool CGameDesk::ReSetGameState(BYTE bLastStation)
{
	KillAllTimer();

	m_bPlaying = false;
	for(int i = 0;i < CONFIG_PALYER_COUNT; i ++)
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if ( NULL != pPlayer )
		{
			pPlayer->SetIsAgree(false);
			pPlayer->SetIsNetCut(false);
		}

		m_bPlayerTuoGuan[i] = false;
		m_bPlayerTimerRunning[i] = false;
		m_nPlayerTimeOutCount[i] = 0;
	}

	//m_nSaveAutoHuChairID = -1;

	return TRUE;
}
//��Ϸ��ʼ
bool CGameDesk::GameBegin(BYTE bBeginFlag)
{
	if (m_nGameStation > GS_WAIT_ARGEE && m_nGameStation < GS_WAIT_NEXT )
	{
		MYLOG_ERROR(logger,"Error: CGameDesk::GameBegin m_nGameStation error..[RoomID=%d,DeskID=%d,m_nGameStation=%d]",GetRoomID(),GetTableID(),m_nGameStation);
		return TRUE;
	}

	KillAllTimer();
	MYLOG_INFO(logger,"CGameDesk::GameBegin..[RoomID=%d,DeskID=%d] m_byCfgLaiziCard=%d", GetRoomID(), GetTableID(), m_byCfgLaiziCard);

	//��������
	for (int i=0;i<CONFIG_PALYER_COUNT;i++)
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if ( NULL == pPlayer )
		{
			continue;
		}

		pPlayer->GameBeginClear();
	}
	
	//������Ϸ������ʱ��
	SetGameTimer(TIME_GAME_BASE_TIME,500);

	m_pGameManager->OnGameBegin(GetRoomID(),m_nDeskIndex);    ///��Ϸ��ʼ
	m_dwBeginTime=(long int)time(NULL);
	memset(&m_GameEndInfo,0,sizeof(m_GameEndInfo));
	memset(m_arrSaveDetailScore,0,sizeof(m_arrSaveDetailScore));
	memset(m_arrSaveDetailCount,0,sizeof(m_arrSaveDetailCount));
	m_iCurRound++;

	//////////////////////////////////////////////////////////////////////////
	//ׯ��
	if ( m_iNextNtStation >= 0 )
	{
		m_iNtStation = m_iNextNtStation;
		m_iNextNtStation = -1;
	}

	if ( m_iNtStation < 0 )
	{
		//��ϷΪ����������ʱ
		if (m_pGameManager->IsRoomCardPlaform() == 1)
		{
			for (int i=0;i<CONFIG_PALYER_COUNT;i++)
			{
				PlayerInfo* pPlayer = GetPlayerInfo(i);
				if ( NULL == pPlayer )
				{
					continue;
				}

				if( pPlayer->GetUserID() == m_CardRoomConfig.m_unOwnerID )
				{
					m_iNtStation = i;
					break;
				}
			}
			
			if ( m_iNtStation < 0 )
			{
				//���ׯ
				m_iNtStation = CRand::Rand(CONFIG_PALYER_COUNT);
			}
		}
		else
		{
			//���ׯ
			m_iNtStation = CRand::Rand(CONFIG_PALYER_COUNT);
		}

		PlayerInfo* pTempPlayer = GetPlayerInfo(m_iNtStation);
		if ( NULL != pTempPlayer )
		{
			m_dwNtUserID = pTempPlayer->GetUserID();
		}
	}
	else
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(m_iNtStation);
		if ( NULL != pTempPlayer )
		{
			if ( m_dwNtUserID != pTempPlayer->GetUserID() )
			{
				//���ׯ
				m_iNtStation = CRand::Rand(CONFIG_PALYER_COUNT);
			}
		}

		pTempPlayer = GetPlayerInfo(m_iNtStation);
		if ( NULL != pTempPlayer )
		{
			m_dwNtUserID = pTempPlayer->GetUserID();
		}
	}

	//////////////////////////////////////////////////////////////////////////

	//ϴ��
	m_iTotalMjCardCount = 0;
	CMJAlogrithm::InitAllCard(m_byAllCardList,MAX_MJ_TOTAL_COUTN,CONFIG_PALYER_COUNT,GetCfgLaiziCard(),m_iTotalMjCardCount);
	CMJAlogrithm::RandAllCard(m_byAllCardList,m_iTotalMjCardCount);

	/*///
	//���Ʋ��� ���� ����
 	BYTE byArrFixedCards[112] = 
 	{
		0x19, 0x18, 
		0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x08,
		0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x08,
		0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x08,
		0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x08,
		0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x08,
		0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
		0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x12, 0x11, 0x11,							//72

		0x12, 0x12, 0x22, 0x22, 0x13, 0x13, 0x23, 0x23, 0x27, 0x27, 0x27, 0x11, 0x21,		//13
		0x11, 0x11, 0x21, 0x21, 0x14, 0x14, 0x12, 0x12, 0x17, 0x17, 0x19, 0x19, 0x08,		//13
		0x41, 0x41, 0x41, 0x41, 0x02, 0x05, 0x09, 0x12, 0x15, 0x19, 0x11, 0x24, 0x27, 0x29	//14
 	};
	memcpy(m_byAllCardList, byArrFixedCards, sizeof(m_byAllCardList));
	///*/

	/*///
	//���Ʋ��� ���� ����
 	BYTE byArrFixedCards[112] = 
 	{
		0x08, 0x08, 
		0x08, 0x08, 0x18, 0x18, 0x08, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,							//72

		0x01, 0x01, 0x02, 0x02, 0x02, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x06, 0x07,		//13
		0x01, 0x01, 0x02, 0x02, 0x02, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x06, 0x07,		//13
		0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x06, 0x07	//14
 	};
	memcpy(m_byAllCardList, byArrFixedCards, sizeof(m_byAllCardList));
	///*/

	/*///
	//���Ʋ��� ���ܺ�
 	BYTE byArrFixedCards[112] = 
 	{
		0x08, 0x08, 
		0x08, 0x08, 0x18, 0x18, 0x08, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
		0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x08, 0x08, 0x08, 0x08,							//72

		0x01, 0x01, 0x02, 0x02, 0x02, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x06, 0x07,		//13
		0x08, 0x08, 0x02, 0x02, 0x02, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x06, 0x07,		//13
		0x01, 0x01, 0x01, 0x08, 0x02, 0x05, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x06, 0x07	//14
 	};
	memcpy(m_byAllCardList, byArrFixedCards, sizeof(m_byAllCardList));
	///*/

	///////////////////////////////////////
	//����
	// ·��
	char szPath[MAX_PATH]={0};
	GetCurrentDirectory(MAX_PATH, szPath);

	char szFileName[200];
	sprintf( szFileName, "%s/cshz.txt", szPath);
	FILE * file = NULL;
	file = fopen(szFileName, "r");

	vector<byte> vecCards;

	if (file != NULL)
	{	
		byte byCard = 0;
		while (EOF != fscanf(file, "%x", &byCard))
		{	
			vecCards.push_back(byCard);
		}
		fclose(file);
	}	
	else
	{
		MYLOG_ERROR(logger, "LoadAllFinalCards file notExist =%s", szFileName);
	}

	int nSize = vecCards.size();
	if ( nSize > 0)
	{
		for ( int i = 0; i < nSize && i < MAX_MJ_TOTAL_COUTN; i++ )
		{
			m_byAllCardList[i] = vecCards[i];
		}
	}
	///////////////////////////////////////*/

	m_iLeftMjCount = m_iTotalMjCardCount;

	//Ͷ����
	OnCastDice();

	//����
	OnFaPai();

	//��Ϸ��
	m_nGameStation = GS_PLAY_GAME;
	SetCurOperateStation(m_iNtStation);
	SetCurSeatOpStatus(TSEAT_STATUS_OP_ING);

	//��⵱ǰ����ܽ��еĲ���
	KillGameTimer(TIME_SEND_CARD_FINISH);
	SetGameTimer(TIME_SEND_CARD_FINISH, 3000);

	return true;
}

BYTE CGameDesk::GetNextDeskStation(BYTE bDeskStation)
{
	BYTE bNextStation = bDeskStation;
	for (int i =0; i < CONFIG_PALYER_COUNT; i++)
	{
		bNextStation = (bNextStation + 1 ) % CONFIG_PALYER_COUNT;//˳ʱ��
		PlayerInfo* pPlayer = GetPlayerInfo(bNextStation);
		if (NULL == pPlayer)
		{
			continue;
		}
		break;
	}
	return bNextStation;
}

//��Ϸ����
bool CGameDesk::GameFinish(BYTE bDeskStation, BYTE bCloseFlag)
{
	//��д����
	switch (bCloseFlag)
	{
	case GF_NORMAL:		//��Ϸ��������
		{
			m_nGameStation = GS_WAIT_NEXT;
			KillAllTimer();
	//		if (m_iNtStation < 0 || m_iNtStation >= PLAY_COUNT)
	//		{
	//			GameFinish(0,GF_SAFE);
	//			return true;
	//		}

	//		MYLOG_INFO(logger, "CGameDesk::GameFinish Normal End..[RoomID=%d,DeskID=%d]",GetRoomID(),GetTableID());

	//		///��Ӯ�ж�
	//		GameCutStruct GameEnd;
	//		memset(&GameEnd, 0, sizeof(GameEnd)); 
	//		int nPoint[PLAY_COUNT]={0};
	//		int nNoteMoney[PLAY_COUNT]={0};
	//		//int nRobotPoint[PLAY_COUNT]={0};//��¼ �����˺����֮�����Ӯ

	//		memcpy(nNoteMoney,m_iXiaZhuNum,sizeof(nNoteMoney));
	//		GameIntoBalance(GameEnd/*,nRobotPoint*/);


	//		for(int i = 0; i < PLAY_COUNT; i++)
	//		{
	//			if (0 == m_arUserInfo[i]._dwUserID)
	//			{
	//				continue;
	//			}

	//			m_arUserInfo[i]._nPoint = GameEnd.iTurePoint[i];

	//			// ������ʱȥ�� [8/25/2016 Administrator]
	//			//m_arUserInfo[i]._dUserType =0;
	//			if (m_arUserInfo[i]._nPoint >0)
	//				m_arUserInfo[i]._dWinLost=1;
	//			else if (m_arUserInfo[i]._nPoint == 0)
	//				m_arUserInfo[i]._dWinLost=2;
	//			else if(m_arUserInfo[i]._nPoint < 0)
	//				m_arUserInfo[i]._dWinLost=3;

	//			nPoint[i] = m_arUserInfo[i]._nPoint;

	//			//��ʱ�� ��ֹ����
	//			SetGameTimer(TIME_WAITUSER_AGREE + i,(CONFIG.m_iBeginTime + 5)*1000 );
	//		}

	//		//////////////////////////////////////////////////////////////////////////
	//		SendUserResultInfo(); //������������ϸ��Ϣ
	//		//////////////////////////////////////////////////////////////////////////
	//		for(int i = 0; i < PLAY_COUNT; i++)
	//		{
	//			if (0 == m_arUserInfo[i]._dwUserID)
	//			{
	//				continue;
	//			}
	//			//�ж��Ƿ������ţ9����
	//			if (UG_BULL_NINE <= m_Logic.GetCardShape(m_iUserCard[i],m_iUserCardCount[i]))
	//			{
	//				m_iNtStation = -1; //ׯ�����·���
	//				break;
	//			}
	//		}

	//		////д���ݿ�
	//		//m_pGameManager->ChangeUserPoint(resInfo, m_arUserInfo);
	//		// ���� [8/25/2016 jimy.zhu]
 //			for (int i =0; i < PLAY_COUNT; i++)
 //			{
 //				if (0 == m_arUserInfo[i]._dwUserID)
 //				{
 //					continue;
 //				}

	//			// �仯��Ǯ
	//			long long n64MoneyChange = (long long)m_arUserInfo[i]._nPoint - m_pGameManager->GetTaxConfig();  // ���Ͽ�̨�� [9/7/2016 jimy.zhu]

	//			m_arUserInfo[i]._iUserMoney += n64MoneyChange;

	//			//�����˲�����
	//			if ( !m_arUserInfo[i]._bIsRobot )
	//			{
	//				MYLOG_DEBUG(logger, "CGameDesk::GameFinish end jiesuan before: [room = %d,desk = %d , user = %d, %d ,  n64MoneyChange = %lld] , m_arUserInfo[i]._nPoint=%d , m_pGameManager->GetTaxConfig()=%d,GameEnd.iTurePoint[i]=%d",GetRoomID(),GetTableID()
	//					,i,m_arUserInfo[i]._dwUserID,n64MoneyChange,m_arUserInfo[i]._nPoint,m_pGameManager->GetTaxConfig(),GameEnd.iTurePoint[i]);

	//				// �仯������
	//				unsigned int unChageType = E_CHAGEMONEY_GAME_ADD;
	//				if (n64MoneyChange < 0)
	//				{
	//					n64MoneyChange= -n64MoneyChange;
	//					unChageType = E_CHAGEMONEY_GAME_REMOVE;
	//				}
	//				m_pGameManager->ModifyGameMoney(GAME_ID, GetRoomID(), GetTableID(), m_arUserInfo[i]._dwUserID, unChageType, n64MoneyChange);

	//				MYLOG_DEBUG(logger, "CGameDesk::GameFinish end jiesuan: [room = %d,desk = %d , user = %d, %d ,  n64MoneyChange = %lld]",GetRoomID(),GetTableID(),i,m_arUserInfo[i]._dwUserID,n64MoneyChange);
	//			}
 //			}
	//		// end ����

	//		RecordGameStaticData();

	//		///Ϊ�˽�����ʾ
	//		for (int i = 0; i < PLAY_COUNT; i++)
	//		{
	//			if (0 == m_arUserInfo[i]._dwUserID)
	//			{
	//				continue;
	//			}
	//			GameEnd.iCardType[i] = m_iCardType[i];
	//		}

	//		//////////////////////////////////////////////////////////////////////////
	//		//ת�����͵�����
	//		for (int i = 0; i < PLAY_COUNT; i ++)
	//		{
	//			MYLOG_DEBUG(logger, "CGameDesk::GameFinish end send jiesuan: [room = %d,desk = %d , user = %d, %d ,  GameEnd.iTurePoint[i] = %d,GameEnd.llXiaZhuCount[i]=%d],GameEnd.iCountCard[i]=%d,GameEnd.iCardType[i]=%d"
	//				,GetRoomID(),GetTableID(),i,m_arUserInfo[i]._dwUserID,GameEnd.iTurePoint[i],GameEnd.llXiaZhuCount[i],GameEnd.iCountCard[i],GameEnd.iCardType[i]);

	//			GameEnd.iTurePoint[i] = ntoh32(GameEnd.iTurePoint[i]);
	//			GameEnd.llXiaZhuCount[i] = ntoh32(GameEnd.llXiaZhuCount[i]);
	//			GameEnd.iCountCard[i] = ntoh32(GameEnd.iCountCard[i]);
	//			GameEnd.iCardType[i]  = ntoh32(GameEnd.iCardType[i]);
	//			GameEnd.llCurMoney[i] = ntoh64(m_arUserInfo[i]._iUserMoney);
	//		}
	//		//////////////////////////////////////////////////////////////////////////

	//		for (int i = 0; i < PLAY_COUNT; i ++)   
	//		{
	//			if (0 == m_arUserInfo[i]._dwUserID)
	//			{
	//				continue;
	//			}
	//			GameEnd.iFuWuFeiMoney = ntoh32(m_pGameManager->GetTaxConfig()/*m_arUserInfo[i]._iTax*/);/*m_pGameManager->GetTaxConfig()*/

	//			SendGameData( i, ASS_CONTINUE_END, (BYTE*)&GameEnd, sizeof(GameCutStruct));

	//			//zfy todo Ŀǰû��  [8/24/2016 Administrator]
	//			//m_pGameManager->OnNoticeGameOfWin(m_arUserInfo[i]._dwUserID, NAME_ID, GameEnd.iTurePoint[i] > 0, GameEnd.iTurePoint[i]);
	//		}
	//		SendWatchData( PLAY_COUNT, ASS_CONTINUE_END, (BYTE*)&GameEnd, sizeof(GameCutStruct));

	//		//����״̬ 
	//		m_nGameStation = GS_WAIT_NEXT;

	//		ReSetGameState(bCloseFlag);
	//		m_pGameManager->OnGameFinish(GetRoomID(),m_nDeskIndex);

	//		//////////////////////////////////////////////////////////////////////////
	//		//������Ҳٶ���Ѻ�ܵ�
	//		for(int i = 0; i < PLAY_COUNT; i++)
	//		{
	//			if (0 == m_arUserInfo[i]._dwUserID || m_arUserInfo[i]._bIsRobot)
	//			{
	//				continue;
	//			}

	//			if ( en_GMCtrlType_no != GetGMCurCtrlData(i).data.usControlType )
	//			{
	//				int iUserWin = nPoint[i]/* - nNoteMoney[i]*/;
	//				UpdateCurOperateWinListAddMoney(i,iUserWin);
	//			}
	//		}

	//		DelayedUserTuoGuan(5000);

	//		//��ʱ����뿪
	//		SetGameTimer(TIME_CHECK_END_MONEY,2000);
			return true;
		}
	case GF_SAFE:			//��Ϸ��ȫ����
		{
			m_nGameStation = GS_WAIT_ARGEE;
			KillAllTimer();

			MYLOG_INFO(logger, "CGameDesk::GameFinish GF_SAFE End..[RoomID=%d,DeskID=%d]",GetRoomID(),GetTableID());

			//if (bDeskStation == 255)
			//{
			//	bDeskStation = 0;
			//}
			////////////////////////////////////////////////////////////////////////////
			////��������
			//for (int i = 0; i < CONFIG_PALYER_COUNT; i ++)
			//{
			//	PlayerInfo* pPlayer = GetPlayerInfo(i);
			//	if (NULL != pPlayer)
			//	{
			//		pPlayer->SetIsAgree(false);
			//	}
			//}
			//GameCutStruct CutEnd;
			//::memset(&CutEnd,0,sizeof(CutEnd));

			////////////////////////////////////////////////////////////////////////////
			////ת�����͵�����
			//CutEnd.iFuWuFeiMoney = ntoh32(CutEnd.iFuWuFeiMoney);
			//for (int i = 0; i < PLAY_COUNT; i ++)
			//{
			//	CutEnd.iTurePoint[i] = ntoh32(CutEnd.iTurePoint[i]);
			//	CutEnd.llXiaZhuCount[i] = ntoh32(CutEnd.llXiaZhuCount[i]);
			//	CutEnd.iCountCard[i] = ntoh32(CutEnd.iCountCard[i]);
			//	CutEnd.iCardType[i] = ntoh32(CutEnd.iCardType[i]);
			//}
			////////////////////////////////////////////////////////////////////////////

			//for (int i = 0; i < PLAY_COUNT; i ++) 
			//	SendGameData( i, ASS_SAFE_END, (BYTE*)&CutEnd, sizeof(CutEnd));
			//SendWatchData( PLAY_COUNT, ASS_SAFE_END, (BYTE*)&CutEnd, sizeof(CutEnd));

			//bCloseFlag = GF_FORCE_QUIT;

			ReSetGameState(bCloseFlag);
			m_pGameManager->OnGameFinish(GetRoomID(),m_nDeskIndex);
			return true;
		}
	default:
		{
			MYLOG_ERROR(logger, "CGameDesk::GameFinish Error End..[RoomID=%d,DeskID=%d,bCloseFlag=%d]",GetRoomID(),GetTableID(),bCloseFlag);

			m_nGameStation = GS_WAIT_ARGEE;
			//ErrorPrintf("[%d]�����������Ϸ", m_nDeskIndex);

			return true;
		}

	}

	//��������
	ReSetGameState(bCloseFlag);
	m_pGameManager->OnGameFinish(GetRoomID(),m_nDeskIndex);

	return true;
}

//�����ı��û����������ĺ���
bool CGameDesk::CanNetCut(BYTE nDeskStation)
{
	return true;
}

int CGameDesk::LogMsg(const char* szBuf,int len)
{
	string msg;

	char buf[8];
	for (int j = 0; j < len; ++j)
	{
		snprintf(buf, sizeof(buf), " %X%X", (szBuf[j] >> 4) & 0x0F, szBuf[j]&0x0F);
		msg += buf;
	}

	MYLOG_DEBUG(logger, "LogMsg:size[%u]  msg[%s]\n", msg.size(), msg.c_str());
	return 0;
}

// ��������
int  CGameDesk::SendGameData(BYTE bDeskStation, UINT nSubCmdID, const BYTE* pMsgBuf, const int nMsgParaLen)
{
	if ( bDeskStation >= CONFIG_PALYER_COUNT )
	{
		MYLOG_ERROR(logger, "CGameDesk::SendGameData  bDeskStation Error! [bDeskStation=%d]",bDeskStation);
		return -1;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(bDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger, "CGameDesk::SendGameData  NULL == pPlayer! [bDeskStation=%d]",bDeskStation);
		return 0;
	}

	if ( pPlayer->GetUserID() == 0 )
	{
		MYLOG_ERROR(logger, "CGameDesk::SendGameData  _dwUserID==0 Error! [bDeskStation=%d,nSubCmdID=%d]",bDeskStation,nSubCmdID);
	}

	// msg head
	CMsgHead msgHead;
	msgHead.m_unMsgID   = MSG_ID_CS_GAME_111_LOGIC_GAME; 
	msgHead.m_usMsgType = Response;
	msgHead.m_usDstFE   = FE_QIPAI_WURENNIUNIU;
	msgHead.m_usSrcID   = GetRoomID();
	msgHead.m_usDstID   = GetTableID();

	// buff����
	const int NBUFF_LEN = 2048;
	char szMsgSendBuf[NBUFF_LEN] = {0};

	int nOffSet = 0;
	NetMessageHead& sHead = *(NetMessageHead*)&szMsgSendBuf[nOffSet];
	sHead.uMainID      = ntoh32(MDM_GM_GAME_NOTIFY);
	sHead.uAssistantID = ntoh32(nSubCmdID);
	sHead.uHandleCode  = ntoh32(0);
	nOffSet += sizeof(NetMessageHead);

	// ����ԭ�����ݰ���С
	if (0 < nMsgParaLen && NULL != pMsgBuf)
	{
		char* pBuffer = &szMsgSendBuf[nOffSet];
		memcpy(pBuffer, pMsgBuf, nMsgParaLen);
		nOffSet += nMsgParaLen;
	}

	sHead.uMessageSize = ntoh32(nOffSet);
	m_pGameManager->SendMsgToClient(pPlayer->GetUserID(), &msgHead, szMsgSendBuf, nOffSet);

	return 0;
}

// ���͹۲�����
int CGameDesk::SendWatchData(BYTE bDeskStation, UINT nSubCmdID, const BYTE* pMsgBuf, const int nMsgParaLen)
{
	return 0;
}

// �������ӹ㲥����
int CGameDesk::SendDeskBroadCast(UINT nSubCmdID, const BYTE* pMsgBuf, const int nMsgParaLen)
{
	for (int i=0;i<CONFIG_PALYER_COUNT;i++)
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if ( NULL == pPlayer )
		{
			continue;
		}

		SendGameData(i,nSubCmdID,pMsgBuf,nMsgParaLen);
	}
	SendWatchData(CONFIG_PALYER_COUNT,nSubCmdID,pMsgBuf,nMsgParaLen);
	return 0;
}

//------------------------------------------------------------------------------------------------------------
int CGameDesk::GetVersion(char* pVersion)
{
	//if (pVersion == NULL)
	//{
	//	return -1;
	//}

	std::cout<<MY_VERSION_ADDR<<std::endl;
	std::cout<<MY_FRAME_VERSION<<std::endl;
	std::cout<<__DATE__<<"|"<<__TIME__<<std::endl;

	//printf("so version addr:[%s]  version no:[%s]  time[%s %s]",MY_VERSION_ADDR,MY_FRAME_VERSION, __DATE__, __TIME__);


	return 0;

}

//����GM������Ϣ
int CGameDesk::SetGameMangerInfo(void *pGameManagerData, unsigned short unGMType)
{
	T_GameManagerData* pRefGameManagerData = (T_GameManagerData*)pGameManagerData;


	return 0;
}

//�߳����
int  CGameDesk::DoUserLeftDeskOp(int nDeskStation,int iCode)
{
	if ( (nDeskStation<0) || (nDeskStation>=CONFIG_PALYER_COUNT))
	{
		MYLOG_ERROR(logger, "[CGameDesk::DoUserLeftDeskOp UserLeftDesk nDeskStation  error] [nDeskStation=%d, iCode=%d]", nDeskStation, iCode);
		return -1;
	}

	PlayerInfo* pPlayer = GetPlayerInfo(nDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger, "[CGameDesk::DoUserLeftDeskOp UserLeftDesk.. NULL == pPlayer] [nDeskStation=%d, iCode=%d]", nDeskStation, iCode);
		return -1;
	}

	/// ���������Ϸ�У����Զ��ߴ���
	if (m_nGameStation > GS_WAIT_ARGEE && m_nGameStation < GS_WAIT_NEXT)
	{
		MYLOG_ERROR(logger,"[CGameDesk::DoUserLeftDeskOp UserLeftDesk nDeskStation in game palying]: [RoomID=%d,deskID = %d, station = %d, userID=%d,iCode=%d]"
				,GetRoomID(),m_nDeskIndex,nDeskStation,pPlayer->GetUserID(),iCode);
		return 0;
	}

	MYLOG_DEBUG(logger, "[CGameDesk::DoUserLeftDeskOp UserLeftDesk ..] [RoomID=%d,DeskID=%d,nDeskStation=%d , iCode=%d , UserID=%d]"
		,GetRoomID(), GetTableID(),nDeskStation, iCode,pPlayer->GetUserID());

	m_pGameManager->UserLeftDesk(pPlayer->GetUserID(),GetRoomID(),m_nDeskIndex,nDeskStation,iCode);
	return 0;
}

//������Ǯ�Ƿ�,������������߳�
int CGameDesk::CheckUserMoneyLeftGame()
{
	KillGameTimer(TIME_CHECK_END_MONEY);

	//��ϷΪ����������ʱ
	if (m_pGameManager->IsRoomCardPlaform() == 1)
	{
		return -1;
	}

	if (m_nGameStation > GS_WAIT_ARGEE && m_nGameStation < GS_WAIT_NEXT)
	{
		return -1;
	}

	for(int i = 0; i < CONFIG_PALYER_COUNT; i++)
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if (NULL == pPlayer || IsPlayGame(i))
		{
			continue;
		}

		if( pPlayer->GetUserMoney() <= m_un64RoomLevelMinMoney )
		{
			DoUserLeftDeskOp(i,E_ERROR_USER_NOT_ENOUGH_MONEY_2_PLAY);
		}
	}

	return 0;
}

//֪ͨ��Ҷ���
void CGameDesk::NotifyUserNetCut(int nDeskStation,bool bNetCut)
{
	//��Ϸ��ʼ
	MSG_NOTIFY_USER_NET_CUT sMsg;
	memset(&sMsg, 0, sizeof(sMsg));

	sMsg.byNetCutSeatNo = nDeskStation;
	sMsg.bIsNetCut      = bNetCut;

	SendDeskBroadCast(ASS_NOTIFY_USER_NET_CUT,(BYTE*)&sMsg, sizeof(sMsg));
}

//��ȡ���
PlayerInfo* CGameDesk::GetPlayerInfo(int iSeatNo)
{
	if ( iSeatNo<0 || iSeatNo>=m_vecPlayer.size() )
	{
		return NULL;
	}

	if( 0 == m_vecPlayer[iSeatNo]->GetUserID() )
	{
		return NULL;
	}

	return m_vecPlayer[iSeatNo];
}

//��ȡ������
int CGameDesk::GetTotalPlayerCount()
{
	int iCount = 0;
	for (int i=0; i<m_vecPlayer.size(); ++i)
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(i);
		if (NULL != pTempPlayer)
		{
			++iCount;
		}
	}
	return iCount;
}

//��ȡ��ͬ������
int CGameDesk::GetTotalAgreeCount()
{
	int nAgreeCount = 0;
	for (int i=0; i<m_vecPlayer.size(); ++i)
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(i);
		if (NULL != pTempPlayer && pTempPlayer->GetIsAgree())
		{
			++nAgreeCount;
		}
	}
	return nAgreeCount;
}

//��ȡ����������
int CGameDesk::GetTotalOnlineCount()
{
	int iOnlineCount = 0;
	for (int i=0; i<m_vecPlayer.size(); ++i)
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(i);
		if (NULL != pTempPlayer && !pTempPlayer->GetIsNetCut())
		{
			++iOnlineCount;
		}
	}
	return iOnlineCount;
}

//��������
int CGameDesk::ResetPlayerCount(int iCount)
{
	if ( iCount < 2 )
	{
		iCount = 2;
	}
	else if ( iCount > 4 )
	{
		iCount = 4;
	}

	for ( int i = 0 ; i < m_vecPlayer.size() ; i++ )
	{
		if ( NULL != m_vecPlayer[i] )
		{
			delete m_vecPlayer[i];
			m_vecPlayer[i] = NULL;
		}
	}

	m_vecPlayer.clear();

	for ( int i = 0 ; i < iCount ; i++ )
	{
		PlayerInfo* pPlayer = new PlayerInfo(&m_playerHuTips);
		m_vecPlayer.push_back(pPlayer);
		pPlayer->setNaiZiCard(0x41);	//��������
	}

	return 0;
}

//���ÿ������������� 
int CGameDesk::ResetCardRoomConfigInfo()
{
	m_nGameStation = GS_WAIT_ARGEE;
	m_iNtStation = -1;
	m_iNextNtStation = -1;
	m_dwNtUserID = 0;

	//��������һ�������µ�ʱ��  ��ȡ����
	T_CardRoomConfig* pCardRoomConfig = m_pGameManager->GetCardRoomInfo(GetRoomID(),GetTableID());
	if ( NULL == pCardRoomConfig )
	{
		MYLOG_DEBUG(logger,"CGameDesk::UserSitDesk  NULL == pCardRoomConfig !![RoomID=%d,deskID = %d]",GetRoomID(),GetTableID());
		return -1;
	}

	memcpy(&m_CardRoomConfig,pCardRoomConfig,sizeof(m_CardRoomConfig));

	//����
	ResetPlayerCount(pCardRoomConfig->m_usMaxUserNum);

	//����
	m_iTotalRound = pCardRoomConfig->m_usTotalNum;
	m_iCurRound   = pCardRoomConfig->m_usUsedNum;

	//��ѡ1
	//���ں�
	m_bCfgCanDianPaoHu = true;

	//ׯ�����
	if ( pCardRoomConfig->m_unOpt1&E_MJ_OP1_TYPE_ZHUANGXIAN )
	{
		m_bCfgZhuangXianScore = true;
	}
	else
	{
		m_bCfgZhuangXianScore = false;
	}

	//�ɺ��߶�
	if ( pCardRoomConfig->m_unOpt1&E_MJ_OP1_TYPE_HU_QIDUI )
	{
		m_bCfgHuQiDui = true;
		m_nPlayMode |= enPlayMode_7Dui;
	}
	else
	{
		m_bCfgHuQiDui = false;
	}

	//��ѡ2
	//���󲹸�
	if ( pCardRoomConfig->m_unOpt2&E_CSHZ_MJ_OP2_GANG_1 )
	{
		m_bCfgGuoHouBuGang = true;
	}
	else
	{
		m_bCfgGuoHouBuGang = false;
	}
	//�������
	if ( pCardRoomConfig->m_unOpt2&E_CSHZ_MJ_OP2_GANG_2 )
	{
		m_bCfgXianPengHouGang = true;
	}
	else
	{
		m_bCfgXianPengHouGang = false;
	}

	//��ѡ2
	if ( E_CSHZ_MJ_OP3_HU_NIAO_1 == pCardRoomConfig->m_unOpt3 )
	{
		m_byAddNiaoCount = 1;
	}
	else if ( E_CSHZ_MJ_OP3_HU_NIAO_2 == pCardRoomConfig->m_unOpt3 )
	{
		m_byAddNiaoCount = 2;
	}

	//��ѡ4
	//��
	m_iCfgZhuaNiaoCount = 0;
	if ( E_CSHZ_MJ_OP4_NIAO2 == pCardRoomConfig->m_unOpt4 )
	{
		m_iCfgZhuaNiaoCount = 2;
	}
	else if ( E_CSHZ_MJ_OP4_NIAO3 == pCardRoomConfig->m_unOpt4 )
	{
		m_iCfgZhuaNiaoCount = 3;
	}
	else if ( E_CSHZ_MJ_OP4_NIAO4 == pCardRoomConfig->m_unOpt4 )
	{
		m_iCfgZhuaNiaoCount = 4;
	}
	else if ( E_CSHZ_MJ_OP4_NIAO6 == pCardRoomConfig->m_unOpt4 )
	{
		m_iCfgZhuaNiaoCount = 6;
	}

	MYLOG_INFO(logger, "CGameDesk::ResetCardRoomConfigInfo %d %d %d %d",  pCardRoomConfig->m_unOpt1,  pCardRoomConfig->m_unOpt2,  pCardRoomConfig->m_unOpt3,  pCardRoomConfig->m_unOpt4);
	MYLOG_INFO(logger, "CGameDesk::ResetCardRoomConfigInfo m_bCfgZhuangXianScore=%d m_bCfgHuQiDui=%d m_bCfgGuoHouBuGang=%d m_bCfgXianPengHouGang=%d m_byAddNiaoCount=%d m_iCfgZhuaNiaoCount=%d",
		m_bCfgZhuangXianScore,  m_bCfgHuQiDui,  m_bCfgGuoHouBuGang,  m_bCfgXianPengHouGang, m_byAddNiaoCount, m_iCfgZhuaNiaoCount);

	//��������
	m_bCfgHongZhong = true;
	SetCfgLaiziCard(MJ_TYPE_WIND_MID);
	
	m_nPlayMode |= enPlayMode_NaiZi;
	m_playerHuTips.setPlayMode(m_nPlayMode);	

	// ��ҳ�֮��ʹ�õ���չ����
	T_OPT_INFO_TYPE* pOptInfoType = (T_OPT_INFO_TYPE*)pCardRoomConfig->m_szOpt; 
	T_OPT_INFO_CARDROOM* pOptInfoCardRoom = (T_OPT_INFO_CARDROOM*)pOptInfoType->szOpt;

	//��������:E_ENTITY_ROOM_TYPE����0:��ͨ;1:����;2:��ҳ�;3:������
	m_nRoomCardPlaform = pOptInfoCardRoom->unType;
	m_llBaseMoney      = pOptInfoCardRoom->unBaseMoney;
	m_llLessTakeMoney  = pOptInfoCardRoom->unLessTake;
	m_nTax             = pOptInfoCardRoom->usFlag3;

	if (!IsGoldRoom())
	{
		m_llBaseMoney = 1;
	}

	MYLOG_DEBUG(logger, "@@@@@@HZMJ HongZhongMaJiang@@@@@@ GoldRoomConfig, m_nRoomCardPlaform=%d, m_llLessTakeMoney=%ld m_llBaseMoney=%ld m_nTax=%d", m_nRoomCardPlaform, m_llLessTakeMoney, m_llBaseMoney, m_nTax);

	return 0;
}

//ɾ���齫����
int CGameDesk::DeleteMJDCard(bool bZhuaNiao, std::list<BYTE>& lsOutDeleteMj,int iCount/*=1*/,bool bFront/*=true*/)
{
	lsOutDeleteMj.clear();

	if ( iCount <= 0 )
	{
		return -1;
	}

	if ( iCount > m_iLeftMjCount )
	{
		return -2;
	}

	//Ҫ�����㹻������ץ���ƣ���ʣ����=ץ����������ׯ
	int nZhuaNiaoRemian = m_iCfgZhuaNiaoCount;
	if (bZhuaNiao)
	{
		nZhuaNiaoRemian = 0;
	}

	if ( (m_iLeftMjCount-nZhuaNiaoRemian) <= 0 )
	{
		return -3;
	}

	if ( bFront )
	{
		m_iStartIndex += iCount;
	}
	else
	{
		m_iEndIndex -= iCount;
		if (m_iEndIndex < 0)
		{
			m_iEndIndex = /*TOTAL_MJ_CARD_NUM*/m_iTotalMjCardCount - 1;
		}
	}

	for ( int i = 0 ; i < iCount && m_iLeftMjCount > 0 ; i++ )
	{
		lsOutDeleteMj.push_back(m_byAllCardList[m_iLeftMjCount-1]);

		m_iLeftMjCount--;
	}

	return 0;
}

//Ͷ����
void CGameDesk::OnCastDice()
{
	m_nGameStation = GS_NT_TOUZEZI;

	m_byDicePoint[0] = CMJAlogrithm::GetDiceVal();
	m_byDicePoint[1] = CMJAlogrithm::GetDiceVal();

	BYTE byToalPoint = (m_byDicePoint[0] + m_byDicePoint[1] - 1) % /*4*/CONFIG_PALYER_COUNT;
	m_byStartSeat  = (m_iNtStation - 1 + byToalPoint) % /*4*/CONFIG_PALYER_COUNT;
	m_byStartDun  = m_byDicePoint[0] + m_byDicePoint[1];

	//������ץ�Ƶ����к�
	m_iStartIndex = (m_byStartSeat) * (/*TOTAL_MJ_CARD_NUM*/m_iTotalMjCardCount / 4) + m_byStartDun * 2;
	if (m_iStartIndex >= /*TOTAL_MJ_CARD_NUM*/m_iTotalMjCardCount)
	{
		m_iStartIndex = 0;
	}
	m_iEndIndex = m_iStartIndex - 1;
	if (m_iEndIndex < 0)
	{
		m_iEndIndex = /*TOTAL_MJ_CARD_NUM*/m_iTotalMjCardCount - 1;
	}


	if (m_iNtStation == 0 || m_iNtStation == 2)
	{
		if (m_byStartSeat == 1)
		{
			m_byStartSeat = 3;
		}
		else if(m_byStartSeat == 3)
		{
			m_byStartSeat = 1;
		}	

	}

	if (m_iNtStation == 1 || m_iNtStation == 3)
	{
		BYTE byTempSeat = m_byStartSeat;
		if (m_byStartSeat == 1)
		{
			byTempSeat = 3;
		}
		else if(m_byStartSeat == 3)
		{
			byTempSeat = 1;
		}
		else if(m_byStartSeat == 0)
		{
			byTempSeat = 2;
		}	
		else if(m_byStartSeat == 2)
		{
			byTempSeat = 0;
		}	

		//������ץ�Ƶ����к�
		m_iStartIndex = (byTempSeat) * (/*TOTAL_MJ_CARD_NUM*/m_iTotalMjCardCount / 4) + m_byStartDun * 2;
		if (m_iStartIndex >= /*TOTAL_MJ_CARD_NUM*/m_iTotalMjCardCount)
		{
			m_iStartIndex = 0;
		}
		m_iEndIndex = m_iStartIndex - 1;
		if (m_iEndIndex < 0)
		{
			m_iEndIndex = /*TOTAL_MJ_CARD_NUM*/m_iTotalMjCardCount - 1;
		}

	}

	//��Ϸ��ʼ
	CMD_S_GameStart BeginMessage;
	memset(&BeginMessage, 0, sizeof(BeginMessage));

	BeginMessage.byBankerSeatNo = m_iNtStation;
	BeginMessage.iTotalRound    = ntoh32(m_iTotalRound);
	BeginMessage.iLeftRound     = ntoh32(m_iCurRound);

	BeginMessage.byDicePoint[0] = m_byDicePoint[0];
	BeginMessage.byDicePoint[1] = m_byDicePoint[1];

	BeginMessage.byStartSeat   = m_byStartSeat;
	BeginMessage.byStartPos    = m_byStartDun;
	BeginMessage.iLeftMjCount  = ntoh32(m_iLeftMjCount);

	SendDeskBroadCast(SUB_S_GAME_START,(BYTE*)&BeginMessage, sizeof(BeginMessage));
}

//����
void CGameDesk::OnFaPai()
{
	m_nGameStation = GS_SEND_CARD;

	for (int i=0;i<CONFIG_PALYER_COUNT;i++)
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if ( NULL == pPlayer )
		{
			continue;
		}

		int iFaPaiCount = 13;
		if ( i == m_iNtStation )
		{
			iFaPaiCount = 14;
		}

		std::list<BYTE> lsOutDeleteMj;
		int iRet = DeleteMJDCard(false, lsOutDeleteMj,iFaPaiCount,true);
		if ( iRet < 0 )
		{
			MYLOG_ERROR(logger,"[CGameDesk::OnFaPai DeleteMJDCard error]: [RoomID=%d,deskID = %d, station = %d, userID=%d,iFaPaiCount=%d,iRet=%d]"
				,GetRoomID(),m_nDeskIndex,i,pPlayer->GetUserID(),iFaPaiCount,iRet);

			continue;
		}

		pPlayer->SetHandMjCard(lsOutDeleteMj);
	}

	////////////////////////////////////////////////////////////////////////////
	////test ���� begin
	//for (int i=0;i<CONFIG_PALYER_COUNT;i++)
	//{
	//	PlayerInfo* pPlayer = GetPlayerInfo(i);
	//	if ( NULL == pPlayer )
	//	{
	//		continue;
	//	}

	//	std::list<BYTE> lsOutDeleteMj;
	//	int iFaPaiCount = 13;
	//	if ( i == m_iNtStation )
	//	{
	//		lsOutDeleteMj.push_back(0x01);
	//		lsOutDeleteMj.push_back(0x02);
	//		lsOutDeleteMj.push_back(0x03);

	//		lsOutDeleteMj.push_back(0x04);
	//		lsOutDeleteMj.push_back(0x05);
	//		lsOutDeleteMj.push_back(0x06);

	//		lsOutDeleteMj.push_back(0x07);
	//		lsOutDeleteMj.push_back(0x08);
	//		lsOutDeleteMj.push_back(0x09);

	//		lsOutDeleteMj.push_back(0x11);
	//		lsOutDeleteMj.push_back(0x11);

	//		lsOutDeleteMj.push_back(0x12);
	//		lsOutDeleteMj.push_back(0x41);

	//		lsOutDeleteMj.push_back(0x41);

	//		pPlayer->SetHandMjCard(lsOutDeleteMj);
	//	}
	//	//else
	//	//{
	//	//	lsOutDeleteMj.push_back(0x21);
	//	//	lsOutDeleteMj.push_back(0x22);
	//	//	lsOutDeleteMj.push_back(0x23);

	//	//	lsOutDeleteMj.push_back(0x24);
	//	//	lsOutDeleteMj.push_back(0x25);
	//	//	lsOutDeleteMj.push_back(0x26);

	//	//	lsOutDeleteMj.push_back(0x27);
	//	//	lsOutDeleteMj.push_back(0x28);
	//	//	lsOutDeleteMj.push_back(0x29);

	//	//	lsOutDeleteMj.push_back(0x01);
	//	//	lsOutDeleteMj.push_back(0x02);
	//	//	lsOutDeleteMj.push_back(0x03);

	//	//	lsOutDeleteMj.push_back(0x11);

	//	//	pPlayer->SetHandMjCard(lsOutDeleteMj);
	//	//}
	//}
	////end
	////////////////////////////////////////////////////////////////////////////

	//���͵��ͻ���
	CMD_S_SendMj  sendMjMsg;
	memset(&sendMjMsg, 0, sizeof(sendMjMsg));
	sendMjMsg.byBankerSeatNo = m_iNtStation;
	sendMjMsg.bySatrtIndex   = m_iStartIndex;
	sendMjMsg.byEndIndex     = m_iEndIndex;
	sendMjMsg.iLeftMjCount   = ntoh32(m_iLeftMjCount);

	for (int i=0;i<CONFIG_PALYER_COUNT;i++)
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if ( NULL == pPlayer )
		{
			continue;
		}

		std::list<BYTE> lsOutHandMj;
		sendMjMsg.byMjCount = pPlayer->GetHandMjCard(lsOutHandMj);

		std::list<BYTE>::iterator it = lsOutHandMj.begin();
		int iCardIndex = 0;
		for ( ; it != lsOutHandMj.end() && iCardIndex < HAND_MJ_MAX ;  it++,iCardIndex++ )
		{
			sendMjMsg.byMjs[iCardIndex] = (*it);
		}

		SendGameData(i,SUB_S_SEND_MJ,(BYTE*)&sendMjMsg, sizeof(sendMjMsg));
	}
	SendWatchData(CONFIG_PALYER_COUNT,SUB_S_SEND_MJ,(BYTE*)&sendMjMsg, sizeof(sendMjMsg));

	//ׯ�����óɵ�һ������
	PlayerInfo *pPlayer = GetPlayerInfo(m_iNtStation);
	if (pPlayer != NULL)
	{
		pPlayer->UpdateMoPaiCount();
	}
}

//����
int  CGameDesk::OnTouchCard(int iDeskStation,bool bFront)
{
	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction OnTouchCard NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}
	pPlayer->UpdateMoPaiCount();

	MYLOG_DEBUG(logger,"CGameDesk::OnMsgUserDoAction OnTouchCard ..[roomID=%d,DeskNo=%d,iDeskStation=%d,bFront=%d]",GetRoomID(),GetTableID(),iDeskStation,bFront);

	int iTouchCount = 1;
	std::list<BYTE> lsOutDeleteMj;
	int iRet = DeleteMJDCard(false, lsOutDeleteMj,iTouchCount,bFront);
	if ( iRet < 0 || lsOutDeleteMj.size() <= 0 )
	{
		if ( (m_iLeftMjCount-m_iCfgZhuaNiaoCount) <= 0 )
		{
			MYLOG_INFO(logger,"m_iLeftMjCount=%d m_iCfgZhuaNiaoCount=%d", m_iLeftMjCount, m_iCfgZhuaNiaoCount);

			//û����,��Ϸ����,����
			OnMsgProcAction_JieSuan(true);
			return -3;
		}
		else
		{
			MYLOG_ERROR(logger,"[CGameDesk::OnTouchCard DeleteMJDCard error]: [RoomID=%d,deskID = %d, station = %d, userID=%d,iFaPaiCount=%d,iRet=%d]"
				,GetRoomID(),m_nDeskIndex,iDeskStation,pPlayer->GetUserID(),iTouchCount,iRet);
		}
		return -2;
	}

	//MYLOG_DEBUG(logger,"CGameDesk::OnMsgUserDoAction OnTouchCard  22..[roomID=%d,DeskNo=%d,iDeskStation=%d,bFront=%d]",GetRoomID(),GetTableID(),iDeskStation,bFront);

	BYTE byTouchCard = lsOutDeleteMj.front();
	pPlayer->DoTouchCard(byTouchCard);
	m_nFinalMoPaiChair = iDeskStation;

	if ( 0 == byTouchCard )
	{
		MYLOG_ERROR(logger,"CGameDesk::OnMsgUserDoAction OnTouchCard 0 == byTouchCard..[roomID=%d,DeskNo=%d,iDeskStation=%d,bFront=%d]",GetRoomID(),GetTableID(),iDeskStation,bFront);
	}
	else
	{
		MYLOG_DEBUG(logger,"CGameDesk::OnMsgUserDoAction OnTouchCard success..[roomID=%d,DeskNo=%d,iDeskStation=%d,bFront=%d,byTouchCard=%02X]",GetRoomID(),GetTableID(),iDeskStation,bFront,byTouchCard);
	}

	//���ø����Ʊ��
	pPlayer->SetIsLastGangTouchCard(!bFront);

	//���͵��ͻ���
	CMD_S_TouchMj  sTouchMjMsg;
	memset(&sTouchMjMsg, 0, sizeof(sTouchMjMsg));
	sTouchMjMsg.byTouchMj      = byTouchCard;
	sTouchMjMsg.byCurUserSeatNo = iDeskStation;
	sTouchMjMsg.bySatrtIndex   = m_iStartIndex;
	sTouchMjMsg.byEndIndex     = m_iEndIndex;
	sTouchMjMsg.iLeftMjCount   = ntoh32(m_iLeftMjCount);

	for (int i=0;i<CONFIG_PALYER_COUNT;i++)
	{
		PlayerInfo* pPlayer = GetPlayerInfo(i);
		if ( NULL == pPlayer )
		{
			continue;
		}

		if ( i == iDeskStation )
		{
			sTouchMjMsg.byTouchMj = byTouchCard;
		}
		else
		{
			sTouchMjMsg.byTouchMj = 0;
		}

		SendGameData(i,SUB_S_TOUCH_MJ,(BYTE*)&sTouchMjMsg, sizeof(sTouchMjMsg));
	}
	SendWatchData(CONFIG_PALYER_COUNT,SUB_S_TOUCH_MJ,(BYTE*)&sTouchMjMsg, sizeof(sTouchMjMsg));

	return 0;
}

//�Ƿ�����һ��λ��
bool CGameDesk::IsNextStation(int iCurStation,int iSecondStation)
{
	if ( (iCurStation + 1)%CONFIG_PALYER_COUNT == iSecondStation )
	{
		return true;
	}
	return false;
}

//��ȡ��һ��λ��
int  CGameDesk::GetNextStation(int iCurStation)
{
	int iNextStation = (iCurStation + 1)%CONFIG_PALYER_COUNT;
	return iNextStation;
}

//�ж��Ƿ��ܽ�������
bool CGameDesk::IsCanGiveUp(DWORD dwEnableAction)
{
	if (   dwEnableAction&EACTION_CHI_LEFT 
		|| dwEnableAction&EACTION_CHI_MID 
		|| dwEnableAction&EACTION_CHI_RIGHT 
		|| dwEnableAction&EACTION_PENG 
		|| dwEnableAction&EACTION_GANG_DIAN 
		|| dwEnableAction&EACTION_GANG_BU 
		|| dwEnableAction&EACTION_GANG_AN 
		|| dwEnableAction&EACTION_HU_ZIMO 
		|| dwEnableAction&EACTION_HU_DIAN 
	 )
	{
		return true;
	}

	return false;
}

//���õ�ǰ����λ��
void CGameDesk::SetCurOperateStation(int iCurOperateStation)
{ 
	int iBeforeCurOperateStation = m_iCurOperateStation;
	m_iCurOperateStation = iCurOperateStation; 

	//������
	PlayerInfo* pPlayer = GetPlayerInfo(iCurOperateStation);
	if ( NULL != pPlayer )
	{
		pPlayer->ClearDisableDianPaoHuFlag();
	}

	if ( iBeforeCurOperateStation != m_iCurOperateStation )
	{
		//�ж��Ƿ������Լ�
		int iCount = 0;
		int bNextStation = (iBeforeCurOperateStation + 1 ) % CONFIG_PALYER_COUNT;//˳ʱ��
		while ( bNextStation != m_iCurOperateStation )
		{
			PlayerInfo* pPlayerTemp = GetPlayerInfo(bNextStation);
			if ( NULL != pPlayerTemp )
			{
				pPlayerTemp->ClearDisableDianPaoHuFlag();
			}

			bNextStation = (bNextStation + 1 ) % CONFIG_PALYER_COUNT;//˳ʱ��
			iCount++;

			if ( iCount > CONFIG_PALYER_COUNT )
			{
				MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction SetCurOperateStation..[roomID=%d,DeskNo=%d,m_iCurOperateStation=%d,iBeforeCurOperateStation=%d]"
					,GetRoomID(),GetTableID(),m_iCurOperateStation,iBeforeCurOperateStation);
				break;
			}
		}
	}
}

//ת����һ����Ҳ���
int  CGameDesk::TurnToNextStation()
{
	int iNextStation = GetNextStation(GetCurOperateStation());
	SetCurOperateStation(iNextStation);
	SetCurSeatOpStatus(TSEAT_STATUS_OP_ING);

	//����
	int iRet = OnTouchCard(iNextStation,true);
	if ( iRet < 0 )
	{
		return -1;
	}

	//��⵱ǰ����ܽ��еĲ���
	CheckCurPlayerEnableOperates();
	return 0;
}

//ת��ָ�����
int  CGameDesk::TurnToAimStation(int iDeskStation,bool bNeedTouchCard,bool bTouchFront)
{
	PlayerInfo* pPlayer = GetPlayerInfo(iDeskStation);
	if ( NULL == pPlayer )
	{
		MYLOG_ERROR(logger,"Error:CGameDesk::OnMsgUserDoAction TurnToAimStation NULL == pPlayer..[roomID=%d,DeskNo=%d,iDeskStation=%d]",GetRoomID(),GetTableID(),iDeskStation);
		return -1;
	}

	SetCurOperateStation(iDeskStation);
	SetCurSeatOpStatus(TSEAT_STATUS_OP_ING);

	if ( bNeedTouchCard )
	{
		//����
		int iRet = OnTouchCard(iDeskStation,bTouchFront);
		if ( iRet < 0 )
		{
			return -1;
		}
	}

	//��⵱ǰ����ܽ��еĲ���
	CheckCurPlayerEnableOperates();
	return 0;
}

//������һ��ׯλ��
void CGameDesk::SetNextNtStation(int iDeskStation)
{
	MYLOG_DEBUG(logger,"SetNextNtStation iDeskStation=%d, m_nFinalMoPaiChair=%d", iDeskStation, m_nFinalMoPaiChair);

	m_iNextNtStation = iDeskStation;

	if ( iDeskStation >= 0 )
	{
		PlayerInfo* pTempPlayer = GetPlayerInfo(iDeskStation);
		if ( NULL != pTempPlayer )
		{
			m_dwNtUserID = pTempPlayer->GetUserID();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//����ʱ
//����ʱ��ʱ��
int CGameDesk::OnDjsBaseTime()
{
	if ( EGPDJS_NO == m_eGameDjsType )
	{
		return 0;
	}

	// ����
	time_t tmCurTime = CTimeLib::GetCurMSecond();
	if ( tmCurTime >= m_tmGameDjsStartMS + m_tmGameDjsTimes*1000 )
	{
		//����ʱ��ʱ�䴦��
		switch (m_eGameDjsType)
		{
		case EGPDJS_ZHUANIAO: //ץ�񵹼�ʱ
			{
				m_eGameDjsType = EGPDJS_NO;
				OnTimeZhuaNiao();
			}
			break;
		case EGPDJS_WAITEND:  //�ȴ����㵹��ʱ
			{
				m_eGameDjsType = EGPDJS_NO;
				OnTimeGameEnd();
			}
			break;
		}
	}

	return 0;
}

//ץ��ʱ��
int CGameDesk::OnTimeZhuaNiao()
{

	return 0;
}

//��Ϸ����
int CGameDesk::OnTimeGameEnd()
{
	return 0;
}

//���õ���ʱ
void CGameDesk::SetDjs(E_DJS_GAME_PLAYING  eGameDjsType,int tmGameDjsTimes,int iGameDjsStation)
{
	m_eGameDjsType    = eGameDjsType;
	m_tmGameDjsTimes  = tmGameDjsTimes;
	m_iGameDjsStation = iGameDjsStation;

	//����
	m_tmGameDjsStartMS = CTimeLib::GetCurMSecond();

	if ( EGPDJS_NO != m_eGameDjsType && -1 != m_iGameDjsStation )
	{
		//�㲥���͵��ͻ���
		//����ʱ
		CMD_S_NOTIFY_TIME sMsg;
		memset(&sMsg, 0, sizeof(sMsg));

		sMsg.bySeatNo  = m_iGameDjsStation;
		sMsg.iDjsTimes = ntoh32(m_tmGameDjsTimes);

		SendDeskBroadCast(SUB_S_NOTIFY_TIME,(BYTE*)&sMsg, sizeof(sMsg));
	}
}

//��ȡ����ʱʣ��ʱ��(��)
int  CGameDesk::GetLeftDjsSec()
{
	int iLeftDjsSecond = 0;
	// ����
	time_t tmCurTime = CTimeLib::GetCurMSecond();

	if ( tmCurTime < m_tmGameDjsStartMS + m_tmGameDjsTimes*1000 )
	{
		iLeftDjsSecond = (m_tmGameDjsStartMS + m_tmGameDjsTimes*1000 - tmCurTime)/1000;
	}

	return iLeftDjsSecond;
}

//�����ܺ�����
int CGameDesk::NotifyEnableHuCards(int iNotifyStation)
{
	PlayerInfo* pPlayer = GetPlayerInfo(iNotifyStation);
	if ( NULL == pPlayer )
	{
		return -1;
	}

	MYLOG_INFO(logger,"NotifyEnableHuCards iDeskStation=%d", iNotifyStation);

	std::list<BYTE> lsOutWinCard;
	//pPlayer->GetCanWinCardList(m_byCfgLaiziCard,m_bCfgShiSanYao,lsOutWinCard);
	pPlayer->GetCanWinCardList(m_byCfgLaiziCard,false,lsOutWinCard);

	CMD_S_NOTIFY_CAN_HU_CARDS sMsg;
	memset(&sMsg, 0, sizeof(sMsg));
	std::list<BYTE>::iterator it = lsOutWinCard.begin();
	int iTemp = 0;
	for ( ; it != lsOutWinCard.end() && iTemp < 28 ; it++ ,iTemp++ )
	{
		sMsg.byCanHuCards[iTemp] = *it;
	}

	SendGameData(iNotifyStation,SUB_S_NOTIFY_CAN_HU_CARDS,(BYTE*)&sMsg, sizeof(sMsg));
	return 0;
}

//����Ƿ��ܸ�
bool CGameDesk::GetIsEnableGang()
{
	if( (m_iLeftMjCount - m_iCfgZhuaNiaoCount) >= 1 )
		return true;

	return false;
}

//�й�
bool CGameDesk::OnUserTuoGuan(WORD wChairID, byte IsTuoGuan)
{
	if (wChairID >= CONFIG_PALYER_COUNT)
	{
		MYLOG_ERROR(logger, "OnUserTuoGuan wChairID=%d", wChairID);
		return false;
	}

	m_bPlayerTuoGuan[wChairID] = TRUE == IsTuoGuan;

	if (!m_bPlayerTuoGuan[wChairID])
	{
		m_nPlayerTimeOutCount[wChairID] = 0;
	}
	else
	{
		//��ҵ�ǰ��Ҫ��������ʱ���ѿ�������Ҫ���̼�ʱ��ʱ��
		if (m_bPlayerTimerRunning[wChairID])
		{
			KillGameTimer(TIME_PLAYER_1+wChairID);
			SetGameTimer(TIME_PLAYER_1+wChairID, 2000);
		}
	}

	BroadcastUserTuoGuanMsg();

	return true;
}

void CGameDesk::BroadcastUserTuoGuanMsg()
{
	MYLOG_INFO(logger, "BroadcastUserTuoGuanMsg ");

	CMD_S_NOTIFY_USER_TUOGUAN msg;
	for (int i=0; i<GAME_PLAYER; i++)
	{
		msg.byUserTuoGuan[i] = m_bPlayerTuoGuan[i]?TRUE:FALSE;
	}

	SendDeskBroadCast(SUB_S_NOTIFY_USER_TUOGUAN, (byte*)&msg, sizeof(msg));
}

//��Ҳ�����ʱ��
int CGameDesk::OnPlayerOpTimer(int nTimeID)
{
	//�ǽ�ҳ�return
	if (!IsGoldRoom())
	{
		return -1;
	}

	byte byChairID = nTimeID - TIME_PLAYER_1;
	if (byChairID >= CONFIG_PALYER_COUNT)
	{
		return -2;
	}

	MYLOG_INFO(logger, "OnPlayerOpTimer byChairID=%d ", byChairID);

	//׼������ʱ
	if (m_nGameStation != GS_PLAY_GAME)
	{
		MYLOG_INFO(logger, "OnPlayerTimer m_nGameStation=%d m_bPlaying=%d", m_nGameStation, m_bPlaying);
		//UserLeftDesk(unsigned int unUin, int nRoomID, int nDeskIndex, BYTE bDeskStation, int nReasonCode)=0;
		KickUserOutTable(byChairID, E_ERROR_DISSOLVED_COIN_ROOM_4_READY_TIME_OUT);
		return 0;
	}

	//��Ϸ�е���ʱ
	PlayerInfo* pPlayer = GetPlayerInfo(byChairID);
	if ( NULL == pPlayer)
	{
		return -1;
	}

	//��ʱ���й�
	if (!m_bPlayerTuoGuan[byChairID])
	{
		m_nPlayerTimeOutCount[byChairID]++;
		if (m_nPlayerTimeOutCount[byChairID] >= m_nTimeOutLimit)
		{
			m_nPlayerTimeOutCount[byChairID] = 0;
			m_bPlayerTuoGuan[byChairID] = true;
			BroadcastUserTuoGuanMsg();
		}
	}
	else
	{
		m_nPlayerTimeOutCount[byChairID] = 0;
	}

	//��ʱ����
	DWORD dwPlayerAction = 0;
	byte byOutCard = 0;
	pPlayer->GetOpTimeOutAction(dwPlayerAction, byOutCard);
	//int iDeskStation,CMD_C_USER_DO_ACTION* pMsg
	CMD_C_USER_DO_ACTION msg;
	memset(&msg, 0, sizeof(msg));
	msg.dwAction = dwPlayerAction;
	msg.byCardNum = 0 == byOutCard ? 0:1;
	msg.byCard[0] = byOutCard;
	OnMsgUserDoAction(byChairID, &msg);

	return 0;
}

bool CGameDesk::IsGoldRoom()
{
	if (E_ENTITY_ROOM_TYPE_COIN == m_nRoomCardPlaform)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CGameDesk::KickUserOutTable(int nDeskStation, int nReason)
{
	MYLOG_INFO(logger, "KickUserOutTable m_nGameStation=%d m_bPlaying=%d nDeskStation=%d nReason=%d", m_nGameStation, m_bPlaying, nDeskStation, nReason);

	if (nDeskStation >= GAME_PLAYER)
	{
		MYLOG_ERROR(logger, "KickUserOutTable m_nGameStation=%d m_bPlaying=%d nDeskStation=%d nReason=%d", m_nGameStation, m_bPlaying, nDeskStation, nReason);
		return;
	}

	if (GS_PLAY_GAME == m_nGameStation)
	{
		MYLOG_ERROR(logger, "KickUserOutTable m_nGameStation=%d m_bPlaying=%d nDeskStation=%d nReason=%d", m_nGameStation, m_bPlaying, nDeskStation, nReason);
		return;
	}

	if (!IsGoldRoom())
	{
		return;
	}

	m_pGameManager->DissolveRoom(m_nRoomID, m_nDeskIndex, nReason);
}

//////////////////////////////////////////////////////////////////////////