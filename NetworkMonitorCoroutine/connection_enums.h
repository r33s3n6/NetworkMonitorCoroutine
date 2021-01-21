#pragma once

namespace proxy_server {

	typedef enum {
		success,//完整
		with_appendix,// 有后缀，需要进行分割
		failed,//数据损坏
		wait, //继续接收
		wait_chunked,//继续接收chunked包
		https_handshake //https握手包
	} integrity_status;

	typedef enum {
		not_begin,
		finished
	} handshake_status;

	typedef enum {
		respond_and_keep_alive,
		respond_and_close,
		respond_as_tunnel,
		respond_and_keep_reading,
		ignore
	} connection_behaviour;

	typedef enum {
		http,
		https,
		error,
		handshake
	} request_protocol;
}