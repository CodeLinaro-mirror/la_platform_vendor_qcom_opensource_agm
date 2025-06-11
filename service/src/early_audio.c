/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.  */
/* SPDX-License-Identifier: BSD-3-Clause-Clear */


#define LOG_TAG "Early_Audio:"
#include <agm/agm_api.h>
//#include <log/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define ALOGE(arg,...) printf("[E][%s][%s][%d]:" arg "\n", LOG_TAG, __func__, __LINE__, ##__VA_ARGS__)
#define ALOGD(arg,...) printf("[D][%s][%s][%d]:" arg "\n", LOG_TAG, __func__, __LINE__, ##__VA_ARGS__)
#define ALOGI(arg,...) printf("[I][%s][%s][%d]:" arg "\n", LOG_TAG, __func__, __LINE__, ##__VA_ARGS__)
#define ALOGV(arg,...) printf("[V][%s][%s][%d]:" arg "\n", LOG_TAG, __func__, __LINE__, ##__VA_ARGS__)

#define TINYMIX "tinymix"

size_t num_aif_info = 0;
int aif_id_rx = 7;
uint32_t session_id_rx = 1;
uint64_t sess_handle_rx = 0;
uint32_t dev_rx_metadata[] = {
    1,                                                     /* No of GKVS*/
    0xA2000000, 0xA2000001,                                /*GKVS*/
    3,                                                     /* No of CKVS*/
    0xA5000000, 48000,      0xA6000000, 16, 0xD2000000, 0, /*CKVS*/
    0,                                                     /* Property ID*/
    0,                                                     /* No of Properties*/
    /* Properties*/
};
uint32_t stream_metadata[] = {
    2,                                     /* No of GKVS*/
    0xA1000000, 0xA100000E, 0xAB000000, 1, /*GKVS*/
    1,                                     /* No of CKVS*/
    0xA4000000, 0,                         /*CKVS*/
    0,                                     /* Property ID*/
    0,                                     /* No of Properties*/
    /* Properties*/
};
uint32_t dev_rx_stream_metadata[] = {
    2,                                              /* No of GKVS*/
    0xA1000000, 0xA100000E, 0xAC000000, 0xAC000002, /*GKVS*/
    /* No of CKVS*/
    /*CKVS*/
    /* Property ID*/
    /* No of Properties*/
    /* Properties*/
};
uint32_t dev_rx_stream_params[] = {
    0x40c8, 0x8001024, 0xc, 0x0, 0xbb80, 0x20010, 0x20001, 0x0,
};
struct agm_session_config stream_config = {
    RX, AGM_SESSION_DEFAULT, 2048, 4096, {0}, 0, 0};
struct agm_media_config media_config = {48000, 2, 2, 1};
struct agm_buffer_config buffer_config = {4, 4096, 0};
#define BUFF_SIZE (32 * 10 * 2)
uint8_t audio_buff[BUFF_SIZE] = {0};

int session_play(int argc, char *argv[]) {
    int rst = 0;
    const char *filename = NULL;
    FILE *file = NULL;
    int count = 0;

    if (argc != 2) {
        ALOGE("Usage: %s <input.pcm>", argv[0]);
        goto end;
    }
    filename = argv[1];
    file = fopen(filename, "rb");
    if (!file) {
        ALOGE("Failed to open file");
        goto end;
    }
    if (fseek(file, 44, SEEK_SET) != 0) {
        ALOGE("Failed to skip WAV header");
        goto end;
    }
    while ((count = fread(audio_buff, 1, BUFF_SIZE, file)) == BUFF_SIZE) {
        // session write
        ALOGV("agm_session_write");
        rst = agm_session_write(sess_handle_rx, audio_buff, &count);
        if (rst) {
            ALOGE("agm_session_write failed "
                  "rst %d",
                  rst);
            goto end;
        }
    }

end:
    if (file) {
        fclose(file);
    }
    return rst;
}

extern char **environ;
char *env[] = {
    "PATH=/system/bin:/vendor/bin:/vendor_early_services/system/bin:/vendor_early_services/vendor/bin",
    "LD_LIBRARY_PATH=/system/lib64:/vendor/lib64:/vendor_early_services/system/lib64",
    NULL
};
int execute_tinymix(const char *control, const char *value) {
    pid_t pid = fork();
    int rst = 0;

    if (pid == -1) {
        ALOGE("fork failed");
        rst = -1;
    }
    else if (pid == 0) {
        environ = env;
        char *args[] = {TINYMIX, (char *)control, (char *)value, NULL};
        rst = execvpe(TINYMIX, args, env);
        ALOGE("execvp %s failed %s %d", TINYMIX, strerror(errno), rst);
    }
    else {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                ALOGE("tinymix %s %s failed with status %d",
                      control, value, exit_status);
                rst = -1;
            }
        } else if (WIFSIGNALED(status)) {
            ALOGE("tinymix %s %s killed by signal %d",
                  control, value, WTERMSIG(status));
            rst = -1;
        }
    }
    return rst;
}

int configure_audio() {
    int rst = 0;
    const char *commands[][2] = {
        {"WSA RX0 MUX", "AIF1_PB"},
        {"WSA RX1 MUX", "AIF1_PB"},
        {"WSA_RX0 INP0", "RX0"},
        {"WSA_RX1 INP0", "RX1"},
        {"WSA_COMP1 Switch", "1"},
        {"WSA_COMP2 Switch", "1"},
        {"SpkrLeft COMP Switch", "1"},
        {"SpkrLeft VISENSE Switch", "1"},
        {"SpkrLeft SWR DAC_Port Switch", "1"},
        {"SpkrRight COMP Switch", "1"},
        {"SpkrRight VISENSE Switch", "1"},
        {"SpkrRight SWR DAC_Port Switch", "1"},
        {NULL, NULL}  // Termination marker
    };

    for (int i = 0; commands[i][0] != NULL; i++) {
        if (execute_tinymix(commands[i][0], commands[i][1]) != 0) {
            ALOGE("Failed to execute command %d", i+1);
            rst =-1;
            break;
        }
    }
    return rst;
}

int main(int argc, char *argv[]) {
    int rst = 0;
    int i = 0;
    struct aif_info *aifinfo = NULL;

    freopen("/dev/kmsg", "w", stdout);
    rst = configure_audio();
    if (rst) {
        ALOGE("configure_audio failed");
        goto end;
    }

    ALOGI("agm_init");
    rst = agm_init();
    if (rst) {
        ALOGE("agm_init failed rst %d", rst);
        goto end;
    }

    // get audio interface list
    ALOGI("agm_get_aif_info_list");
    rst = agm_get_aif_info_list(aifinfo, &num_aif_info);
    if (rst) {
        ALOGE("agm_get_aif_info_list failed rst %d", rst);
        goto end;
    }
    ALOGI("agm_get_aif_info_list num_aif_info %d", num_aif_info);

    if (num_aif_info > 0) {
        aifinfo = calloc(num_aif_info, sizeof(struct aif_info));
        if (!aifinfo) {
            rst = -1;
            ALOGE("aifinfo calloc failed");
            goto end;
        }
    } else {
        rst = -1;
        ALOGE("num_aif_info is zero");
        goto end;
    }

    rst = agm_get_aif_info_list(aifinfo, &num_aif_info);
    if (rst) {
        ALOGE("agm_get_aif_info_list failed num_aif_info %d rst %d",
              num_aif_info, rst);
        goto end;
    }
    for (i = 0; i < num_aif_info; i++) {
        ALOGI("aif [%d] name %s dir %d", i, aifinfo[i].aif_name,
              aifinfo[i].dir);
    }

    // setup device RX
    ALOGI("agm_aif_set_media_config");
    rst = agm_aif_set_media_config(aif_id_rx, &media_config);
    if (rst) {
        ALOGE("agm_aif_set_media_config failed aif_id_rx %d rst %d", aif_id_rx,
              rst);
        goto end;
    }
    ALOGI("agm_aif_set_metadata");
    // set device metadata
    rst = agm_aif_set_metadata(aif_id_rx, sizeof(dev_rx_metadata),
                               dev_rx_metadata);
    if (rst) {
        ALOGE("agm_aif_set_metadata failed aif_id_rx %d rst %d", aif_id_rx,
              rst);
        goto end;
    }

    // set playback session stream metadata
    ALOGI("agm_session_set_metadata");
    rst = agm_session_set_metadata(session_id_rx, sizeof(stream_metadata),
                                   stream_metadata);
    if (rst) {
        ALOGE("agm_session_set_metadata failed session_id_rx %d rst %d",
              session_id_rx, rst);
        goto end;
    }

    // set device session stream metadata
    ALOGI("agm_session_aif_set_metadata");
    rst = agm_session_aif_set_metadata(session_id_rx, aif_id_rx,
                                       sizeof(dev_rx_stream_metadata),
                                       dev_rx_stream_metadata);
    if (rst) {
        ALOGE("agm_session_aif_set_metadata failed session_id_rx %d aif_id_rx "
              "%d rst %d",
              session_id_rx, aif_id_rx, rst);
        goto end;
    }

    // session connect
    ALOGI("agm_session_aif_connect");
    rst = agm_session_aif_connect(session_id_rx, aif_id_rx, true);
    if (rst) {
        ALOGE("agm_session_aif_connect connect failed session_id_rx %d "
              "aif_id_rx %d rst %d",
              session_id_rx, aif_id_rx, rst);
        goto end;
    }

    // set device session stream params
    ALOGI("agm_session_aif_set_params");
    rst = agm_session_aif_set_params(session_id_rx, aif_id_rx,
                                     dev_rx_stream_params,
                                     sizeof(dev_rx_stream_params));
    if (rst) {
        ALOGE("agm_session_aif_set_params failed session_id_rx %d "
              "aif_id_rx %d rst %d",
              session_id_rx, aif_id_rx, rst);
        goto end;
    }

    // session open
    ALOGI("agm_session_open");
    rst = agm_session_open(session_id_rx, AGM_SESSION_DEFAULT, &sess_handle_rx);
    if (rst) {
        ALOGE("agm_session_open failed session_id_rx %d "
              "rst %d",
              session_id_rx, rst);
        goto end;
    }
    // session config
    ALOGI("agm_session_set_config");
    rst = agm_session_set_config(sess_handle_rx, &stream_config, &media_config,
                                 &buffer_config);
    if (rst) {
        ALOGE("agm_session_set_config failed "
              "rst %d",
              rst);
        goto end;
    }
    // session prepare
    ALOGI("agm_session_prepare");
    rst = agm_session_prepare(sess_handle_rx);
    if (rst) {
        ALOGE("agm_session_prepare failed "
              "rst %d",
              rst);
        goto end;
    }
    // session start
    ALOGI("agm_session_start");
    rst = agm_session_start(sess_handle_rx);
    if (rst) {
        ALOGE("agm_session_start failed "
              "rst %d",
              rst);
        goto end;
    }
    // session play
    ALOGI("session_play");
    rst = session_play(argc, argv);
    if (rst) {
        goto end;
    }

    // session stop
    ALOGI("agm_session_stop");
    rst = agm_session_stop(sess_handle_rx);
    if (rst) {
        ALOGE("agm_session_stop failed "
              "rst %d",
              rst);
        goto end;
    }
    // session disconnect
    ALOGI("agm_session_aif_connect disconnect");
    rst = agm_session_aif_connect(session_id_rx, aif_id_rx, false);
    if (rst) {
        ALOGE("agm_session_aif_connect disconnect failed session_id_rx %d "
              "aif_id_rx %d rst %d",
              session_id_rx, aif_id_rx, rst);
        goto end;
    }
    // session close
    ALOGI("agm_session_close");
    rst = agm_session_close(sess_handle_rx);
    if (rst) {
        ALOGE("agm_session_close failed "
              "rst %d",
              rst);
        goto end;
    }
end:
    if (aifinfo) {
        free(aifinfo);
    }
    ALOGI("agm_deinit");
    rst = agm_deinit();
    if (rst) {
        ALOGE("agm_deinit failed rst %d", rst);
        goto end;
    }
    ALOGI("exit rst %d", rst);
    exit(rst);
}
