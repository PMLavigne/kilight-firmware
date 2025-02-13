/**
 * WifiSubsystem.h
 *
 * @author Patrick Lavigne
 */

#pragma once

#include <format>

#include <lwip/tcp.h>
#include <pico/cyw43_arch.h>


#include <mpf/util/FixedFormattedString.h>
#include <mpf/core/Logging.h>
#include <mpf/core/List.h>
#include <mpf/core/Subsystem.h>
#include <mpf/util/macros.h>

#include "kilight/com/server_protocol.h"
#include "kilight/core/Alarm.h"

#define KILIGHT_FIXED_STRING_BUFFER(name, templateString) \
        char name[sizeof(templateString)] { templateString };

namespace kilight::com {

    class WifiSubsystem final : public mpf::core::Subsystem {
        LOGGER(Wifi);
    public:
        static constexpr uint32_t WifiConnectRetryMsec = 5000;

        static constexpr uint32_t VerifyConnectionEveryMsec = 1000;

        static constexpr uint16_t BufferSize = 2048;

        static constexpr size_t MaxConnections = 4;

        static constexpr std::format_string<uint64_t> HardwareIdFormatString = "hwid={:016X}";

        static constexpr std::format_string<uint64_t> HostNameFormatString = "KiLightMono_{:016X}";

        [[nodiscard]]
        static int32_t rssi();

        explicit WifiSubsystem(mpf::core::SubsystemList * list);

        void setUp() override;

        [[nodiscard]]
        bool hasWork() const override;

        void work() override;

        [[nodiscard]]
        state_data_t const & stateData() const;

        template<typename UpdateFuncT>
        void updateStateData(UpdateFuncT && updateFunc) {
            cyw43_arch_lwip_begin();
            updateFunc(m_stateData);
            cyw43_arch_lwip_end();
        }

        template<typename CallbackT>
        void setWriteRequestCallback(CallbackT && callback) {
            m_writeRequestCallback = std::forward<CallbackT>(callback);
        }

    private:
        enum class State {
            Invalid,
            Disconnected,
            Connecting,
            Connected,
            PreIdle,
            Idle,
            ProcessClientData,
            VerifyConnected,
            Waiting
        };

        struct connected_session_t {
            tcp_pcb * clientPCB = nullptr;

            std::array<uint8_t, BufferSize> sendBuffer {};

            std::array<uint8_t, BufferSize> receiveBuffer {};

            uint16_t sendLength = 0;

            uint16_t receiveLength = 0;

            bool volatile inUse = false;

            bool volatile dataPending = false;
        };

        static inline WifiSubsystem * instance = nullptr;

        static err_t closeSession(connected_session_t * session);

        static void queueSystemInfoReply(connected_session_t & session);

        static void sendResponse(connected_session_t & session);

        State volatile m_state = State::Invalid;

        State m_stateAfterWait = State::Invalid;

        int m_lastLinkStatus = 0;

        core::Alarm m_alarm;

        tcp_pcb * m_serverPCB = nullptr;

        std::array<connected_session_t, MaxConnections> m_connectedSessions = {};

        state_data_t m_stateData = {};

        std::function<void(write_request_t const &)> m_writeRequestCallback;

        bool volatile m_verifyConnectionNeeded = false;

        mpf::util::FixedFormattedString<32> m_mdnsHardwareId;

        mpf::util::FixedFormattedString<32> m_hostname;

        void retryConnectionWait();

        [[nodiscard]]
        bool isClientDataPending() const;

        void disconnectedState();

        void connectingState();

        void connectedState();

        void preIdleState();

        void processClientDataState();

        void verifyConnectedState();

        void processClientData(connected_session_t & session) const;


        void queueStateReply(connected_session_t & session) const;

        void processWrite(connected_session_t & session) const;

        err_t acceptCallback(tcp_pcb *clientPCB, err_t error);

        err_t receiveCallback(connected_session_t * session, tcp_pcb* tpcb, pbuf * data, err_t error);

    };

}
