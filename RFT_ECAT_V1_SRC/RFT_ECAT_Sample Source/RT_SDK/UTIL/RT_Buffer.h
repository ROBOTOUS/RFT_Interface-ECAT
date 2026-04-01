#ifndef __RT_BUFFER_H__
#define __RT_BUFFER_H__
#pragma once

#include <mutex>

#ifndef uint32
typedef unsigned int uint32;
#endif

template <typename T>
class RT_CircularBuffer
{
public:
	RT_CircularBuffer()
	{
		mtx_write = new std::mutex;
	}
	~RT_CircularBuffer()
	{
		delete_buf();
		delete mtx_write;
	}
	// set buffer size. buffer size should be larger than 1
	void set_size(uint32 buffer_size)
	{
		if (buffer_size < 2)
			return;
		delete_buf();
		size = buffer_size;
		buffer = new T[size];
		clear();
	}
	// save to buffer 
	void write(T& data)
	{
		if (size < 2) // buffer area not defined
			return;
		lock_guard<mutex> lg(*mtx_write);

		write_idx++;
		write_idx = write_idx % size;
		buffer[write_idx] = data;
		num_of_data_save++;
		if (read_idx == write_idx)
		{
			read_idx = (read_idx+1) % size;
			num_of_lost_data++;
		}
	}
	// num of datas saved newly
	uint32 get_new_data_size()
	{
		lock_guard<mutex> lg(*mtx_write);
		if (read_idx <= write_idx)
			return write_idx - read_idx;
		else
			return size - (read_idx - write_idx);
	}
	// read all of new data. return the size of new data read.
	uint32 read_new_data(T *data)
	{
		if (size < 2)
			return 0;
		uint32 read_size = get_new_data_size();
		uint32 returned_size;
		read(data, read_size, returned_size);
		return returned_size;
	}
	// read data from the last read.
	bool read(T *data, uint32 read_size, uint32& returned_size)
	{
		if (size < 2)
			return false;
		uint32 new_size = get_new_data_size();
		if (new_size < read_size)
			returned_size = new_size;
		else
			returned_size = read_size;
		mtx_write->lock();
		for (uint32 i = 0; i < returned_size; i++)
		{
			read_idx++;
			read_idx %= size;
			data[i] = buffer[read_idx];
		}
		mtx_write->unlock();
		return true;
	}


	// read the latest data saved. return acutal read count
	// 저장된 숫자가 버퍼사이즈 보다 작으면 저장된 숫자만 읽어 들어 들이고, 해당 카운트를 반환.
	// 주의: 읽지 않은 데이터보다 읽으려는 데이터 수가 작아도 읽고 나면, read idx가 write_idx로 바뀜.
	uint32 read_latest(T *data, uint32 read_size)
	{
		if (size < 2)
			return 0;

		// read sizet가 버퍼 사이즈 보다 크면 버퍼사이즈로 제한 한다.
		if (read_size > size)
			read_size = size;

		mtx_write->lock();
		// 저장된 데이터 수가 read count보다 작으면 저장된 수로 제한
		if (read_size > num_of_data_save)
			read_size = num_of_data_save;
		uint32 count_to_zero_idx;
		uint32 count_back; // 버퍼의 끝부터 읽어야할 카운트
		if ((write_idx + 1) >= read_size)
		{
			count_to_zero_idx = read_size;
			count_back = 0;
		} 
		else
		{
			count_to_zero_idx = write_idx + 1;
			count_back = read_size - count_to_zero_idx;
		}
		for (uint32 i = 0; i < count_to_zero_idx; i++)
		{
			data[i] = buffer[write_idx - i];
		}
	//	T *data_back = &data[count_to_zero_idx];
		for (uint32 i = 0 ; i < count_back; i++)
		{
			data[count_to_zero_idx + i] = buffer[size - 1 - i];
		}
		read_idx = write_idx;
		mtx_write->unlock();

		return read_size;
	}
	// read all data saved. return acutal read count
	uint32 read_all(T *data)
	{
		return read_latest(data,size);
	}
	// 모든 인덱스를 초기화시켜 내용만 다 지운다.
	void clear()
	{
		mtx_write->lock();
		read_idx = 0;
		write_idx = 0;
		num_of_lost_data = 0;
		num_of_data_save = 0;
		mtx_write->unlock();
	}

private:
	T *buffer;
	uint32 size = 0;		// buffer size
	uint32 read_idx = 0;	// 마지막으로 읽은 위치 
	uint32 write_idx = 0;	// 마지막으로 쓰여진 위치
	uint32 num_of_lost_data = 0;
	uint32 num_of_data_save = 0;

	std::mutex *mtx_write;  // 뮤텍스 객체는 복사가 되지 않기 때문에 new로 버퍼가 생성될 때를 대비하여 

	void delete_buf()
	{
		delete[] buffer;
		size = 0;
		read_idx = 0;
		write_idx = 0;
		num_of_lost_data = 0;
		num_of_data_save = 0;
	}
};

#endif // end of __RT_BUFFER_H__