#include <atomic>

using namespace std;

class Node {
public:
	uint8_t value;
	shared_ptr<Node> next;
	Node(uint8_t _value) : next(nullptr) {
		value = _value;
	}
};


class FixedAtomicQueue2 {
	shared_ptr<Node> head;
	shared_ptr<Node> tail;
public:
	FixedAtomicQueue2() : head(new Node(uint8_t(0))) {
		tail = atomic_load(&head);
	}
	void push(uint8_t value);
	bool pop(uint8_t& value);
};

void FixedAtomicQueue2::push(uint8_t value) {
	shared_ptr<Node> node(new Node(value));
	while (true) {
		shared_ptr<Node> tail_copy = atomic_load(&tail);
		shared_ptr<Node> dummy(nullptr);
		if (atomic_compare_exchange_strong(&(tail_copy->next), &dummy, node)) {
			atomic_compare_exchange_strong(&tail, &tail_copy, node);
			break;
		}
		else {
			atomic_compare_exchange_strong(&tail, &tail_copy, tail_copy->next);
		}
	}
}

bool FixedAtomicQueue2::pop(uint8_t &value) {
	uint8_t result;
	int is_first_try = true;
	while (true) {
		shared_ptr<Node> head_copy = atomic_load(&head);
		shared_ptr<Node> tail_copy = atomic_load(&tail);
		shared_ptr<Node> next_copy = atomic_load(&(head_copy->next));
		if (head_copy == tail_copy) {
			if (next_copy == nullptr) {
				return false;
			}
		}
		else {
			result = next_copy->value;
			if (atomic_compare_exchange_strong(&head, &head_copy, next_copy)) {
				value = result;
				return true;
			}
		}
	}
}