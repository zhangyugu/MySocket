#ifndef CELL_MSG_STREAM_HPP
#define CELL_MSG_STREAM_HPP
#include"CellStream.hpp"

class CellMsgStream : public CellStream
{
public:
	CellMsgStream(DataHeader *hreader) : CellStream((char*)hreader, hreader->datalength, false)
	{ 
		//跳过消息头
		_readPos += sizeof(DataHeader);	
	}

	//CellMsgStream(char *pBuf, uint32_t nSize, bool isDelete = true) : CellStream(pBuf,  nSize, isDelete)
	//{
	//	_readPos += sizeof(DataHeader);
	//}

	CellMsgStream(uint32_t nSize = 1024) :CellStream(nSize)
	{
		//跳过消息头
		_readPos += sizeof(DataHeader);
		_wtirePos += sizeof(DataHeader);
	}
	~CellMsgStream() {

	}

	int16_t getNetCmd()
	{
		return *(int16_t*)(data() + sizeof(int16_t));
	}

	void setNetCmd(CMD value)
	{
		*(int16_t*)(data() + sizeof(int16_t)) = static_cast<int16_t>(value);
	}

	int16_t getMsglength()
	{
		*(int16_t*)data() = _wtirePos;
		return *(int16_t*)data();
	}

	DataHeader * getheader()
	{
		*(int16_t*)data() = _wtirePos;
		return (DataHeader*)data();
	}
	
	//void setMsglength(int16_t value)
	//{
	//	*(int16_t*)data() = value;
	//}

	bool writeString(const char* str)
	{
		return write_array(str, (int)strlen(str) + 1);
	}

	bool writeString(const char* str, uint32_t len)
	{
		return write_array(str, len);
	}

	bool writeString(std::string& str)
	{
		return write_array(str.c_str(), (int)str.length() + 1);
	}



private:


};




#endif // !CELL_MSG_STREAM_HPP
