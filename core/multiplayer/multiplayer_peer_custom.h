/*************************************************************************/
/*  multiplayer_peer_custom.h                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef MULTIPLAYER_PEER_CUSTOM_H
#define MULTIPLAYER_PEER_CUSTOM_H

#include "core/multiplayer/multiplayer_peer.h"

class MultiplayerPeerCustom : public MultiplayerPeer {
	GDCLASS(MultiplayerPeerCustom, MultiplayerPeer);

protected:

	int64_t self_id;
	ConnectionStatus connection_status;
	int64_t target_id;

	struct Packet {
		PackedByteArray data;
		int64_t from;
	};

	List<Packet> incoming_packets;

	Packet current_packet;

public:
	static void _bind_methods();

	MultiplayerPeerCustom();
	~MultiplayerPeerCustom();

	// PacketPeer.
	Error get_packet(const uint8_t **r_buffer, int &r_buffer_size) override;
	Error put_packet(const uint8_t *p_buffer, int p_buffer_size) override;
	int get_available_packet_count() const override;
	int get_max_packet_size() const override;

	// NetworkedMultiplayerPeer.
	void set_target_peer(int p_peer) override;
	int get_packet_peer() const override;
	bool is_server() const override;
	void poll() override;
	int get_unique_id() const override;
	ConnectionStatus get_connection_status() const override;

	// Custom methods.
	void initialize(int64_t p_self_id);
	void set_connection_status(int64_t p_connection_status);
	void deliver_rpc(const PackedByteArray &p_data, int64_t p_from_peer_id);

};

#endif
