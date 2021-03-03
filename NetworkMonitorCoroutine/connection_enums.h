#pragma once

namespace proxy_tcp {

	typedef enum {
		intact,//完整
		//with_appendix,// 有后缀，需要进行分割
		broken,//数据损坏
		wait, //继续接收
		wait_chunked,//此chunked包不完整，继续接收
		chunked,//此chunked完整，仍有后续chunked包
		https_handshake, //https握手包
		websocket_intact//websocket
	} integrity_status;

	typedef enum {//deprecated
		not_begin,
		finished
	} handshake_status;

	typedef enum {
		respond_and_keep_alive,//保持连接
		respond_and_close,//
		keep_receiving_data,//继续从远端读数据
		protocol_websocket,
		respond_error,
		ignore
	} connection_behaviour;

	typedef enum {
		http,
		//https,
		websocket,
		websocket_handshake,
		//websocket_with_ssl,
		unknown

	} connection_protocol;

	typedef enum {
		_OPTIONS,
		_HEAD,
		_GET,
		_POST,
		_PUT,
		_DELETE,
		_TRACE,
		_CONNECT //https 单独处理
	} request_type;

}