#ifndef CMD_OX_HEAD_FILE
#define CMD_OX_HEAD_FILE

//////////////////////////////////////////////////////////////////////////
//�����궨��
#pragma pack(1)

#define GAME_RULE_SUIJIZHUANG		1//���ׯ
#define GAME_RULE_QIANGZHUANG		2//��ׯ
#define GAME_RULE_NOT_WAITE			3
#define GAME_RULE_FANGZHU_ZHUANG	4//����ׯ
#define GAME_RULE_NO_ADD_SCORE		5
#define GAME_RULE_NO_ZHUANG			6//��ׯ
#define GAME_RULE_LUNLIU_ZHUANG     7//����ׯ

#define GAME_RULE_DIBEI				8//1����
#define	GAME_RULE_GAOBEI			9//10����

#define GAME_RULE_10LUN				10
#define GAME_RULE_20LUN				11

#define GAME_RULE_30LIM				12//30�ⶥ
#define GAME_RULE_50LIM				13//50�ⶥ
#define GAME_RULE_80LIM				14//80�ⶥ
#define GAME_RULE_100LIM			15//100�ⶥ

#define GAME_RULE_1DIZHU			16//��ע1��
#define GAME_RULE_3DIZHU			17
#define GAME_RULE_5DIZHU			18

#define GAME_RULE_3BEIDI			19//������
#define GAME_RULE_6PLAYER			20//6�˷���

#define KIND_ID							20									//��Ϸ I D
#define GAME_PLAYER						8									//��Ϸ����
#define GAME_NAME						TEXT("ը��")						//��Ϸ����
#define MAX_COUNT						3									//�����Ŀ
#define MAX_TURN_NUM					8									//�������

#define VERSION_SERVER					PROCESS_VERSION(6,0,3)				//����汾
#define VERSION_CLIENT					PROCESS_VERSION(6,0,3)				//����汾

//����ԭ��
#define GER_NO_PLAYER					0x10								//û�����

//��Ϸ״̬
#define GS_TK_FREE						GAME_STATUS_FREE					//�ȴ���ʼ
#define GS_TK_BANKER					GAME_STATUS_PLAY					//��ׯ״̬
#define GS_TK_SCORE						GAME_STATUS_PLAY+1					//��ע״̬
#define GS_TK_PLAYING					GAME_STATUS_PLAY+2					//��Ϸ����


#define  CALL_BANKER_SUIJI    1
#define  CALL_BANKER_QIANGZHUANG
#define CALL_BANKER_FANGZHU

#define SERVER_LEN						32 

//////////////////////////////////////////////////////////////////////////
//����������ṹ

#define SUB_S_GAME_START				100									//��Ϸ��ʼ
#define SUB_S_ADD_SCORE					101									//��ע���
#define SUB_S_PLAYER_EXIT				102									//�û�ǿ��
#define SUB_S_SEND_CARD					103									//������Ϣ
#define SUB_S_GAME_END					104									//��Ϸ����
#define SUB_S_OPEN_CARD					105									//�û�̯��
#define SUB_S_CALL_BANKER				106									//�û���ׯ
#define SUB_S_ALL_CARD					107									//������Ϣ
#define SUB_S_AMDIN_COMMAND				108									//ϵͳ����


#define SUB_S_GUODI						109									//������Ϣ
#define SUB_S_TOTLE_SCORE				110									//ÿ�������ע�ܷ�����֪ͨ�ͻ���
#define SUB_S_KANPAI					111									//����
#define SUB_S_QIPAI						112									//����
#define	SUB_S_BAI						113									//��

#define SUB_S_GET_SCORE					114									//��ȡ��ҷ���


#define SUB_S_CALL_BANKER_NOTICE		200									//��ׯ֪ͨ
#define SUB_S_ADD_SCORE_NOTICE			201									//��ע֪ͨ


#ifndef _UNICODE
#define myprintf	_snprintf
#define mystrcpy	strcpy
#define mystrlen	strlen
#define myscanf		_snscanf
#define	myLPSTR		LPCSTR
#define myatoi      atoi
#define myatoi64    _atoi64
#else
#define myprintf	swprintf
#define mystrcpy	wcscpy
#define mystrlen	wcslen
#define myscanf		_snwscanf
#define	myLPSTR		LPWSTR
#define myatoi      _wtoi
#define myatoi64	_wtoi64
#endif

struct CMD_S_GuoDi
{
	LONGLONG						lGuodi;	//����׷���
	LONGLONG						lGeng;	//���ĵ׷���

	CMD_S_GuoDi()
	{
		memset(this, 0, sizeof(*this));
	}
};

struct CMD_S_SCORE
{
	WORD								wUserCharID;
	LONGLONG							lScore;								//��ǰ����µ�ע��
	LONGLONG							lCurScore;							//��ǰ��ע����
	WORD								wCurrentUserID;						//�µĵ�ǰ���
	bool								bNeedSound;							//�Ƿ���Ҫ���������Ƶ�ʱ���õ�
	WORD								wTurn;								//��
	CMD_S_SCORE()
	{
		bNeedSound = true;
	}
};

struct CMD_S_KanPai
{
	WORD								wUserCharID;
};

struct CMD_S_QiPai
{
	WORD								wUserCharID;
	WORD								wCurrentUserID;
};

struct CMD_S_BiPai
{
	WORD								wDoUserID;				//�������������
	WORD								wCompUserID;			//������������
	WORD								wLoseUserID;			//�������
};

//ÿ���û���ע����
struct CMS_S_Totle_Score
{
	LONGLONG						lTotle[GAME_PLAYER];
	LONGLONG						lLeft[GAME_PLAYER];			//ʣ����ٷ�����ע
	WORD							nLeftTurn[GAME_PLAYER];		//ʣ������
};

//��Ϸ״̬
struct CMD_S_StatusFree
{
	LONGLONG							lCellScore;							//��������

	//��ʷ����
	LONGLONG							lTurnScore[GAME_PLAYER];			//������Ϣ
	LONGLONG							lCollectScore[GAME_PLAYER];			//������Ϣ
	TCHAR								szGameRoomName[SERVER_LEN];			//��������
};

//��Ϸ״̬
struct CMD_S_StatusCall
{
	WORD								wCallBanker;						//��ׯ�û�
	BYTE                                cbDynamicJoin;                      //��̬���� 
	BYTE                                cbPlayStatus[GAME_PLAYER];          //�û�״̬
	BYTE								bCallStatus[GAME_PLAYER];						//��ׯ״̬

	//�˿���Ϣ
	BYTE								cbHandCardData[GAME_PLAYER][MAX_COUNT];//�����˿�
	//��ʷ����
	LONGLONG							lTurnScore[GAME_PLAYER];			//������Ϣ
	LONGLONG							lCollectScore[GAME_PLAYER];			//������Ϣ
};

//��Ϸ״̬
struct CMD_S_StatusScore
{
	//��ע��Ϣ
	BYTE								cbPlayStatus[GAME_PLAYER];          //�û�״̬
	BYTE								cbDynamicJoin;                      //��̬����
	LONGLONG							lTableScore[GAME_PLAYER];			//��ע��Ŀ
	WORD								wBankerUser;						//ׯ���û�

	//�˿���Ϣ
	BYTE								cbHandCardData[GAME_PLAYER][MAX_COUNT];//�����˿�
	//��ʷ����
	LONGLONG							lTurnScore[GAME_PLAYER];			//������Ϣ
	LONGLONG							lCollectScore[GAME_PLAYER];		//������Ϣ
	TCHAR								szGameRoomName[SERVER_LEN];		//��������
};

//��Ϸ״̬
struct CMD_S_StatusPlay
{
	//״̬��Ϣ
	BYTE                                cbPlayStatus[GAME_PLAYER];          //�û�״̬(�Ƿ�����)
	WORD								wUserStatus[GAME_PLAYER];			//��ע/����/����/����
	LONGLONG							lScore[GAME_PLAYER];				//��ע��Ŀ
	LONGLONG							lLeft[GAME_PLAYER];					//ÿ���������ע���ж��ٳ��������
	WORD								wBankerUser;						//ׯ���û�
	WORD								wCurrentUser;						//��ǰ�ֵ������
	LONGLONG							lCurScore;							//��ǰ���з���

	WORD								lUserChouMa[GAME_PLAYER][16];
	WORD								wXiaZhuCount[GAME_PLAYER];

	//�˿���Ϣ
	BYTE								cbHandCardData[GAME_PLAYER][MAX_COUNT];//�����˿�
};

//�û���ׯ
struct CMD_S_CallBanker
{
	WORD								wCallChairID;						//��ׯ�û�
	BYTE								cbCallDouble;						//����
};

//��Ϸ��ʼ
struct CMD_S_GameStart
{
	//��ע��Ϣ
	WORD								wBankerUser;						//ׯ���û�
	WORD								wCurrentUser;
//	bool								bQiangPlayer[GAME_PLAYER];			//��ׯ���
	WORD								wCurrentScore;							//��ǰע��С
};

//�û���ע
struct CMD_S_AddScore
{
	WORD								wAddScoreUser;						//��ע�û�
	LONGLONG							lAddScoreCount;						//��ע��Ŀ
	bool								bAn;								//�Ƿ�Ϊ��ע
};

//��Ϸ����
struct CMD_S_GameEnd
{
	LONGLONG							lGameTax[GAME_PLAYER];				//��Ϸ˰��
	LONGLONG							lGameScore[GAME_PLAYER];			//��Ϸ�÷�
	BYTE								cbCardData[GAME_PLAYER][MAX_COUNT];	//�û��˿�
	bool								bBiPaiUser[GAME_PLAYER][GAME_PLAYER];//����������
};

//�������ݰ�
struct CMD_S_SendCard
{
	BYTE								cbCardData[GAME_PLAYER][MAX_COUNT];	//�û��˿�
};

//�������ݰ�
struct CMD_S_AllCard
{
	bool								bAICount[GAME_PLAYER];
	BYTE								cbPlayStatus[GAME_PLAYER];			//��Ϸ״̬
	BYTE								cbCardData[GAME_PLAYER][MAX_COUNT];	//�û��˿�
	bool                            bIsQiang;                         //�Ƿ�����ׯ
};

//�û��˳�
struct CMD_S_PlayerExit
{
	WORD								wPlayerID;							//�˳��û�
};

//�û�̯��
struct CMD_S_Open_Card
{
	WORD								wPlayerID;							//̯���û�
	BYTE								bOpen;								//̯�Ʊ�־
	//BYTE								bType;								//�û��Ƶ�����
	//BYTE								bPoint;								//�û��Ƶĵ���
};

//////////////////////////////////////////////////////////////////////////
//�ͻ�������ṹ
#define SUB_C_CALL_BANKER				1									//�û���ׯ
#define SUB_C_ADD_SCORE					2									//�û���ע
#define SUB_C_OPEN_CARD					3									//�û�̯��
#define SUB_C_COMP_CARD					4									//�û�����

#define	SUB_C_GEN_SCORE					6									//��ס
#define SUB_C_JIA_SCORE					7									//��ע
#define	SUB_C_KANPAI					8									//����
#define	SUB_C_QIPAI						9									//����
#define	SUB_C_BIPAI						10									//����

#define SUB_C_HAVE_SCORE				11									//��ҷ���

struct CMD_C_HaveScore
{
	LONGLONG lHaveScore[GAME_PLAYER];
};

struct CMD_C_GuoDi
{
	WORD								wUserCharID;
	LONGLONG							lGuodiScore;
};

struct CMD_C_Gen
{
	WORD								wUserCharID;
	LONGLONG							lScore;
};

struct CMD_C_JiaZhu
{
	WORD								wUserCharID;
	LONGLONG							lScore;
	bool								bAn;								//�Ƿ�Ϊ��ע
};

struct CMD_C_KanPai
{
	WORD								wUserCharID;
};

struct CMD_C_QiPai
{
	WORD								wUserCharID;
};

struct CMD_C_BiPai
{
	WORD								wUserCharID;
	WORD								toCompUserChairID;
	LONGLONG							lScore;
};


//�û���ׯ
struct CMD_C_CallBanker
{
	BYTE								bBanker;							//��ׯ��־
};

//�ն�����
struct CMD_C_SPECIAL_CLIENT_REPORT        
{
	WORD                                wUserChairID;                       //�û���λ
};

//�û���ע
struct CMD_C_AddScore
{
	bool								bAn;								//�Ƿ�Ϊ��ע
	LONGLONG							lScore;								//��ע��Ŀ
};

//�û�̯��
struct CMD_C_OxCard
{
	BYTE								bOX;								//ţţ��־
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//���ƶ���
#define SUB_C_AMDIN_COMMAND			5									//����Ա����

#define RQ_OPTION_CANCLE		1		//ȡ��
#define RQ_OPTION_QUERYING		2		//��ѯ
#define RQ_OPTION_SETING		3		//����

#define CHEAT_TYPE_LOST		0		//��
#define CHEAT_TYPE_WIN		1		//Ӯ

struct CMD_C_AdminReq
{
	BYTE cbReqType;							//��������
	BYTE cbCheatCount;						//���ƴ���
	BYTE cbCheatType;						//��������
	DWORD dwGameID;							//��ұ�ʶ
};

#pragma pack()
#endif
