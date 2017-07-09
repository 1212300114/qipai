/********************************************************************
created:	2016/10/11
created:	8:22:2016   14:33
filename: 	CGameDesk.h
author:		jimy.zhu

purpose:	��Ϸ��̬��142����ɳ����
*********************************************************************/
#ifndef CGameDesk_h__
#define CGameDesk_h__

#include <vector>
#include "CMD_Mj.h"
#include <list>

using namespace std;

#define IDT_USER_CUT				    1L	   			   //���߶�ʱ�� ID Ԥ��ʮ��

//��ʱ�� ID(20-50֮��)
//#define TIME_WAITUSER_AGREE             20                 //Ԥ��ʮ��ID 20 - 29

#define TIME_PLAYER_1					21			       //���1���ƻ������ʱ��
#define TIME_PLAYER_2					22			       //���2���ƻ������ʱ��
#define TIME_PLAYER_3					23			       //���3���ƻ������ʱ��
#define TIME_PLAYER_4					24			       //���4���ƻ������ʱ��

#define TIME_WAIT_QIANGZHUANG			30			       //�ȴ���ׯ��ʱ��
#define TIME_CHECK_END_MONEY 		    31			       //���Ǯ�Ƿ�
#define TIME_GAME_BASE_TIME 		    32			       //��Ϸ�л�����ʱ��
#define TIME_GAME_AUTO_HU 				33			       //�Զ�����ʱ

#define TIME_ONE_COUNT					37			       //ÿ���ʱ��
#define TIME_SEND_CARD_FINISH			38			       //�ͻ��˷���ʱ��

#define TIME_NOT_ENOUGH_PLAYER			42			       //�ȴ�������ʱ��
#define TIME_MAX_ID						50			       //���

#define TIME_MUL_D                      10000L          
#define TIME_SPACE					    100L   		    ///��Ϸ ID ���
#define TIME_START_ID				    1L		    	///��ʱ����ʼ ID

//��Ϸ������־����
#define GF_NORMAL						10				//��Ϸ��������
#define GF_SAFE							11				//��Ϸ��ȫ����

class CGameDesk : public IGameDesk
{
private:
	/// ����Ϊ˽�г�Ա��������Ϸͨ��
	IGameMainManager	*m_pGameManager;				///< �������ָ��
	int                 m_nRoomLevel;                   ///< ����ȼ�
	int                 m_nRoomID;                      ///< �����
	int					m_nDeskIndex;					///< ���Ӻ�
	bool				m_bPlaying;						///< ��Ϸ�Ƿ����ڽ�����
	int					m_nGameStation;					///< ��Ϸ����״̬���ɸ���Ϸ����ָ��    
	
	UINT				m_dwBeginTime;					///< ������Ϸ��ʼʱ��

	uint64_t            m_un64RoomLevelMinMoney;        ///< ��С�������

	ManageInfoStruct    m_ManageInfoStruct;             ///< ��������

	//״̬
private:
	/// ������Ϸ״̬�����ڸս���������������
	bool SendGameStation(BYTE bDeskStation, UINT uIndex, void * pStationData, UINT uSize);

	/// �ȴ�ͬ��ʱ�·�״̬
	int GetStationWhileWaitAgree(int nDeskStation, bool bIsWatch, UINT uIndex);

	/// �ȴ�������ʱ�·�״̬
	int GetStationWhileWaitTouShaiZi(int nDeskStation, bool bIsWatch, UINT uIndex);

	/// �ȴ�����ʱ�·�״̬
	int GetStationWhileWaitSendCard(int nDeskStation, bool bIsWatch, UINT uIndex);

	/// ��Ϸ��ʱ�·�״̬
	int GetStationWhileWaitPlaying(int nDeskStation, bool bIsWatch, UINT uIndex);

	/// ����״̬
	int GetStationWhileGameEnd(int nDeskStation, bool bIsWatch, UINT uIndex);


	//��������
public:
	//���캯��
	CGameDesk(); 

	//��������
	virtual ~CGameDesk();

	//���غ���
public:
	//-------------------------------------------------------------------------
	// ��IGameDesk����
	//-------------------------------------------------------------------------
	/// ��ʼ������
	virtual int InitialDesk(unsigned int unRoomID, int nDeskNo, int nMaxPeople, IGameMainManager* pIMainManager, unsigned int unExchangeRate);

	// �ڲ����ͷ�
	virtual void Release(){}

	//��ѯ�汾��
	virtual int GetVersion(char* pVersion);

	/// ��ȡ��Ϸ״̬�������͵��ͻ���
	virtual int OnGetGameStation(int nDeskStation, UserInfoForGame_t& userInfo);

	/// �������
	virtual int UserReCome(int nDeskStation, UserInfoForGame_t& userInfo);

	/// ��ʱ��ʱ�䵽
	virtual int OnGameTimer(unsigned int unTimerID,void* pData);

	/// ��Ҷ���
	virtual int UserNetCut(int nDeskStation, UserInfoForGame_t& userInfo);

	/// ���������ĳλ��
	virtual int UserSitDesk(int nDeskStation, UserInfoForGame_t& userInfo);

	/// �������
	virtual int UserLeftDesk(int nDeskStation, UserInfoForGame_t& userInfo);

	///���������Ϣ
	virtual int UpdateUserInfo(int nDeskStation, UserInfoForGame_t& userInfo);

	/// ���ͬ����Ϸ
	virtual int UserAgreeGame(int nDeskStation, UserInfoForGame_t& userInfo);

	/// ��Ϸ�Ƿ��ڽ�����
	virtual bool IsPlayingByGameStation(void);

	/// ĳ��������Ƿ�����Ϸ��
	virtual bool IsPlayGame(int bDeskStation);

	// ����Ƿ��������(������Ϸ�У�ĳЩ��Ϸ�������ʱ���ǲ��������µ�)
	virtual bool CanSit2Desk();

	//�Ƿ�Ϊ������Ϸ
	virtual bool IsChessCard();

	/// ��ȡ��ǰ��Ϸ״̬
	virtual int GetCurGameStation(){return m_nGameStation;}

	///������Ϸ�û���Ϸ��ǿ��
	virtual int ForceQuit() ;

	// ���佱�ر仯֪ͨ
	virtual void AwardPoolChangedNotify(){}

	// ����proxy��Ϣ�����Է�����
	virtual int HandleProxyMsg(unsigned int unMsgID,void * pMsgHead ,const char* pMsgParaBuf, const int nLen);

	// ����ͻ�����Ϣ������Lotus
	virtual int HandleClientMsg(unsigned int unUin,int nDeskStation, void * pMsgHead ,const char* pMsgParaBuf, const int nLen);

	//��ȡ�����
	virtual int GetRoomID(){return m_nRoomID;}

	//����������Ϸʵʱ�㷨������Ϣ
	//(usAlgorithmType, ��ʾ�㷨���ͣ�����ο�(GameDef.h)ö��: E_ALGORITHM_TYPE)
	//(usModifyFlag,��ʾ�㷨û���޸ģ�����ο�(GameDef.h)ö��: E_ALGORITHM_MODIFY_TYPE)
	virtual int UpdateDeskRealtimeData(void *pDeskRealTimeData, unsigned short usAlgorithmType, unsigned short usModifyFlag){ return 0; }

	//����GM������Ϣ��usGmType�ο�(GameDef.h)ö��:E_GM_TYPE
	virtual int SetGameMangerInfo(void *pGameManagerData, unsigned short usGmType);

	//-------------------------------------------------------------------------
	//��Ϸ��ʼ
	virtual bool GameBegin(BYTE bBeginFlag);
	//��Ϸ����
	virtual bool GameFinish(BYTE bDeskStation, BYTE bCloseFlag);
	//�����ı��û����������ĺ���
	virtual bool CanNetCut(BYTE bDeskStation);

	//���ش��麯��
public:

	//������Ϸ״̬
	virtual bool ReSetGameState(BYTE bLastStation);

	//������
private:

	// ���ö�ʱ��
	bool SetGameTimer(int nTimeID, unsigned int unMSDelay);

	// ɾ����ʱ��
	int  KillGameTimer(int nTimeID);

	//������Ӌ�r��
	void KillAllTimer();

	//��ȡ��һ�����λ��
	BYTE GetNextDeskStation(BYTE bDeskStation);

	int LogMsg(const char* szBuf, int len);

	//���Ӻ�
	int GetTableID() { return m_nDeskIndex; }

	// ���Ϳͻ�������
	int SendGameData(BYTE bDeskStation, UINT nSubCmdID, const BYTE* pMsgBuf, const int nMsgParaLen);

	// ���͹۲�����
	int SendWatchData(BYTE bDeskStation, UINT nSubCmdID, const BYTE* pMsgBuf, const int nMsgParaLen);

	// �������ӹ㲥����
	int SendDeskBroadCast(UINT nSubCmdID, const BYTE* pMsgBuf, const int nMsgParaLen);

	//�߳����
	int  DoUserLeftDeskOp(int nDeskStation,int iCode);

	//������Ǯ�Ƿ�,������������߳�
	int  CheckUserMoneyLeftGame();

	//֪ͨ��Ҷ���
	void NotifyUserNetCut(int nDeskStation,bool bNetCut);

	//��ȡ���
	PlayerInfo* GetPlayerInfo(int iSeatNo);

	//��ȡ������
	int GetTotalPlayerCount();

	//��ȡ��ͬ������
	int GetTotalAgreeCount();

	//��ȡ����������
	int GetTotalOnlineCount();

	//��������
	int ResetPlayerCount(int iCount);

	//���ÿ������������� 
	int ResetCardRoomConfigInfo();

	//��ʱ��
private:
	//����ʱ��ʱ��
	int OnDjsBaseTime();
	int OnProcDjsOnTime();

	//ץ��ʱ��
	int OnTimeZhuaNiao();

	//��Ϸ����
	int OnTimeGameEnd();

	//���õ���ʱ
	void SetDjs(E_DJS_GAME_PLAYING  eGameDjsType,int tmGameDjsTimes,int iGameDjsStation);

	//��ȡ����ʱʣ��ʱ��(��)
	int  GetLeftDjsSec();

	//��Ϣ
private:
	//�û�������Ϣ
	int OnMsgUserDoAction(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg);

	//�й�
	bool OnUserTuoGuan(WORD wChairID, byte IsTuoGuan);

	//����
	int OnMsgProcAction_OutCard(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg);

	//����
	int OnMsgProcAction_EatCard(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg);

	//����
	int OnMsgProcAction_PengCard(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg);

	//��--���
	int OnMsgProcAction_DianGang(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg);

	//��--����
	int OnMsgProcAction_BuGang(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg);

	//��--����
	int OnMsgProcAction_AnGang(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg);

	//��--����
	int OnMsgProcAction_ZiMoHu(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg);

	//��--���ں�
	int OnMsgProcAction_DianPaoHu(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg);

	//��(����)
	int OnMsgProcAction_GiveUp(int iDeskStation,CMD_C_USER_DO_ACTION* pMsg);

	//ץ��
	int OnMsgProcAction_ZhuaNiao();

	//����
	int OnMsgProcAction_JieSuan(bool bLiuJu);

	//��Ϸ�߼�
private:

	//ɾ���齫����
	//int DeleteMJDCard(std::list<BYTE>& lsOutDeleteMj,int iCount=1,bool bFront=true);
	int DeleteMJDCard(bool bZhuaNiao, std::list<BYTE>& lsOutDeleteMj,int iCount/*=1*/,bool bFront/*=true*/);
	//Ͷ����
	void OnCastDice();

	//����
	void OnFaPai();

	//����
	int  OnTouchCard(int iDeskStation,bool bFront);

	//�Ƿ�����һ��λ��
	bool IsNextStation(int iCurStation,int iSecondStation);

	//��ȡ��һ��λ��
	int  GetNextStation(int iCurStation);

	//�ж��Ƿ��ܽ�������
	bool IsCanGiveUp(DWORD dwEnableAction);

	//��ȡ��ǰ����λ��
	int  GetCurOperateStation()                      { return m_iCurOperateStation; }
	void SetCurOperateStation(int iCurOperateStation);

	E_CUR_SEAT_OP_STATUS GetCurSeatOpStatus()                     { return m_iCurSeatOpStatus;   }
	void SetCurSeatOpStatus(E_CUR_SEAT_OP_STATUS iCurSeatOpStatus){ m_iCurSeatOpStatus = iCurSeatOpStatus; }

	//�����齫��ͷ��ʾλ��
	BYTE GetCurMjShowSeat()                          { return m_byCurMjShowSeat; }
	void SetCurMjShowSeat(BYTE byCurMjShowSeat)      { m_byCurMjShowSeat = byCurMjShowSeat;}

	//��⵱ǰ����ܽ��еĲ���
	bool CheckCurPlayerEnableOperates(bool bNeedCheckZiMoHu=true);

	//�����������Ƿ��ܶԳ����ƽ��в���
	bool CheckOutCardOtherEnableOperates(BYTE byOutMJCard,BYTE byOutMJSeatNo,bool bOnlyCheckDianHu,bool bCheckQiangGangHu);
	//��Ⲣ��ִ�еȴ��Ĳ���
	int  CheckDoWaitOperate(int iNotifyWatiOpStation=-1);

	//��ȡû�в��������Ȩ�����ֵ
	int  GetMaxNotOperateRightIndex(int& iOutMaxDeskStation,DWORD& dwOutMaxOneAction);

	//��ȡ�Ѿ����������Ȩ�����ֵ
	int  GetMaxHasOperateRightIndex(int& iOutMaxDeskStation,DWORD& dwOutMaxOneAction);

	//�����ҵ͵�Ȩ��
	int  ClearAllPlayerLowQuanXian(DWORD dwOneAction);

	//��ȡ���������
	int  GetHuPlayerCount();

	//ת����һ����Ҳ���
	int  TurnToNextStation();

	//ת��ָ�����
	int  TurnToAimStation(int iDeskStation,bool bNeedTouchCard,bool bTouchFront);

	//֪ͨ�ͻ��˲����ɹ�
	int  NotifyClientOperatesSuccess(int iDeskStation,DWORD dwAction,const std::vector<BYTE>& vecCardList,int iNotifyStation=-1);

	//֪ͨ�ͻ��˲���ʧ��
	int  NotifyClientOperatesFail(int iNotifyStation,int iCode);

	//֪ͨ������ҵ�ǰ��ʾ
	int  NotifyAdjustPlayerEnableAction(int iDeskStation,DWORD dwOneAction);

	//�������
	void CalcGameEndScore(CardRoomScoreStatement& CardRoomInfo);

	//������һ��ׯλ��
	void SetNextNtStation(int iDeskStation);

	//����������ֵ
	BYTE GetCfgLaiziCard()                   { return m_byCfgLaiziCard; }
	void SetCfgLaiziCard(BYTE byCfgLaiziCard){ m_byCfgLaiziCard = byCfgLaiziCard; }

	//�����ܺ�����
	int  NotifyEnableHuCards(int iNotifyStation);

	////����Ƿ��ܸ�
	bool GetIsEnableGang();
	
	//�㲥�й���Ϣ
	void BroadcastUserTuoGuanMsg();

	//��Ҳ�����ʱ��
	int OnPlayerOpTimer(int nTimeID);

	//�ж��Ƿ��ҳ�
	bool IsGoldRoom();

	//�����δ������δ׼������ɢ����
	void KickUserOutTable(int nDeskStation, int nReason);

	//��ҽ�ҳ���Ϸ�Ҳ���ʱ�����¼���
	int ReCalcGameEndScore(double lfPercent[]);
	//��������
private:
	int                      m_iCfgZhuaNiaoCount;    //ץ���������
	bool                     m_bCfgCanDianPaoHu;     //�ܷ���ں�
	bool                     m_bCfgZhuangXianScore;  //�Ƿ���ׯ�����
	bool                     m_bCfgGuoHouBuGang;	 //�Ƿ��ǹ��󲹸�
	bool                     m_bCfgXianPengHouGang;	 //�Ƿ����������
	bool                     m_bCfgHuQiDui;          //�Ƿ�֧����С��
	bool                     m_bCfgHongZhong;        //��������
	BYTE                     m_byCfgLaiziCard;       //������ֵ
	byte					 m_byAddNiaoCount;		 //�����Ӻ��Ƽ�����

	//��Ϸ��һЩ����
private:

	std::vector<PlayerInfo*> m_vecPlayer;      //����б�
	int					     m_iNtStation;     //ׯ��λ��
	int                      m_dwNtUserID;     //ׯ���û�ID

	BYTE                     m_byAllCardList[MAX_MJ_TOTAL_COUTN]; //���е���
	int                      m_iTotalMjCardCount;                 //���齫��
	int                      m_iLeftMjCount;   //ʣ���Ƹ���
	int                      m_iStartIndex;    //�齫����ʼλ��
	int                      m_iEndIndex;      //�齫�ս���λ��

	BYTE                     m_byStartSeat;    //��ʼ����λ��
	BYTE					 m_byStartDun;     //��ʼ���ƶ�


	BYTE                     m_byDicePoint[2]; //��ȡ������

	int                      m_iTotalRound;    //�ܾ���
	int                      m_iCurRound;      //��ǰ�ڼ���

	int                      m_iCurOperateStation;    //��ǰ����λ��
	E_CUR_SEAT_OP_STATUS     m_iCurSeatOpStatus;      //��ǰ��λ����״̬

	BYTE                     m_byLastOutMjSetaNo; //�����Ƶ����
	BYTE					 m_byLastOutMj;       //�����Ƶ��齫
	BYTE                     m_byCurMjShowSeat;   //��ǰ�����齫��ͷ��ʾ���,255��ʾû�м�ͷָ��

	//����ʱ
	E_DJS_GAME_PLAYING       m_eGameDjsType;      //����ʱ����
	time_t                   m_tmGameDjsStartMS;  //����ʱ��ʼʱ��(����)
	int                      m_tmGameDjsTimes;    //����ʱ��ʱʱ��(��)
	int                      m_iGameDjsStation;   //����ʱָ��λ�� , -1��ʾ��ָ��

	int                      m_iNextNtStation;    //��һ��ׯ��λ��
	int						 m_nFinalMoPaiChair;  //���һ���������

	T_CardRoomConfig         m_CardRoomConfig;    //��������Ϣ

	//int						 m_nSaveAutoHuChairID;//�����Զ��������

	//����������� 
	CMD_S_GameEnd            m_GameEndInfo;
	//��Ӧ�ķ��� ���� ��������
	int                      m_arrSaveDetailCount[GAME_PLAYER][E_CARD_GAME_SCORE_TYPE_MAX];  //�������Ͷ�Ӧ�ĸ���,�±�ο� E_CARD_GAME_SCORE_TYPE
	__int64                  m_arrSaveDetailScore[GAME_PLAYER][E_CARD_GAME_SCORE_TYPE_MAX];  //�������Ͷ�Ӧ�ķ���,�±�ο� E_CARD_GAME_SCORE_TYPE

	CHuTipsMJ				 m_playerHuTips;	  //������ʾ�㷨��
	int						 m_nPlayMode;		  //�淨ģʽ

	//��ҳ�����
private:
	int								m_nRoomCardPlaform; 
	INT64							m_llBaseMoney;
	INT64							m_llLessTakeMoney;
	int								m_nTax;

private:
	int								m_nTempCount;							//У׼��������ʱ����
	int								m_nOperateTime;							//�����ȴ�ʱ��
	int								m_nReadyTime;							//׼��ʱ��
	int								m_nWaitForFullTime;						//����ʱ�ȴ���������������룬��ɢ����

private:
	bool							m_bPlayerTuoGuan[GAME_PLAYER];			//�й�
	int								m_nPlayerTimeOutCount[GAME_PLAYER];		//��ʱ����
	int								m_nTimeOutLimit;						//��ʱ�����������Զ�����й�

	bool							m_bPlayerTimerRunning[GAME_PLAYER];		//�Ƿ���������Ҽ�ʱ��
};

extern "C"
{
	IGameDesk*  CreateNewGameDesk();
}

#endif

/******************************************************************************************************/
