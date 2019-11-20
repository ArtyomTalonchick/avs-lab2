#include <iostream>
#include <queue>
#include <mutex>
#include <thread>


class DynamicQueue {
private:
	std::queue<uint8_t> queue;
	std::mutex mutex;

public:
	void push(uint8_t value);
	bool pop(uint8_t& value);
};

void DynamicQueue::push(uint8_t value) {
	std::lock_guard<std::mutex> lock(mutex);

	queue.push(value);
}

bool DynamicQueue::pop(uint8_t& value) {
	mutex.lock();
	if (queue.empty()) {
		mutex.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		mutex.lock();
		if (queue.empty()) {
			mutex.unlock();
			return false;
		}
	}

	value = queue.front();
	queue.pop();
	mutex.unlock();
	return true;
}




