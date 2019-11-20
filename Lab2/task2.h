#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "DynamicQueue.h"
#include "FixedAtomicQueue.h"
#include "FixedAtomicQueue2.h"
#include "FixedMutexQueue.h"
#include <ctime>

const std::vector<int> producerNums = { 1, 2, 4 };
const std::vector<int> consumerNums = { 1, 2, 4 };
const std::vector<size_t> queueSizes = { 1 };
const int TASK_NUM = 1024;

void task2() {
	int taskNum = TASK_NUM;
	for (auto queueSize : queueSizes) {
		for (int producerNum : producerNums) {
			for (int consumerNum : consumerNums) {

				//DynamicQueue queue;
				//FixedMutexQueue queue(queueSize);
				FixedAtomicQueue2 queue;
				std::atomic_int sum(0);

				std::vector<std::thread> consumers(consumerNum);
				std::vector<std::thread> producers(producerNum);
				auto start = clock();
				for (auto &thread : consumers) {
					thread = std::thread(
						[&sum, &queue, taskNum, producerNum]() {
						uint8_t val;
						while (sum.load() < producerNum * taskNum) {
							sum += queue.pop(val);
						}
					}
					);
				}
				for (auto &thread : producers) {
					thread = std::thread(
						[&taskNum, &queue]() {
						for (int i = 0; i < taskNum; ++i)
							queue.push(1);
					}
					);
				}
				for (auto &thread : producers) {
					thread.join();
				}

				for (auto &thread : consumers) {
					thread.join();
				}
				auto time = clock() - start;
				std::cout << "producerNum: " << producerNum << ", consumerNum: " << consumerNum << ", queueSize : " << queueSize << "\n"
					<< sum.load() << ", time: " << time << " ms";
				std::cout << std::endl;
			}
		}

		//break;
	}
}