#include <iostream>
#include <vector>
#include <array>
#include <mutex>
#include <condition_variable>


class FixedMutexQueue {
private:
	std::vector<uint8_t> array;
	std::mutex mutex_pop, mutex_push, mutex_common;
	std::condition_variable cond_var_push, cond_var_pop;
	size_t head, tail, size;

public:
	FixedMutexQueue(size_t size) : head(0), tail(0), size(0) {
		array.resize(size);
	}
	void push(uint8_t value);
	bool pop(uint8_t& value);
};

void FixedMutexQueue::push(uint8_t value) {
	std::unique_lock<std::mutex> lock(mutex_push);
	cond_var_push.wait(lock, [&] { return size < array.size(); });
	array[tail % array.size()] = value;
	tail++;
	{
		std::lock_guard<std::mutex> lck(mutex_common);
		size++;
	}
	cond_var_pop.notify_one();
}

bool FixedMutexQueue::pop(uint8_t &value) {
	std::unique_lock<std::mutex> lock(mutex_pop);
	if (cond_var_pop.wait_for(lock, std::chrono::milliseconds(1), [&] {return size > 0; })) {
		value = array[head % array.size()];
		head++;
		{
			std::lock_guard<std::mutex> lck(mutex_common);
			size--;
		}
		cond_var_push.notify_one();
		return true;
	}
	else {
		return false;
	}
}







class FixedMutexQueue2 {
private:
	std::vector<uint8_t> array;
	std::mutex mutex;
	std::condition_variable condition_variable_push, condition_variable_pop;
	size_t head, tail, size;

public:
	FixedMutexQueue2(size_t size) : head(0), tail(0), size(0) {
		array.resize(size);
	}
	void push(uint8_t value);
	bool pop(uint8_t& value);
};

void FixedMutexQueue2::push(uint8_t value) {
	std::unique_lock<std::mutex> lock(mutex);
	condition_variable_push.wait(lock, [&] { return size < array.size(); });
	array[tail % array.size()] = value;
	tail++;
	size++;
	lock.unlock();
	condition_variable_pop.notify_one();
}

bool FixedMutexQueue2::pop(uint8_t &value) {
	std::unique_lock<std::mutex> lock(mutex);
	if (condition_variable_pop.wait_for(lock, std::chrono::milliseconds(1), [&] {return size > 0; })) {
		value = array[head % array.size()];
		head++;
		size--;
		lock.unlock();
		condition_variable_push.notify_one();
		return true;
	}
	else {
		return false;
	}
}
