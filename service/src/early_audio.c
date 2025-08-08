/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.  */
/* SPDX-License-Identifier: BSD-3-Clause-Clear */

#define LOG_TAG "Early_Audio"
#include <agm/agm_api.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <agm/utils.h>

#define TINYMIX "tinymix"

// Audio interface configuration
#define DEFAULT_AIF_ID_RX 7
#define DEFAULT_SESSION_ID_RX 1
#define DEFAULT_SAMPLE_RATE 48000
#define DEFAULT_BIT_WIDTH 16
#define DEFAULT_CHANNELS 2

size_t num_aif_info = 0;
int aif_id_rx = DEFAULT_AIF_ID_RX;
uint32_t session_id_rx = DEFAULT_SESSION_ID_RX;
uint64_t sess_handle_rx = 0;
uint32_t dev_rx_metadata[] = {
    1,                      /* No of GKVS*/
    0xA2000000, 0xA2000001, /*GKVS*/
    3,                      /* No of CKVS*/
    0xA5000000, DEFAULT_SAMPLE_RATE, 0xA6000000, DEFAULT_BIT_WIDTH, 0xD2000000,
    0, /*CKVS*/
    0, /* Property ID*/
    0, /* No of Properties*/
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
struct agm_media_config media_config = {DEFAULT_SAMPLE_RATE, DEFAULT_CHANNELS,
                                        AGM_FORMAT_PCM_S16_LE, 1};
struct agm_buffer_config buffer_config = {4, 4096, 0};
#define BUFF_SIZE (32 * 10 * 2)
uint8_t audio_buff[BUFF_SIZE] = {0};

int session_play(int argc, char *argv[]) {
    int rst = 0;
    const char *filename = NULL;
    FILE *file = NULL;
    int count = 0;
    int first_frame = 0;

    if (argc != 2) {
        AGM_LOGE("Usage: %s <input.pcm>", argv[0]);
        goto end;
    }
    filename = argv[1];
    AGM_LOGI("session_play audio file %s", filename);
    file = fopen(filename, "rb");
    if (!file) {
        AGM_LOGE("Failed to open file error %d %s", errno, strerror(errno));
        goto end;
    }
    if (fseek(file, 44, SEEK_SET) != 0) {
        AGM_LOGE("Failed to skip WAV header");
        goto end;
    }
    while ((count = fread(audio_buff, 1, BUFF_SIZE, file)) == BUFF_SIZE) {
        // session write
        // AGM_LOGV("agm_session_write");
        rst = agm_session_write(sess_handle_rx, audio_buff, &count);
        if (rst) {
            AGM_LOGE("agm_session_write failed "
                  "rst %d",
                  rst);
            goto end;
        }
        if (first_frame == 0) {
            AGM_LOGI("EA - AGM write first audio frame done");
            ar_write_marker("EA - AGM write first audio frame done");
            first_frame = 1;
        }
    }

end:
    if (file) {
        fclose(file);
    }
    return rst;
}

extern char **environ;
char *env[] = {"PATH=/system/bin:/vendor/bin:/vendor_early_services/system/"
               "bin:/vendor_early_services/vendor/bin",
               "LD_LIBRARY_PATH=/system/lib64:/vendor/lib64:/"
               "vendor_early_services/system/lib64",
               NULL};
int execute_tinymix(const char *control, const char *value) {
    pid_t pid = fork();
    int rst = 0;

    if (pid == -1) {
        AGM_LOGE("fork failed");
        rst = -1;
    } else if (pid == 0) {
        environ = env;
        char *args[] = {TINYMIX, (char *)control, (char *)value, NULL};
        rst = execvpe(TINYMIX, args, env);
        AGM_LOGE("execvp %s failed %s %d", TINYMIX, strerror(errno), rst);
    } else {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
                AGM_LOGE("tinymix %s %s failed with status %d", control, value,
                      exit_status);
                rst = -1;
            }
        } else if (WIFSIGNALED(status)) {
            AGM_LOGE("tinymix %s %s killed by signal %d", control, value,
                  WTERMSIG(status));
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
        {NULL, NULL} // Termination marker
    };

    for (int i = 0; commands[i][0] != NULL; i++) {
        if (execute_tinymix(commands[i][0], commands[i][1]) != 0) {
            AGM_LOGE("Failed to execute command %d", i + 1);
            rst = -1;
            break;
        }
    }
    return rst;
}

int play_tone_standalone(int argc, char *argv[]) {
    int rst = 0;
    int i = 0;
    struct aif_info *aifinfo = NULL;

    rst = configure_audio();
    if (rst) {
        AGM_LOGE("configure_audio failed");
        goto end;
    }

    AGM_LOGI("agm_init");
    ar_write_marker("EA - AGM init");
    rst = agm_init();
    if (rst) {
        AGM_LOGE("agm_init failed rst %d", rst);
        goto end;
    }
    ar_write_marker("EA - AGM init Done");

    // get audio interface list
    AGM_LOGI("agm_get_aif_info_list");
    rst = agm_get_aif_info_list(aifinfo, &num_aif_info);
    if (rst) {
        AGM_LOGE("agm_get_aif_info_list failed rst %d", rst);
        goto end;
    }
    AGM_LOGI("agm_get_aif_info_list num_aif_info %d", num_aif_info);

    if (num_aif_info > 0) {
        aifinfo = calloc(num_aif_info, sizeof(struct aif_info));
        if (!aifinfo) {
            rst = -1;
            AGM_LOGE("aifinfo calloc failed");
            goto end;
        }
    } else {
        rst = -1;
        AGM_LOGE("num_aif_info is zero");
        goto end;
    }

    rst = agm_get_aif_info_list(aifinfo, &num_aif_info);
    if (rst) {
        AGM_LOGE("agm_get_aif_info_list failed num_aif_info %d rst %d",
              num_aif_info, rst);
        goto end;
    }
    for (i = 0; i < num_aif_info; i++) {
        AGM_LOGI("aif [%d] name %s dir %d", i, aifinfo[i].aif_name,
              aifinfo[i].dir);
    }

    // setup device RX
    AGM_LOGI("agm_aif_set_media_config");
    rst = agm_aif_set_media_config(aif_id_rx, &media_config);
    if (rst) {
        AGM_LOGE("agm_aif_set_media_config failed aif_id_rx %d rst %d", aif_id_rx,
              rst);
        goto end;
    }
    AGM_LOGI("agm_aif_set_metadata");
    // set device metadata
    rst = agm_aif_set_metadata(aif_id_rx, sizeof(dev_rx_metadata),
                               dev_rx_metadata);
    if (rst) {
        AGM_LOGE("agm_aif_set_metadata failed aif_id_rx %d rst %d", aif_id_rx,
              rst);
        goto end;
    }

    // set playback session stream metadata
    AGM_LOGI("agm_session_set_metadata");
    rst = agm_session_set_metadata(session_id_rx, sizeof(stream_metadata),
                                   stream_metadata);
    if (rst) {
        AGM_LOGE("agm_session_set_metadata failed session_id_rx %d rst %d",
              session_id_rx, rst);
        goto end;
    }

    // set device session stream metadata
    AGM_LOGI("agm_session_aif_set_metadata");
    rst = agm_session_aif_set_metadata(session_id_rx, aif_id_rx,
                                       sizeof(dev_rx_stream_metadata),
                                       dev_rx_stream_metadata);
    if (rst) {
        AGM_LOGE("agm_session_aif_set_metadata failed session_id_rx %d aif_id_rx "
              "%d rst %d",
              session_id_rx, aif_id_rx, rst);
        goto end;
    }

    // session connect
    AGM_LOGI("agm_session_aif_connect");
    rst = agm_session_aif_connect(session_id_rx, aif_id_rx, true);
    if (rst) {
        AGM_LOGE("agm_session_aif_connect connect failed session_id_rx %d "
              "aif_id_rx %d rst %d",
              session_id_rx, aif_id_rx, rst);
        goto end;
    }

    // set device session stream params
    AGM_LOGI("agm_session_aif_set_params");
    rst = agm_session_aif_set_params(session_id_rx, aif_id_rx,
                                     dev_rx_stream_params,
                                     sizeof(dev_rx_stream_params));
    if (rst) {
        AGM_LOGE("agm_session_aif_set_params failed session_id_rx %d "
              "aif_id_rx %d rst %d",
              session_id_rx, aif_id_rx, rst);
        goto end;
    }

    // session open
    AGM_LOGI("agm_session_open");
    rst = agm_session_open(session_id_rx, AGM_SESSION_DEFAULT, &sess_handle_rx);
    if (rst) {
        AGM_LOGE("agm_session_open failed session_id_rx %d "
              "rst %d",
              session_id_rx, rst);
        goto end;
    }
    // session config
    AGM_LOGI("agm_session_set_config");
    rst = agm_session_set_config(sess_handle_rx, &stream_config, &media_config,
                                 &buffer_config);
    if (rst) {
        AGM_LOGE("agm_session_set_config failed "
              "rst %d",
              rst);
        goto end;
    }
    // session prepare
    AGM_LOGI("agm_session_prepare");
    rst = agm_session_prepare(sess_handle_rx);
    if (rst) {
        AGM_LOGE("agm_session_prepare failed "
              "rst %d",
              rst);
        goto end;
    }
    // session start
    AGM_LOGI("agm_session_start");
    rst = agm_session_start(sess_handle_rx);
    if (rst) {
        AGM_LOGE("agm_session_start failed "
              "rst %d",
              rst);
        goto end;
    }
    // session play
    AGM_LOGI("session_play");
    rst = session_play(argc, argv);
    if (rst) {
        goto end;
    }

    // session stop
    AGM_LOGI("agm_session_stop");
    rst = agm_session_stop(sess_handle_rx);
    if (rst) {
        AGM_LOGE("agm_session_stop failed "
              "rst %d",
              rst);
        goto end;
    }
    // session disconnect
    AGM_LOGI("agm_session_aif_connect disconnect");
    rst = agm_session_aif_connect(session_id_rx, aif_id_rx, false);
    if (rst) {
        AGM_LOGE("agm_session_aif_connect disconnect failed session_id_rx %d "
              "aif_id_rx %d rst %d",
              session_id_rx, aif_id_rx, rst);
        goto end;
    }
    // session close
    AGM_LOGI("agm_session_close");
    rst = agm_session_close(sess_handle_rx);
    if (rst) {
        AGM_LOGE("agm_session_close failed "
              "rst %d",
              rst);
        goto end;
    }
end:
    if (aifinfo) {
        free(aifinfo);
    }
    AGM_LOGI("agm_deinit");
    rst = agm_deinit();
    if (rst) {
        AGM_LOGE("agm_deinit failed rst %d", rst);
    }
    return rst;
}

int main(int argc, char *argv[]) {
    int rst = 0;
    ar_write_marker("EA - Early Audio enter");
#ifdef AR_EARLY_AUDIO
    freopen("/dev/kmsg", "a", stdout);
    freopen("/dev/kmsg", "a", stderr);
#endif

    rst = play_tone_standalone(argc, argv);

    AGM_LOGI("exit rst %d", rst);
    ar_write_marker("EA - Early Audio exit");
    exit(rst);
}
