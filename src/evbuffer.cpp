#include "evbuffer.h"
namespace mynet{
Chain::Chain(){

}
void Chain::Reset(){
    m_lpNext = nullptr;
	m_nOff = 0;// 有效数据偏移
	m_nLen = 0;// 有效数据尺寸
}

int32_t Chain::CalculateCap(int32_t nBufCap){
	int32_t nCount = (sizeof(Chain) + nBufCap) / (4 * 1024);
	int32_t nRem = (sizeof(Chain) + nBufCap) % (4 * 1024);
	if(nRem > 0){
		nCount++;
	}
	return nCount * 4 * 1024;
}

Chain* AllocChain(int32_t nCap){
	nCap = Chain::CalculateCap(nCap);
	Chain* lpChain = (Chain*)malloc(nCap);
	if(lpChain == nullptr){
		return nullptr;
	}

	lpChain->m_lpBuffer = (char*)lpChain + sizeof(Chain);
	lpChain->m_lpNext = nullptr;
	lpChain->m_nCapacity = nCap;
	lpChain->m_nLen = 0;
	lpChain->m_nOff = 0;
	return lpChain;
}
void FreeChain(Chain* lpChain){
    if(lpChain == nullptr){
		return;
	}

	free(lpChain);
}

char* Buffer::GetBuff(){
	if(m_lpFirst){
		return m_lpFirst->m_lpBuffer + m_lpFirst->m_nOff;
	}else{
		return nullptr;
	}
}
int32_t Buffer::GetLength(){
	if(m_lpFirst){
		return m_lpFirst->m_nLen;
	}else{
		return 0;
	}
}
void Buffer::Drain(int32_t nLength){
	assert(nLength <= m_lpFirst->m_nLen);
	if(m_lpFirst){
		m_lpFirst->m_nOff += nLength;
		m_lpFirst->m_nLen -= nLength;
		m_nSize -= nLength;
		if(m_lpFirst->m_nLen == 0){
			Chain* lpCur = m_lpFirst;
			m_lpFirst = m_lpFirst->m_lpNext;
			lpCur->Reset();
			if(m_lpFirst == nullptr){
				m_lpLast = nullptr;
			}
			auto iter = m_stFixChainMap.find(lpCur->m_nCapacity);
			if(iter == m_stFixChainMap.end()){
				m_stFixChainMap.insert(std::pair<int32_t, Chain*>(lpCur->m_nCapacity, lpCur));
			}else{
				iter->second->m_lpNext = lpCur;
			}
		}
	}else{
		return;
	}
}
int32_t Buffer::PutData(char* lpBuf, int32_t nLen){
	if(m_lpLast){
		// 计算余量
		int32_t nRem = m_lpLast->m_nCapacity - m_lpLast->m_nOff - m_lpLast->m_nLen;
		int32_t nReal = nRem;
		if(nReal > nLen){
			nReal = nLen;
		}
		memcpy(m_lpLast->m_lpBuffer+m_lpLast->m_nOff+m_lpLast->m_nLen, lpBuf, nReal);
		m_lpLast->m_nLen += nReal;
		m_nSize += nReal;
		nLen -= nReal;
		lpBuf += nReal;
		if(nLen){
			// 确定新块尺寸
			int32_t nCapSize = Chain::CalculateCap(nLen);
			Chain* lpNewChain = nullptr;
			auto iter = m_stFixChainMap.find(nCapSize);
			if(iter != m_stFixChainMap.end()){
				// 取块
				Chain* lpCur = iter->second;
				Chain* lpNext = lpCur->m_lpNext;
				lpCur->Reset();
				if(lpNext){
					(*iter).second = lpNext;
				}else{
					m_stFixChainMap.erase(iter->first);
				}
				lpNewChain = lpCur;
			}else{
				lpNewChain = AllocChain(nLen);
				if(lpNewChain == nullptr){
					return -1;
				}
			}

			// 剩余部分放入新块
			memcpy(lpNewChain->m_lpBuffer, lpBuf, nLen);
			lpNewChain->m_nLen = nLen;
			m_nSize += nLen;
			// 新块入队列
			m_lpLast->m_lpNext = lpNewChain;
			m_lpLast = lpNewChain;
		}		
	}
	else{
		// 分配新块，数据写入新块，新块入队列
		Chain* lpNewChain = (Chain*)AllocChain(nLen);
		if(lpNewChain == nullptr){
			return -1;
		}
		memcpy(lpNewChain->m_lpBuffer, lpBuf, nLen);
		lpNewChain->m_nLen = nLen;
		m_nSize += nLen;
		m_lpFirst = lpNewChain;
		m_lpLast = lpNewChain;
	}
	return 0;
}

void Buffer::Reset(){
	Chain* lpChain = m_lpFirst;
    while(lpChain){
        Chain* lpCur = lpChain;
        lpChain = lpChain->m_lpNext;
        lpCur->Reset();
        FreeChain(lpCur);
    }
    for(auto iter = m_stFixChainMap.begin(); iter != m_stFixChainMap.end(); iter++){
        Chain* lpChain = iter->second;
        while(lpChain){
            Chain* lpCur = lpChain;
            lpChain = lpChain->m_lpNext;
            FreeChain(lpCur);
        }
    }
	m_lpFirst = nullptr;
	m_lpLast = nullptr;
	m_nSize = 0;
	m_stFixChainMap.clear();
}
}