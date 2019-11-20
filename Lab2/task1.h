#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <array>
#include <vector>
#include <ctime>

const int NUM_TASKS = 1024 * 1024;

void atomic(int num_thread, int sleep_time) {
	std::array<char, NUM_TASKS> array{ {0} };
	std::atomic_int index(0);
	std::vector<std::thread> threads(num_thread);

	for (auto& thread : threads) {
		thread = std::thread([sleep_time](std::array<char, NUM_TASKS>& array, std::atomic_int& index) {
			while (true) {
				int old_index = index++;
				if (old_index >= array.size())
					break;
				std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_time));
				array[old_index]++;
			};
		}, std::ref(array), std::ref(index));
	}

	for (auto& thread : threads) {
		thread.join();
	}
	for (auto x : array) {
		if (x != 1) {
			std::cout << "error";
		}
	}

}

void mutex(int num_thread, int sleep_time) {
	std::array<char, NUM_TASKS> array{ {0} };
	std::mutex mutex;
	int index = 0;
	std::vector<std::thread> threads(num_thread);

	for (auto& thread : threads) {
		thread = std::thread([sleep_time](std::array<char, NUM_TASKS>& array, int& index, std::mutex& mutex) {
			while (true) {
				int old_index;
				{
					std::lock_guard<std::mutex> lock(mutex);
					if (index >= array.size()) {
						break;
					}
					old_index = index;
					index++;
				}

				std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_time));
				array[old_index]++;
			}
		}, std::ref(array), std::ref(index), std::ref(mutex));
	}

	for (auto& thread : threads) {
		thread.join();
	}
	for (auto x : array) {
		if (x != 1) {
			std::cout << "error";
		}
	}


}


void task1() {
	std::vector<int> numThreads = { 4, 8, 16, 32 };
	std::vector<int> sleepTimes = { 0, 10 };

	for (int sleepTime : sleepTimes) {
		for (int numThread : numThreads) {

			auto start = clock();
			mutex(numThread, sleepTime);
			auto mutexTime = clock() - start;

			start = clock();
			atomic(numThread, sleepTime);
			auto atomicTime = clock() - start;

			std::cout << "numThread = " << numThread << ", sleepTime = " << sleepTime << "\n"
				<< "mutex: " << mutexTime << " ms" << "\n"
				<< "atomic: " << atomicTime << " ms" << "\n" << std::endl;
		}
	}
}