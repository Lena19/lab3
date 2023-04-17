#pragma once

#include <chrono>
#include <exception>
#include <list>
#include <map>


template<class Pr, class T, class Compare>
class MessageQueue {

using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;

private:
	/*
	std::map<int, std::list<std::pair<T, time_point>> > _mqueue;	//priority, <message, time_point when expired>
																	//decided not to bother with unordered_map due to little range of possible priorities.

																	//otherwise, if there were many types of priorities,
																	//performance of Get() wouldn't have improved if I changed map to unordered_map,
																	//Add(int, T, time_point) would have become faster though

																	//also could have used forward_list instead of double linked to save some space,
																	//but I think for this amount of data it's not such a big difference, so let it be


																	//why this structure overall? map allows to maintain order of priorities.
																	//now when I write this, I realize I could've just used simple array with priorities as its indices :/
																	//so let's pretend map is here just for fun, because I wan't to play with different containers 
																	//(please don't be angry, I know it's not optimal T_T)


																	//inner container might have been queue, because basically I just have to be able to 
																	//extract the first element and to add elements to the back.
																	//but it's not really possible to easily remove expired elements from inside without extra copying 
																	//so I chose list
	*/
	//you know what, let's actually make usage of (ordered) map justified [it is how the task is intended to be completed, isn't it? feel a little bit stupid. ANYWAY]
	//let's say we can have priority of any type, and require comparator from the user

	std::map<Pr, std::list<std::pair<T, time_point>>, Compare> _mqueue;

	size_t _maxsize;
	size_t _size;

	bool clearExpired() {										//true if managed to delete any non-zero amount of messages
		bool is_message_deleted = false;
		_size = 0;
		for(auto it = _mqueue.begin(); it != _mqueue.end(); it++) {
			auto subqueue_begin = it->second.begin();
			auto subqueue_end = it->second.end();
			time_point tp_now = std::chrono::high_resolution_clock::now();
			auto is_expired = [&tp_now](std::pair<T, time_point> p){
				return p.second < tp_now;
			};
			auto new_subqueue_end = remove_if(subqueue_begin, subqueue_end, is_expired);
			is_message_deleted |= (subqueue_end != new_subqueue_end);
			_size += it->second.size();
		}
		return is_message_deleted;
		//note: list has it's own remove_if, but it can return not void (size_type, number of removed items) only since C++20
	}

public:
	MessageQueue(size_t maxsize) : 
				_maxsize(maxsize), _size(0) {}

	void Add(const Pr& priority, const T& message, const time_point& tp_expired) {
		if(_size == _maxsize) {
			if(!clearExpired()) {
				throw std::overflow_error("Queue is full!");
			}
		}
		_mqueue[priority].push_back({message, tp_expired});
		_size++;
	}

	T Get() {
		time_point tp_now = std::chrono::high_resolution_clock::now();
		for(auto it = _mqueue.begin(); it != _mqueue.end(); it++) {			//if the basic container was unordered_map or simple array, iteration would have looked different
																			//I wouldn't use iterators, just go for every priority:
																			//for(int i = min_priority; i <= max_priority; i++)

			std::list<std::pair<T, time_point>>* messages = &(it->second); 	//is there more beautiful solution for extracting this sub-queue than using a pointer?	
			while(messages->size() != 0) {			
				//T message = messages->front().first; 						//no need to extract it every time, it's relevant only for non-expired messages
				time_point tp_expired = messages->front().second;
				if(tp_expired >= tp_now) {
					T message = messages->front().first; 
					messages->erase(messages->begin());
					_size--;	
					return message;
				}
				messages->erase(messages->begin());
				_size--;
			}
		}
		//I know this is not the most appropriate type of exception for this case, but I cannot make it work with just overall std::exception("some error message")
		//Not sure if I just can't google how to, or it actually has to have particular type
		throw std::overflow_error("The queue is empty or all the messages have been expired!");
	}

	//template<class Pr, class T, class Compare, template<typename, typename, typename> class MQueue = MessageQueue > 				
	template<class, class, class> friend class QueueAnalyzer;
};