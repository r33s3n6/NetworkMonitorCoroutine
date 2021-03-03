#pragma once

namespace proxy_tcp {
namespace error_response {

	const char error404[] = "HTTP/1.1 404 Not Found\r\n"
		"Content-Length: 85\r\n"
		"Content-Type: text/html\r\n"
		"\r\n"
		"<html>"
		"<head><title>Not Found</title></head>"
		"<body><h1>404 Not Found</h1></body>"
		"</html>";



}

}

