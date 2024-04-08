#ifndef _MYNET_EVBUFFER_H
#define _MYNET_EVBUFFER_H
#include "std.h"
namespace mynet{

struct Chain {
public:
    Chain();
    void Reset();
	static int32_t CalculateCap(int32_t nBufCap);
	struct Chain *m_lpNext = nullptr;
	int32_t m_nCapacity = 0;// 缓存区容量
	int32_t m_nOff = 0;// 有效数据偏移
	int32_t m_nLen = 0;// 有效数据尺寸
	char *m_lpBuffer = nullptr;
};

Chain* AllocChain(int32_t nCap);
void FreeChain(Chain* lpChain);
struct Buffer {
public:
	char* GetBuff();
	int32_t GetLength();
	void Drain(int32_t nLength);
	int32_t PutData(char* lpBuf, int32_t nLen);
	void Reset();
	int32_t GetSize(){
		return m_nSize;
	}
private:
	Chain *m_lpFirst = nullptr;
	Chain *m_lpLast = nullptr;
    int32_t m_nChainNum = 0;
    int32_t m_nSize = 0;
	std::map<int32_t, Chain*> m_stFixChainMap;
};
}
#endif