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
	float maximum = 1.0f;
	float duration = maximum;
	float minimum = 0.005f;
	
	// Recalculates the rtt duration given a new sample
	void onSample(float sample)
	{
		duration = std::min(maximum, std::max(minimum, 0.8f * duration + 0.2f * sample));
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
