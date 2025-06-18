// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * MGMTD Frontend Client Library api interfaces
 * Copyright (C) 2021  Vmware, Inc.
 *		       Pushpasis Sarkar <spushpasis@vmware.com>
 */

#ifndef _FRR_MGMTD_FE_CLIENT_H_
#define _FRR_MGMTD_FE_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "frrevent.h"
#include "mgmt_defines.h"
#include "mgmt_msg_native.h"

/***************************************************************
 * Macros
 ***************************************************************/

/*
 * The server port MGMTD daemon is listening for Backend Client
 * connections.
 */

#define MGMTD_FE_MSG_PROC_DELAY_USEC 10

#define MGMTD_FE_MAX_NUM_MSG_PROC  500
#define MGMTD_FE_MAX_NUM_MSG_WRITE 100
#define MGMTD_FE_MAX_MSG_LEN	   (64 * 1024)

/***************************************************************
 * Data-structures
 ***************************************************************/

#define MGMTD_SESSION_ID_NONE 0

#define MGMTD_CLIENT_ID_NONE 0

struct mgmt_fe_client;


/*
 * All the client specific information this library needs to
 * initialize itself, setup connection with MGMTD FrontEnd interface
 * and carry on all required procedures appropriately.
 *
 * FrontEnd clients need to initialise a instance of this structure
 * with appropriate data and pass it while calling the API
 * to initialize the library (See mgmt_fe_client_lib_init for
 * more details).
 */
struct mgmt_fe_client_cbs {
	void (*client_connect_notify)(struct mgmt_fe_client *client,
				      uintptr_t user_data, bool connected);

	void (*client_session_notify)(struct mgmt_fe_client *client,
				      uintptr_t user_data, uint64_t client_id,
				      bool create, bool success,
				      uintptr_t session_id,
				      uintptr_t user_session_client);

	void (*lock_ds_notify)(struct mgmt_fe_client *client, uintptr_t user_data,
			       uint64_t client_id, uintptr_t session_id,
			       uintptr_t user_session_client, uint64_t req_id, bool lock_ds,
			       bool success, enum mgmt_ds_id ds_id, char *errmsg_if_any);

	void (*commit_config_notify)(struct mgmt_fe_client *client, uintptr_t user_data,
				     uint64_t client_id, uintptr_t session_id,
				     uintptr_t user_session_client, uint64_t req_id, bool success,
				     enum mgmt_ds_id src_ds_id, enum mgmt_ds_id dst_ds_id,
				     bool validate_only, bool unlock, char *errmsg_if_any);

	/* Called when get-tree result is returned */
	int (*get_tree_notify)(struct mgmt_fe_client *client, uintptr_t user_data,
			       uint64_t client_id, uint64_t session_id, uintptr_t session_ctx,
			       uint64_t req_id, enum mgmt_ds_id ds_id, LYD_FORMAT result_type,
			       void *result, size_t len, int partial_error);

	/* Called when edit result is returned */
	int (*edit_notify)(struct mgmt_fe_client *client, uintptr_t user_data,
			   uint64_t client_id, uint64_t session_id,
			   uintptr_t session_ctx, uint64_t req_id,
			   const char *xpath);

	/* Called when RPC result is returned */
	int (*rpc_notify)(struct mgmt_fe_client *client, uintptr_t user_data,
			  uint64_t client_id, uint64_t session_id,
			  uintptr_t session_ctx, uint64_t req_id,
			  const char *result);

	/* Called with asynchronous notifications from backends */
	int (*async_notification)(struct mgmt_fe_client *client,
				  uintptr_t user_data, uint64_t client_id,
				  uint64_t session_id, uintptr_t session_ctx,
				  const char *result);

	/* Called when new native error is returned */
	int (*error_notify)(struct mgmt_fe_client *client, uintptr_t user_data,
			    uint64_t client_id, uint64_t session_id,
			    uintptr_t session_ctx, uint64_t req_id, int error,
			    const char *errstr);
};

extern struct debug mgmt_dbg_fe_client;

/***************************************************************
 * API prototypes
 ***************************************************************/

#define debug_fe_client(fmt, ...)                                              \
	DEBUGD(&mgmt_dbg_fe_client, "FE-CLIENT: %s: " fmt, __func__,           \
	       ##__VA_ARGS__)
#define log_err_fe_client(fmt, ...)                                            \
	zlog_err("FE-CLIENT: %s: ERROR: " fmt, __func__, ##__VA_ARGS__)
#define debug_check_fe_client()                                                \
	DEBUG_MODE_CHECK(&mgmt_dbg_fe_client, DEBUG_MODE_ALL)

/*
 * Initialize library and try connecting with MGMTD FrontEnd interface.
 *
 * params
 *    Frontend client parameters.
 *
 * master_thread
 *    Thread master.
 *
 * Returns:
 *    Frontend client lib handler (nothing but address of mgmt_fe_client)
 */
extern struct mgmt_fe_client *
mgmt_fe_client_create(const char *client_name, struct mgmt_fe_client_cbs *cbs,
		      uintptr_t user_data, struct event_loop *event_loop);

/*
 * Initialize library vty (adds debug support).
 *
 * This call should be added to your component when enabling other vty
 * code to enable mgmtd client debugs. When adding, one needs to also
 * add a their component in `xref2vtysh.py` as well.
 */
extern void mgmt_fe_client_lib_vty_init(void);

/*
 * Create a new Session for a Frontend Client connection.
 *
 * lib_hndl
 *    Client library handler.
 *
 * client_id
 *    Unique identifier of client.
 *
 * user_client
 *    Client context.
 *
 * Returns:
 *    MGMTD_SUCCESS on success, MGMTD_* otherwise.
 */
extern enum mgmt_result
mgmt_fe_create_client_session(struct mgmt_fe_client *client, uint64_t client_id,
			      uintptr_t user_client);

/*
 * Delete an existing Session for a Frontend Client connection.
 *
 * lib_hndl
 *    Client library handler.
 *
 * client_id
 *    Unique identifier of client.
 *
 * Returns:
 *    0 on success, otherwise msg_conn_send_msg() return values.
 */
extern enum mgmt_result
mgmt_fe_destroy_client_session(struct mgmt_fe_client *client,
			       uint64_t client_id);

/*
 * Send UN/LOCK_DS_REQ to MGMTD for a specific Datastore DS.
 *
 * lib_hndl
 *    Client library handler.
 *
 * session_id
 *    Client session ID.
 *
 * req_id
 *    Client request ID.
 *
 * ds_id
 *    Datastore ID (Running/Candidate/Oper/Startup)
 *
 * lock_ds
 *    TRUE for lock request, FALSE for unlock request.
 *
 * Returns:
 *    0 on success, otherwise msg_conn_send_msg() return values.
 */
extern int mgmt_fe_send_lockds_req(struct mgmt_fe_client *client, uint64_t session_id,
				   uint64_t req_id, enum mgmt_ds_id ds_id, bool lock_ds, bool scok);

/*
 * Send SET_COMMMIT_REQ to MGMTD for one or more config data(s).
 *
 * lib_hndl
 *    Client library handler.
 *
 * session_id
 *    Client session ID.
 *
 * req_id
 *    Client request ID.
 *
 * src_ds_id
 *    Source datastore ID from where data needs to be committed from.
 *
 * dst_ds_id
 *    Destination datastore ID where data needs to be committed to.
 *
 * validate_only
 *    TRUE if data needs to be validated only, FALSE otherwise.
 *
 * abort
 *    TRUE if need to restore Src DS back to Dest DS, FALSE otherwise.
 *
 * unlock
 *    Passed through to the resulting reply.
 *
 * Returns:
 *    0 on success, otherwise msg_conn_send_msg() return values.
 */
extern int mgmt_fe_send_commitcfg_req(struct mgmt_fe_client *client, uint64_t session_id,
				      uint64_t req_id, enum mgmt_ds_id src_ds_id,
				      enum mgmt_ds_id dst_ds_id, bool validate_only, bool abort,
				      bool unlock);

/*
 * Send GET-DATA to MGMTD daemon.
 *
 * client
 *    Client object.
 *
 * session_id
 *    Client session ID.
 *
 * req_id
 *    Client request ID.
 *
 * datastore
 *    Datastore for getting data.
 *
 * result_type
 *    The LYD_FORMAT of the result.
 *
 * flags
 *    Flags to control the behavior of the request.
 *
 * defaults
 *    Options to control the reporting of default values.
 *
 * xpath
 *    the xpath to get.
 *
 * Returns:
 *    0 on success, otherwise msg_conn_send_msg() return values.
 */
extern int mgmt_fe_send_get_data_req(struct mgmt_fe_client *client,
				     uint64_t session_id, uint64_t req_id,
				     uint8_t datastore, LYD_FORMAT result_type,
				     uint8_t flags, uint8_t defaults,
				     const char *xpath);

/*
 * Send EDIT to MGMTD daemon.
 *
 * client
 *    Client object.
 *
 * session_id
 *    Client session ID.
 *
 * req_id
 *    Client request ID.
 *
 * datastore
 *    Datastore for editing.
 *
 * request_type
 *    The LYD_FORMAT of the request.
 *
 * flags
 *    Flags to control the behavior of the request.
 *
 * operation
 *    NB_OP_* operation to perform.
 *
 * xpath
 *    the xpath to edit.
 *
 * data
 *    the data tree.
 *
 * Returns:
 *    0 on success, otherwise msg_conn_send_msg() return values.
 */
extern int mgmt_fe_send_edit_req(struct mgmt_fe_client *client,
				 uint64_t session_id, uint64_t req_id,
				 uint8_t datastore, LYD_FORMAT request_type,
				 uint8_t flags, uint8_t operation,
				 const char *xpath, const char *data);

/*
 * Send RPC request to MGMTD daemon.
 *
 * client
 *    Client object.
 *
 * session_id
 *    Client session ID.
 *
 * req_id
 *    Client request ID.
 *
 * result_type
 *    The LYD_FORMAT of the result.
 *
 * xpath
 *    the xpath of the RPC.
 *
 * data
 *    the data tree.
 *
 * Returns:
 *    0 on success, otherwise msg_conn_send_msg() return values.
 */
extern int mgmt_fe_send_rpc_req(struct mgmt_fe_client *client,
				uint64_t session_id, uint64_t req_id,
				LYD_FORMAT request_type, const char *xpath,
				const char *data);

/*
 * Destroy library and cleanup everything.
 */
extern void mgmt_fe_client_destroy(struct mgmt_fe_client *client);

/*
 * Get count of open sessions.
 */
extern uint mgmt_fe_client_session_count(struct mgmt_fe_client *client);

/*
 * True if the current handled message is being short-circuited
 */
extern bool
mgmt_fe_client_current_msg_short_circuit(struct mgmt_fe_client *client);

/**
 * Get the name of the client
 *
 * Args:
 *	The client object.
 *
 * Return:
 *	The name of the client.
 */
extern const char *mgmt_fe_client_name(struct mgmt_fe_client *client);

#ifdef __cplusplus
}
#endif

#endif /* _FRR_MGMTD_FE_CLIENT_H_ */
