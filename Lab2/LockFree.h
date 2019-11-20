#include <atomic>
#include <thread>
#include <utility>


struct WipData {
	int desc; // descriptor’s identifier loaded from buffer
	int res; // descriptor’s identifier loaded from descriptor
	int pos; // position loaded from descriptor
	int val; // original entry loaded from descriptor
};

// dequeue descriptor
struct DeqDesc {
	std::atomic<int> res;
	std::atomic<int> pos;
	std::atomic<int> val;
};


struct CircularBuffer {
	static const int N = 100;	//temp
	static const int THREADS = 100;	//temp
	bool enq(int elem);
	std::pair<int, bool> deq();
	std::atomic<int> head;
	std::atomic<int> tail;
	std::atomic<int> buf[N];
	DeqDesc ti[THREADS];


	bool enq(int val) {
		int pos = tail.load(rlx);
		while (pos < head.load(acq) + N) {
			std::atomic<int>& entry = buf[pos%N];
			int elem = empty_val(pos);
			if (entry.compare_exchange_strong(elem, val, rel, rlx)) {
				update_counter<rlx>(tail, pos + 1);
				return true;

			}
			std::atomic_thread_fence(cns);
			if (is_val(elem))
				pos = pos + 1;
			else if (in_progress(elem) && !this_was_delayed(elem, pos))
				check_descr(elem);
			else
				pos = tail.load(rlx);
		}
		return false;
	}	std::pair<int, bool> deq() {
		const int NUM_THREADS = 100; // temp
		int descriptors[NUM_THREADS];
		int threadid = this_thread_id();
		int pos = head.load(rlx);
		while (pos < tail.load(rlx)) {
			std::atomic<int>& entry = buf[pos%N];
			const int elem = entry.load(rlx);
			int pvel = elem;
			if (is_val(elem)) {
				const int descr = make_descr(pos, elem, threadid);
				const bool succ = entry.compare_exchange_strong(pvel, descr, rel, rlx);
				if (succ && check_descr(descr, get_descriptor(threadid), WipData(descr, descr, pos, elem), descriptors))
					return make pair(elem, true);
			}
			if (in_progress(pvel)) {
				std::atomic_thread_fence(cns);
				if (eqpos_descr_counter(pvel, pos)) {
					descriptors[pos % THRDS] = pvel;
					pos = pos + 1;

				}
				else if (this_was_delayed(pvel, pos))
					pos = head.load(rlx);
				else
					check_descr(pvel);
			}
			else
				pos = head.load(rlx);

		}
		return std::make_pair(-1, false);
	}





	int chkENTRY = 1, chkSTATE = 3, stMAXBITS = 2, stVALID = 0, stEMPTY = 1, stWIP = 3; //??????

	template <std::memory_order mo>
	void update_counter(std::atomic<int>& ctr, int pos) {
		int oldval = ctr.load(rlx);
		while ((oldval < pos) && !ctr.compare exchange strong(oldval, pos, rlx, rlx));
		atomic thread fence(mo);
	}
	int make_descr(int headpos, int entryval, int threadno) {
		DeqDesc& ti = get_descriptor(threadno);
		const int descr = encode_descr(headpos, threadno);
		ti.res.store(descr, rlx);
		ti.val.store(entryval, rlx);
		ti.pos.store(headpos, rlx);
		return descr;
	}
	void decide(DeqDesc& ti, WipData& wip, int descr) {
		if (ready(wip)) return;
		int min = head.load(seq);
		const int valid = (wip.pos >= min);
		const int result = (wip.desc & ˜stWIP) | valid;
		const bool succ = ti.res.compare_exchange_strong(wip.res, result, rlx, rlx);
		if (succ) wip.res = result;
	}
	bool check_descr(int descr) {
		DeqDesc& ti = get_descriptor(descr);
		return check_descr(descr, ti, load_threadinfo(ti, descr));
	}
	bool check_descr(int descr, DeqDesc& ti, WipData wip, int∗ descriptors = nullptr) {
		if (inconsistent(wip)) return false;
		decide(ti, wip, descr);
		if (in_progress(wip)) return false;
		return complete(descr, wip, descriptors);

	}
	bool inconsistent(const WipData& wip) {
		return ((wip.desc ^ wip.res) & stWIP) != 0 || !eqpos_descr_counter(wip.res, wip.pos);
	}

	void help_delayed(int pos, int∗ descriptors) {
		if (!descriptors) return;
		int p = pos − 1;
		int h = head.load(rlx);
		while (p >= h) {
			int entryval = descriptors[p % THRDS];
			validate_descr(entryval);
			p = p − 1;
			h = head.load(rlx);
		}

	}
	int validate_descr(DeqDesc& ti, int descr, bool valid) {
		const int result = (descr & ˜stWIP) | valid;
		const bool succ
			= ti.res.compare_exchange_strong(descr, result, rlx, rlx);
		return result;
	}
	int validate_descr(int descr) {
		return validate_descr(get_descriptor(descr), descr, true);
	}
	bool in_progress(int v) {
		return (v & chkSTATE) == stWIP;
	}
	bool this_was_delayed(int descr, int thispos) {
		return thispos <= get_descriptor(descr).pos.load(rlx);
	}
	bool complete(int descr, const WipData& wip, int* descriptors) {
		const bool succ = success(wip);
		if (succ) {
			help_delayed(wip.pos, descriptors);
			update_counter<seq>(head, wip.pos + 1);
		}
		const int entryval = (succ ? empty_val(wip.pos + 1) : wip.val);
		std::atomic<int>& entry = buf[idx(wip.pos)];
		entry.compare_exchange_strong(descr, entryval, rlx, rlx);
		return succ;

	}
	int empty_val(int pos) {
		return ((pos << stMAXBITS) | stEMPTY);	}
};