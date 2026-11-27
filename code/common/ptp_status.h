#pragma once

#include <cstdint>

#include <json_dto/pub.hpp>

// Status message sent by a microphone node over the existing TCP connection.
struct PTP_STATUS
{
	std::uint16_t node_id = 0;
	std::int64_t offset_budget_ns = 0;
	bool synchronized = false;

	template <typename Json_Io>
	void json_io(Json_Io &io)
	{
		io & json_dto::mandatory("node_id", node_id)
		   & json_dto::mandatory("offset_budget_ns", offset_budget_ns)
		   & json_dto::mandatory("synchronized", synchronized);
	}
};
