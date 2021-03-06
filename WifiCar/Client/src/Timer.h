#pragma once
#include <chrono>

class Timer {
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
public:
	Timer()
	{}

	void Start() { m_Start = std::chrono::high_resolution_clock::now(); }
	float End() { return std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - m_Start).count(); } // Returns x seconds after Start() was called
};