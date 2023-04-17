#include <algorithm>
#include <ctime>
#include <iostream>
#include <random>
#include <thread>

#include "QueueAnalyzer.h"


using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;

//message may have one of the 10 levels of importance
//0 - the most important
//9 - the least important
//for the messages with int priority, priority is the index of array
//for the messages with std::string priority, priority and the message text are the same
const int NUM_OF_PRIORITIES = 10;
const std::string messages[] = {
	"Error",
	"Message with the 1st level of importance",
	"Message with the 2nd level of importance",
	"Message with the 3rd level of importance",
	"Message with the 4th level of importance",
	"Message with the 5th level of importance",
	"Message with the 6th level of importance",
	"Message with the 7th level of importance",
	"Message with the 8th level of importance",
	"Log"
};

struct Message {
	std::string text;
	Message(std::string txt) : text(txt) {}
	size_t size() {
		return text.length();
	}
	friend std::ostream& operator<<(std::ostream& os, const Message& msg);
};

std::ostream& operator<<(std::ostream& os, const Message& msg) {
	os << msg.text;
	return os;
}

struct cmp {
	bool operator() (const std::string& lhs, const std::string& rhs) const {
		if(lhs == "Log") return false;
		return lhs < rhs;
	}
};

//generate both message and its priority
//message is relevant for [(priority + 1) * 2] seconds
//e.g. message with priority 3 will be relevant for 8 seconds
//message with priority 7 will be relevant for 16 seconds 
template<class Pr,
		typename std::enable_if_t<std::is_same<Pr, std::string>::value>* = nullptr>
Message generate_message(Pr& priority, time_point& tp_expired) {
	time_point tp_now = std::chrono::high_resolution_clock::now();
	int priority_idx = rand() % NUM_OF_PRIORITIES;
	tp_expired = tp_now + std::chrono::seconds((priority_idx + 1) * 2);
	priority = messages[priority_idx];
	return Message(priority);
}

template<class Pr,
		typename std::enable_if_t<std::is_integral<Pr>::value>* = nullptr>
Message generate_message(Pr& priority, time_point& tp_expired) {
	time_point tp_now = std::chrono::high_resolution_clock::now();
	priority = rand() % NUM_OF_PRIORITIES;
	tp_expired = tp_now + std::chrono::seconds((priority + 1) * 2);
	return Message(messages[priority]);
}

template<class Pr, class T, class Compare>
void emulate_queue(MessageQueue<Pr, T, Compare>& mQueue, const std::string& file_path) {
	//generated message is added to the queue every second
	//top message is received from the queue every 3 seconds
	QueueAnalyzer qAnalyzer(&mQueue, file_path);
	for(int i = 0; i < 20; i++) {	
		time_point tp_now = std::chrono::high_resolution_clock::now();
		std::cout << "time: " << std::chrono::duration_cast<std::chrono::seconds>(tp_now.time_since_epoch()).count() << '\n';
		try {
			Pr priority;
			time_point tp_expired;
			Message msg = generate_message(priority, tp_expired);
			mQueue.Add(priority, msg, tp_expired);
			std::cout << "Added: " << priority << ' ' << msg << ' ' << std::chrono::duration_cast<std::chrono::seconds>(tp_expired.time_since_epoch()).count() << "\n\n";	
		}
		catch(const std::exception& e) {		//regarding MessageQueue.h, row 100. Seems like I don't have to specify the type when catching exception, only when throwing. why?
			std::cerr << e.what() << '\n';
		}
		if(i % 3 == 0) {
			try {
				Message top_msg = mQueue.Get();
				std::cout << "Get message: " << top_msg << "\n\n";
			}
			catch(const std::exception& e) {		
				std::cerr << e.what() << '\n';
			}
		}
		qAnalyzer.Analyze();
		//_sleep(1000);							//got warning that this is deprecated.
												//is there universal solution for this, 
												//without using threads or include different functions for different OSs?
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

int main() {
	srand(time(0));
	MessageQueue<int, Message, std::less<int>> intPr_mQueue(10);
	emulate_queue(intPr_mQueue, "intPr_mQueueLog.txt");
	
	/*
	auto comp = [](std::string lhs, std::string rhs) {
		if(lhs == "Log") return false;
		return lhs < rhs;
	};
	MessageQueue<std::string, Message, decltype(comp)> stringPr_mQueue(10); //doesn't work
																			//LET ME INNNNNNN
	
	*/
	MessageQueue<std::string, Message, cmp> stringPr_mQueue(10);			//that's what despair looks like
																			//can I somehow make lambda work?
	emulate_queue(stringPr_mQueue, "stringPr_mQueueLog.txt");
	return 0;
}