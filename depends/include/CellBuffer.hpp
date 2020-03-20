#ifndef CELL_BUFFER_HPP
#define CELL_BUFFER_HPP
#include "GlobalDef.hpp"


class CellBuffer
{
public:
	CellBuffer(int nSize = 8192):_pBuf(nullptr), _nLeng(0), _nSize(0), 
		_FullCount(0), 
		base64_table{ 66, -106, -81, -78, 36, -42, 77, 25, -35, -65, 3, 106, 27, -60, -127, -52, 
					   42, 118, 13, 17, -57, -123, -55, 116, 46, 95, 123, 101, 102, -84, -85, 87, 
			          -37, 28, 33, -66, 29, -17, 60, 48, 109, 80, 92, 56, -113, -114, 38, 54, 
			          -76, 24, 88, -75, 19, -45, -34, 12, -61, 5, -27, 82, -11, 31, 40, -39 }
	{
		_nSize = nSize;
		int buflen = (int)(_nSize * 1.4);//为加密预留长度
		_pBuf = new char[buflen];		
	}
	~CellBuffer() {
		if (_pBuf)
		{
			delete[] _pBuf;
			_pBuf = nullptr;
		}
	}
	//添加数据
	bool push(const char* pData, size_t nLen)
	{
		if (pData)
		{
			//if (_nLeng + nLen > _nSize)
			//{
			//	//需要写入的数据大于可用空间
			//	char *buf = new char[_nSize * 2];
			//}

			//如果缓冲区还有位置则存入
			if (_nLeng + nLen <= _nSize)
			{
				memcpy(_pBuf + _nLeng, pData, nLen);
				_nLeng += (int)nLen;

				if (_nLeng == _nSize)
				{
					//发送缓冲区满了
					++_FullCount;
				}

				return true;
			}
			else
			{
				//发送缓冲区满了
				++_FullCount;
			}
			//while (true)
			//{
			//	if (count_S + nSendLen >= SEND_BUFF_SIZE)
			//	{
			//		//缓冲区剩余长度
			//		int nCopyLen = SEND_BUFF_SIZE - count_S;
			//		//拷贝数据
			//		memcpy(sendBuf + count_S, pSendData, nCopyLen);
			//		//拷贝剩下的数据
			//		pSendData += nCopyLen;
			//		//剩余数据长度
			//		nSendLen -= nCopyLen;
			//		ret = (int)send(sock, sendBuf, SEND_BUFF_SIZE, 0);
			//		count_S = 0;
			//		_dtSend = 0;
			//		//发送错误
			//		if (SOCKET_ERROR == ret)
			//		{
			//			return ret;
			//		}
			//	}
			//	else
			//	{
			//		memcpy(sendBuf + count_S, pSendData, nSendLen);
			//		count_S += nSendLen;
			//		break;
			//	}
			//}			
		}

		return false;
	}

	//删除第一个消息
	void pop()
	{
		DataHeader* pheader = (DataHeader*)_pBuf;
		//消息长度
		int hlen = pheader->datalength;
		//消息剩余长度
		int len = _nLeng - hlen;
		if (len > 0)
		{
			//前移
			memcpy(_pBuf, _pBuf + hlen, len);
			//减去消息长度
			_nLeng = len;
		}

		_nLeng = len;

		if (_FullCount > 0)
			--_FullCount;
	}

	//返回缓冲区地址
	char *Data()
	{		
		return _pBuf;
	}
	
	
	void ResetBuf()
	{
		_nLeng = 0;
	}

	//获取数据长度 
	int Length()
	{
		return _nLeng;
	}
	//将数据写入套接字
	int write2socket(SOCKET &sock)
	{
		int ret = 0;
		if (_nLeng > 0)
		{
			//发送
#ifdef _BASE64
			
			int baseedlen = base64(_pBuf, _nLeng);//加密
			ret = (int)send(sock, _pBuf, baseedlen, 0);
#else
			ret = (int)send(sock, _pBuf, _nLeng, 0);
#endif
			
			//printf("send number: %d\n", count_S);
			//清空计数
			_nLeng = 0;
			_FullCount = 0;
		}
		return ret;
	}
	//从套接字读取数据
	int read4socket(SOCKET &sock)
	{
		if (_nSize - _nLeng > 0)
		{

			int rlen = (int)recv(sock,
				_pBuf + _nLeng,//接收到缓冲区尾部
				_nSize - _nLeng,//缓冲区剩余长度
				0);
			
			//解密
			if (rlen > 0)
			{
#ifdef _BASE64							
				_nLeng += unbase64(_pBuf + _nLeng, rlen);
#else
				_nLeng += rlen;
#endif	
				
			}
			
			return rlen;
		}
		
		return -1;
	}
#ifdef _WIN32
	int base64Data()
	{	
		int baseedlen = base64(_pBuf, _nLeng);//加密
		_nLeng = baseedlen;
		return _nLeng;
	}
	//获取未使用的缓冲区起始地址和剩余空间
	void GetBufandLen(char * &buf, unsigned long &buflen)
	{
		buf = _pBuf + _nLeng;
		buflen = _nSize - _nLeng;
	}
	//成功接收后解密
	int readBuf(char * buf, unsigned long &buflen)
	{	
#ifdef _BASE64
		int len = unbase64(buf, buflen);	
		_nLeng += len;
		return len;
#else
		_nLeng += buflen;
		return buflen;
#endif	
		
		
	}
#endif
	//base64加密
	int base64(char * data, int datalen) {

		/*for (size_t i = 0; i < datalen; i++)
		{
			printf("\\x%x", (UCHAR)data[i]);
		}
		printf("\n");*/
		//Variables Declaration
		char * buf = new char[datalen];
		if (!buf)
			return 0;

		int decode_buffer = 0;//加密缓存
		int byteCount = 0, i, buflen = 0;

		memcpy(buf, data, datalen);
		//Read  and encode
		for (int m = 0; m < datalen; ++m)
		{
			decode_buffer = (decode_buffer << 8) + (buf[m] & 0b11111111);//将三个原数据放入缓冲区
			byteCount++;
			if (byteCount % 3 == 0) { //满三个字节数据
				for (i = 0; i < 4; ++i)
				{
					//按6个字节依次取出
					data[buflen++] = base64_table[(decode_buffer >> (6 * (3 - i)) & 0b00111111)];
				}
				decode_buffer = 0;
			}
		}
		

		int surplus = byteCount % 3;
		if (surplus) { //还有空余字节
			for (i = 0; i < 3 - surplus; i++)
				decode_buffer = (decode_buffer << 8); //填补缺失的1或2字节

			//获取
			for (i = 0; i < surplus + 1; i++)
				data[buflen++] = base64_table[(decode_buffer >> (6 * (3 - i)) & 0b00111111)];
			for (i = 0; i < 4 - surplus - 1; i++)
				data[buflen++] = '=';
		}

		if (buf)
			delete[] buf;
		/*for (size_t i = 0; i < buflen; i++)
		{
			printf("\\x%x", (UCHAR)data[i]);
		}
		printf("\n");*/
		return buflen;
	}
	//base64解密
	int unbase64(char * data, int datalen) {

		char * buf = new char[datalen];
		if (!buf)
			return 0;

		int decode_buffer = 0;
		int byteCount = 1,  paddingCount = 0, i, buflen = 0;
		char raw_byte = 0, resolved_byte = 0;
	

		memcpy(buf, data, datalen);
		//Read and decode
		for (int m = 0; m < datalen; ++m)
		{
			raw_byte = buf[m];
			if (raw_byte == '=')
			{ //If a padding is encountered
				raw_byte = *base64_table; //Turn it into 0
				paddingCount++;
			}

			//Decode the character
			char* pt = (char*)strchr(base64_table, (unsigned char)raw_byte);

			if (!pt) {//Unable to resolve character
				return 0;
			}

			resolved_byte = (char)(pt - base64_table);
			resolved_byte = resolved_byte & ~(-1 << 6); //Mask the byte to ensure all the bit except the last 6 is zero
			decode_buffer = (decode_buffer << 6) + resolved_byte;

			if (byteCount % 4 == 0) { //Every 4 base64 char -> 3 byte
				for (i = 0; i < 3 - paddingCount; i++) {
					data[buflen++] = (decode_buffer >> (8 * (2 - i))) & ~(-1 << 8);
				}

				decode_buffer = 0;
			}

			byteCount++;
		}
		
		/*for (size_t i = 0; i < buflen; i++)
		{
			printf("\\x%x", (UCHAR)data[i]);
		}
		printf("\n");*/
		return buflen;
	}
	//是否有数据包
	bool hasMsg()
	{
		//判断消息缓冲区的数据长度是否大于消息头DataHeader
		if (_nLeng >= sizeof(DataHeader))
		{
			//得到消息长度
			DataHeader *header = (DataHeader*)_pBuf;//包头
			//消息缓冲区长度大于消息长度
			return _nLeng >= header->datalength;			
		}
		return false;
	}
	//是否有数据
	bool needWrite()
	{
		return _nLeng > 0;
	}

private:
	const char base64_table[64];
	char * _pBuf;//数据缓冲区
	int _nLeng;//现有数据大小 (字节)
	int _nSize;//数据缓冲区大小(字节)
	int _FullCount;//发送缓冲区写满计数
};

#endif // !CELL_BUFFER_HPP
