#ifndef CELL_STREAM_HPP
#define CELL_STREAM_HPP
#include "GlobalDef.hpp"
#include <cstdint>

class CellStream
{
public:
	CellStream(char *pBuf, uint32_t nSize, bool isDelete = true):_pBuf(pBuf),
		_nSize(nSize), _isDelete(isDelete), _wtirePos(_nSize), _readPos(0)
	{

	}
	CellStream(uint32_t nSize = 1024) :_pBuf(nullptr), _nSize(nSize), _isDelete(true),
		_wtirePos(0), _readPos(0)
	{
		_pBuf = new char[_nSize];
	}

	virtual ~CellStream() {
		if (_isDelete && _pBuf)
		{
			delete _pBuf;
			_pBuf = nullptr;
		}
	}

	template<class Type>
	bool write(Type &t)
	{
		auto typelen = sizeof(Type);
		if (_wtirePos + typelen <= _nSize)
		{
			memcpy(_pBuf + _wtirePos, &t, typelen);
			
			_wtirePos += typelen;
			return true;
		}

		return false;
	}

	template<class Type>
	bool write_array(Type *t, uint32_t len)
	{
		auto typelen = sizeof(Type) * len;
		if (_wtirePos + sizeof(uint32_t) + typelen <= _nSize)
		{
			// 先写入数组长度
			write(len);
			memcpy(_pBuf + _wtirePos, t, typelen);
			_wtirePos += typelen;
			return true;
		}

		return false;
	}



	bool write_int8(int8_t value)
	{
		return write(value);
	}

	bool write_int16(int16_t value)
	{
		return write(value);
	}

	bool write_int32(int32_t value)
	{
		return write(value);
	}

	bool write_int64(int64_t value)
	{
		return write(value);
	}

	bool write_uint8(uint8_t value)
	{
		return write(value);
	}

	bool write_uint16(uint16_t value)
	{
		return write(value);
	}

	bool write_uint32(uint32_t value)
	{
		return write(value);
	}

	bool write_uint64(uint64_t value)
	{
		return write(value);
	}

	bool write_float(float value)
	{
		return write(value);
	}

	bool write_double(double value)
	{
		return write(value);
	}

	template<class Type>
	bool read(Type &t, bool addpos = true)
	{
		auto typelen = sizeof(Type);
		if (_readPos + typelen <= _wtirePos)
		{
			memcpy(&t, _pBuf + _readPos, typelen);
			if (addpos)
				_readPos += typelen;
			return true;
		}
		return false;
	}

	template<class Type>
	Type read(bool addpos = true)
	{
		auto typelen = sizeof(Type);
		Type ret{0};
		if (_readPos + typelen <= _wtirePos)
		{
			memcpy(&ret, _pBuf + _readPos, typelen);
			if (addpos)
				_readPos += typelen;
		}
		return ret;
	}

	template<class Type>
	bool read_array(Type *t, uint32_t len)
	{
		uint32_t arrlen;
		read(arrlen, false);
		if (arrlen <= len)
		{
			//auto typelen = sizeof(Type) * len;
			if (_readPos + sizeof(uint32_t) + arrlen <= _wtirePos)
			{
				_readPos += sizeof(uint32_t);
				//printf("%u-%s\n", arrlen, _pBuf + _readPos);
				memcpy(t, _pBuf + _readPos, arrlen);
				_readPos += arrlen;
				return true;
			}
			
		}

		return false;
	}

	////读取以'\0'结尾的字符串
	//bool readCstring(char *str)
	//{
	//	//保存读取指针  如果失败则不递增
	//	uint32_t tmp = _readPos;
	//	while (tmp <=_wtirePos)
	//	{
	//		memcpy(str++, _pBuf + tmp++, 1);
	//		/*printf("%c\n", *str);*/
	//		if (*str == '\0')
	//		{
	//			_readPos = tmp;
	//			return true;
	//		}
	//	}

	//	return false;
	//}

	bool read_int8(int8_t &value)
	{

		return read(value);
	}

	bool read_int16(int16_t &value)
	{
		return read(value);
	}

	bool read_int32(int32_t &value)
	{
		return read(value);
	}

	bool read_int64(int64_t &value)
	{
		return read(value);
	}

	bool read_uint8(uint8_t &value)
	{
		return read(value);
	}

	bool read_uint16(uint16_t &value)
	{
		return read(value);
	}

	bool read_uint32(uint32_t &value)
	{
		return read(value);
	}

	bool read_uint64(uint64_t &value)
	{
		return read(value);
	}

	bool read_float(float &value)
	{
		return read(value);
	}

	bool read_double(double &value)
	{
		return read(value);
	}


	int8_t read_int8()
	{
		return read<int8_t>();
	}

	int16_t read_int16()
	{
		return read<int16_t>();
	}

	int32_t read_int32()
	{
		return read<int32_t>();
	}

	int64_t read_int64()
	{
		return read<int64_t>();
	}

	uint8_t read_uint8()
	{
		return read<uint8_t>();
	}

	uint16_t read_uint16()
	{
		return read<uint16_t>();
	}

	uint32_t read_uint32()
	{
		return read<uint32_t>();
	}

	uint64_t read_uint64()
	{
		return read<uint64_t>();
	}

	float read_float()
	{
		return read<float>();
	}

	double read_double()
	{
		return read<double>();
	}

	char* data()
	{
		return _pBuf;
	}

	uint32_t length()
	{
		return _wtirePos;
	}
private:
	char * _pBuf;//数据缓冲区
	uint32_t _nSize;//数据缓冲区大小(字节)
	bool _isDelete;

protected:
	uint32_t _wtirePos;//已写入大小 (字节)
	uint32_t _readPos;//已读取大小(字节)


};




#endif