/*
 * fabric.h
 *
 * Copyright (C) 2008-2017 Aerospike, Inc.
 *
 * Portions may be licensed to Aerospike, Inc. under one or more contributor
 * license agreements.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 */

// Internode network communications.

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "msg.h"
#include "socket.h"
#include "util.h"

#include "fabric/hb.h"


//==========================================================
// Typedefs & constants.
//

#define AS_FABRIC_SUCCESS			(0)
#define AS_FABRIC_ERR_UNKNOWN		(-1)
#define AS_FABRIC_ERR_QUEUE_FULL	(-2)
#define AS_FABRIC_ERR_NO_NODE		(-3)
#define AS_FABRIC_ERR_BAD_MSG		(-4)
#define AS_FABRIC_ERR_UNINITIALIZED	(-5)
#define AS_FABRIC_ERR_TIMEOUT		(-6)

typedef enum {
	AS_FABRIC_CHANNEL_RW = 0,	// duplicate resolution and replica writes
	AS_FABRIC_CHANNEL_CTRL = 1,	// clustering, migration ctrl and services info
	AS_FABRIC_CHANNEL_BULK = 2,	// migrate records
	AS_FABRIC_CHANNEL_META = 3, // smd

	AS_FABRIC_N_CHANNELS
} as_fabric_channel;

// Fabric must support the maximum cluster size.
#define MAX_NODES_LIST (AS_CLUSTER_SZ)

#define MAX_FABRIC_CHANNEL_THREADS 128
#define MAX_FABRIC_CHANNEL_SOCKETS 128

typedef struct as_node_list_t {
	uint32_t sz;
	uint32_t alloc_sz;
	cf_node nodes[MAX_NODES_LIST];
} as_node_list;

typedef struct fabric_rate_s {
	uint64_t s_bytes[AS_FABRIC_N_CHANNELS];
	uint64_t r_bytes[AS_FABRIC_N_CHANNELS];
} fabric_rate;

typedef int (*as_fabric_msg_fn) (cf_node node_id, msg *m, void *udata);
typedef int (*as_fabric_transact_recv_fn) (cf_node node_id, msg *m, void *transact_data, void *udata);
typedef int (*as_fabric_transact_complete_fn) (msg *rsp, void *udata, int as_fabric_err);


//==========================================================
// Globals.
//

extern cf_serv_cfg g_fabric_bind;
extern cf_ip_port g_fabric_port;


//==========================================================
// Public API.
//

//------------------------------------------------
// msg
//

msg *as_fabric_msg_get(msg_type t);
void as_fabric_msg_put(msg *);
void as_fabric_msg_queue_dump();

//------------------------------------------------
// as_fabric
//

int as_fabric_init(void);
int as_fabric_start(void);
void as_fabric_set_recv_threads(as_fabric_channel channel, uint32_t count);
int as_fabric_send(cf_node node_id, msg *m, as_fabric_channel channel);
int as_fabric_send_list(cf_node *node_ids, int nodes_sz, msg *m, as_fabric_channel channel);
int as_fabric_get_node_lasttime(cf_node node_id, uint64_t *lasttime);
int as_fabric_register_msg_fn(msg_type type, const msg_template *mt, size_t mt_sz, size_t scratch_sz, as_fabric_msg_fn msg_cb, void *udata_msg);
void as_fabric_rate_capture(fabric_rate *rate);
void as_fabric_dump(bool verbose);


//==============================================================================
// Fabric transact.
//

// Used to send a request, and receive a response, reliably. This is guaranteed
// to NEVER return an error directly, but might call the callback function
// saying that we ran out of time or had some other error.
//
// Requires field 0 be a uint64_t which will be used by the fabric system - an
// unknown error will be thrown if this is not true.

void as_fabric_transact_init(void);
void as_fabric_transact_start(cf_node node_id, msg *m, int timeout_ms, as_fabric_transact_complete_fn cb, void *userdata);
int as_fabric_transact_register(msg_type type, const msg_template *mt, size_t mt_sz, size_t scratch_sz, as_fabric_transact_recv_fn cb, void *udata);
int as_fabric_transact_reply(msg *reply_msg, void *transact_data);
