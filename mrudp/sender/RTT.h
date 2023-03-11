#pragma once

namespace timprepscius {
namespace mrudp {

// --------------------------------------------------------------------------------
// RTT
// 
// RTT is calculated by generating a weighted average of the current RTT with the
// next sample.
// --------------------------------------------------------------------------------

struct RTTConstant
{
	float duration = 1.0f;

	void onSample(float) {}
	void onAckFailure () {}
} ;

struct RTTSimple
{
	static constexpr float a = 0.9f;
	static constexpr float b = 1 - a;

	float maximum = 1.0f;
	float duration = maximum;
	float minimum = 0.005f;
	
	float calculate(float duration, float sample)
	{
		return std::min(maximum, std::max(minimum, a * duration + b * sample));
	}
	
	// Recalculates the rtt duration given a new sample
	void onSample(float sample)
	{
		duration = calculate(duration, sample);
	}
	
	// Recalculates the rtt duration given a failure, which is evaluated as
	// a sample taking the maximum transit time.
	void onAckFailure ()
	{
		onSample(maximum);
	}

} ;

typedef RTTSimple RTT;

} // namespace
} // namespace
