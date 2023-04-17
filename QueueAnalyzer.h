#pragma once

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include "MessageQueue.h"

//template<template <class Pr, class T, class Compare> class MessageQueue >   //error: QueueAnalyzer is not a template
                                                                   			  //ffs what is it then? it's like 100500th try to fit parameters

																			  //upd: I see, should have use the template parameter 
																			  //list when first declared in MessageQueue.h
																			  //at the beginning, I just declared it as 
																			  //friend class ... without specifying it was a template


template<class Pr, class T, class Compare> 				
class QueueAnalyzer {
using time_point = std::chrono::time_point<std::chrono::high_resolution_clock>;

private:
	MessageQueue<Pr, T, Compare>* _pmQueue; //[priority, list<T message, time_point when expired>, Compare]
	//std::filesystem::path _fpath;
	std::string _fpath;		
	std::ofstream _fout;						
public:
	QueueAnalyzer(MessageQueue<Pr, T, Compare>* pmQueue, const std::string& filepath) :
					_pmQueue(pmQueue), _fpath(filepath) {
		//std::filesystem::create_directories(_fpath.parent_path()); 	doesn't work
		//																seems like filesystem doesn't allow to create files anyway, only directories
		//																is it bad to use fstream?
		
		_fout.open(_fpath);			//file is rewritten every time with the new queue
		_fout.close();
	}
	void Analyze() { //format for a human to read, except for current time_point
		time_point tp_now = std::chrono::high_resolution_clock::now();
		size_t queue_size = _pmQueue->_size;
		int queue_size_bytes = 0;
		std::vector<size_t> sizes;
		std::map<Pr, double> stats;
		time_point first_expired = tp_now + std::chrono::hours(100500), 
					last_expired = tp_now - std::chrono::hours(100500);
		double max_difference = 0; 			//in seconds
		for(auto it = _pmQueue->_mqueue.begin(); it != _pmQueue->_mqueue.end(); it++) {			
			std::list<std::pair<T, time_point>>* subqueue = &it->second;
    		if(subqueue->empty()) continue;
			stats[it->first] = 100.0 * subqueue->size() / queue_size;
    		std::transform(subqueue->begin(), subqueue->end(), std::back_inserter(sizes),
                   [](std::pair<T, time_point> p) { return p.first.size(); });
    		first_expired = min(first_expired, std::min_element(subqueue->begin(), subqueue->end(),
    											[](std::pair<T, time_point> p1, std::pair<T, time_point> p2){
    												return p1.second < p2.second;	 
    											})->second);
    		last_expired = max(last_expired, std::max_element(subqueue->begin(), subqueue->end(),
    											[](std::pair<T, time_point> p1, std::pair<T, time_point> p2){
    												return p1.second > p2.second;	 
    											})->second);
		}
		queue_size_bytes = std::accumulate(sizes.begin(), sizes.end(), 0);
		if(first_expired <= last_expired) {
			max_difference = std::chrono::duration<double>(last_expired - first_expired).count();
		}
		_fout.open(_fpath, std::ios::app);
    	_fout << "Current time: " << std::chrono::duration_cast<std::chrono::seconds>(tp_now.time_since_epoch()).count() << "\n";							 //seconds since epoch
    	_fout << "Queue size: " << queue_size << " elements\n";
    	_fout << "Stats:\n";
    	_fout << "Priority\t%\n";
    	for(auto it = stats.begin(); it != stats.end(); it++) {
    		_fout << it->first << "\t\t\t" << it->second << "%\n";
    	}
    	_fout << "Total size of the queue: " << queue_size_bytes / 1024.0 << "Kb\n";
    	_fout << "The biggest difference between time expired: " << max_difference << "s\n\n";
    	_fout.close();
	}
};