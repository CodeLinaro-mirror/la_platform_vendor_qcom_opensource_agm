/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.  */
/* SPDX-License-Identifier: BSD-3-Clause-Clear */

#pragma once

#include "agm_ls_be_wrapper.h"
#include <agm/agm_api.h>
#include <string>

// AgmIpcLsBackend class: Handles socket communication and AGM API execution
class AgmIpcLsBackend {
  private:
    int backendSocket;
    int cbBackendSocket;
    AgmLsBackend agmLsBeWrapper;

    // Handle a single client connection
    void handleClient(int clientSocket, int cbClientSocket);

    // Process individual requests based on opcode
    void ipc_agm_aif_set_media_config(int clientSocket, const char *payload,
                                      size_t payloadSize);
    void ipc_agm_aif_set_metadata(int clientSocket, const char *payload,
                                  size_t payloadSize);
    void ipc_agm_session_set_metadata(int clientSocket, const char *payload,
                                      size_t payloadSize);
    void ipc_agm_session_aif_set_metadata(int clientSocket, const char *payload,
                                          size_t payloadSize);
    void ipc_agm_session_aif_connect(int clientSocket, const char *payload,
                                     size_t payloadSize);
    void ipc_agm_session_aif_get_tag_module_info(int clientSocket,
                                                 const char *payload,
                                                 size_t payloadSize);
    void ipc_agm_aif_set_params(int clientSocket, const char *payload,
                                size_t payloadSize);
    void ipc_agm_session_aif_set_params(int clientSocket, const char *payload,
                                        size_t payloadSize);
    void ipc_agm_session_aif_set_cal(int clientSocket, const char *payload,
                                     size_t payloadSize);
    void ipc_agm_session_set_params(int clientSocket, const char *payload,
                                    size_t payloadSize);
    void ipc_agm_session_get_params(int clientSocket, const char *payload,
                                    size_t payloadSize);
    void ipc_agm_get_params_from_acdb_tunnel(int clientSocket,
                                             const char *payload,
                                             size_t payloadSize);
    void ipc_agm_set_params_with_tag(int clientSocket, const char *payload,
                                     size_t payloadSize);
    void ipc_agm_set_params_with_tag_to_acdb(int clientSocket,
                                             const char *payload,
                                             size_t payloadSize);
    void ipc_agm_set_params_to_acdb_tunnel(int clientSocket,
                                           const char *payload,
                                           size_t payloadSize);
    void ipc_agm_session_register_cb(int clientSocket, int cbClientSocket,
                                     const char *payload, size_t payloadSize);
    void ipc_agm_session_register_for_events(int clientSocket,
                                             const char *payload,
                                             size_t payloadSize);
    void ipc_agm_session_open(int clientSocket, const char *payload,
                              size_t payloadSize);
    void ipc_agm_session_set_config(int clientSocket, const char *payload,
                                    size_t payloadSize);
    void ipc_agm_session_close(int clientSocket, const char *payload,
                               size_t payloadSize);
    void ipc_agm_session_prepare(int clientSocket, const char *payload,
                                 size_t payloadSize);
    void ipc_agm_session_start(int clientSocket, const char *payload,
                               size_t payloadSize);
    void ipc_agm_session_stop(int clientSocket, const char *payload,
                              size_t payloadSize);
    void ipc_agm_session_pause(int clientSocket, const char *payload,
                               size_t payloadSize);
    void ipc_agm_session_flush(int clientSocket, const char *payload,
                               size_t payloadSize);
    void ipc_agm_sessionid_flush(int clientSocket, const char *payload,
                                 size_t payloadSize);
    void ipc_agm_session_resume(int clientSocket, const char *payload,
                                size_t payloadSize);
    void ipc_agm_session_suspend(int clientSocket, const char *payload,
                                 size_t payloadSize);
    void ipc_agm_session_read(int clientSocket, const char *payload,
                              size_t payloadSize);
    void ipc_agm_session_write(int clientSocket, const char *payload,
                               size_t payloadSize);
    void ipc_agm_get_hw_processed_buff_cnt(int clientSocket,
                                           const char *payload,
                                           size_t payloadSize);
    void ipc_agm_get_aif_info_list(int clientSocket, const char *payload,
                                   size_t payloadSize);
    void ipc_agm_session_set_loopback(int clientSocket, const char *payload,
                                      size_t payloadSize);
    void ipc_agm_session_set_ec_ref(int clientSocket, const char *payload,
                                    size_t payloadSize);
    void ipc_agm_session_eos(int clientSocket, const char *payload,
                             size_t payloadSize);
    void ipc_agm_get_session_time(int clientSocket, const char *payload,
                                  size_t payloadSize);
    void ipc_agm_get_buffer_timestamp(int clientSocket, const char *payload,
                                      size_t payloadSize);
    void ipc_agm_session_get_buf_info(int clientSocket, const char *payload,
                                      size_t payloadSize);
    void ipc_agm_register_service_crash_callback(int clientSocket,
                                                 int cbClientSocket,
                                                 const char *payload,
                                                 size_t payloadSize);
    void ipc_agm_set_gapless_session_metadata(int clientSocket,
                                              const char *payload,
                                              size_t payloadSize);
    void ipc_agm_session_write_with_metadata(int clientSocket,
                                             const char *payload,
                                             size_t payloadSize);
    void ipc_agm_session_read_with_metadata(int clientSocket,
                                            const char *payload,
                                            size_t payloadSize);
    void ipc_agm_session_set_non_tunnel_mode_config(int clientSocket,
                                                    const char *payload,
                                                    size_t payloadSize);
    void ipc_agm_get_group_aif_info_list(int clientSocket, const char *payload,
                                         size_t payloadSize);
    void ipc_agm_aif_group_set_media_config(int clientSocket,
                                            const char *payload,
                                            size_t payloadSize);
    void ipc_agm_session_write_datapath_params(int clientSocket,
                                               const char *payload,
                                               size_t payloadSize);
    void ipc_agm_dump(int clientSocket, const char *payload,
                      size_t payloadSize);

  public:
    AgmIpcLsBackend() : backendSocket(-1), cbBackendSocket(-1) {}
    ~AgmIpcLsBackend();

    // Start the back end
    void start();

    // Stop the back end
    void stop();
};
