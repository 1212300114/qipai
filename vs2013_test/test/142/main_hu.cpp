#include <iostream>
#include <windows.h>
#include <vector>

#include "Define.h"
#include "DefineHuTip.h"
#include "PlayerHuTips.h"

using namespace std;

static BYTE s_HuCardAll[] = 
{
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,				//��
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,				//��
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,				//��
	0x31, 0x32, 0x33, 0x34,                                             //��������
	0x41, 0x42, 0x43,                                                   //�з���

	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,				//��
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,				//��
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,				//��
	0x31, 0x32, 0x33, 0x34,                                             //��������
	0x41, 0x42, 0x43,                                                   //�з���

	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,				//��
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,				//��
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,				//��
	0x31, 0x32, 0x33, 0x34,                                             //��������
	0x41, 0x42, 0x43,                                                   //�з���

	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,				//��
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,				//��
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,				//��
	0x31, 0x32, 0x33, 0x34,                                             //��������
	0x41, 0x42, 0x43,                                                   //�з���
};

#define MAX_COUNT 1000000

void main()
{
	srand(GetTickCount()+time(0));
	CHuTipsMJ		m_cAlgorithm;
	CPlayerHuTips	cHuTips(&m_cAlgorithm);
		
	DWORD dwTimeBegin = GetTickCount();
	BYTE byNaiZi = s_HuCardAll[135];
	BYTE byCards[14] = {};
	int  nAll = 0;
	//BYTE			 byNaiZiNote[MAX_COUNT] = {};
	//vector<stNodeMJ> vctAnswer[MAX_COUNT];
	vector<stNodeMJ> vctNodeOut;
	int hu = 0;
	// �������10000*9�Σ�ÿ�ν���һ����ֵ��Ϊ����
	for (int n = 0; n<MAX_COUNT; ++n)
	{
		random_shuffle(s_HuCardAll, s_HuCardAll+136);
		for (int i=0; i<9; ++i)	// 136/14 -> 9
		{		
			stCardData stData(s_HuCardAll+i*14+1, 13);	// ��Ҫ�ѵ�һ����ֵ���ȥ
			hu += cHuTips.JustTryDianHu(stData, *(s_HuCardAll+i*14), *(s_HuCardAll+i*14), vctNodeOut);
			//if (bSuc)
			//{
			//	byNaiZiNote[nAll] = *(s_HuCardAll+i*14);
			//	vctAnswer[nAll++] = vctNodeOut;
			//}
		}
	}	
	cout << "nAll:" << (int)MAX_COUNT << "  time:" << GetTickCount() - dwTimeBegin << "ms" << endl;
	cout << "hu: "<< hu << endl;
//	for (int i=0; i<nAll; ++i)
//	{
//		cout<<hex<<"i="<<i<<" NaiZi="<<(int)byNaiZiNote[i]<<endl;
//		for (size_t n=0; n<vctAnswer[i].size(); ++n)
//		{
//			vctAnswer[i][n].printNode();
//		}
//	}

	cin>>nAll;
}