/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2023 Zhennan Tu <zhennan.tu@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <cstdint>

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <ltlib/io/client.h>
#include <ltlib/io/ioloop.h>
#include <ltlib/settings.h>
#include <ltlib/threads.h>
#include <ltlib/time_sync.h>
#include <transport/transport.h>

#include <audio/player/audio_player.h>
#include <graphics/drpipeline/video_decode_render_pipeline.h>
#include <inputs/capturer/input_capturer.h>
#include <platforms/pc_sdl.h>

namespace lt {

namespace cli {

struct SignalingParams {
    SignalingParams(const std::string& _client_id, const std::string& _room_id,
                    const std::string& _addr, uint16_t _port)
        : client_id(_client_id)
        , room_id(_room_id)
        , addr(_addr)
        , port(_port) {}
    std::string client_id;
    std::string room_id;
    std::string addr;
    uint16_t port;
};

class Client {
public:
    struct Params {
        std::string client_id;
        std::string room_id;
        std::string auth_token;
        std::string user;
        std::string pwd;
        std::string signaling_addr;
        uint16_t signaling_port;
        std::string codec;
        uint32_t width;
        uint32_t height;
        uint32_t screen_refresh_rate;
        uint32_t audio_freq;
        uint32_t audio_channels;
        bool enable_driver_input;
        bool enable_gamepad;
        std::vector<std::string> reflex_servers;
    };

public:
    static std::unique_ptr<Client> create(std::map<std::string, std::string> options);
    ~Client();
    void wait();

private:
    Client(const Params& params);
    bool init();
    bool initSettings();
    bool initSignalingClient();
    bool initAppClient();
    void mainLoop(const std::function<void()>& i_am_alive);
    void onPlatformRenderTargetReset();
    void onPlatformExit();
    void stopWait();
    void postTask(const std::function<void()>& task);
    void postDelayTask(int64_t delay_ms, const std::function<void()>& task);
    void syncTime();
    void toggleFullscreen();
    void switchMouseMode();
    void checkWorkerTimeout();
    void tellAppKeepAliveTimeout();

    // app
    void onAppConnected();
    void onAppDisconnected();
    void onAppReconnecting();
    void onAppMessage(uint32_t type, std::shared_ptr<google::protobuf::MessageLite> msg);

    // 信令.
    void onSignalingNetMessage(uint32_t type, std::shared_ptr<google::protobuf::MessageLite> msg);
    void onSignalingDisconnected();
    void onSignalingReconnecting();
    void onSignalingConnected();
    void onJoinRoomAck(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onSignalingMessage(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onSignalingMessageAck(std::shared_ptr<google::protobuf::MessageLite> msg);
    void dispatchSignalingMessageRtc(std::shared_ptr<google::protobuf::MessageLite> msg);
    void dispatchSignalingMessageCore(std::shared_ptr<google::protobuf::MessageLite> msg);
    void sendKeepaliveToSignalingServer();

    // transport
    bool initTransport();
    tp::Client* createTcpClient();
    tp::Client* createRtcClient();
    tp::Client* createRtc2Client();
    static void onTpData(void* user_data, const uint8_t* data, uint32_t size, bool is_reliable);
    static void onTpVideoFrame(void* user_data, const lt::VideoFrame& frame);
    static void onTpAudioData(void* user_data, const lt::AudioData& audio_data);
    static void onTpConnected(void* user_data, lt::LinkType link_type);
    static void onTpConnChanged(void* user_data /*old_conn_info, new_conn_info*/);
    static void onTpFailed(void* user_data);
    static void onTpDisconnected(void* user_data);
    static void onTpSignalingMessage(void* user_data, const char* key, const char* value);

    // 数据通道.
    void dispatchRemoteMessage(uint32_t type,
                               const std::shared_ptr<google::protobuf::MessageLite>& msg);
    void sendKeepAlive();
    void onKeepAliveAck();
    bool sendMessageToHost(uint32_t type, const std::shared_ptr<google::protobuf::MessageLite>& msg,
                           bool reliable);
    void onStartTransmissionAck(const std::shared_ptr<google::protobuf::MessageLite>& msg);
    void onTimeSync(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onSendSideStat(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onCursorInfo(std::shared_ptr<google::protobuf::MessageLite> msg);

private:
    std::unique_ptr<ltlib::Settings> settings_;
    std::string auth_token_;
    std::string p2p_username_;
    std::string p2p_password_;
    SignalingParams signaling_params_;
    InputCapturer::Params input_params_{};
    VideoDecodeRenderPipeline::Params video_params_;
    AudioPlayer::Params audio_params_{};
    std::vector<std::string> reflex_servers_;
    std::mutex dr_mutex_;
    std::unique_ptr<VideoDecodeRenderPipeline> video_pipeline_;
    std::unique_ptr<InputCapturer> input_capturer_;
    std::unique_ptr<AudioPlayer> audio_player_;
    std::mutex ioloop_mutex_;
    std::unique_ptr<ltlib::IOLoop> ioloop_;
    std::unique_ptr<ltlib::Client> signaling_client_;
    std::unique_ptr<ltlib::Client> app_client_;
    lt::tp::Client* tp_client_ = nullptr;
    std::unique_ptr<PcSdl> sdl_;
    std::unique_ptr<ltlib::BlockingThread> main_thread_;
    std::unique_ptr<ltlib::TaskThread> hb_thread_;
    std::condition_variable exit_cv_;
    std::mutex exit_mutex_;
    bool should_exit_ = true;
    ltlib::TimeSync time_sync_;
    int64_t rtt_ = 0;
    int64_t time_diff_ = 0;
    bool windowed_fullscreen_ = true;
    bool signaling_keepalive_inited_ = false;
    std::optional<bool> is_p2p_;
    bool absolute_mouse_ = true;
    bool last_w_or_h_is_0_ = false;
    int64_t last_received_keepalive_;
    bool connected_to_app_ = false;
};

} // namespace cli

} // namespace lt