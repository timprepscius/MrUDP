
#include "Common.h"
#include "../mrudp/Packet.h"

namespace timprepscius {
namespace mrudp {
namespace tests {

SCENARIO("packet id")
{
    GIVEN( "a set of interesting packet id numbers" )
    {
		auto test_packet_id_greater_than = [](auto l, auto r) {
			REQUIRE(packet_id_greater_than(l,r));
			REQUIRE(!packet_id_greater_than(r,l));
		} ;

		auto test_packet_id_equal = [](auto l, auto r) {
			REQUIRE(!packet_id_greater_than(l,r));
			REQUIRE(!packet_id_greater_than(r,l));
		};

		PacketID top_1 = (PacketID)1<<31;
		PacketID top_01 = (PacketID)1<<30;
		PacketID max_num = std::numeric_limits<PacketID>::max();
		

		PacketID pairs[][2] = {
			{ (max_num ^ top_1) - 1, max_num },
			{ 0, max_num },
			{ max_num, top_1 },
			{ top_01, max_num },
			{ 1, 0 },
			{ 2, 1 },
			{ 4, 2 },
			{ 8, 4 },
			{ 16, 8 },
			{ top_1, 1 },
			{ top_1 + 1, 1 },
			{ 1, top_1 + 2 },
			{ 1, top_1 + 2 },
		} ;
		
		for (auto &p : pairs)
		{
			auto &[l, r] = p;
			auto lr = r - l;
			auto rl = l - r;
			(void)lr;
			(void)rl;
			
			test_packet_id_equal(l, l);
			test_packet_id_equal(r, r);
			test_packet_id_greater_than(l,r);
		};
	}
}

} // namespace
} // namespace
} // namespace
