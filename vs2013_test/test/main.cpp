#include <iostream>
#include <windows.h>
#include <algorithm>
#include "CMD_Ox.h"

using namespace std;

void quickSort(int nData[100], int i, int j)
{
	int iTemp = i, jTemp = j;
	while (i<j)
	{
		while(nData[i] <= nData[j] && i<j)
			--j;

		if (i<j)
			swap(nData[i], nData[j]);

		while(nData[i] <= nData[j] && i<j)
			++i;

		if (i<j)
			swap(nData[i], nData[j]);
	}
	if (iTemp < i-1)
		quickSort(nData, iTemp, i-1);
	if (i+1 < jTemp)
		quickSort(nData, i+1, jTemp);
}
//������
template<typename T>
void MinHeapify(T* arry, int size, int element)
{
	int lchild = element*2+1, rchild = lchild+1;//��������
	while(rchild < size)
	{
		if(arry[element] <= arry[lchild] && arry[element] <= arry[rchild])	//���������������С���������
			return;

		if(arry[lchild] <= arry[rchild])			//��������С
		{
			swap(arry[element], arry[lchild]);		//��������ᵽ����
			element = lchild;						//ѭ��ʱ��������
		}
		else										//����������С
		{
			swap(arry[element], arry[rchild]);		//ͬ��
			element = rchild;
		}
		lchild = element*2 + 1;
		rchild = lchild+1;							//���¼�������λ��
	}
	if(lchild<size && arry[lchild]<arry[element])	//ֻ��������������С���Լ�
	{
		swap(arry[lchild],arry[element]);
	}
}

template<typename T>
void HeapSort(T* arry, int size)
{
	int i;
	for(i=size/2; i>=0; i--)			//��������ʼ������
	{
		MinHeapify(arry, size, i);
	}
	while(size>0)						//�����
	{
		swap(arry[size-1],arry[0]);		//��������С����������ĩ����
		size--;							//����С��С
		MinHeapify(arry, size, 0);		//������
	}
}

template<typename T>
void HeapSortHalf(T* arry, int size, int nSort)
{
	int i;
	for(i=nSort/2; i>=0; i--)			//
	{
		MinHeapify(arry, nSort, i);
	}
	for (i=nSort; i<size; ++i)
	{
		if (arry[i] > arry[0])
		{
			swap(arry[i], arry[0]);
			MinHeapify(arry, nSort, 0);
		}
	}
	while(nSort>0)						//�����
	{
		swap(arry[nSort-1], arry[0]);		//��������С����������ĩ����
		nSort--;							//����С��С
		MinHeapify(arry, nSort, 0);		//������
	}
}


//��ֵ����
#define	LOGIC_MASK_COLOR			0xF0								//��ɫ����
#define	LOGIC_MASK_VALUE			0x0F								//��ֵ����

//�˿�����
#define CT_SINGLE					1									//��������
#define CT_DOUBLE					2									//��������
#define	CT_SHUN_ZI					3									//˳������
#define CT_JIN_HUA					4									//������
#define	CT_SHUN_JIN					5									//˳������
#define	CT_BAO_ZI					6									//��������
#define CT_SPECIAL					7	

//����ά��
#define CountArray(Array) (sizeof(Array)/sizeof(Array[0]))

//�˿�����
BYTE m_cbCardListData[52]=
{
	0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//���� A - K
	0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//÷�� A - K
	0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//���� A - K
	0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D	//���� A - K
};

//��ȡ��ֵ
BYTE GetCardValue(BYTE cbCardData) { return cbCardData&LOGIC_MASK_VALUE; }
//��ȡ��ɫ
BYTE GetCardColor(BYTE cbCardData) { return cbCardData&LOGIC_MASK_COLOR; }

//�߼���ֵ
BYTE GetCardLogicValue(BYTE cbCardData)
{
	//�˿�����
	BYTE bCardColor=GetCardColor(cbCardData);
	BYTE bCardValue=GetCardValue(cbCardData);
	//ת����ֵ
	return (bCardValue==1)?(bCardValue+13):bCardValue;
}

//��ȡ����
BYTE GetCardType(BYTE cbCardData[], BYTE cbCardCount)
{
	if (cbCardCount==MAX_COUNT)
	{
		//��������
		bool cbSameColor=true,bLineCard=true;
		BYTE cbFirstColor=GetCardColor(cbCardData[0]);
		BYTE cbFirstValue=GetCardLogicValue(cbCardData[0]);

		//���η���
		for (BYTE i=1;i<cbCardCount;i++)
		{
			//���ݷ���
			if (GetCardColor(cbCardData[i])!=cbFirstColor)
				cbSameColor=false;
			if (cbFirstValue != (GetCardLogicValue(cbCardData[i])+i)) 
				bLineCard=false;
		}

		//����A32
		if(!bLineCard)
		{
			bool bOne=false,bTwo=false,bThree=false;
			for(BYTE i=0;i<MAX_COUNT;i++)
			{
				if(GetCardValue(cbCardData[i])==1)		bOne=true;
				else if(GetCardValue(cbCardData[i])==2)	bTwo=true;
				else if(GetCardValue(cbCardData[i])==3)	bThree=true;
			}
			if(bOne && bTwo && bThree)bLineCard=true;
		}

		//˳������
		if ((cbSameColor)&&(bLineCard)) 
			return CT_SHUN_JIN;

		//˳������
		if ((!cbSameColor)&&(bLineCard)) 
			return CT_SHUN_ZI;

		//������
		if((cbSameColor)&&(!bLineCard)) 
			return CT_JIN_HUA;

		//���η���
		bool bDouble=false, bPanther=true;

		//���Ʒ���
		for (BYTE i=0;i<cbCardCount-1;i++)
		{
			for (BYTE j=i+1;j<cbCardCount;j++)
			{
				if (GetCardLogicValue(cbCardData[i])==GetCardLogicValue(cbCardData[j])) 
				{
					bDouble=true;
					break;
				}
			}
			if(bDouble) break;
		}

		//����(����)����
		for (BYTE i=1; i<cbCardCount;i++)
		{
			if (cbFirstValue != GetCardLogicValue(cbCardData[i])) 
				bPanther = false;
		}

		//���Ӻͱ����ж�
		if (bDouble==true) return (bPanther) ? CT_BAO_ZI : CT_DOUBLE;

		//����235
		bool bTwo=false, bThree=false, bFive=false;
		for (BYTE i=0;i<cbCardCount;i++)
		{
			if(GetCardValue(cbCardData[i])==2)	bTwo=true;
			else if(GetCardValue(cbCardData[i])==3)bThree=true;
			else if(GetCardValue(cbCardData[i])==5)bFive=true;			
		}	
		if (bTwo && bThree && bFive) return CT_SPECIAL;
	}
	return CT_SINGLE;
}

void getRandCardsByType(BYTE cbCardData[], int &nLeft, BYTE cbCardBuffer[], BYTE type)
{
	for (int i=0; i<nLeft-2; ++i)
	{
		cbCardBuffer[0] = cbCardData[i];
		for (int j=i+1; j<nLeft-1; ++j)
		{
			cbCardBuffer[1] = cbCardData[j];
			for (int k=j+1; k<nLeft; ++k)
			{
				cbCardBuffer[2] = cbCardData[k];
				BYTE byTemp = GetCardType(cbCardBuffer, 3);
				if (type == CT_SINGLE)
				{
					if (byTemp == type || byTemp == CT_SPECIAL)
					{
						std::swap(cbCardData[i], cbCardData[nLeft-1]);
						std::swap(cbCardData[j], cbCardData[nLeft-2]);
						std::swap(cbCardData[k], cbCardData[nLeft-3]);
						nLeft -= 3;
						return;
					}
				}
				else
				{
					if (byTemp == type)
					{
						std::swap(cbCardData[i], cbCardData[nLeft-1]);
						std::swap(cbCardData[j], cbCardData[nLeft-2]);
						std::swap(cbCardData[k], cbCardData[nLeft-3]);
						nLeft -= 3;
						return;
					}
				}
			}
		}
	}	
	nLeft -= 3;
}

//�����˿�(�ɿػ���)
void RandCardList_AI(BYTE cbCardBuffer[], BYTE byNum)
{
	//����׼��
	BYTE cbCardData[CountArray(m_cbCardListData)];
	CopyMemory(cbCardData,m_cbCardListData,sizeof(m_cbCardListData));
	std::random_shuffle(cbCardData, cbCardData+CountArray(m_cbCardListData));
	int nLeftCards = CountArray(m_cbCardListData);

	int nRandPer[CT_SPECIAL] = {0, 40, 25, 20, 10, 3, 2};
	for (int n=0; n<byNum; ++n)
	{
		int nRandKey = rand()%100;
		int type = CT_SINGLE;
		for ( ; type<CT_SPECIAL; ++type )
		{
			if (nRandKey < nRandPer[type])
				break;
			else
				nRandKey -= nRandPer[type];
		}
		getRandCardsByType(cbCardData, nLeftCards, cbCardBuffer+n*3, type);
	}
}

void main()
{
	srand(GetTickCount());
	int nSort[100] = {};
	for ( int i=0; i<100; ++i)
	{
		if(i%10 == 0)
			cout<<endl;
		nSort[i] = rand()%1000;
		cout<<nSort[i]<<'\t';		
	}
	
	cout<<"sorted show:"<<endl;
//	quickSort(nSort, 0, 99);
//	HeapSort(nSort, 100);
//	HeapSortHalf(nSort, 100, 40);

	DWORD dwTicks = GetTickCount();
	BYTE bTempArray[GAME_PLAYER*MAX_COUNT];

	RandCardList_AI(bTempArray, GAME_PLAYER);	

	cout<<GetTickCount()-dwTicks<<"ms"<<endl;
// 
// 	for ( int i=0; i<100; ++i)
// 	{
// 		if(i%10 == 0)
// 			cout<<endl;
// 		cout<<nSort[i]<<'\t';		
// 	}

	Sleep(100000);
}