#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <atomic>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "systemmanagerservice.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

namespace grpcexamples {
std::ostream& operator<<(std::ostream& os, PowerState state) {
    switch (state) {
        case POWERSTATE__OFF:
            os << "POWERSTATE__OFF";
            break;
        case POWERSTATE__TRANSITION_TO_LPM:
            os << "POWERSTATE__TRANSITION_TO_LPM";
            break;
        case POWERSTATE__LPM:
            os << "POWERSTATE__LPM";
            break;
        case POWERSTATE__TRANSITION_TO_ACTIVE:
            os << "POWERSTATE__TRANSITION_TO_ACTIVE";
            break;
        case POWERSTATE__ACTIVE:
            os << "POWERSTATE__ACTIVE";
            break;
    }
    return os;
}

class SystemManagerClient {
public:
    SystemManagerClient(std::shared_ptr<Channel> channel) :
        stub_(SystemManagerService::NewStub(channel)),
        m_running{true},
        m_threadSendKeepActive{std::thread(&SystemManagerClient::threadHandler, this)} {}

    void threadHandler() {
        std::cout << "Enter thread handler\n";
        while(m_running.load()) {
            sendKeepActive();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void sendKeepActive() {
        ClientContext context{};
        NoResponseRequired response{};
        std::unique_ptr< ClientWriter< KeepActiveRequest > > writer(
            stub_->NotifyOnKeepActive(&context, &response));

        KeepActiveRequest request{};
        request.set_client_id(CLIENT_ID);
        if (!writer->Write(request)) {
            std::cout << "Write failed!\n";
        }
        std::cout << "Send keep active success\n";
    }

    PowerState waitPowerState() {
        ClientContext context{};
        NotifyOnPowerStateChangeRequest request{};
        request.set_client_id(CLIENT_ID);
        std::unique_ptr<ClientReader<PowerStateChangeEvent>> reader(
            stub_->NotifyOnPowerStateChange(&context, request));

        PowerStateChangeEvent state{};        
        while(reader->Read(&state)) {
            std::cout << "Received notification for state " << state.new_state() << "\n";
        }

        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "Client was successfully notified\n" ;
        } else {
            std::cout << "Client notification failed\n" ;
        }

        return state.new_state();
    }

    void acknowledgePowerState(PowerState powerState) {
        ClientContext context{};
        AcknowledgePowerStateChangeRequest request{};
        request.set_client_id(CLIENT_ID); request.set_state(powerState);
        NoResponseRequired response{};
        Status status = stub_->AcknowledgePowerStateChange(&context, request, &response);
        std::cout << "Acnowledged state " << powerState << " for client " << CLIENT_ID << "\n";
    }

    ~SystemManagerClient() {
        m_running.store(false);
        m_threadSendKeepActive.join();
    }

private:
    std::unique_ptr<SystemManagerService::Stub> stub_;
    std::thread m_threadSendKeepActive;
    std::atomic_bool m_running{};
    const std::string CLIENT_ID = "cpp_client";
};
} // grpcexamples

int main(int argc, char** argv) {
    grpcexamples::SystemManagerClient client(
        grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    while(true) {
        const auto new_state = client.waitPowerState();
        client.acknowledgePowerState(new_state);
        if (grpcexamples::POWERSTATE__ACTIVE == new_state) {
            break;
        }
    }
    std::cout << "Exit main\n";
  
    return 0;
}
