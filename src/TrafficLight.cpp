#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <random>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{ 
    std::unique_lock<std::mutex> lock(_mutex);
    _condition.wait(lock, [this]{
        return !_queue.empty();
    });
    T msg = std::move(_queue.front());
    _queue.pop_front(); 
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _queue.emplace_back(msg);
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */
 
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    while(true){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto msg = pusher.receive();
        if(msg == green)
            return;
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}
// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    std::random_device device;
    std::default_random_engine generator(device());
    std::uniform_int_distribution<int> dist(4,6);
    int cycle = dist(generator);
    auto lastSwitched = std::chrono::system_clock::now();
    while(true){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto switched = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now() - lastSwitched);
        int sinceSwitch = switched.count();
        if(sinceSwitch >= cycle){
            if(_currentPhase == red)
                _currentPhase = green;
            else 
                _currentPhase = red;
        }
        auto future = std::async(std::launch::async, &MessageQueue<TrafficLightPhase> ::send, &pusher, std::move(_currentPhase));
        future.wait();
        lastSwitched = std::chrono::system_clock::now();
        cycle = dist(generator);
    }
    
}

