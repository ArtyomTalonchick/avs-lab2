#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <condition_variable>

class FixedAtomicQueue {
private:
	std::vector<uint8_t> array;
	std::atomic_size_t head, tail, size;
	std::mutex mutex_pop, mutex_push;
	std::condition_variable cond_var_pop, cond_var_push;
	std::atomic_bool free;

public:
	FixedAtomicQueue(size_t size) : free(true), head(0), tail(0), size(0) {
		array.resize(size);
	}

	bool pop(uint8_t& value);
	void push(uint8_t value);
};

void FixedAtomicQueue::push(uint8_t value) {
	while (true) {
		bool required = true;
		bool is_free = free.compare_exchange_strong(required, false);
		while (!is_free) {
			std::unique_lock<std::mutex>lock(mutex_push);
			cond_var_push.wait(lock, [&]() {
				bool required = true;
				return free.compare_exchange_strong(required, false);
			});
			is_free = true;
		}
		if (size < array.size()) {
			break;
		}
		else {
			free = true;
			cond_var_pop.notify_one();
		}
	}
	array[tail++ % array.size()] = value;
	size++;
	free = true;
	cond_var_pop.notify_one();
}

bool FixedAtomicQueue::pop(uint8_t &value) {
	bool result = false;
	bool required = true;
	bool is_free = free.compare_exchange_strong(required, false);
	if (!is_free) {
		std::unique_lock < std::mutex > lock(mutex_pop);
		is_free = cond_var_pop.wait_for(lock, std::chrono::milliseconds(1), [&]() {
			bool required = true;
			return free.compare_exchange_strong(required, false);
		});
	}
	if (is_free) {
		if (size == 0) {
			result = false;
		}
		else {
			value = array[head++ % array.size()];
			size--;
			result = true;
		}
		free = true;
		cond_var_push.notify_one();
	}
	return result;
}






//class FixedAtomicQueue2 {
//private:
//	std::vector<uint8_t> array;
//	std::atomic_size_t head, tail, size;
//	std::mutex mutex;
//	std::condition_variable condition_variable;
//	std::atomic_bool free;
//	std::atomic_int index_push, index_pop;
//
//
//public:
//	FixedAtomicQueue2(size_t size) : free(true), head(0), tail(0), size(0), index_push(0), index_pop(0) {
//		array.resize(size);
//		for (auto x : array) {
//			x = NULL;
//		}
//	}
//
//	bool pop(uint8_t& value);
//	void push(uint8_t value);
//};
//
//void FixedAtomicQueue2::push(uint8_t value) {
//	while (true) {
//		int newIndex = index_push++;
//		if (array[newIndex] != NULL) {
//			index_push--;
//			continue;
//		}
//		else {
//			array[newIndex % array.size()] = value;
//		}
//	}
//}
//
//bool FixedAtomicQueue2::pop(uint8_t &value) {
//	int newIndex = index_pop++;
//	if (array[newIndex] != NULL) {
//		value = array[newIndex];
//		array[newIndex] = NULL;
//		return true;
//	}
//	std::this_thread::sleep_for(std::chrono::milliseconds(1));
//	if (array[newIndex] != NULL) {
//		value = array[newIndex];
//		array[newIndex] = NULL;
//		return true;
//	}
//	return false;
//
//}

