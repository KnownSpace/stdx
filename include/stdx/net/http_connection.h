#pragma once
#include <stdx/net/connection.h>
#include <stdx/net/http_parser.h>

namespace stdx
{
	using http_connection = stdx::connection<stdx::http_response,stdx::http_request>;


}